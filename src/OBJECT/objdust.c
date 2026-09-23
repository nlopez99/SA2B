#include "OBJECT/objdust.h"

#include "samt/ninja/njmatrix.h"

extern particle_info _rename_smoke_info;
extern particle_info _rename_objdust1_info;
extern particle_info _rename_objdust2_info;
extern particle_info _rename_objdust3_info;

// the direction ObjDust3's gravity pulls in, kept by the stage
extern Float lbl_801E5624[3];

// ^ extern
// v in this file

particle *_rename_CreateSmoke(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_smoke_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    // where the puff starts in the 16-step swing its exec animates
    p->phase.f = 16.0f * ParticleRandom();
    p->frame += ParticleRandom();
  }
  return p;
}

particle *_rename_CreateObjDust1(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_objdust1_info);
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

particle *CreateObjDust2(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                         Float phase) {
  particle *p = fn_80032B78(&_rename_objdust2_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->phase.f = phase;
    p->frame += ParticleRandom();
  }
  return p;
}

particle *CreateObjDust3(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                         Float phase) {
  particle *p = fn_80032B78(&_rename_objdust3_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->phase.f = phase;
    p->frame += ParticleRandom();
  }
  return p;
}

// n counted up and back down again over 2*half steps
Uint32 PingPongFrame(Uint32 n, Uint32 half) {
  Uint32 len = half * 2;
  Uint32 m = n % len;

  if (m >= half) {
    m = len - m;
  }
  return m;
}

Bool SmokeExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  // rocks the puff a quarter turn either side of level as its phase swings
  p->ang2 = ParticleDegAng((Sint32)PingPongFrame((Uint32)p->phase.f, 16) - 8) +
            0x4000;
  p->phase.f += 0.26666668f;
  if ((Sint16)p->frame >= info->frame_num) {
    // out of frames: hold the last one and halve the alpha until it is gone
    p->frame = (Float)(info->frame_num - 1);
    p->argb = (p->argb & 0x00FFFFFF) | ((p->argb >> 1) & 0xFF000000);
    if (p->argb & 0xFF000000) {
      return TRUE;
    }
    return FALSE;
  }
  return TRUE;
}

Bool ObjDust1Exec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = ParticleDegAng((Sint32)PingPongFrame((Uint32)p->phase.f, 16) - 8) +
            0x4000;
  p->phase.f += 0.26666668f;
  if ((Sint16)p->frame >= info->frame_num) {
    p->frame = (Float)(info->frame_num - 1);
    return FALSE;
  }
  return TRUE;
}

Bool ObjDust2Exec(particle_info *info, particle *p) {
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
    p->frame = (Float)(info->frame_num - 1);
    return FALSE;
  }
  return TRUE;
}

Bool ObjDust3Exec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  // gravity pulls along the stage's vector
  p->spd.x = p->spd.x * info->friction - info->gravity * lbl_801E5624[0];
  p->spd.y = p->spd.y * info->friction - info->gravity * lbl_801E5624[1];
  p->spd.z = p->spd.z * info->friction - info->gravity * lbl_801E5624[2];
  p->frame += info->frame_spd;
  if ((Sint16)p->frame >= info->frame_num) {
    p->frame = (Float)(info->frame_num - 1);
    return FALSE;
  }
  return TRUE;
}

void _rename_CreateBoxDust(NJS_POINT3 *pos, Float x, Float z, Angle3 *ang,
                           Float scl, Float density, Float spd) {
  NJS_POINT3 p;
  NJS_POINT3 dust_pos;
  NJS_VECTOR dust_spd;
  Sint32 i;
  Sint32 nx;
  Sint32 nz;
  Float t;

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(pos);
  njRotateY(NULL, ang->y);
  njRotateZ(NULL, ang->z);
  density = 10.0f / density;
  nx = (Sint32)(2.0f * x / density);
  nz = (Sint32)(2.0f * z / density);
  p.y = 0.0f;
  // edges parallel to x, then to z; each puff flies away from the box centre
  for (i = 0; i < nx; i++) {
    p.x = (Float)i * density - x;
    p.z = z;
    njCalcPoint(NULL, &p, &dust_pos);
    njCalcVector(NULL, &p, &dust_spd);
    t = spd / njScalor(&dust_spd);
    dust_spd.x *= t;
    dust_spd.y *= t;
    dust_spd.z *= t;
    _rename_CreateObjDust1(&dust_pos, &dust_spd, scl);
    p.z = -z;
    njCalcPoint(NULL, &p, &dust_pos);
    njCalcVector(NULL, &p, &dust_spd);
    t = spd / njScalor(&dust_spd);
    dust_spd.x *= t;
    dust_spd.y *= t;
    dust_spd.z *= t;
    _rename_CreateObjDust1(&dust_pos, &dust_spd, scl);
  }
  for (i = 0; i < nz; i++) {
    p.z = (Float)i * density - z;
    p.x = x;
    njCalcPoint(NULL, &p, &dust_pos);
    njCalcVector(NULL, &p, &dust_spd);
    t = spd / njScalor(&dust_spd);
    dust_spd.x *= t;
    dust_spd.y *= t;
    dust_spd.z *= t;
    _rename_CreateObjDust1(&dust_pos, &dust_spd, scl);
    p.x = -x;
    njCalcPoint(NULL, &p, &dust_pos);
    njCalcVector(NULL, &p, &dust_spd);
    t = spd / njScalor(&dust_spd);
    dust_spd.x *= t;
    dust_spd.y *= t;
    dust_spd.z *= t;
    _rename_CreateObjDust1(&dust_pos, &dust_spd, scl);
  }
  njPopMatrixEx();
}

void _rename_CreateBreakSmoke(NJS_POINT3 *pos, Float rad, Float spd,
                              Float scl) {
  Sint32 stack_pad[2]; // unused
  NJS_POINT3 p;
  NJS_POINT3 dust_pos;
  NJS_VECTOR v;
  NJS_VECTOR dust_spd;

  v.x = 0.0f;
  v.y = spd;
  v.z = 0.0f;
  p.x = 0.0f;
  p.y = rad;
  p.z = 0.0f;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(pos);
  // anywhere round the compass, and up to a third of a turn off vertical
  njRotateY(NULL, ParticleDegAng(2.0f * (360.0f * ParticleRandom())));
  njRotateX(NULL, ParticleDegAng(120.0f * (2.0f * (ParticleRandom() - 0.5f))));
  njCalcPoint(NULL, &p, &dust_pos);
  njCalcVector(NULL, &v, &dust_spd);
  _rename_CreateObjDust1(&dust_pos, &dust_spd, scl);
  njPopMatrixEx();
}

// the ring's outward speed and its unrotated seed point, defined below
extern const NJS_VECTOR dust_circle_spd;
extern const NJS_POINT3 dust_circle_pos;

void _rename_PutDustCircle(NJS_POINT3 *pos, Float rad, Float scl, Sint32 num) {
  NJS_VECTOR v = dust_circle_spd;
  NJS_POINT3 p = dust_circle_pos;
  NJS_POINT3 dust_pos;
  NJS_VECTOR dust_spd;
  Uint8 i;
  Angle step = 0x10000 / num;
  Angle ang = (Angle)(65536.0f * ParticleRandom());

  p.z = rad;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  v.y *= scl;
  v.z *= scl;
  for (i = 0; i < num; i++) {
    njRotateY(NULL, ang);
    njCalcPoint(NULL, &p, &dust_pos);
    njCalcVector(NULL, &v, &dust_spd);
    dust_pos.x += pos->x;
    dust_pos.y += pos->y;
    dust_pos.z += pos->z;
    _rename_CreateObjDust1(&dust_pos, &dust_spd, scl);
    ang += step;
  }
  njPopMatrixEx();
}

const NJS_VECTOR dust_circle_spd = {0.0f, 0.03f, 0.2f};
const NJS_POINT3 dust_circle_pos = {0.0f, 0.0f, 0.0f};
