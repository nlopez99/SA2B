#ifndef _EF_EXPLOSION_H_
#define _EF_EXPLOSION_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

// the GROUND kinds are drawn raised above pos, the others centred on it
enum {
  EXPLOSION_L_GROUND,
  EXPLOSION_L,
  EXPLOSION_LL_GROUND,
  EXPLOSION_LL,
  EXPLOSION_S_GROUND,
  EXPLOSION_S,
  EXPLOSION_M_GROUND,
  EXPLOSION_M,
  EXPLOSION_SS_GROUND,
  EXPLOSION_SS,
};

void LoadExplosionTexture(void);
void CreateExplosion(Sint32 kind, NJS_POINT3 *pos);

#endif // !_EF_EXPLOSION_H_
