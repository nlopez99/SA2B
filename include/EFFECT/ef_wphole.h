#ifndef _EF_WPHOLE_H_
#define _EF_WPHOLE_H_

#include "sa2b_types.h"
#include "samt/ninja/njcommon.h"
#include "samt/sonic/task.h"

#include "EFFECT/ef_particle.h"

task *CreateWpHole(NJS_POINT3 *pos, Float r, Sint32 num, Uint8 flag, Float scl,
                   task **owner);

particle *CreateWpHoleParticle(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);

#endif // !_EF_WPHOLE_H_
