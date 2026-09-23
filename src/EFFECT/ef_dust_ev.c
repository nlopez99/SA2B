#include "EFFECT/ef_dust.h"

// older revision of ef_dust.c: same code, only these five functions, and
// type 1 descriptors with no init and their own friction and scale rates

extern NJS_TEXLIST _rename_dust_texlist;

// ^ extern
// v in this file

particle_info dust_info = {
    1,     &_rename_dust_texlist, 0, 8, 0.11f, 0.93f, 0.0065f,
    0.05f, DustExec,              75000.0f,
};

particle_info dust_short_info = {
    1,     &_rename_dust_texlist, 0, 8, 0.2f, 0.92f, 0.0065f,
    0.05f, DustShortExec,         22500.0f,
};

particle *CreateDust(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&dust_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->phase.f = 16.0f * ParticleRandom();
    p->frame += ParticleRandom();
  }
  return p;
}

particle *CreateDustShort(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&dust_short_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->phase.f = 16.0f * ParticleRandom();
    p->frame += ParticleRandom();
  }
  return p;
}

// triangle wave: 0..n..0 with a period of 2n
static Sint32 DustTriangleWave(Uint32 x, Uint32 n) {
  Uint32 period = n << 1;
  Sint32 r = x % period;
  if (r >= n) {
    r = period - r;
  }
  return r;
}

Bool DustExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = ParticleDegAng(DustTriangleWave(p->phase.f, 16) - 8) + 0x4000;
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

Bool DustShortExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = ParticleDegAng(DustTriangleWave(p->phase.f, 16) - 8) + 0x4000;
  p->phase.f += 0.26666668f;
  if ((Sint16)p->frame >= info->frame_num) {
    p->frame = info->frame_num - 1;
    return FALSE;
  }
  return TRUE;
}
