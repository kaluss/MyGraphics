#include "GluGraphicsView.h"
#include <QApplication>
#include <QDebug>

GluGraphicsView::GluGraphicsView(QWidget *parent) :
    QGraphicsView(parent)
{
    setDragMode(RubberBandDrag);            //可多选
    setBackgroundBrush(QBrush(Qt::lightGray));
    setRenderHints(QPainter::Antialiasing|
                   QPainter::TextAntialiasing);
//    setBackgroundBrush(QColor(Qt::black));
    setBackgroundBrush(QBrush(Qt::white));
}

void GluGraphicsView::zoomIn()
{
    QTransform transform;
    qreal tmp;
    tmp = this->transform().m22()+0.1;
    if (tmp > 3.0)         //缩放范围0.1-3.0
        return;
    transform.scale(tmp, tmp);
    setTransform(transform);
}

void GluGraphicsView::zoomOut()
{
    QTransform transform;
    qreal tmp;
    tmp = this->transform().m22()-0.1;
    if (tmp < 0.1)         //缩放范围0.1-3.0
        return;
    transform.scale(tmp, tmp);
    setTransform(transform);
}

void GluGraphicsView::restore()
{
    resetTransform();           //重置缩放
}

void GluGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers () == Qt::ControlModifier)
    {
        //        scaleBy(std::pow(4.0 / 3.0, (-event->delta() / 240.0))); //原方法
        QTransform transform;
        qreal tmp;
        tmp = (event->delta()>0 ? this->transform().m22()+0.1 : this->transform().m22()-0.1);
        if (tmp > 3.0 || tmp < 0.1)         //缩放范围0.1-3.0
            return;
        transform.scale(tmp, tmp);
        setTransform(transform);
    }
    else {
        QGraphicsView::wheelEvent(event);
    }
}

void GluGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button()==Qt::MidButton)
    {
//        resetCachedContent () ;
//        resetMatrix () ;
        resetTransform();           //重置缩放
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void GluGraphicsView::scaleBy(double factor)
{
    scale(factor, factor);
}
