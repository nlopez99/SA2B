#ifndef _E_LASER_H_
#define _E_LASER_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

void EnemyLaserLoadTexture(void);
void CreateEnemyLaser(task *ptp, Float pow, NJS_VECTOR *spd, NJS_POINT3 *pos);

#endif // !_E_LASER_H_
