#ifndef _EF_SNOWPUFF_H_
#define _EF_SNOWPUFF_H_

#include "EFFECT/ef_particle.h"

particle *CreateSnowPuff(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool SnowPuffExec(particle_info *info, particle *p);
particle *CreateSnowPuffSmall(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool SnowPuffSmallExec(particle_info *info, particle *p);
Bool SnowPuffColorExec(particle_info *info, particle *p);
particle *CreateSnowPuffColor(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                              Float phase);

#endif // !_EF_SNOWPUFF_H_
