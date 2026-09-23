#ifndef _E_LIGHT_H_
#define _E_LIGHT_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

void EnemyLightLoadTexture(void);
void CreateEnemyLight(task *ptp, Sint32 smode, NJS_POINT3 *pos);

#endif // !_E_LIGHT_H_
