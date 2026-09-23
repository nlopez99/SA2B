#ifndef _E_JET_H_
#define _E_JET_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

void EnemyJetLoadTexture(void);
void CreateEnemyJet(task *ptp, Sint32 smode, NJS_POINT3 *pos,
                    NJS_VECTOR *spd);

#endif // !_E_JET_H_
