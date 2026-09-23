#include "OBJECT/o_effobj0.h"

#include "EFFECT/ef_dirt.h"
#include "EFFECT/ef_dust.h"
#include "EFFECT/ef_kiran.h"
#include "EFFECT/ef_lnspark.h"
#include "EFFECT/ef_ringsparkle.h"
#include "EFFECT/ef_rocketthrust.h"
#include "EFFECT/ef_snowpuff.h"
#include "EFFECT/ef_splash.h"
#include "EFFECT/ef_spspark.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/player.h"
#include "set.h"

// low byte of ang.x picks the effect, the next byte is the chance per frame;
// scl x and y give the spread, z the particle size

extern void fn_800E29BC(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void _rename_CreateObjDust1(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void CreateFlame(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void _rename_CreateSmoke(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void _rename_CreateSikake(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void CreateFlash(Float x, Float y, Float z, Float scl);

extern NJS_TEXLIST lbl_803AD5A0;
extern NJS_TEXLIST _rename_splash_texlist;
extern NJS_TEXLIST _rename_kiran_texlist;
extern NJS_TEXLIST _rename_ring_texlist;
extern NJS_TEXLIST _rename_rocketthrust_texlist;
extern NJS_TEXLIST _rename_dust_texlist;
extern NJS_TEXLIST _rename_snowpuff_texlist;
extern NJS_TEXLIST _rename_dirt_texlist;
extern NJS_TEXLIST _rename_flame_texlist;
extern NJS_TEXLIST _rename_flash_texlist;
extern NJS_TEXLIST _rename_sikake_texlist;

// ^ extern
// v in this file

static void ObjectEffObj0Dest(task *tp);
static void ObjectEffObj0Exec(task *tp);

// texlists of the effects below
NJS_TEXLIST *effobj0_texlists[] = {
    &_rename_splash_texlist,
    &lbl_803AD5A0,
    &_rename_kiran_texlist,
    &_rename_ring_texlist,
    &_rename_rocketthrust_texlist,
    &_rename_dust_texlist,
    &_rename_snowpuff_texlist,
    &_rename_dirt_texlist,
    &spspark_texlist,
    &_rename_flame_texlist,
    &_rename_flash_texlist,
    &_rename_sikake_texlist,
    NULL,
};

void ObjectEffObj0(task *tp) {
  if (CheckRangeOut(tp)) {
    return;
  }
  tp->exec = ObjectEffObj0Exec;
  tp->dest = ObjectEffObj0Dest;
}

static void ObjectEffObj0Dest(task *tp) {}

static void ObjectEffObj0Exec(task *tp) {
  taskwk *twp = tp->twp;
  Float rate;
  Float scl;
  NJS_VECTOR v;
  Sint32 kind;

  if (CheckRangeOut(tp)) {
    return;
  }
  rate = ((twp->ang.x >> 8) & 0xFF) / 255.0f;
  if (lbl_801CC168._37) {
    return;
  }
  // negated '<' to match
  if (!(njRandom() < rate)) {
    return;
  }
  v.y = twp->scl.x + twp->scl.y * (njRandom() - 0.5f);
  v.x = twp->scl.y * (njRandom() - 0.5f);
  v.z = twp->scl.y * (njRandom() - 0.5f);

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateV(NULL, &twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njCalcVector(NULL, &v, &v);
  njPopMatrix(1);

  scl = 0.05f + twp->scl.z;
  if (scl > 20.0f) {
    scl = 20.0f;
  }

  kind = twp->ang.x & 0xFF;
  switch (kind) {
  case 0:
    CreateSplash(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 1:
    fn_800E29BC(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 2:
    CreateKiran(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 3:
    CreateDustShort(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 4:
    CreateRingSparkle(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 5:
    CreateRocketThrust(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 6:
    _rename_CreateObjDust1(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 7:
    CreateSnowPuffSmall(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 8:
    CreateDirt(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 9:
    CreateLnSpark(&twp->pos, &v, 0.0f, twp->pos.y - 0.1f);
    break;
  case 10:
    CreateSpSpark(&twp->pos, &v, 0.0f, twp->pos.y - 0.1f);
    break;
  case 11:
    CreateFlame(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 12:
    _rename_CreateSmoke(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 13:
    CreateSnowPuff(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 14:
    CreateDust(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 15:
    CreateFlash(twp->pos.x, twp->pos.y, twp->pos.z,
                        scl * (0.9f + 0.2f * njRandom()));
    break;
  case 16:
    _rename_CreateSikake(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()));
    break;
  case 17:
    CreateSnowPuffColor(&twp->pos, &v, scl * (0.9f + 0.2f * njRandom()), 0.0f);
    break;
  }
}
