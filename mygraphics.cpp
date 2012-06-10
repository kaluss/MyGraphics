#include "aqp/alt_key.hpp"
#include "aqp/aqp.hpp"
#include "mygraphics.h"
#include "ui_mygraphics.h"
#include "item/MyItem.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include <QtGui>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsItemGroup>
#include <QGraphicsScene>
#include <QImageWriter>
#include <QMenu>
#include <QMenuBar>
#ifdef ANIMATE_ALIGNMENT
#include <QPropertyAnimation>
#endif
#ifdef ANIMATE_IN_PARALLEL
#include <QParallelAnimationGroup>
#endif
#include <QPrintDialog>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#ifdef USE_STL
#include <algorithm>
#endif
#include <cmath>
#include <limits>
#include <QDebug>
#include <QSvgGenerator>

const int OffsetIncrement = 5;
const qint32 MagicNumber = 0x5A93DE5;
const qint16 VersionNumber = 1;
const QString MostRecentFile("MostRecentFile");
const QString MimeType = "application/MyGraphic";

MyGraphics::MyGraphics(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MyGraphics),
    pasteOffset(OffsetIncrement)
{
    ui->setupUi(this);

    printer = new QPrinter(QPrinter::HighResolution);

    createActions();
    createMenusAndToolBars();
    createConnections();

    scene = new MyGraphicsScene(this);
    scene->setSceneRect(QRectF(0, 0, 800, 800));

    originP = QPointF(scene->width()/2, scene->height()/2);

    connect(scene, SIGNAL(itemClicked(QGraphicsItem*)), this, SLOT(itemClicked(QGraphicsItem*)));
    connect(scene, SIGNAL(itemMoved(QGraphicsItem*)), this, SLOT(itemMoved(QGraphicsItem*)));
    connect(scene, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

    view = new GluGraphicsView();
    view->setScene(scene);
    view->setDragMode(QGraphicsView::RubberBandDrag);     //可多选
    view->setBackgroundBrush(QBrush(Qt::white));
    setCentralWidget(view);

    currentItem = 0;

    variantManager = new QtVariantPropertyManager(this);

    connect(variantManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
            this, SLOT(valueChanged(QtProperty *, const QVariant &)));

    QtVariantEditorFactory *variantFactory = new QtVariantEditorFactory(this);


    QDockWidget *dock = new QDockWidget(tr("属性"),this);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    propertyEditor = new QtTreePropertyBrowser(dock);
    propertyEditor->setFactoryForManager(variantManager, variantFactory);
    dock->setWidget(propertyEditor);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
}

MyGraphics::~MyGraphics()
{
    delete printer;
    delete ui;
}

//输出视图
void MyGraphics::fileExport()
{
    QString suffixes = AQP::filenameFilter(tr("Bitmap image"),
            QImageWriter::supportedImageFormats());
    suffixes += tr(";;Vector image (*.svg)");
    const QString filename = QFileDialog::getSaveFileName(this,
            tr("%1 - Export").arg(QApplication::applicationName()),
            ".", suffixes);
    if (filename.isEmpty())
        return;
    if (filename.toLower().endsWith(".svg"))
        exportSvg(filename);
    else
        exportImage(filename);
}

//输出scene的内容
void MyGraphics::paintScene(QPainter *painter)
{
//    bool showGrid = viewShowGridAction->isChecked();
//    if (showGrid)
//        viewShowGrid(false);
    QList<QGraphicsItem*> items = scene->selectedItems();
    scene->clearSelection();

    scene->render(painter);

//    if (showGrid)
//        viewShowGrid(true);
    foreach (QGraphicsItem *item, items)
        item->setSelected(true);
    selectionChanged();
}

//输出为svg
void MyGraphics::exportSvg(const QString &filename)
{
    QSvgGenerator svg;
    svg.setFileName(filename);
    svg.setSize(printer->paperSize(QPrinter::Point).toSize());
    {
        QPainter painter(&svg);
        paintScene(&painter);
    }
    statusBar()->showMessage(tr("Exported %1").arg(filename),
                             1000);
}

//输出为图片
void MyGraphics::exportImage(const QString &filename)
{
    QImage image(printer->paperSize(QPrinter::Point).toSize(),
                 QImage::Format_ARGB32);
    {
        QPainter painter(&image);
        painter.setRenderHints(QPainter::Antialiasing|
                               QPainter::TextAntialiasing);
        paintScene(&painter);
    }
    if (image.save(filename))
        statusBar()->showMessage(tr("Exported %1").arg(filename),
                                 2000);
    else
        AQP::warning(this, tr("Error"), tr("Failed to export: %1")
                                        .arg(filename));
}

//打印
void MyGraphics::filePrint()
{
    QPrintDialog dialog(printer);
    if (dialog.exec()) {
        {
            QPainter painter(printer);
            paintScene(&painter);
        }
        statusBar()->showMessage(tr("Printed %1")
                .arg(windowFilePath()), 2000);
    }
}

//创建Action
void MyGraphics::createActions()
{
    fileExportAction = new QAction(QIcon(":images/fileexport.png"),
            tr("Export..."), this);
    filePrintAction = new QAction(QIcon(":images/fileprint.png"),
            tr("Print..."), this);

    editAlignmentAction = new QAction(QIcon(":images/align-left.png"),
                                      tr("Alignment"), this);
    editAlignmentAction->setData(Qt::AlignLeft);
    editAlignLeftAction = new QAction(QIcon(":images/align-left.png"),
                                      tr("Left"), this);
    editAlignLeftAction->setData(Qt::AlignLeft);
    editAlignRightAction = new QAction(QIcon(":images//align-right.png"),
                                       tr("Right"), this);
    editAlignRightAction->setData(Qt::AlignRight);
    editAlignTopAction = new QAction(QIcon(":images//align-top.png"),
                                     tr("Top"), this);
    editAlignTopAction->setData(Qt::AlignTop);
    editAlignBottomAction = new QAction(QIcon(":images//align-bottom.png"),
                                        tr("Bottom"), this);
    editAlignBottomAction->setData(Qt::AlignBottom);
}

//创建menu和toolbar
void MyGraphics::createMenusAndToolBars()
{
    QAction *separator = 0;
    setUnifiedTitleAndToolBarOnMac(true);

    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));
    populateMenuAndToolBar(fileMenu, fileToolBar, QList<QAction*>()
            << fileExportAction << separator << filePrintAction);

    QMenu *alignmentMenu = new QMenu(tr("Align"), this);
    foreach (QAction *action, QList<QAction*>()
            << editAlignLeftAction << editAlignRightAction
            << editAlignTopAction << editAlignBottomAction)
        alignmentMenu->addAction(action);
    editAlignmentAction->setMenu(alignmentMenu);

    QMenu *editMenu = menuBar()->addMenu(tr("Edit"));
    QToolBar *editToolBar = addToolBar(tr("Edit"));
    populateMenuAndToolBar(editMenu, editToolBar, QList<QAction*>()
                           << editAlignmentAction);
}

//创建连接
void MyGraphics::createConnections()
{
    connect(fileExportAction, SIGNAL(triggered()),
            this, SLOT(fileExport()));
    connect(filePrintAction, SIGNAL(triggered()),
            this, SLOT(filePrint()));

    foreach (QAction *action, QList<QAction*>()
            << editAlignmentAction << editAlignLeftAction
            << editAlignRightAction << editAlignTopAction
            << editAlignBottomAction)
        connect(action, SIGNAL(triggered()), this, SLOT(editAlign()));
}

void MyGraphics::populateCoordinates(const Qt::Alignment &alignment,
                                     QVector<double> *coordinates,
                                     const QList<QGraphicsItem *> &items)
{
    QListIterator<QGraphicsItem*> i(items);
    while (i.hasNext()) {
        QRectF rect = i.next()->sceneBoundingRect();
        switch (alignment) {
            case Qt::AlignLeft:
                coordinates->append(rect.x()); break;
            case Qt::AlignRight:
                coordinates->append(rect.x() + rect.width()); break;
            case Qt::AlignTop:
                coordinates->append(rect.y()); break;
            case Qt::AlignBottom:
                coordinates->append(rect.y() + rect.height()); break;
        }
    }
}

//item对齐
void MyGraphics::editAlign()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    Qt::Alignment alignment = static_cast<Qt::Alignment>(
            action->data().toInt());
    if (action != editAlignmentAction) {
        editAlignmentAction->setData(action->data());
        editAlignmentAction->setIcon(action->icon());
    }

    QList<QGraphicsItem*> items = scene->selectedItems();
    QVector<double> coordinates;
    populateCoordinates(alignment, &coordinates, items);
    double offset;
    if (alignment == Qt::AlignLeft || alignment == Qt::AlignTop)
        offset = *std::min_element(coordinates.constBegin(),
                                   coordinates.constEnd());
    else
        offset = *std::max_element(coordinates.constBegin(),
                                   coordinates.constEnd());


    QList<QPointF> positions;
    if (alignment == Qt::AlignLeft || alignment == Qt::AlignRight) {
        for (int i = 0; i < items.count(); ++i)
            positions << items.at(i)->pos() +
                         QPointF(offset - coordinates.at(i), 0);
    }
    else {
        for (int i = 0; i < items.count(); ++i)
            positions << items.at(i)->pos() +
                         QPointF(0, offset - coordinates.at(i));
    }

    animateAlignment(items, positions);
//    setDirty(true);
}

//创建populateMenuAndToolBar
void MyGraphics::populateMenuAndToolBar(QMenu *menu,
        QToolBar *toolBar, QList<QAction*> actions)
{
#ifdef Q_WS_MAC
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#endif
    foreach (QAction *action, actions) {
        if (!action) {
            menu->addSeparator();
            toolBar->addSeparator();
        }
        else {
            menu->addAction(action);
            toolBar->addAction(action);
        }
    }
}

void MyGraphics::animateAlignment(const QList<QGraphicsItem*> &items,
                                  const QList<QPointF> &positions)
{
    int duration = ((qApp->keyboardModifiers() & Qt::ShiftModifier)
                    != Qt::ShiftModifier) ? 1000 : 5000;

#ifdef ANIMATE_IN_PARALLEL
    QParallelAnimationGroup *group = new QParallelAnimationGroup;
#endif
    for (int i = 0; i < items.count(); ++i) {
        QObject *object = dynamic_cast<QObject*>(items.at(i));
        if (!object)
            continue;
        QPropertyAnimation *animation = new QPropertyAnimation(
                object, "pos", this);
        animation->setDuration(duration);
        animation->setEasingCurve(QEasingCurve::InOutBack);
#ifdef ANIMATE_IN_PARALLEL
        animation->setStartValue(items.at(i)->pos());
        animation->setEndValue(positions.at(i));
        group->addAnimation(animation);
#else
        animation->setKeyValueAt(0.0, items.at(i)->pos());
        animation->setKeyValueAt(1.0, positions.at(i));
        animation->start(QAbstractAnimation::DeleteWhenStopped);
#endif
    }
#ifdef ANIMATE_IN_PARALLEL
    group->start(QAbstractAnimation::DeleteWhenStopped);
#endif
}

//矩形item
void MyGraphics::on_toolButton_clicked()
{
    iRect *rect = new iRect;
//    rect->setRect(0,0,110,110);
//    rect->setBrush(QBrush(QColor(Qt::blue)));
    scene->addItem(rect);
//    rect->setPos(originP);
    scene->selectedItem(rect);
//    rect->setZValue(0.0);
    qDebug() << rect->zValue();
}

//椭圆item
void MyGraphics::on_toolButton_2_clicked()
{
    iEllipse *ellipse = new iEllipse;
    ellipse->setRect(0,0,100,100);
    ellipse->setBrush(QBrush(QColor(Qt::darkGreen)));
    //    ellipse->setOpacity(0.7);
    scene->addItem(ellipse);
    ellipse->setPos(originP);
    scene->selectedItem(ellipse);
    ellipse->setZValue(0.0);
    qDebug() << ellipse->zValue();
}

//更新窗口修改标志*
void MyGraphics::setDirty(bool on)
{
    setWindowModified(on);
}

//保存
bool MyGraphics::fileSave()
{
    //    const QString filename = windowFilePath();
    //    if (filename.isEmpty() || filename == tr("Unnamed"))
    //        return fileSaveAs();

    QString filenameas = QFileDialog::getSaveFileName(this,
                                                      tr("%1 - Save As").arg(QApplication::applicationName()),
                                                      "test.pd", tr("Page Designer (*.pd)"));
    if (filenameas.isEmpty())
        return false;
    if (!filenameas.toLower().endsWith(".pd"))
        filenameas += ".pd";
    setWindowFilePath(filenameas);

    const QString filename = windowFilePath();
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_5);
    writeItems(out, scene->items());
    file.close();
    //    setDirty(false);
    return true;
}

//另存为
bool MyGraphics::fileSaveAs()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("%1 - Save As").arg(QApplication::applicationName()),
                                                    ".", tr("Page Designer (*.pd)"));
    if (filename.isEmpty())
        return false;
    if (!filename.toLower().endsWith(".pd"))
        filename += ".pd";
    setWindowFilePath(filename);
    return fileSave();
}

//保存item
void MyGraphics::writeItems(QDataStream &out,
                            const QList<QGraphicsItem*> &itemList)
{
    //    foreach (QGraphicsItem *item, itemList) {
    //        //        if (item == gridGroup || item->group() == gridGroup)
    //        //            continue;
    //        qint32 type = static_cast<qint32>(item->type());
    //        out << type;
    //        switch (type) {
    //        case MG_TYPE_IRECT:
    //            out << dynamic_cast<iRect*>(item);
    //            qreal zvalue;
    //            zvalue = item->zValue();
    //            out << zvalue;
    //            break;
    //        default: Q_ASSERT(false);
    //        }
    //    }
    out << MagicNumber << VersionNumber;
    int sceneItemsize = 0;
    for(int i=0;i<itemList.size();i++)
    {
        QGraphicsItem *pParentItem= itemList.at(i)->parentItem();
        if(pParentItem != 0)
            continue;
        sceneItemsize++ ;
    }
    out << sceneItemsize;
    for(int i=0;i<itemList.size();i++)
    {
        //        QGraphicsItem *pParentItem= itemList.at(i)->parentItem();
        //        if(pParentItem != 0)
        //            continue;
        qint32 type = static_cast<qint32>(itemList.at(i)->type());
        switch (type)
        {
        case MG_TYPE_IRECT:
        {
            out << iRect::Type;
            iRect *piRect = dynamic_cast<iRect*>(itemList.at(i));
            out << piRect;
            piRect->setZValue(0.0-i);
            out << piRect->zValue();
            break;
        }
        case MG_TYPE_IELLIPSE:
        {
            out << iEllipse::Type;
            iEllipse *piEllipse = dynamic_cast<iEllipse*>(itemList.at(i));
            out << piEllipse;
            piEllipse->setZValue(0.0-i);
            out << piEllipse->zValue();
            break;
        }
        case MG_TYPE_ITEXT:
        {
            out << iText::Type;
            iText *piText = dynamic_cast<iText*>(itemList.at(i));
            out << piText;
            piText->setZValue(0.0-i);
            out << piText->zValue();
            break;
        }
        default:
            Q_ASSERT(false);
        }
    }
}

//打开文件
void MyGraphics::on_actionOpen_triggered()
{
    const QString &filename = QFileDialog::getOpenFileName(this,
                                                           tr("%1 - Open").arg(QApplication::applicationName()),
                                                           ".", tr("Page Designer (*.pd)"));
    if (filename.isEmpty())
        return;
    setWindowFilePath(filename);
    loadFile();
}

//保存文件
void MyGraphics::on_actionSave_triggered()
{
    fileSave();
}

//读取文件
void MyGraphics::loadFile()
{
    QFile file(windowFilePath());
    file.open(QIODevice::ReadOnly);
    QDataStream in;
    in.setDevice(&file);

    in.setVersion(QDataStream::Qt_4_5);
    clear();
    readItems(in);
    //    setDirty(false);
}

//清空scene
void MyGraphics::clear()
{
    //    QList<QGraphicsItem*> list = scene->items();
    //    for (int i = 0; i < list.size(); ++i) {
    //        scene->removeItem(list.at(i));
    //    }
    scene->clear();
}

//读取文件中的item
void MyGraphics::readItems(QDataStream &in, int offset, bool select)
{
    QSet<QGraphicsItem*> items;
    qint32 itemType;
    QGraphicsItem *item = 0;
    qint32 magicNumber;
    qint16 versionNumber;

    in >> magicNumber >> versionNumber;

    int sceneItemsize = 0;
    in >> sceneItemsize;
    qDebug() << sceneItemsize;

    while (sceneItemsize > 0 && !in.atEnd())
    {
        sceneItemsize-- ;
        in >> itemType;
        switch (itemType)
        {
        case MG_TYPE_IRECT:
        {
            iRect *it = new iRect();
            in >> it;
            scene->addItem(it);
            qreal zvalue;
            in >> zvalue;
            it->setZValue(zvalue);
            item = it;
            break;
        }
        case MG_TYPE_IELLIPSE:
        {
            iEllipse *it = new iEllipse();
            in >> it;
            scene->addItem(it);
            qreal zvalue;
            in >> zvalue;
            it->setZValue(zvalue);
            item = it;
            break;
        }
        case MG_TYPE_ITEXT:
        {
            iText *it = new iText();
            in >> it;
            scene->addItem(it);
            qreal zvalue;
            in >> zvalue;
            it->setZValue(zvalue);
            item = it;
            break;
        }
        }
        if (item) {
            item->moveBy(offset, offset);
            if (select)
                items << item;
            item = 0;
        }
    }
    if (select)
        selectItems(items);
    //    else
    //        selectionChanged();
}

//清空scene
void MyGraphics::on_actionClear_scene_triggered()
{
    clear();
}

//item移到后方
void MyGraphics::on_actionSendtoback_triggered()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems)
    {
        if (item->zValue() <= zValue)
            zValue = item->zValue() - 1.0;
    }
    selectedItem->setZValue(zValue);
    itemClicked(selectedItem);
}

//item移到前方
void MyGraphics::on_actionBringtofront_triggered()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems)
    {
        if (item->zValue() >= zValue)
            zValue = item->zValue() + 1.0;
    }
    selectedItem->setZValue(zValue);
    itemClicked(selectedItem);
}

//展开
void MyGraphics::updateExpandState()
{
    QList<QtBrowserItem *> list = propertyEditor->topLevelItems();
    QListIterator<QtBrowserItem *> it(list);
    while (it.hasNext()) {
        QtBrowserItem *item = it.next();
        QtProperty *prop = item->property();
        idToExpanded[propertyToId[prop]] = propertyEditor->isExpanded(item);
    }
}

//增加属性
void MyGraphics::addProperty(QtVariantProperty *property, const QString &id)
{
    propertyToId[property] = id;
    idToProperty[id] = property;
    QtBrowserItem *item = propertyEditor->addProperty(property);
    if (idToExpanded.contains(id))
        propertyEditor->setExpanded(item, idToExpanded[id]);
}

//item点击效果实现
void MyGraphics::itemClicked(QGraphicsItem *item)
{
    updateExpandState();

    QMap<QtProperty *, QString>::ConstIterator itProp = propertyToId.constBegin();
    while (itProp != propertyToId.constEnd()) {
        delete itProp.key();
        itProp++;
    }
    propertyToId.clear();
    idToProperty.clear();

    currentItem = item;
    if (!currentItem) {
        //        deleteAction->setEnabled(false);
        return;
    }
    //    qDebug() << item;
    //    qDebug() << "scenewidth:" << scene->width();
    //    qDebug() << "sceneheight:" << scene->height();
    //    qDebug() << "x:" << item->scenePos().x();
    //    qDebug() << "y:" << item->scenePos().y();
    //    qDebug() << "z:" << item->scenePos().y();

    //    deleteAction->setEnabled(true);

    QtVariantProperty *property;

    property = variantManager->addProperty(QVariant::Double, tr("Position X"));
    property->setAttribute(QLatin1String("minimum"), 0);
    property->setAttribute(QLatin1String("maximum"), scene->width());
    property->setValue(item->pos().x());
    addProperty(property, QLatin1String("xpos"));

    property = variantManager->addProperty(QVariant::Double, tr("Position Y"));
    property->setAttribute(QLatin1String("minimum"), 0);
    property->setAttribute(QLatin1String("maximum"), scene->height());
    property->setValue(item->pos().y());
    addProperty(property, QLatin1String("ypos"));

    property = variantManager->addProperty(QVariant::Double, tr("Position Z"));
    property->setAttribute(QLatin1String("minimum"), -1000);
    property->setAttribute(QLatin1String("maximum"), 1000);
    property->setValue(item->zValue());
    addProperty(property, QLatin1String("zpos"));

    property = variantManager->addProperty(QVariant::Double, tr("Angle"));
    property->setAttribute(QLatin1String("minimum"), -360.0);
    property->setAttribute(QLatin1String("maximum"), 360.0);
    property->setValue(item->rotation());
    addProperty(property, QLatin1String("angle"));

    if (item->type() == MG_TYPE_IRECT)
    {
        iRect *i = (iRect *)item;

        property = variantManager->addProperty(QVariant::Color, tr("Brush Color"));
        property->setValue(i->brush().color());
        addProperty(property, QLatin1String("brush"));

        property = variantManager->addProperty(QVariant::Color, tr("Pen Color"));
        property->setValue(i->pen().color());
        addProperty(property, QLatin1String("pen"));

        property = variantManager->addProperty(QVariant::SizeF, tr("Size"));
        property->setValue(i->rect().size());
        addProperty(property, QLatin1String("size"));
    }
    else if (item->type() == MG_TYPE_IELLIPSE)
    {
        iEllipse *i = (iEllipse *)item;

        property = variantManager->addProperty(QVariant::Color, tr("Brush Color"));
        property->setValue(i->brush().color());
        addProperty(property, QLatin1String("brush"));

        property = variantManager->addProperty(QVariant::Color, tr("Pen Color"));
        property->setValue(i->pen().color());
        addProperty(property, QLatin1String("pen"));

        property = variantManager->addProperty(QVariant::SizeF, tr("Size"));
        property->setValue(i->rect().size());
        addProperty(property, QLatin1String("size"));
    }
    else if (item->type() == MG_TYPE_ITEXT)
    {
        iText *i = (iText *)item;

        property = variantManager->addProperty(QVariant::Color, tr("Color"));
        property->setValue(i->defaultTextColor());
        addProperty(property, QLatin1String("color"));

        property = variantManager->addProperty(QVariant::String, tr("Text"));
        property->setValue(i->toPlainText());
        addProperty(property, QLatin1String("text"));

        property = variantManager->addProperty(QVariant::Font, tr("Font"));
        property->setValue(i->font());
        addProperty(property, QLatin1String("font"));
    }
    //else if (item->rtti() == QtCanvasItem::Rtti_Line) {
    //        QtCanvasLine *i = (QtCanvasLine *)item;

    //        property = variantManager->addProperty(QVariant::Color, tr("Pen Color"));
    //        property->setValue(i->pen().color());
    //        addProperty(property, QLatin1String("pen"));

    //        property = variantManager->addProperty(QVariant::Point, tr("Vector"));
    //        property->setValue(i->endPoint());
    //        addProperty(property, QLatin1String("endpoint"));
    //    }
}

//选取
void MyGraphics::selectionChanged()
{
    QList<QGraphicsItem*> items = scene->selectedItems();
    if (items.count() == 1) {
        itemClicked(items.at(0));
    }
    else if (items.count() == 0)
    {
        itemClicked(0);
    }
}

//移动item
void MyGraphics::itemMoved(QGraphicsItem *item)
{
    if (item != currentItem)
        return;

    disconnect(variantManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
            this, SLOT(valueChanged(QtProperty *, const QVariant &)));

    variantManager->setValue(idToProperty[QLatin1String("xpos")], item->pos().x());
    variantManager->setValue(idToProperty[QLatin1String("ypos")], item->pos().y());

    connect(variantManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
            this, SLOT(valueChanged(QtProperty *, const QVariant &)));
}

//修改item属性
void MyGraphics::valueChanged(QtProperty *property, const QVariant &value)
{
    if (!propertyToId.contains(property))
        return;

    if (!currentItem)
        return;

    QString id = propertyToId[property];
    if (id == QLatin1String("xpos"))
    {
        currentItem->setX(value.toDouble());
    }
    else if (id == QLatin1String("ypos"))
    {
        currentItem->setY(value.toDouble());
    }
    else if (id == QLatin1String("zpos"))
    {
        currentItem->setZValue(value.toDouble());
    }
    else if (id == QLatin1String("angle"))
    {
        currentItem->setRotation(value.toDouble());
    }
    else if (id == QLatin1String("text"))
    {
        if (currentItem->type() == MG_TYPE_ITEXT)
        {
            QGraphicsTextItem *i = (QGraphicsTextItem *)currentItem;  //textitem
            i->setPlainText(qVariantValue<QString>(value));
        }
    }
    else if (id == QLatin1String("brush"))
    {
        if (currentItem->type() == MG_TYPE_IRECT)
        {
            iRect *i = (iRect *)currentItem;
            QBrush b = i->brush();
            b.setColor(qVariantValue<QColor>(value));
            i->setBrush(b);
        }
        else if(currentItem->type() == MG_TYPE_IELLIPSE)
        {
            iEllipse *i = (iEllipse *)currentItem;
            QBrush b = i->brush();
            b.setColor(qVariantValue<QColor>(value));
            i->setBrush(b);
        }
    }
    else if (id == QLatin1String("color"))
    {
        if (currentItem->type() == MG_TYPE_ITEXT)
        {
            iText *i = (iText *)currentItem;
            i->setDefaultTextColor(qVariantValue<QColor>(value));
        }
    }
    else if (id == QLatin1String("pen"))
    {
        if (currentItem->type() == MG_TYPE_IRECT)
        {
            iRect *i = (iRect *)currentItem;
            QPen p = i->pen();
            p.setColor(qVariantValue<QColor>(value));
            i->setPen(p);
        }
        else if(currentItem->type() == MG_TYPE_IELLIPSE)
        {
            iEllipse *i = (iEllipse *)currentItem;
            QPen p = i->pen();
            p.setColor(qVariantValue<QColor>(value));
            i->setPen(p);
        }
        else if(currentItem->type() == MG_TYPE_ILINE)
        {
        }
    }
    else if (id == QLatin1String("font"))
    {
        if (currentItem->type() == MG_TYPE_ITEXT)
        {
            iText *i = (iText *)currentItem;
            i->setFont(qVariantValue<QFont>(value));
        }
    }
    //    else if (id == QLatin1String("endpoint"))
    //    {
    //        if (currentItem->type() == MG_TYPE_ILINE)
    //        {
    ////            QtCanvasLine *i = (QtCanvasLine *)currentItem;
    ////            QPoint p = qVariantValue<QPoint>(value);
    ////            i->setPoints(i->startPoint().x(), i->startPoint().y(), p.x(), p.y());
    //        }
    //    }
    else if (id == QLatin1String("size"))
    {
        if (currentItem->type() == MG_TYPE_IRECT)
        {
            iRect *i = (iRect *)currentItem;
            QSizeF s = qVariantValue<QSizeF>(value);
            QPointF p = i->rect().topLeft();
            QRectF r(p, s);
            i->setRect(r);
        }
        else if(currentItem->type() == MG_TYPE_IELLIPSE)
        {
            iEllipse *i = (iEllipse *)currentItem;
            QSizeF s = qVariantValue<QSizeF>(value);
            QPointF p = i->rect().topLeft();
            QRectF r(p, s);
            i->setRect(r);
        }
    }
    scene->update();
}

//文本item
void MyGraphics::on_textiTemButton_clicked()
{
    iText *i = new iText;
    i->setPlainText("Text");
    i->setDefaultTextColor(QColor(Qt::blue));
    i->setFont(QFont("宋体", 14));
    scene->addItem(i);
    i->setPos(originP);
    scene->selectedItem(i);
}

//刪除选择的item
void MyGraphics::on_actionDelete_item_triggered()
{
    QList<QGraphicsItem*> items = scene->selectedItems();
    if (items.isEmpty())
        return;
    QListIterator<QGraphicsItem*> i(items);
    while (i.hasNext()) {
#if QT_VERSION >= 0x040600
        QScopedPointer<QGraphicsItem> item(i.next());
        scene->removeItem(item.data());
#else
        QGraphicsItem *item = i.next();
        scene->removeItem(item);
        delete item;
#endif
    }
    itemClicked(0);
    //    setDirty(true);             //窗口标题增加*,用来标记文件已被修改
}

//复制item实现
void MyGraphics::copyItems(const QList<QGraphicsItem*> &items)
{
    QByteArray copiedItems;
    QDataStream out(&copiedItems, QIODevice::WriteOnly);
    writeItems(out, items);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(MimeType, copiedItems);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setMimeData(mimeData);
}

//选取item
void MyGraphics::selectItems(const QSet<QGraphicsItem *> &items)
{
    scene->clearSelection();
    foreach (QGraphicsItem *item, items)
        item->setSelected(true);
}

//复制
void MyGraphics::on_actionCopy_item_triggered()
{
    QList<QGraphicsItem*> items = scene->selectedItems();
    if (items.isEmpty())
        return;
    pasteOffset = OffsetIncrement;
    copyItems(items);
}

//剪切
void MyGraphics::on_actionCut_item_triggered()
{
    QList<QGraphicsItem*> items = scene->selectedItems();
    if (items.isEmpty())
        return;
    copyItems(items);
    QListIterator<QGraphicsItem*> i(items);
    while (i.hasNext()) {
#if QT_VERSION >= 0x040600
        QScopedPointer<QGraphicsItem> item(i.next());
        scene->removeItem(item.data());
#else
        QGraphicsItem *item = i.next();
        scene->removeItem(item);
        delete item;
#endif
    }
    //    setDirty(true);
}

//粘贴
void MyGraphics::on_actionPaste_item_triggered()
{
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData)
        return;

    if (mimeData->hasFormat(MimeType)) {
        QByteArray copiedItems = mimeData->data(MimeType);
        QDataStream in(&copiedItems, QIODevice::ReadOnly);
        readItems(in, pasteOffset, true);
        pasteOffset += OffsetIncrement;
    }
    else
        return;
    //    setDirty(true);
}
