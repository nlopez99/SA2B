#ifndef _O_ITEMBOX_H_
#define _O_ITEMBOX_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

// expanding explosion sphere, kept in a list by the ExpMan task
typedef struct expwk // sizeof=0x18
{
  /* 0x00 */ NJS_POINT3 pos;
  /* 0x0C */ Float r;
  /* 0x10 */ Float r_max;
  /* 0x14 */ struct expwk *next;
} expwk;

// ObjectItemBox, also the exec of the goal ring's child task
void _rename_GoalRingChildExec(task *tp);

void _rename_GiveItemP(Sint32 pno, Sint32 kind);
expwk *_rename_ItemBombSet(NJS_POINT3 *pos);
expwk *ItemBombSetRange(NJS_POINT3 *pos, Float range);

#endif // !_O_ITEMBOX_H_
