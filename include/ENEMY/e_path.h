#ifndef _E_PATH_H_
#define _E_PATH_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

// smode, the low nibble of the SET angle x: the skin and what it carries
enum {
  PATH_KUMI,       // pathkumi, shoots bullets
  PATH_KUMI_BOMB,  // pathkumi, drops bombs
  PATH_KYOKO,      // pathkyoko, shoots bullets
  PATH_KYOKO_BEAM, // pathkyoko, fires a beam
  PATH_NUM,
};

// SET object executor
void EnemyPath(task *tp);

#endif // !_E_PATH_H_
