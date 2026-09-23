#ifndef _O_ROCKETMISSILE_H_
#define _O_ROCKETMISSILE_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

typedef struct rocketmissiletarget // sizeof=0x8
{
  /* 0x00 */ Sint32 id;
  /* 0x04 */ NJS_POINT3 **pos;
} rocketmissiletarget;

rocketmissiletarget *RocketMissileSetTarget(Sint32 id, NJS_POINT3 **pos);
rocketmissiletarget *RocketMissileFreeTarget(NJS_POINT3 **pos);
void ObjectRocketMissile(task *tp);

#endif // !_O_ROCKETMISSILE_H_
