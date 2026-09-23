#ifndef _E_BOMB_H_
#define _E_BOMB_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

// kind
enum {
  ENEMYBOMB_NORMAL, // bounces, then counts down
  ENEMYBOMB_TOUCH,  // explodes on whatever it touches
  ENEMYBOMB_BIG,
  ENEMYBOMB_PILLAR, // like TOUCH, and the explosion leaves a pillar
  ENEMYBOMB_RING,   // turns into a ring most of the time
};

// type
enum {
  ENEMYBOMB_THROW, // thrown at the nearest player
  ENEMYBOMB_DROP,
  ENEMYBOMB_HOLD,  // grows in the parent's hands until its flag 0x8000 clears
  ENEMYBOMB_EXPLODE,
};

// also drawn by other objects
extern NJS_TEXLIST e_bomb_texlist;
extern NJS_CNK_MODEL e_bomb_model;

void EnemyBombLoadTexture(void);
task *CreateEnemyBomb(task *ptp, Sint32 kind, Sint32 type, NJS_POINT3 *pos);

#endif // !_E_BOMB_H_
