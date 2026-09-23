#include "OBJECT/o_kdasiba.h"

#include "EFFECT/ef_ringsparkle.h"
#include "samt/ninja/gjdraw.h"
#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njbasic.h"
#include "samt/ninja/njcollision.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/sound.h"
#include "set.h"

// mobile land collision entry: register / withdraw
extern void fn_800232A4(Uint32 attr, task *tp, NJS_OBJECT *object);
extern void fn_800231CC(task *tp, NJS_OBJECT *object);
extern s32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, float);
extern void _rename_gjSetTexMtx(NJS_POINT3 *pos, Angle3 *ang);
extern Sint32 _rename_GetRingGroupState(Sint32 no);
extern Sint32 _rename_GetRingGroupPos(Sint32 no, NJS_POINT3 *pos);
extern void fn_80066820(Sint32, NJS_POINT3 *pos, Float r);
extern void fn_8006648C(void);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern void fn_801218C8(Sint32, Sint32);
extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_8002B304(void);
extern void fn_8002B35C(void);
extern void fn_8002B348(void);
extern void fn_8002B2F8(void);

extern NJS_OBJECT _rename_kdasiba_col_object;
extern NJS_TEXLIST _rename_kdasiba_texlist;
extern GJS_MODEL _rename_kdasiba_model;
extern NJS_CNK_MODEL _rename_kdasiba_ghost_model;
extern Float _rename_kdasiba_bob_scale;
extern Sint32 _rename_kdasiba_bob_speed;

// ^ extern
// v in this file

static void ObjectKdasibaDest(task *tp);
static void ObjectKdasibaExec(task *tp);
static void ObjectKdasibaDisp(task *tp);
static void ObjectKdasibaDispSort(task *tp);

#define KDASIBA_GROUP_MAX 8

// which ring group this pedestal waits on
#define GetGroupNo(twp) (((twp)->ang.x & 0xF) % KDASIBA_GROUP_MAX)

typedef struct kdasibawk // sizeof=0x40
{
  /* 0x00 */ NJS_OBJECT object;
  /* 0x38 */ Float alpha; // materialise fade, 0 -> 1
  /* 0x3C */ Float bob;   // vertical bob, driven off the global frame counter
} kdasibawk;

#define GetWork(task) ((kdasibawk *)task->mwp)
#define GetObject(task) (&GetWork(task)->object)

// invisible until its ring group is collected, then it sparkles, fades in
// through the sorted pass and becomes a solid, bobbing platform
void ObjectKdasiba(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object;
  Uint32 attr;
  Float r;
  NJS_POINT3 pos;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(kdasibawk));
  if (tp->mwp == NULL) {
    return;
  }

  // per-instance bob phase
  twp->btimer = (Uint8)(Sint32)(30.0f * njRandom());
  tp->disp = NULL;
  tp->exec = ObjectKdasibaExec;
  tp->dest = ObjectKdasibaDest;
  twp->smode = 0;

  object = GetObject(tp);
  *object = _rename_kdasiba_col_object;
  object->evalflags &= ~3;
  object->ang.y = twp->ang.y;
  object->ang.z = 0;
  object->ang.x = 0;
  object->pos.x = twp->pos.x;
  object->pos.y = twp->pos.y;
  object->pos.z = twp->pos.z;

  attr = 0x08000001;
  r = 5.0f + object->model->r;
  if (r > 200.0f) {
    attr |= 0x00000000;
  } else if (r > 100.0f) {
    attr |= 0x20000000;
  } else if (r > 30.0f) {
    attr |= 0x40000000;
  } else {
    attr |= 0x60000000;
  }
  fn_800232A4(attr, tp, object);

  // the group was already collected: skip the show
  if (_rename_GetRingGroupState(GetGroupNo(twp)) == 1 &&
      _rename_GetRingGroupPos(GetGroupNo(twp), &pos)) {
    twp->wtimer = 0;
    tp->disp = NULL;
    tp->disp_sort = ObjectKdasibaDispSort;
    twp->smode = 2;
    GetWork(tp)->alpha = 1.0f;
  }
}

static void ObjectKdasibaDest(task *tp) {
  STACK_PAD_VAR(2);

  if (GetObject(tp)->model != NULL) {
    fn_800231CC(tp, GetObject(tp));
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectKdasibaExec(task *tp) {
  taskwk *twp = tp->twp;
  kdasibawk *wk;
  NJS_POINT3 grp;
  NJS_POINT3 pos;
  NJS_VECTOR spd;
  Float d;
  Float y;
  Float dy;

  STACK_PAD_VAR(3);

  if (CheckRangeOut(tp)) {
    return;
  }

  switch (twp->smode) {
  case 0:
    // wait for the group, then time the sparkle trail to how far away it is
    if (_rename_GetRingGroupState(GetGroupNo(twp)) == 1 &&
        _rename_GetRingGroupPos(GetGroupNo(twp), &grp)) {
      d = 0.8f * njDistanceP2P(&twp->pos, &grp);
      if (d > 160.0f) {
        d = 160.0f;
      }
      twp->wtimer = d;
      twp->smode = 1;
    }
    break;
  case 1:
    // the last 30 frames throw sparkles round a ring above the pedestal
    if (twp->wtimer < 30 && njRandom() < 0.9f) {
      pos.x = 35.0f * njRandom();
      pos.y = 5.0f * njRandom();
      pos.z = 0.0f;
      njPushMatrix(NULL);
      njUnitMatrix(NULL);
      njTranslate(NULL, twp->pos.x, twp->pos.y, twp->pos.z);
      njRotateY(NULL, (Angle)(182.04445f * (360.0f * njRandom())));
      njCalcPoint(NULL, &pos, &pos);
      njPopMatrix(1);
      spd.x = 0.2f * (njRandom() - 0.5f);
      spd.y = njRandom() - 0.5f;
      spd.z = 0.2f * (njRandom() - 0.5f);
      CreateRingSparkle(&pos, &spd, 1.5f);
    }
    if (twp->wtimer-- == 0) {
      tp->disp_sort = ObjectKdasibaDispSort;
      tp->disp = NULL;
      twp->smode = 2;
      GetWork(tp)->alpha = 0.0f;
      SE_Call(0x100B, NULL, 0, 0);
    }
    break;
  case 2:
    GetWork(tp)->alpha += 0.0125f;
    if (GetWork(tp)->alpha > 1.0f) {
      GetWork(tp)->alpha = 1.0f;
      twp->smode = 3;
      tp->disp = ObjectKdasibaDisp;
      tp->disp_sort = NULL;
    }
    break;
  case 3:
    // solid, only the bob below runs
    break;
  }

  if (twp->smode == 2 || twp->smode == 3) {
    GetWork(tp)->bob =
        _rename_kdasiba_bob_scale *
        njSin(_rename_kdasiba_bob_speed * (lbl_801CC168._7C + twp->btimer));
  }

  wk = GetWork(tp);
  if (wk->object.model != NULL) {
    // the collision entry is moved by hand, so the rider force is the delta
    y = twp->pos.y + wk->bob;
    dy = y - wk->object.pos.y;
    tp->fwp[0].pos_spd.y = dy;
    tp->fwp[1].pos_spd.y = dy;
    wk->object.pos.y = y;
    if (twp->smode != 0 &&
        _rename_EitherPlayerWithinSphere(&wk->object.pos,
                                         5.0f + wk->object.model->r)) {
      twp->flag |= 0x100;
    } else {
      twp->flag &= ~0x100;
    }
  }
}

static void ObjectKdasibaDisp(task *tp) {
  taskwk *twp = tp->twp;
  Angle3 ang;
  NJS_POINT3 pos;

  pos = twp->pos;
  pos.y += GetWork(tp)->bob;
  ang.x = 0;
  ang.y = twp->ang.y;
  ang.z = 0;
  njSetTexture(&_rename_kdasiba_texlist);
  OnControl3D(0x2400);
  njPushMatrix(NULL);
  njTranslateEx(&pos);
  njRotateY(NULL, twp->ang.y);
  fn_80066820(0xff, &pos, _rename_kdasiba_model.r);
  _rename_gjSetTexMtx(&pos, &ang);
  gjDrawModel(&_rename_kdasiba_model);
  fn_8006648C();
  njPopMatrix(1);
  OffControl3D(0x2400);
}

// the materialising ghost, drawn in the sorted pass while alpha climbs
static void ObjectKdasibaDispSort(task *tp) {
  taskwk *twp = tp->twp;

  OnControl3D(0x2400);
  fn_8002B304();
  fn_8002B35C();
  OffControl3D(0x220);
  OnControl3D(0x810);
  fn_801218C8(0xff, 0xa00);
  fn_800156FC(GetWork(tp)->alpha, 1.0f, 1.0f, 1.0f);
  njSetTexture(&_rename_kdasiba_texlist);
  njPushMatrix(NULL);
  njTranslate(NULL, twp->pos.x, twp->pos.y + GetWork(tp)->bob, twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  fn_8011E158(&_rename_kdasiba_ghost_model);
  njPopMatrix(1);
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  fn_8002B348();
  fn_8002B2F8();
  OffControl3D(0x2400);
}
