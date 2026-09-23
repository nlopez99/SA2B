#ifndef _E_KUMI_H_
#define _E_KUMI_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

// smode, set from the SET scale x
enum {
  KUMI_FLOAT,     // bobs up and down in place
  KUMI_MOVE,      // chases the player
  KUMI_ELEC,      // floats, electrifies itself at intervals
  KUMI_ELEC_MOVE, // chases, electrifies itself at intervals
  KUMI_GUN,       // gold, shoots bullets
  KUMI_HIDE,      // invisible until a player comes near
  KUMI_SPRING,    // spring on its back
  KUMI_BOMB,      // circles around its home and drops bombs
  KUMI_NUM,
};

void EnemyKumi(task *tp);

#endif // !_E_KUMI_H_
