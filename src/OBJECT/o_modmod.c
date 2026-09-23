#include "OBJECT/o_modmod.h"

#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "set.h"

extern void fn_8011E23C_ret_0(NJS_CNK_MODEL *model);

// one pair of models per variant, picked by 'wtimer'
extern NJS_CNK_MODEL *_rename_modmod_models[2][2];

// ^ extern
// v in this file

static void ObjectModModDest(task *tp);
static void ObjectModModExec(task *tp);
static void ObjectModModDisp(task *tp);

void ObjectModMod(task *tp) {
  taskwk *twp = tp->twp;

  if (!CheckRangeOut(tp)) {
    tp->disp = ObjectModModDisp;
    tp->exec = ObjectModModExec;
    tp->dest = ObjectModModDest;
    twp->wtimer = (twp->ang.z & 0xF) % 2;
  }
}

static void ObjectModModDest(task *tp) {}

static void ObjectModModExec(task *tp) {
  if (CheckRangeOut(tp)) {
    return;
  }
}

static void ObjectModModDisp(task *tp) {
  taskwk *twp = tp->twp;

  njPushMatrix(NULL);
  njTranslate(NULL, twp->pos.x, twp->pos.y, twp->pos.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  njScale(NULL, 1.0f + twp->scl.x, 1.0f + twp->scl.y, 1.0f + twp->scl.z);
  fn_8011E23C_ret_0(_rename_modmod_models[twp->wtimer][1]);
  njPopMatrix(1);
}
