#ifndef _EF_RINGSPARKLE_H_
#define _EF_RINGSPARKLE_H_

#include "EFFECT/ef_particle.h"

particle *CreateRingSparkle(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool RingSparkleExec(particle_info *info, particle *p);
particle *_rename_MakeParticle(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool RingSparkleFastExec(particle_info *info, particle *p);

#endif // !_EF_RINGSPARKLE_H_
