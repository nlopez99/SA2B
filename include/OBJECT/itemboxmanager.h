#ifndef _ITEMBOXMANAGER_H_
#define _ITEMBOXMANAGER_H_

#include "sa2b_types.h"
#include "samt/ninja/njcommon.h"
#include "samt/sonic/task.h"

Sint32 _rename_ItemIconDraw(Sint32 kind, NJS_POINT3 *pos, Angle ang, Float scl);
void _rename_ItemGetDisplaySet(Sint32 pno, Sint32 kind);

#endif // !_ITEMBOXMANAGER_H_
