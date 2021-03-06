/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>

#include <QGraphicsItem>
#include "mygraphicsscene.h"
#include "global_types.h"

//! [0]
class MoveCommand : public QUndoCommand
{
public:
    enum { Id = 1234 };

    MoveCommand(QGraphicsItem *graphicsItem, const QPointF &oldPos,
                QUndoCommand *parent = 0); //QMap<QGraphicsItem *, QPointF>

    MoveCommand(QMap<QGraphicsItem *, QPointF> itemsnewpos,
                QMap<QGraphicsItem *, QPointF> itemsoldpos,
                QUndoCommand *parent = 0);

    void undo();
    void redo();
    bool mergeWith(const QUndoCommand *command);
    int id() const { return Id; }

private:
    QGraphicsItem *myGraphicsItem;
    QPointF myOldPos;
    QPointF newPos;

    QMap<QGraphicsItem *, QPointF> myItemsPos;
    QList<QPointF> myOldPosList;
    QList<QPointF> newPosList;
};
//! [0]

//! [1]
class DeleteCommand : public QUndoCommand
{
public:
    DeleteCommand(int etype, MyGraphicsScene *scene, QUndoCommand *parent = 0);

    void undo();
    void redo();

private:
    QGraphicsItem *myDiagramItem;
    QList<QGraphicsItem *> myDiagramItemList;
    MyGraphicsScene *myGraphicsScene;
    QString undoActinText;
    QString redoActinText;
};
//! [1]

//! [2]
class AddCommand : public QUndoCommand
{
public:
    AddCommand(QString itemType, MyGraphicsScene *graphicsScene, qreal value,
               QUndoCommand *parent = 0);
    ~AddCommand();

    void undo();
    void redo();

private:
    QGraphicsItem *myDiagramItem;
    MyGraphicsScene *myGraphicsScene;
    QPointF initialPosition;
    qreal z_value;
};
//! [2]

//! [3]
class PasteCommand : public QUndoCommand
{
public:
    PasteCommand(QList<QGraphicsItem*> pasteList,
                 MyGraphicsScene *graphicsScene,
                 bool select,
                 qreal maxzValue,
                 QUndoCommand *parent = 0);
    ~PasteCommand();

    void undo();
    void redo();

private:
    QList<QGraphicsItem*> itemList;
    MyGraphicsScene *myGraphicsScene;
    bool isSeletctItem;
    qreal maxZValue;

};
//! [3]

QString createCommandString(QGraphicsItem *item, const QPointF &point);

#endif
