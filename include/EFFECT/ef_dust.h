#ifndef _EF_DUST_H_
#define _EF_DUST_H_

#include "EFFECT/ef_particle.h"

particle *CreateDust(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
particle *CreateDustShort(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool DustExec(particle_info *info, particle *p);
Bool DustShortExec(particle_info *info, particle *p);
void CreateDustRing(NJS_POINT3 *pos, NJS_VECTOR *spd, Float r, Float scl,
                    Sint32 num);

#endif // !_EF_DUST_H_
