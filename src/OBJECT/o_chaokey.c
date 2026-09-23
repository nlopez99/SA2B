#include "OBJECT/o_chaokey.h"

#include "CCL.h"
#include "fabsf.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/sound.h"
#include "set.h"

extern void njDisableFog(void);
extern void njEnableFog(void);
extern void gjSetFog(void);
extern void __njColorBlendingMode(Sint32, Sint32);
extern void GXSetZMode(Sint32 compare_enable, Sint32 func, Sint32 update_enable);
extern void fn_801229D4(void);
extern void fn_801229E0(void);
extern void fn_80122B24(void);
// billboard sprite draw
extern void fn_80033620(NJS_POINT3 *pos, Sint32 no, Sint32 ang, Sint32 attr,
                        Float sx, Float sy, Sint32 col, Sint32 mode, Float a,
                        Float b);
extern Sint32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, Float);
extern void _rename_RingDrawShadowModel(void);

extern NJS_TEXLIST     _rename_chaokey_texlist;
extern NJS_CNK_MODEL   _rename_chaokey_model;
extern NJS_CNK_OBJECT  _rename_chaokey_object;
extern Angle           _rename_chaokey_spin;
extern Angle           _rename_chaokey_angx;
extern CCL_INFO        _rename_chaokey_colli_info[1];
extern NJS_POINT3      _rename_chaokey_goal;
extern Float           _rename_chaokey_flare_scl;
extern task           *_rename_chaokey_tp;

// ^ extern
// v in this file

static void ChaoKeyTaskDisp(task *tp);
static void ChaoKeyTask(task *tp);
static void ChaoKeyTaskDest(task *tp);
static void ChaoKeyOnTake(task *tp);
static void ObjectChaoKeyInit(task *tp);
static void ObjectChaoKeyDest(task *tp);
static void ObjectChaoKeyExec(task *tp);
static void ObjectChaoKeyDisp(task *tp);
static void ObjectChaoKeyDispSort(task *tp);

// the key lives for a minute, then leaves unless a player is near
#define CHAOKEY_LIFE (3600)

static void ChaoKeyTaskDisp(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 point;
  Float wave;
  Float r;

  wave = 0.1f * njSin(twp->wtimer * 0x300);
  // the flare is drawn unclipped: the model's radius is zeroed for the draw
  r = _rename_chaokey_object.model->r;
  _rename_chaokey_object.model->r = 0.0f;

  njSetTexture(&_rename_chaokey_texlist);
  njDisableFog();
  gjSetFog();
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->wtimer * 0x120);
  njScale(NULL, twp->scl.z * (_rename_chaokey_flare_scl * (1.0f + wave)),
          twp->scl.z * (0.8f * (_rename_chaokey_flare_scl * (1.0f - wave))),
          twp->scl.z * (_rename_chaokey_flare_scl * (1.0f + wave)));
  GXSetZMode(0, 0, 0);
  fn_801229D4();
  njCnkCacheDrawModel(_rename_chaokey_object.model);
  fn_801229E0();
  fn_80122B24();
  njCalcPoint(NULL, &_rename_chaokey_object.child->pos, &point);
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  njUnitMatrix(NULL);
  fn_80033620(&point, 2, 0x4000, 0, 1.0f, 1.0f, -1, 0x800, 1.05f, 0.0f);
  njPopMatrixEx();
  njEnableFog();
  gjSetFog();
  _rename_chaokey_object.model->r = r;
}

static void ChaoKeyTask(task *tp) {
  taskwk *twp = tp->twp;
  NJS_VECTOR v;
  Float *d;
  Float *s;
  Float *p;
  Float *vp;
  Float *pp;
  Float len;

  if (twp->mode != 0) {
    twp->mode = 0;
    njCalcPoint(NULL, &twp->pos, &twp->pos);
    return;
  }

  // fly towards the goal: v is the vector still to cover
  d = &v.x;
  s = &_rename_chaokey_goal.x;
  p = &twp->pos.x;
  *d++ = *s++ - *p++;
  *d++ = *s++ - *p++;
  *d++ = *s++ - *p++;

  len = njScalor2(&v);
  if (len < 6.0f) {
    twp->wtimer += 1;
  } else {
    twp->wtimer += 3;
  }
  if (len < 3.0f) {
    twp->scl.z *= 0.9f;
  }
  if (len < 1.5f) {
    lbl_801CC168._44 = 1;
  }

  // close a tenth of the remaining distance this frame
  d = &v.x;
  *d++ *= 0.1f;
  *d++ *= 0.1f;
  *d++ *= 0.1f;
  // separate pointers to match
  vp = &v.x;
  pp = &twp->pos.x;
  *vp++ += *pp++;
  *vp++ += *pp++;
  *vp += *pp;
  twp->pos = v;

  if (twp->scl.z < 0.2f) {
    DestroyTask(tp);
  }
}

static void ChaoKeyTaskDest(task *tp) {
  if (_rename_chaokey_tp == tp) {
    _rename_chaokey_tp = NULL;
  }
  lbl_801CC168._44 = 1;
}

BOOL ChaoKeyIsActive(void) {
  if (lbl_801CC168._44 == 1) {
    return TRUE;
  }
  if (_rename_chaokey_tp != NULL) {
    return TRUE;
  }
  return FALSE;
}

void CreateChaoKeyTask(NJS_POINT3 *pos) {
  task *tp;

  if (ChaoKeyIsActive()) {
    return;
  }
  // tested twice, as in the original
  if (_rename_chaokey_tp != NULL) {
    return;
  }
  tp = CreateElementalTask(IM_TWK, LEV_3, ChaoKeyTask, "ChaoKeyTask");
  if (tp == NULL) {
    return;
  }
  _rename_chaokey_tp = tp;
  tp->disp = ChaoKeyTaskDisp;
  tp->dest = ChaoKeyTaskDest;
  tp->twp->scl.z = 1.0f;
  if (pos != NULL) {
    tp->twp->mode = 1;
    tp->twp->pos = *pos;
  } else {
    tp->twp->pos = _rename_chaokey_goal;
  }
  SE_Call(0x8006, 0, 0, 0);
}

static void ChaoKeyOnTake(task *tp) {}

task *CreateChaoKey(NJS_POINT3 *pos, Float ground_y, Float spd) {
  taskwk *twp;
  task *tp;

  tp = CreateFundamentalTask(IM_TWK, LEV_2, ObjectChaoKeyExec);
  if (tp != NULL) {
    twp = tp->twp;
    twp->pos = *pos;
    ObjectChaoKeyInit(tp);
    twp->scl.x = ground_y;
    twp->scl.y = spd;
    return tp;
  }
  return NULL;
}

static void ObjectChaoKeyInit(task *tp) {
  taskwk *twp = tp->twp;

  tp->disp = ObjectChaoKeyDisp;
  tp->disp_sort = ObjectChaoKeyDispSort;
  tp->exec = ObjectChaoKeyExec;
  tp->dest = ObjectChaoKeyDest;
  twp->smode = 0;
  twp->ang.x = 0;
  twp->ang.z = 0;
  CCL_Init(tp, _rename_chaokey_colli_info, ARYLEN(_rename_chaokey_colli_info),
           CID_ITEM);
  // 'scl.x' is the height the key bounces back to, 'scl.y' its speed
  twp->scl.x = twp->pos.y;
  twp->scl.y = 0.0f;
  twp->smode = 1;
}

static void ObjectChaoKeyDest(task *tp) {}

static void ObjectChaoKeyExec(task *tp) {
  taskwk *twp = tp->twp;

  if (tp->work.l > CHAOKEY_LIFE || lbl_801CC168._44 != 0) {
    if (_rename_EitherPlayerWithinSphere(&twp->pos, 800.0f) == 0 ||
        lbl_801CC168._44 != 0) {
      FreeTask(tp);
      return;
    }
  } else {
    tp->work.l++;
  }

  twp->pos.y += twp->scl.y;
  if (twp->pos.y <= twp->scl.x) {
    twp->scl.y *= -0.6f;
    twp->pos.y = twp->scl.x;
    if (fabsf(twp->scl.y) < 0.3f) {
      twp->smode = 2;
    }
  }
  twp->scl.y = 0.985f * twp->scl.y - 0.08f;

  if (twp->smode == 2 && CCL_IsHitPlayer(tp) != NULL) {
    ChaoKeyOnTake(tp);
    twp->scl.y = 2.5f;
    twp->smode = 1;
    CreateChaoKeyTask(&twp->pos);
    if (tp->ocp != NULL) {
      DeadOut(tp);
    } else {
      FreeTask(tp);
    }
  } else {
    CCL_Entry(tp);
  }
  twp->ang.y += _rename_chaokey_spin;
}

static void ObjectChaoKeyDisp(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_chaokey_texlist);
  if (twp->smode != 2 && !(lbl_801CC168._7C & 1)) {
    return;
  }
  njPushMatrix(NULL);
  njTranslate(NULL, twp->pos.x, 5.0f + twp->pos.y, twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, _rename_chaokey_angx);
  njCnkCacheDrawModel(&_rename_chaokey_model);
  njPopMatrix(1);

  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x, 0.1f + twp->scl.x, twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  njTranslate(NULL, 0.0f, 0.0f, -1.0f);
  njScale(NULL, 2.0f, 1.0f, 3.0f);
  _rename_RingDrawShadowModel();
  njPopMatrixEx();
}

static void ObjectChaoKeyDispSort(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_chaokey_texlist);
  if (twp->smode != 2 && !(lbl_801CC168._7C & 1)) {
    return;
  }
  njPushMatrix(NULL);
  njTranslate(NULL, twp->pos.x, 5.0f + twp->pos.y, twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, _rename_chaokey_angx);
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  fn_80033620(&_rename_chaokey_object.child->pos, 2, 0x4000, 0, 1.0f, 1.0f, -1,
              0, 0.0f, 0.0f);
  njPopMatrix(1);
}
