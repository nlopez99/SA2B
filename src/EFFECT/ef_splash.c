#include "EFFECT/ef_splash.h"

extern particle_info _rename_splash_info;
extern particle_info _rename_splash_drop_info;

// ^ extern
// v in this file

particle *CreateSplash(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_splash_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(80.0f * ParticleRandom() - 40.0f);
    p->ang_spd = ParticleDegAng(0.5f * ParticleRandom() - 0.25f);
    p->frame = 0.0f;
  }
  return p;
}

particle *CreateSplashDrop(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_splash_drop_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(80.0f * ParticleRandom() - 40.0f);
    p->ang_spd = ParticleDegAng(0.5f * ParticleRandom() - 0.25f);
    p->frame = 2.0f * ParticleRandom();
    p->argb = 0xFFA090FF;
  }
  return p;
}

Bool SplashExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  if ((Sint16)p->frame >= info->frame_num) {
    return FALSE;
  }
  return TRUE;
}
