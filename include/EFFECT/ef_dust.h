#ifndef _EF_DUST_H_
#define _EF_DUST_H_

#include "EFFECT/ef_particle.h"
#include "samt/sonic/task.h"

particle *CreateDust(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
particle *CreateDustShort(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool DustExec(particle_info *info, particle *p);
Bool DustShortExec(particle_info *info, particle *p);
void CreateDustRing(NJS_POINT3 *pos, NJS_VECTOR *spd, Float r, Float scl,
                    Sint32 num);
void CreateDustRingShort(NJS_POINT3 *pos, NJS_VECTOR *spd, Float r, Float scl,
                         Sint32 num);
void CreateBlackSmokeGenerator(Sint32 pno, Sint32 num);
task *CreateBrokenDownSmoke(Sint32 pno, task *ptp);

#endif // !_EF_DUST_H_
