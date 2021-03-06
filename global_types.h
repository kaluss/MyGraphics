#ifndef GLOBAL_TYPES_H
#define GLOBAL_TYPES_H

#include <QtGlobal>
#include <QString>

#define Qt_USER_TYPE_BASE 65536

enum item_types
{
    MG_TYPE_IRECT = Qt_USER_TYPE_BASE+50,
    MG_TYPE_IELLIPSE,
    MG_TYPE_ITEXT,
    MG_TYPE_ILINE
};
typedef item_types ITEMTYPES;


enum EditType { Cut=0, Delete };
#endif // GLOBAL_TYPES_H
