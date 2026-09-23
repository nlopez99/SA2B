#include "ENEMY/e_capturingbullet.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "stdlib.h"

// enemy work allocated by fn_80018D28, sizeof=0x210
// field names modelled on SADX's enemywk
typedef struct enemywk {
  /* 0x000 */ NJS_VECTOR spd;
  /* 0x00C */ NJS_VECTOR acc;
  /* 0x018 */ Angle swing;
  /* 0x01C */ Uint8 unk_1C[0x30];
  /* 0x04C */ Sint16 flag;
  /* 0x04E */ Uint8 unk_4E[0x6E];
  /* 0x0BC */ NJS_POINT3 colli_center;
  /* 0x0C8 */ Float colli_top;
  /* 0x0CC */ Float colli_radius;
  /* 0x0D0 */ Float colli_bottom;
  /* 0x0D4 */ Float unk_D4;
  /* 0x0D8 */ Float unk_D8;
  /* 0x0DC */ Float unk_DC;
  /* 0x0E0 */ Float unk_E0;
  /* 0x0E4 */ Float unk_E4;
  /* 0x0E8 */ Sint32 unk_E8;
} enemywk;

// one shell fragment, syCalloc'd into the task's any work
typedef struct piecewk {
  /* 0x00 */ NJS_POINT3 pos;
  /* 0x0C */ NJS_VECTOR spd;
  /* 0x18 */ Float scl;
  /* 0x1C */ Float alpha;
} piecewk;

extern NJS_TEXLIST _rename_e_capturingbullet_texlist;
extern NJS_CNK_MODEL _rename_e_capturingbullet_model;
extern NJS_CNK_MODEL _rename_e_capturingbullet_piece_model;
extern NJS_CNK_MODEL _rename_e_capturingbullet_held_model;
extern CCL_INFO _rename_e_capturingbullet_colli_info;

// gravity direction
extern NJS_VECTOR lbl_801E5624;

extern Float asinf(Float x);
extern Float atan2f(Float y, Float x);
extern Float sqrtf(Float x);

extern void fn_800068BC(task *tp, Float range);
extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_80017BE4(taskwk *twp, enemywk *ewp);
extern void fn_80017C00(taskwk *twp, enemywk *ewp);
extern void fn_80018B88(task *tp);
extern enemywk *fn_80018D28(task *tp);
extern Float fn_8002AE5C(Float now, Float dst, Float rate);
extern void fn_8002FB2C(Sint32 pno, Sint32 a2, Sint32 a3, Sint32 a4);
extern void fn_8006AFFC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        Sint32 a5, NJS_POINT3 *pos);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);
extern void fn_80072820(const char *name, NJS_TEXLIST *texlist);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_8011610C(NJS_VECTOR *scl);

// ^ extern
// v in this file

static void EnemyCapturingBulletMove(taskwk *twp, enemywk *ewp);
static void EnemyCapturingBulletDisplayer(task *tp);
static void EnemyCapturingBulletCheckCatch(taskwk *twp, enemywk *ewp);
static void EnemyCapturingBulletFly(taskwk *twp, enemywk *ewp);
static void EnemyCapturingBulletHold(taskwk *twp, enemywk *ewp);
static void EnemyCapturingBulletBurst(taskwk *twp, enemywk *ewp);
static void EnemyCapturingBulletDestructor(task *tp);
static void EnemyCapturingBulletExecutor(task *tp);
static void EnemyCapturingBulletInit(task *tp, NJS_VECTOR *spd);
static void ManTexDestructor(task *tp);
static void ManTex(task *tp);
static void EnemyCapturingBulletPiecesDisplayer(task *tp);
static void EnemyCapturingBulletPiecesExecutor(task *tp);
static void CreateEnemyCapturingBulletPieces(NJS_POINT3 *pos, Angle3 *ang);

enum {
  MD_FLY,
  MD_HOLD,
  MD_BURST,
  MD_END,
};

#define RadAng(n) ((Angle)(10430.38043493439 * (n)))
#define RandomF() (0.000030517578f * (Float)rand())

static task *mantex_tp;

static void EnemyCapturingBulletMove(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37 == 0) {
    njAddVector(&twp->pos, &ewp->spd);
    ewp->acc.x = 0.0f;
    ewp->acc.y = 0.0f;
    ewp->acc.z = 0.0f;
  }
}

static void EnemyCapturingBulletDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  NJS_VECTOR scl;
  NJS_VECTOR bscl;

  if (twp->mode == MD_HOLD) {
    scl.x = twp->scl.x;
    scl.z = 1.0f / sqrtf(twp->scl.x);
    scl.y = 2.0f;
    njSetTexture(&_rename_e_capturingbullet_texlist);
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    njRotateX(NULL, twp->ang.x);
    njRotateZ(NULL, twp->ang.z);
    fn_8011610C(&scl);
    njCnkCacheDrawModel(&_rename_e_capturingbullet_held_model);
    njPopMatrixEx();
  } else if (twp->mode == MD_BURST) {
    bscl.z = bscl.x = bscl.y = twp->scl.x;
    fn_800156FC(twp->scl.y, 0.0f, 0.0f, 0.0f);
    njSetTexture(&_rename_e_capturingbullet_texlist);
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    njRotateX(NULL, twp->ang.x);
    njRotateZ(NULL, twp->ang.z);
    fn_8011610C(&bscl);
    njCnkCacheDrawModel(&_rename_e_capturingbullet_piece_model);
    njPopMatrixEx();
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  } else if (twp->mode == MD_FLY) {
    njSetTexture(&_rename_e_capturingbullet_texlist);
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    njRotateX(NULL, twp->ang.x);
    njRotateZ(NULL, twp->ang.z);
    njCnkCacheDrawModel(&_rename_e_capturingbullet_model);
    njPopMatrixEx();
  }
}

// catches a player, or bursts when hit
static void EnemyCapturingBulletCheckCatch(taskwk *twp, enemywk *ewp) {
  colliwk *cwp = twp->cwp;
  Sint32 hit = 0;
  Sint32 pno;
  task *ptp;
  Float scl;
  NJS_VECTOR dir;

  if ((cwp->flag & 1) &&
      (pno = IsThisTaskPlayer(cwp->hit_cwp->mytask)) != -1) {
    SetInputP(pno, 0x16, 0);
    twp->mode = MD_HOLD;
    twp->smode = (Sint8)pno;
    ptp = cwp->mytask->ptp;
    if (ptp != NULL) {
      ptp->twp->flag |= 0x400;
    }
    twp->pos = playertwp[pno]->cwp->info->center;
    switch (playerpwp[pno]->basechar) {
    case 0:
    case 1:
    case 2:
    case 4:
    case 5:
      scl = 1.0f;
      break;
    case 6:
    case 7:
      scl = 3.0f;
      break;
    }
    twp->scl.x = scl;
    twp->ang = playertwp[pno]->ang;
    fn_8002FB2C(pno, 3, 7, 0);
    return;
  }
  if (twp->flag & 0xD) {
    hit = 1;
  }
  if (hit != 1) {
    return;
  }
  twp->mode = MD_BURST;
  twp->scl.x = 1.0f;
  twp->scl.y = 0.0f;
  twp->cwp->info->attr |= 0x10;
  dir = ewp->spd;
  njUnitVector(&dir);
  twp->pos.x += 3.0f * dir.x;
  twp->pos.y += 3.0f * dir.y;
  twp->pos.z += 3.0f * dir.z;
}

static void EnemyCapturingBulletFly(taskwk *twp, enemywk *ewp) {
  NJS_VECTOR dir;

  if (twp->wtimer > 5) {
    EnemyCapturingBulletCheckCatch(twp, ewp);
  }
  EnemyCapturingBulletMove(twp, ewp);
  fn_80017C00(twp, ewp);
  if (ewp->flag & 0x180) {
    twp->mode = MD_BURST;
    twp->scl.x = 1.0f;
    twp->scl.y = 0.0f;
    twp->cwp->info->attr |= 0x10;
    dir = ewp->spd;
    njUnitVector(&dir);
    twp->pos.x += 3.0f * dir.x;
    twp->pos.y += 3.0f * dir.y;
    twp->pos.z += 3.0f * dir.z;
  }
  if (lbl_801CC168._37 == 0) {
    twp->ang.z += 0x1400;
  }
}

// follows the caught player until they leave mode 0x26
static void EnemyCapturingBulletHold(taskwk *twp, enemywk *ewp) {
  taskwk *ptwp = playertwp[twp->smode];
  playerwk *pwp;
  // unused
  Float f;

  if (ptwp->mode != 0x26) {
    twp->mode = MD_END;
    CreateEnemyCapturingBulletPieces(&twp->pos, &twp->ang);
    return;
  }
  pwp = playerpwp[twp->smode];
  if (ptwp != NULL) {
    twp->pos = pwp->root_pos;
    twp->ang = ptwp->ang;
  }
  twp->scl.x += 0.02f * njSin(ewp->swing);
  ewp->swing += 0x400;
}

static void EnemyCapturingBulletBurst(taskwk *twp, enemywk *ewp) {
  // unused
  Float scl;

  if (lbl_801CC168._37 != 0) {
    return;
  }
  twp->scl.x = fn_8002AE5C(twp->scl.x, 0.0f, 0.02f);
  twp->scl.y = fn_8002AE5C(twp->scl.y, -1.0f, 0.02f);
  if (twp->scl.y <= -1.0f) {
    twp->mode = MD_END;
  }
}

static void EnemyCapturingBulletDestructor(task *tp) { fn_80018B88(tp); }

static void EnemyCapturingBulletExecutor(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;

  if (CheckRangeOut(tp)) {
    return;
  }
  switch (twp->mode) {
  case MD_FLY:
    EnemyCapturingBulletFly(twp, ewp);
    break;
  case MD_HOLD:
    EnemyCapturingBulletHold(twp, ewp);
    if (twp->mode != MD_HOLD) {
      FreeTask(tp);
      return;
    }
    fn_8006AFFC(0x4007, twp, 1, 10, 2, &twp->pos);
    break;
  case MD_BURST:
    EnemyCapturingBulletBurst(twp, ewp);
    if (twp->mode != MD_BURST) {
      FreeTask(tp);
      return;
    }
    break;
  }
  CCL_Entry(tp);
  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }
}

static void EnemyCapturingBulletInit(task *tp, NJS_VECTOR *spd) {
  taskwk *twp = tp->twp;
  enemywk *ewp;
  Float len;
  // unused
  Angle3 ang;

  CCL_Init(tp, &_rename_e_capturingbullet_colli_info, 1, CID_ENEMY2);
  tp->twp->cwp->flag &= ~0x40;
  fn_800068BC(tp, 40.0f);
  ewp = fn_80018D28(tp);
  ewp->spd = *spd;
  twp->scl.x = 1.0f;
  twp->scl.y = 1.0f;
  len = njScalor(&ewp->spd);
  twp->ang.x = RadAng(asinf(-ewp->spd.y / len));
  twp->ang.y = RadAng(atan2f(ewp->spd.x, ewp->spd.z));
  ewp->colli_center.x = 0.0f;
  ewp->colli_center.y = 0.0f;
  ewp->colli_center.z = 0.0f;
  ewp->colli_top = 3.0f;
  ewp->colli_radius = 3.0f;
  ewp->colli_bottom = -3.0f;
  ewp->unk_D8 = 0.0f;
  ewp->unk_DC = 0.0f;
  ewp->unk_E4 = 0.0f;
  ewp->unk_E8 = 0;
  fn_80017BE4(twp, ewp);
  tp->dest = EnemyCapturingBulletDestructor;
  tp->disp = EnemyCapturingBulletDisplayer;
}

static void ManTexDestructor(task *tp) {
  fn_801166F0(&_rename_e_capturingbullet_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

void EnemyCapturingBulletLoadTexture(void) {
  if (mantex_tp == NULL) {
    fn_80072820("E_CAPTEX", &_rename_e_capturingbullet_texlist);
    mantex_tp = CreateFundamentalTask(0, LEV_0, ManTex);
    mantex_tp->dest = ManTexDestructor;
  }
}

void CreateEnemyCapturingBullet(task *tp, NJS_VECTOR *spd, NJS_POINT3 *pos) {
  task *ntp;

  if (tp != NULL) {
    ntp = CreateChildTask(IM_TWK, EnemyCapturingBulletExecutor, tp);
  } else {
    ntp = CreateFundamentalTask(IM_TWK, LEV_3, EnemyCapturingBulletExecutor);
  }
  if (ntp != NULL) {
    taskwk *twp = ntp->twp;

    twp->pos = *pos;
    EnemyCapturingBulletInit(ntp, spd);
    EnemyCapturingBulletLoadTexture();
    fn_8006B7EC(0x4000, NULL, 0, 15, pos);
  }
}

static void EnemyCapturingBulletPiecesDisplayer(task *tp) {
  piecewk *pwk = (piecewk *)tp->awp;
  NJS_VECTOR scl;

  scl.z = scl.x = scl.y = pwk->scl;
  fn_800156FC(pwk->alpha, 0.0f, 0.0f, 0.0f);
  njSetTexture(&_rename_e_capturingbullet_texlist);
  njPushMatrixEx();
  njTranslateEx(&pwk->pos);
  fn_8011610C(&scl);
  njCnkCacheDrawModel(&_rename_e_capturingbullet_piece_model);
  njPopMatrixEx();
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
}

static void EnemyCapturingBulletPiecesExecutor(task *tp) {
  piecewk *pwk = (piecewk *)tp->awp;

  if (lbl_801CC168._37 != 0) {
    return;
  }
  njAddVector(&pwk->pos, &pwk->spd);
  pwk->spd.x += 0.04f * lbl_801E5624.x;
  pwk->spd.y += 0.04f * lbl_801E5624.y;
  pwk->spd.z += 0.04f * lbl_801E5624.z;
  pwk->scl = fn_8002AE5C(pwk->scl, 0.0f, 0.025f);
  pwk->alpha = fn_8002AE5C(pwk->alpha, -1.0f, 0.05f);
  if (pwk->alpha <= -1.0f) {
    FreeTask(tp);
    return;
  }
}

static void CreateEnemyCapturingBulletPieces(NJS_POINT3 *pos, Angle3 *ang) {
  task *tp;
  piecewk *pwk;
  NJS_VECTOR dir;
  Sint32 i;

  njPushMatrixEx();
  njRotateY(NULL, ang->y);
  njRotateX(NULL, ang->x);
  njRotateZ(NULL, ang->z);
  for (i = 0; i < 8; i++) {
    tp = CreateFundamentalTask(0, LEV_3, EnemyCapturingBulletPiecesExecutor);
    if (tp == NULL) {
      break;
    }
    pwk = syCalloc(1, sizeof(piecewk));
    if (pwk == NULL) {
      DestroyTask(tp);
      break;
    }
    tp->awp = (anywk *)pwk;
    pwk->pos = *pos;
    njRotateY(NULL, 0x2000);
    dir.x = 0.4f + 0.6f * RandomF();
    dir.y = -0.6f + 0.4f * RandomF();
    dir.z = 0.0f;
    njCalcVector(NULL, &dir, &pwk->spd);
    njAddVector(&pwk->pos, &dir);
    pwk->scl = 0.8f + 0.4f * RandomF();
    pwk->alpha = 1.0f;
    tp->disp = EnemyCapturingBulletPiecesDisplayer;
  }
  njPopMatrixEx();
}
