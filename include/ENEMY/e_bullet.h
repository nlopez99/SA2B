#ifndef _E_BULLET_H_
#define _E_BULLET_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

void EnemyBulletLoadTexture(void);
void CreateEnemyBullet(task *ptp, Sint32 smode, NJS_VECTOR *spd,
                       NJS_POINT3 *pos);

#endif // !_E_BULLET_H_
