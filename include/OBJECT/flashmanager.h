#ifndef _FLASHMANAGER_H_
#define _FLASHMANAGER_H_

#include "EFFECT/ef_particle.h"

Bool FlashParticleExec(particle_info *info, particle *p);
void CreateFlash(Float x, Float y, Float z, Float scl);
void CreateFlashNum(Float x, Float y, Float z, Float scl, Uint8 num);
void CreateFlashNumPhase(Float x, Float y, Float z, Float scl, Uint8 num,
                         Float phase);

#endif // !_FLASHMANAGER_H_
