#ifndef _EF_SPLASH_H_
#define _EF_SPLASH_H_

#include "EFFECT/ef_particle.h"

particle *CreateSplash(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
particle *CreateSplashDrop(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool SplashExec(particle_info *info, particle *p);

#endif // !_EF_SPLASH_H_
