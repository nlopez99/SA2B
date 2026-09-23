#include "ENEMY/e_jet.h"

#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "stdlib.h"

extern void gjSetFog(void);
extern void njDisableFog(void);
extern void njEnableFog(void);
extern void __njColorBlendingMode(Int, Int);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_80118B5C(NJS_TEXLIST *texlist);
extern void fn_8011F4F8(Sint32, Uint32, Sint32);
extern void fn_8011F4FC(void);
extern void fn_8011F500(NJS_POINT3 *, Sint32, Float, Float);

// ^ extern
// v in this file

static void EnemyJetDisplayer(task *tp);
static void ManTex(task *tp);

// the jet's speed lives in the angle field of its task work
#define GetSpd(twp) ((NJS_VECTOR *)&(twp)->ang)

#define rnd() (0.000030517578f * (Float)rand())

static NJS_TEXNAME e_jet_texname[] = {
    {"jet1"},
};

static NJS_TEXLIST e_jet_texlist = {
    e_jet_texname,
    ARYLEN(e_jet_texname),
};

// task that owns the texture
static task *mantex_tp;

static void EnemyJetDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  Float scl = 16.0f * twp->scl.z;
  Uint32 col;

  // only the first jet of the parent's child list sets the render state up
  if (tp->ptp == NULL || tp->ptp->ctp == tp ||
      tp->last->disp != EnemyJetDisplayer) {
    __njColorBlendingMode(0, 8);
    __njColorBlendingMode(1, 10);
    njSetTexture(&e_jet_texlist);
    njDisableFog();
    gjSetFog();
  }

  col = (Uint32)(255.0f * twp->scl.x);
  col |= col << 8;
  col |= col << 16;
  fn_8011F4F8(0, col, 1);
  fn_8011F500(&twp->pos, 1, scl, scl);
  fn_8011F4FC();

  // and only the last one takes it down again
  if (tp->ptp == NULL || tp->ptp->ctp == tp->next ||
      tp->next->disp != EnemyJetDisplayer) {
    __njColorBlendingMode(0, 8);
    __njColorBlendingMode(1, 6);
    njEnableFog();
    gjSetFog();
  }
}

static void EnemyJetDestructor(task *tp) {}

static void EnemyJetExecutor(task *tp) {
  taskwk *twp = tp->twp;

  if (tp->ptp == NULL && CheckRangeOut(tp)) {
    return;
  }
  if (twp->scl.x <= 0.05f) {
    DestroyTask(tp);
    return;
  }
  if (lbl_801CC168._37) {
    return;
  }

  twp->scl.x -= twp->scl.y;
  if (twp->scl.x < 0.0f) {
    twp->scl.x = 0.0f;
  }
  njAddVector(&twp->pos, GetSpd(twp));
  twp->wtimer++;
}

static void EnemyJetInit(task *tp) {
  taskwk *twp = tp->twp;

  twp->scl.x = 1.0f;
  switch (twp->smode) {
  case 2:
    twp->scl.y = 0.08f;
    twp->scl.z = 0.14f + 0.02f * rnd();
    break;
  case 1:
    twp->scl.y = 0.16f;
    twp->scl.z = 0.18f + 0.04f * rnd();
    break;
  case 0:
  default:
    twp->scl.y = 0.1f;
    twp->scl.z = 0.27f + 0.05f * rnd();
    break;
  }
  tp->dest = EnemyJetDestructor;
  tp->disp = EnemyJetDisplayer;
}

static void ManTexDestructor(task *tp) {
  fn_801166F0(&e_jet_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

void EnemyJetLoadTexture(void) {
  if (mantex_tp != NULL) {
    return;
  }

  fn_80118B5C(&e_jet_texlist);
  mantex_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
  mantex_tp->dest = ManTexDestructor;
}

void CreateEnemyJet(task *ptp, Sint32 smode, NJS_POINT3 *pos,
                    NJS_VECTOR *spd) {
  task *tp;
  NJS_VECTOR *v;

  if (ptp != NULL) {
    tp = CreateChildTask(IM_TWK, EnemyJetExecutor, ptp);
  } else {
    tp = CreateFundamentalTask(IM_TWK, LEV_3, EnemyJetExecutor);
  }
  if (tp == NULL) {
    return;
  }

  v = GetSpd(tp->twp);
  tp->twp->smode = smode;
  tp->twp->pos = *pos;
  *v = *spd;
  EnemyJetInit(tp);
  EnemyJetLoadTexture();
}
