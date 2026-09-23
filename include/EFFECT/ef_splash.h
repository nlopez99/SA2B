#ifndef _EF_SPLASH_H_
#define _EF_SPLASH_H_

#include "EFFECT/ef_particle.h"
#include "samt/sonic/task.h"

particle *CreateSplash(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
particle *CreateSplashDrop(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool SplashExec(particle_info *info, particle *p);
task *CreatePlayerSplash(Sint32 pno, task *ptp);

#endif // !_EF_SPLASH_H_
