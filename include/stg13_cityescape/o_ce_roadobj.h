#ifndef __O_CE_ROADOBJ_H_
#define __O_CE_ROADOBJ_H_

#include "sa2b_types.h"
#include "samt/ninja/njcommon.h"
#include "samt/sonic/task.h"

// table-driven scenery; twp->ang.x picks the entry, the entry picks the displayer
Sint32 GetRoadObjNum(void);
Sint32 GetRoadObjInfo(Sint32 idx, NJS_TEXLIST **tex, void **obj);

void ObjectRoadObj(task *tp);
void ObjectSigns(task *tp);

#endif // !__O_CE_ROADOBJ_H_
