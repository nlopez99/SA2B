#include "EFFECT/ef_rocketthrust.h"

#include "samt/ninja/njmatrix.h"

extern particle_info _rename_rocketthrust_info;
extern particle_info _rename_rocketthrust_ground_info;

// ^ extern
// v in this file

particle *CreateRocketThrust(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_rocketthrust_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang2 = ParticleDegAng(10.0f * ParticleRandom() - 5.0f) + 0x4000;
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->frame += ParticleRandom();
  }
  return p;
}

Bool RocketThrustExec(particle_info *info, particle *p) {
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
    p->frame = info->frame_num - 1;
    return FALSE;
  }
  return TRUE;
}

// phase.f holds the height of the ground the thrust spreads out on
particle *CreateRocketThrustGround(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                                   Float ground_y) {
  particle *p = fn_80032B78(&_rename_rocketthrust_ground_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->ang2 = ParticleDegAng(10.0f * ParticleRandom() - 5.0f) + 0x4000;
    p->phase.f = ground_y;
    p->frame += ParticleRandom();
  }
  return p;
}

Bool RocketThrustGroundExec(particle_info *info, particle *p) {
  Float len;
  Float s;

  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  // slow the fall down close to the ground, keeping most of the speed
  if (p->spd.y < 0.0f && p->pos.y < 10.0f + p->phase.f) {
    len = njScalor(&p->spd);
    p->spd.y *= 0.94f;
    s = 0.99f * len / njScalor(&p->spd);
    p->spd.x *= s;
    p->spd.y *= s;
    p->spd.z *= s;
  }
  // on the ground: turn all of the speed sideways
  if (p->pos.y < p->phase.f) {
    len = njScalor(&p->spd);
    p->spd.y = 0.0f;
    s = njScalor(&p->spd);
    if (s < 0.001f) {
      p->spd.x = 0.1f * (ParticleRandom() - 0.5f);
      p->spd.z = 0.1f * (ParticleRandom() - 0.5f);
      s = njScalor(&p->spd);
    }
    s = len / s;
    p->spd.x *= s;
    p->spd.y *= s;
    p->spd.z *= s;
    p->pos.y = p->phase.f;
  }
  if ((Sint16)p->frame >= info->frame_num) {
    p->frame = info->frame_num - 1;
    return FALSE;
  }
  return TRUE;
}
