#include "OBJECT/o_kddoor.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "set.h"
#include "stdlib.h"

// one term of the door's rumble: a sine of the open timer, scaled
typedef struct kddoorwave // sizeof=0x8
{
  /* 0x00 */ Angle spd;
  /* 0x04 */ Float amp;
} kddoorwave;

extern Sint8 fn_80065388(task *tp);
extern void fn_8006AFFC(Sint32 id, taskwk *twp, Sint32 unk, Sint32 vol,
                        Sint32 unk2, NJS_POINT3 *pos);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern void _rename_SetConditionFlag(task *tp, Sint8 flag);
extern void _rename_CreateObjDust1(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern Sint32 _rename_GetRingGroupState(Sint32 no);
extern Sint32 _rename_GetRingGroupPos(Sint32 no, NJS_POINT3 *pos);

extern NJS_TEXLIST _rename_kddoor_texlist;
extern NJS_CNK_MODEL _rename_kddoor_model;
// the upright door and the same five boxes with y and z swapped, for work.l
extern CCL_INFO _rename_kddoor_colli_info[5];
extern CCL_INFO _rename_kddoor_colli_info_rot[5];
// the rumble, split into the door's own axes: 'slide' is the one it travels on
extern kddoorwave _rename_kddoor_wave_cross[3];
extern kddoorwave _rename_kddoor_wave_slide[3];
extern kddoorwave _rename_kddoor_wave_depth[3];
extern Float _rename_kddoor_dust_scl;
extern Float _rename_kddoor_dust_rnd;
extern Float _rename_kddoor_dust_spd;

// ^ extern
// v in this file

static void ObjectKdDoorDest(task *tp);
static void ObjectKdDoorExec(task *tp);
static void ObjectKdDoorDisp(task *tp);

// the ring group that opens this door, same numbering as OBJECT/o_kddegring.c
#define KDDOOR_GROUP_MAX 8
#define GetGroupNo(twp) (((twp)->ang.x & 0xF) % KDDOOR_GROUP_MAX)

// the door slides along x instead of y, and 0x200 flips which way
#define IsSlideX(twp) ((twp)->ang.x & 0x100)
#define IsSlideBack(twp) ((twp)->ang.x & 0x200)

// how far the door has travelled, the only thing the work holds
#define GetOfs(tp) (*(Float *)(tp)->mwp)

static Float KdDoorShake(kddoorwave *wave, Sint32 num, Uint16 frame) {
  Float ofs = 0.0f;

  while (num--) {
    ofs += wave->amp * njSin(wave->spd * frame);
    wave++;
  }
  return ofs;
}

// a puff of dust at the foot of the door, thrown out along the door's own x
static void KdDoorDust(task *tp, NJS_POINT3 *pos, Angle angx) {
  taskwk *twp = tp->twp;
  NJS_POINT3 p;
  NJS_VECTOR spd;

  p.x = 50.0f * (0.000030517578f * (Float)rand() - 0.5f);
  p.y = 0.0f;
  p.z = 0.0f;
  spd.x = 0.0f;
  spd.y = 0.0f;
  spd.z = _rename_kddoor_dust_spd * (1.0f + twp->scl.y);

  njPushMatrixEx();
  njUnitMatrix(NULL);
  if (tp->work.l) {
    njTranslate(NULL, twp->pos.x, twp->pos.y, twp->pos.z);
  } else {
    njTranslate(NULL, twp->pos.x, twp->pos.y + GetOfs(tp), twp->pos.z);
  }
  njRotateY(NULL, twp->ang.y);
  if (tp->work.l) {
    njRotateX(NULL, 0x4000);
    njTranslate(NULL, 0.0f, GetOfs(tp), 0.0f);
  }
  njScale(NULL, 1.0f + twp->scl.x, 1.0f + twp->scl.y, 1.0f + twp->scl.z);
  njTranslateEx(pos);
  njRotateX(NULL, angx);
  njCalcPoint(NULL, &p, &p);
  njCalcVector(NULL, &spd, &spd);
  njPopMatrixEx();

  _rename_CreateObjDust1(&p, &spd,
                         (1.0f + twp->scl.y) *
                             (_rename_kddoor_dust_scl +
                              _rename_kddoor_dust_rnd *
                                  (0.000030517578f * (Float)rand())));
}

void ObjectKdDoor(task *tp) {
  taskwk *twp = tp->twp;

  if (tp->ocp != NULL && fn_80065388(tp)) {
    DeadOut(tp);
    return;
  }
  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(Float));
  if (tp->mwp == NULL) {
    return;
  }

  tp->disp = ObjectKdDoorDisp;
  tp->exec = ObjectKdDoorExec;
  tp->dest = ObjectKdDoorDest;
  twp->smode = 0;
  if (tp->work.l) {
    CCL_Init(tp, _rename_kddoor_colli_info_rot,
             ARYLEN(_rename_kddoor_colli_info_rot), CID_OBJECT);
  } else {
    CCL_Init(tp, _rename_kddoor_colli_info,
             ARYLEN(_rename_kddoor_colli_info), CID_OBJECT);
  }
  GetOfs(tp) = 0.0f;
}

static void ObjectKdDoorDest(task *tp) {
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectKdDoorExec(task *tp) {
  taskwk *twp = tp->twp;
  colliwk *cwp;
  NJS_POINT3 goal;
  NJS_VECTOR spd;
  Sint32 unused[2]; // unused

  if (CheckRangeOut(tp)) {
    return;
  }

  switch (twp->smode) {

  case 0:
    if (_rename_GetRingGroupState(GetGroupNo(twp)) == 1 &&
        _rename_GetRingGroupPos(GetGroupNo(twp), &goal)) {
      twp->smode = 1;
      if (tp->ocp != NULL) {
        _rename_SetConditionFlag(tp, 1);
      }
    }
    break;

  case 1:
    twp->wtimer++;
    GetOfs(tp) += 0.083333336f;
    fn_8006AFFC(0x1016, twp, 1, 0, 0x1e, &twp->pos);
    if (GetOfs(tp) > 55.0f * (1.0f + twp->scl.y)) {
      if (tp->ocp != NULL) {
        DeadOut(tp);
      } else {
        FreeTask(tp);
      }
      twp->smode = 2;
      return;
    }
    if (!IsSlideX(twp)) {
      spd.x = 0.0f;
      spd.y = 0.0f;
      spd.z = -2.5f;
      KdDoorDust(tp, &spd, 0);
    }
    break;

  case 2:
  default:
    break;
  }

  cwp = twp->cwp;
  if (cwp == NULL) {
    return;
  }
  if (twp->smode == 2) {
    return;
  }

  // drag the first box along with the door; the rest never move
  if (IsSlideX(twp)) {
    if (IsSlideBack(twp)) {
      cwp->info[0].center.x =
          _rename_kddoor_colli_info[0].center.x - GetOfs(tp);
    } else {
      cwp->info[0].center.x =
          _rename_kddoor_colli_info[0].center.x + GetOfs(tp);
    }
  } else if (tp->work.l) {
    cwp->info[0].center.z =
        _rename_kddoor_colli_info_rot[0].center.z + GetOfs(tp);
  } else {
    cwp->info[0].center.y =
        _rename_kddoor_colli_info[0].center.y + GetOfs(tp);
  }
  CCL_Entry(tp);
}

static void ObjectKdDoorDisp(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_kddoor_texlist);
  njPushMatrixEx();
  if (IsSlideX(twp)) {
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    if (IsSlideBack(twp)) {
      njTranslate(NULL, 27.5f - GetOfs(tp), 27.5f, 0.0f);
      njScale(NULL, 1.0f + twp->scl.x, 1.0f + twp->scl.y, 1.0f + twp->scl.z);
      if (twp->wtimer != 0) {
        njTranslate(NULL,
                    KdDoorShake(_rename_kddoor_wave_slide,
                                ARYLEN(_rename_kddoor_wave_slide), twp->wtimer),
                    KdDoorShake(_rename_kddoor_wave_cross,
                                ARYLEN(_rename_kddoor_wave_cross), twp->wtimer),
                    KdDoorShake(_rename_kddoor_wave_depth,
                                ARYLEN(_rename_kddoor_wave_depth),
                                twp->wtimer));
      }
      njRotateZ(NULL, 0x4000);
    } else {
      njTranslate(NULL, -27.5f + GetOfs(tp), 27.5f, 0.0f);
      njScale(NULL, 1.0f + twp->scl.x, 1.0f + twp->scl.y, 1.0f + twp->scl.z);
      if (twp->wtimer != 0) {
        njTranslate(NULL,
                    KdDoorShake(_rename_kddoor_wave_slide,
                                ARYLEN(_rename_kddoor_wave_slide), twp->wtimer),
                    KdDoorShake(_rename_kddoor_wave_cross,
                                ARYLEN(_rename_kddoor_wave_cross), twp->wtimer),
                    KdDoorShake(_rename_kddoor_wave_depth,
                                ARYLEN(_rename_kddoor_wave_depth),
                                twp->wtimer));
      }
      njRotateZ(NULL, -0x4000);
    }
    fn_8011E158(&_rename_kddoor_model);
  } else {
    if (tp->work.l) {
      njTranslate(NULL, twp->pos.x, twp->pos.y, twp->pos.z);
    } else {
      njTranslate(NULL, twp->pos.x, twp->pos.y + GetOfs(tp), twp->pos.z);
    }
    njRotateY(NULL, twp->ang.y);
    if (tp->work.l) {
      njRotateX(NULL, 0x4000);
      njTranslate(NULL, 0.0f, GetOfs(tp), 0.0f);
    }
    njScale(NULL, 1.0f + twp->scl.x, 1.0f + twp->scl.y, 1.0f + twp->scl.z);
    if (twp->wtimer != 0) {
      njTranslate(NULL,
                  KdDoorShake(_rename_kddoor_wave_cross,
                              ARYLEN(_rename_kddoor_wave_cross), twp->wtimer),
                  KdDoorShake(_rename_kddoor_wave_slide,
                              ARYLEN(_rename_kddoor_wave_slide), twp->wtimer),
                  KdDoorShake(_rename_kddoor_wave_depth,
                              ARYLEN(_rename_kddoor_wave_depth), twp->wtimer));
    }
    fn_8011E158(&_rename_kddoor_model);
  }
  njPopMatrixEx();
}
