#include "EFFECT/ef_snowpuff.h"

extern particle_info _rename_snowpuff_info;
extern particle_info _rename_snowpuff_small_info;
extern particle_info _rename_snowpuff_color_info;
extern Uint32 _rename_snowpuff_color;

// ^ extern
// v in this file

particle *CreateSnowPuff(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_snowpuff_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(80.0f * ParticleRandom() - 40.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->phase.f = 16.0f * ParticleRandom();
    p->frame += ParticleRandom();
  }
  return p;
}

// triangle wave: 0..n..0 with a period of 2n
static Sint32 SnowPuffTriangleWave(Uint32 x, Uint32 n) {
  Uint32 period = n << 1;
  Sint32 r = x % period;
  if (r >= n) {
    r = period - r;
  }
  return r;
}

Bool SnowPuffExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = ParticleDegAng(SnowPuffTriangleWave(p->phase.f, 16) - 8) + 0x4000;
  p->phase.f += 0.26666668f;
  if ((Sint16)p->frame >= info->frame_num) {
    p->frame = info->frame_num - 1;
    p->argb = (p->argb & 0x00FFFFFF) | ((p->argb >> 1) & 0xFF000000);
    if (p->argb & 0xFF000000) {
      return TRUE;
    }
    return FALSE;
  }
  return TRUE;
}

particle *CreateSnowPuffSmall(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_snowpuff_small_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(80.0f * ParticleRandom() - 40.0f);
    p->ang_spd = ParticleDegAng(1.3f * ParticleRandom() - 0.6f);
    p->phase.f = 16.0f * ParticleRandom();
    p->frame += ParticleRandom();
  }
  return p;
}

Bool SnowPuffSmallExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = ParticleDegAng(SnowPuffTriangleWave(p->phase.f, 16) - 8) + 0x4000;
  p->phase.f += 0.26666668f;
  if ((Sint16)p->frame >= info->frame_num) {
    p->frame = info->frame_num - 1;
    return FALSE;
  }
  return TRUE;
}

Bool SnowPuffColorExec(particle_info *info, particle *p) {
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

particle *CreateSnowPuffColor(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                              Float phase) {
  particle *p = fn_80032B78(&_rename_snowpuff_color_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(80.0f * ParticleRandom() - 40.0f);
    p->ang_spd = ParticleDegAng(0.2f * ParticleRandom() - 0.1f);
    p->phase.f = phase;
    p->frame += ParticleRandom();
    p->argb = _rename_snowpuff_color;
  }
  return p;
}
