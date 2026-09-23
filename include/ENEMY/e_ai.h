#ifndef _E_AI_H_
#define _E_AI_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

// smode, set from the SET scale x
enum {
  AI_GUN,          // hovers in place, shoots bullets
  AI_GUN_CHASE,    // chases the player, shoots bullets
  AI_LASER,        // hovers in place, shoots a laser
  AI_LASER_CHASE,  // chases the player, shoots a laser
  AI_CAPTURE,      // hovers in place, shoots a capturing bullet
  AI_BIG,          // the big model, laser, cannot be jumped on
  AI_HIDE_GUN,     // drops in and turns into AI_GUN
  AI_HIDE_LASER,   // drops in and turns into AI_LASER
  AI_HIDE_CAPTURE, // drops in and turns into AI_CAPTURE
  AI_STAND,        // scenery, never attacks
  AI_NUM,
};

void EnemyAi(task *tp);

#endif // !_E_AI_H_
