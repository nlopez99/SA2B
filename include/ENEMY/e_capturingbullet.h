#ifndef _E_CAPTURINGBULLET_H_
#define _E_CAPTURINGBULLET_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

void EnemyCapturingBulletLoadTexture(void);
void CreateEnemyCapturingBullet(task *tp, NJS_VECTOR *spd, NJS_POINT3 *pos);

#endif // !_E_CAPTURINGBULLET_H_
