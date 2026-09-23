#include "EFFECT/ef_dust.h"

#include "samt/ninja/njmatrix.h"

extern particle_info _rename_dust_info;
extern particle_info _rename_dust_short_info;

// ^ extern
// v in this file

particle *CreateDust(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_dust_info);
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
  particle *p = fn_80032B78(&_rename_dust_short_info);
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

// throws num dust particles outwards from a circle of radius r around pos
void CreateDustRing(NJS_POINT3 *pos, NJS_VECTOR *spd, Float r, Float scl,
                    Sint32 num) {
  NJS_VECTOR v = {0.0f, 0.03f, 0.05f};
  NJS_POINT3 p = {0.0f, 0.0f, 0.0f};
  NJS_POINT3 ppos;
  NJS_VECTOR pspd;
  Uint8 i;
  Angle step = 0x10000 / num;
  Angle ang = 0;

  p.z = r;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  v.y *= scl;
  v.z *= scl;
  for (i = 0; i < num; i++) {
    njRotateY(NULL, ang);
    njCalcPoint(NULL, &p, &ppos);
    njCalcVector(NULL, &v, &pspd);
    ppos.x += pos->x;
    ppos.y += pos->y;
    ppos.z += pos->z;
    pspd.x += spd->x;
    pspd.y += spd->y;
    pspd.z += spd->z;
    CreateDust(&ppos, &pspd, scl);
    ang += step;
  }
  njPopMatrixEx();
}
