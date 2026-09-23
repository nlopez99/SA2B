#ifndef _EF_FLAME_H_
#define _EF_FLAME_H_

#include "EFFECT/ef_particle.h"

particle *CreateFlame(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool FlameExec(particle_info *info, particle *p);

#endif // !_EF_FLAME_H_
