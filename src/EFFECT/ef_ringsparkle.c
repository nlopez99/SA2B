#include "EFFECT/ef_ringsparkle.h"

#include "samt/ninja/njmath.h"
#include "stdlib.h"

extern particle_info _rename_ringsparkle_info;
extern particle_info _rename_ringsparkle_fast_info;

// ^ extern
// v in this file

#define Random() (0.000030517578f * (Float)rand())
#define DegAng(n) ((Angle)(182.04445f * (n)))

particle *CreateRingSparkle(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_ringsparkle_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = DegAng(15.0f * Random() - 7.5f);
    p->ang_spd = DegAng(Random() - 0.5f);
    p->timer = 0;
    p->frame = 0.5f * Random();
  }
  return p;
}

Bool RingSparkleExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = DegAng(10.0f * njSin(p->timer)) + 0x4000;
  p->timer += 0x222;
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

particle *_rename_MakeParticle(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_ringsparkle_fast_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = DegAng(10.0f * Random() - 5.0f);
    p->ang_spd = DegAng(Random() - 0.5f);
    p->timer = 0;
    p->frame = 0.5f * Random();
  }
  return p;
}

Bool RingSparkleFastExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = DegAng(10.0f * njSin(p->timer)) + 0x4000;
  p->timer += 0x222;
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
