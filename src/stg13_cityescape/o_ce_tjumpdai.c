#include "stg13_cityescape/o_ce_tjumpdai.h"

#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njbasic.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"

// mobile land collision entry: register / withdraw
extern void fn_800232A4(Uint32 attr, task *tp, NJS_OBJECT *object);
extern void fn_800231CC(task *tp, NJS_OBJECT *object);
extern s32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, float);
extern void _rename_SetConditionFlag(task *tp, u8 smode);
extern void late_DrawModel(GJS_MODEL *model);

extern NJS_OBJECT  _rename_tjumpdai_col_object;
extern NJS_TEXLIST _rename_tjumpdai_texlist;
extern GJS_MODEL   _rename_tjumpdai_model;

// ^ extern
// v in this file

static void ObjectCeTjumpdaiDest(task *tp);
static void ObjectCeTjumpdaiExec(task *tp);
static void ObjectCeTjumpdaiDisp(task *tp);

typedef struct tjumpdaiwk // sizeof=0x44
{
  /* 0x00 */ NJS_OBJECT object;
  /* 0x38 */ NJS_POINT3 center;
} tjumpdaiwk;

#define GetWork(task) ((tjumpdaiwk *)task->mwp)
#define GetObject(task) (&GetWork(task)->object)

void ObjectCeTjumpdai(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object;
  Uint32 attr;
  Float r;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(tjumpdaiwk));
  if (tp->mwp == NULL) {
    return;
  }

  tp->disp = ObjectCeTjumpdaiDisp;
  tp->exec = ObjectCeTjumpdaiExec;
  tp->dest = ObjectCeTjumpdaiDest;
  twp->mode = 0;

  object = GetObject(tp);
  *object = _rename_tjumpdai_col_object;
  object->evalflags &= ~3;
  object->ang.y = twp->ang.y;
  object->ang.z = twp->ang.z;
  object->ang.x = twp->ang.x;
  object->pos.x = twp->pos.x;
  object->pos.y = twp->pos.y;
  object->pos.z = twp->pos.z;

  attr = 1;
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

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, twp->ang.x);
  njCalcPoint(NULL, &object->model->center, &GetWork(tp)->center);
  njPopMatrixEx();
  twp->id = 24;
}

static void ObjectCeTjumpdaiDest(task *tp) {
  if (GetObject(tp)->model != NULL) {
    fn_800231CC(tp, GetObject(tp));
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectCeTjumpdaiExec(task *tp) {
  taskwk *twp = tp->twp;
  tjumpdaiwk *wk;

  if (CheckRangeOut(tp)) {
    return;
  }

  wk = GetWork(tp);
  if (wk->object.model != NULL) {
    if (_rename_EitherPlayerWithinSphere(&wk->center,
                                         5.0f + wk->object.model->r)) {
      twp->flag |= 0x100;
    } else {
      twp->flag &= ~0x100;
    }
  }

  if (twp->mode == 0) {
    if (playerpwp[0] != NULL && playerpwp[0]->mlotp == tp) {
      twp->mode = 1;
    }
  } else if (twp->mode == 1) {
    if (playerpwp[0] != NULL && playerpwp[0]->mlotp != tp) {
      twp->mode = 2;
      _rename_SetConditionFlag(tp, 1);
    }
  }
}

static void ObjectCeTjumpdaiDisp(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_tjumpdai_texlist);
  OnControl3D(0x2400);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, twp->ang.x);
  late_DrawModel(&_rename_tjumpdai_model);
  njPopMatrix(1);
  OffControl3D(0x2400);
}
