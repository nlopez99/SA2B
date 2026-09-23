#include "EFFECT/ef_dirt.h"

extern particle_info _rename_dirt_info;

// ^ extern
// v in this file

particle *CreateDirt(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_dirt_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(360.0f * ParticleRandom() - 180.0f);
    p->frame = 0.2f * ParticleRandom();
    p->ang_spd = ParticleDegAng(0.8f * ParticleRandom() - 0.4f);
  }
  return p;
}
