#include "ENEMY/e_gold.h"

#include "ENEMY/e_light.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njmotion.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/player.h"
#include "set.h"

// one entry per motion, read by fn_80015C40
typedef struct enemy_action // sizeof=0x10
{
  /* 0x00 */ NJS_MOTION *motion;
  /* 0x04 */ Sint16 mtnmode;
  /* 0x06 */ Sint16 next;
  /* 0x08 */ Float frame;
  /* 0x0C */ Float racio;
} enemy_action;

// read by fn_8011E19C, which draws the blend between two motions
typedef struct enemy_mtnlink // sizeof=0x10
{
  /* 0x00 */ NJS_MOTION *motion[2];
  /* 0x08 */ Float frame[2];
} enemy_mtnlink;

// motion state, advanced by fn_80015C40
typedef struct enemy_mtnwk // sizeof=0x30
{
  /* 0x00 */ Float nframe;
  /* 0x04 */ Uint8 unk_04[0xC];
  /* 0x10 */ Sint8 mtnmode;
  /* 0x11 */ Sint8 unk_11;
  /* 0x12 */ Sint8 action;
  /* 0x13 */ Sint8 reqaction;
  /* 0x14 */ Uint32 unk_14;
  /* 0x18 */ enemy_action *actptr;
  /* 0x1C */ NJS_CNK_OBJECT *object;
  /* 0x20 */ enemy_mtnlink link;
} enemy_mtnwk;

// enemy work allocated by fn_80018D28, sizeof=0x210
// field names modelled on SADX's enemywk
typedef struct enemywk {
  /* 0x000 */ NJS_VECTOR spd;
  /* 0x00C */ Uint8 unk_0C[0x38];
  /* 0x044 */ Sint8 pno; // player that destroyed it
  /* 0x045 */ Uint8 unk_45[3];
  /* 0x048 */ Sint16 wait;
  /* 0x04A */ Uint8 unk_4A[2];
  /* 0x04C */ Sint16 flag;
  /* 0x04E */ Uint8 unk_4E[0xA];
  /* 0x058 */ NJS_POINT3 home_pos;
  /* 0x064 */ Uint8 unk_64[0x34];
  /* 0x098 */ Float scl;
  /* 0x09C */ Uint8 unk_9C[0x10];
  /* 0x0AC */ NJS_ARGB argb;
  /* 0x0BC */ Uint8 unk_BC[0x30];
  /* 0x0EC */ Float shadow_scl;
  /* 0x0F0 */ Float unk_F0;
  /* 0x0F4 */ Uint8 unk_F4[0xD0];
  /* 0x1C4 */ Angle float_ang;
  /* 0x1C8 */ Angle unk_1C8;
  /* 0x1CC */ Sint32 count;
  /* 0x1D0 */ Uint8 unk_1D0[4];
  /* 0x1D4 */ Angle view_ang;
  /* 0x1D8 */ Float view_range;
  /* 0x1DC */ Float home_range;
  /* 0x1E0 */ enemy_mtnwk mtn;
} enemywk;

extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_80015730(Sint32 pno, NJS_CNK_OBJECT *object,
                        NJS_TEXLIST *texlist, NJS_VECTOR *scl, Sint32);
extern void fn_80015C40(enemy_mtnwk *mtn);
extern void fn_800164FC(taskwk *twp, enemywk *ewp);
extern Sint32 fn_800167E0(taskwk *twp, enemywk *ewp);
extern void fn_80016D10(taskwk *twp, NJS_CNK_MODEL **models,
                        NJS_TEXLIST *texlist);
extern void fn_800176AC(taskwk *twp, enemywk *ewp, Uint32 pno);
extern void fn_80017B94(taskwk *twp, enemywk *ewp);
extern void fn_80017BE4(taskwk *twp, enemywk *ewp);
extern void fn_80017CA4(taskwk *twp, enemywk *ewp);
extern Sint32 fn_80018A4C(taskwk *twp);
extern void fn_80018B88(task *tp);
extern enemywk *fn_80018D28(task *tp);
extern void fn_80024CB8(Sint32);
extern Float fn_8002AE5C(Float val, Float target, Float step);
extern Sint32 fn_80035C54(task *tp);
extern Uint32 fn_80037C84(NJS_POINT3 *pos);
extern void fn_80062E0C(Sint32 score);
extern void fn_80069D60(Sint32 tone, void *id, Sint32 pri, Sint32 volume,
                        Sint32 timer, NJS_POINT3 *pos);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);
extern void fn_80072820(const char *name, NJS_TEXLIST *texlist);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_8011E19C(NJS_CNK_OBJECT *object, enemy_mtnlink *link,
                        Float frame);
extern void fn_8011E214(NJS_CNK_OBJECT *object, NJS_MOTION *motion,
                        Float frame);

extern NJS_MATRIX nj_unit_matrix;
extern Sint8 lbl_803AD926;

extern NJS_TEXLIST _rename_e_gold_texlist;
extern NJS_CNK_OBJECT _rename_e_gold_object;
extern NJS_MDATA2 _rename_e_gold_mdata[];
extern NJS_CNK_MODEL _rename_e_gold_broken_model_0;
extern NJS_CNK_MODEL _rename_e_gold_broken_model_1;
extern NJS_CNK_MODEL _rename_e_gold_broken_model_2;
extern NJS_CNK_MODEL _rename_e_gold_broken_model_3;
extern NJS_CNK_MODEL _rename_e_gold_broken_model_4;
extern Sint16 _rename_e_gold_plist_0[];
extern Sint16 _rename_e_gold_plist_1[];
extern Sint16 _rename_e_gold_plist_2[];
extern Sint16 _rename_e_gold_plist_3[];
extern Sint16 _rename_e_gold_plist_4[];

// the task that owns the textures; static in the original, defined elsewhere
extern task *_rename_e_gold_mantex_tp;
#define mantex_tp _rename_e_gold_mantex_tp

// ^ extern
// v in this file

static void EnemyGoldDisplayer(task *tp);
static void EnemyGoldDestructor(task *tp);

enum {
  MD_INIT,
  MD_NORMAL,
  MD_HIDE,
  MD_APPEAR,
  MD_VANISH,
  MD_DEAD,
  MD_END,
};

// enemy_action index
enum {
  ACT_GOLD,
};

// lo <= n <= hi, as one unsigned byte compare
#define InRange(n, lo, hi) ((Uint8)((n) - (lo)) <= (hi) - (lo))

static NJS_MOTION e_gold_motion = {_rename_e_gold_mdata, 23, 3, 2};

static CCL_INFO e_gold_colli_info[] = {
    {0, CI_FORM_CYLINDER, 7, 0x21, 0x8400, {0.0f, 0.0f, 0.0f}, 6.0f, 6.0f,
     0.0f, 0.0f, 0, 0, 0},
};

static NJS_CNK_MODEL *e_gold_broken_models[] = {
    &_rename_e_gold_broken_model_0, &_rename_e_gold_broken_model_1,
    &_rename_e_gold_broken_model_2, &_rename_e_gold_broken_model_3,
    &_rename_e_gold_broken_model_4, NULL,
};

static enemy_action e_gold_actions[] = {
    {&e_gold_motion, 3, ACT_GOLD, 1.0f, 0.08f},
};

static void ManTexDestructor(task *tp) {
  fn_801166F0(&_rename_e_gold_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

static void EnemyGoldInit(task *tp, taskwk *twp) {
  enemywk *ewp;
  Uint32 count = lbl_801CC168._7C;

  ewp = fn_80018D28(tp);
  twp->smode = 0;
  CCL_Init(tp, e_gold_colli_info, 1, CID_ENEMY2);
  ewp->wait = (twp->ang.x & 0xFF) * 10;
  twp->ang.z &= 0xFF00;
  ewp->scl = 0.0f;
  twp->ang.x &= 0xFF00;
  fn_80015C40(&ewp->mtn);
  EnemyLightLoadTexture();
  ewp->view_range = (60.0f + twp->scl.z) * (60.0f + twp->scl.z);
  ewp->view_ang = 0x4000;
  ewp->home_range = (60.0f + twp->scl.z) * (60.0f + twp->scl.z);
  ewp->spd.x = 0.4f;
  ewp->unk_1C8 = 0x180;
  ewp->float_ang = twp->ang.x + count * twp->ang.z;
  fn_80017BE4(twp, ewp);

  ewp->mtn.actptr = e_gold_actions;
  twp->mode = MD_HIDE;
  ewp->mtn.object = &_rename_e_gold_object;
  ewp->mtn.reqaction = ACT_GOLD;
  twp->cwp->info->attr |= 0x10;

  fn_80015C40(&ewp->mtn);
  ewp->shadow_scl = 12.0f;
  ewp->unk_F0 = 1.2f;
  ewp->flag |= 0x10;
  ewp->flag |= 0x4;
  tp->disp = EnemyGoldDisplayer;
  tp->dest = EnemyGoldDestructor;

  if (mantex_tp == NULL) {
    fn_80072820("E_GOLDTEX", &_rename_e_gold_texlist);
    mantex_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
    mantex_tp->dest = ManTexDestructor;
  }
}

static void EnemyGoldTurn(taskwk *twp, enemywk *ewp) {
  Uint32 pno;

  if (lbl_801CC168._37 != 0) {
    return;
  }

  if (twp->flag & 0x1000) {
    pno = fn_80037C84(&twp->pos);
    if (pno <= 1) {
      fn_800176AC(twp, ewp, pno);
    }
  }
}

static void EnemyGoldSetEnd(taskwk *twp, enemywk *ewp) { twp->mode = MD_END; }

static void EnemyGoldCheckDamage(task *tp, taskwk *twp, enemywk *ewp) {
  Sint32 score;
  Sint32 pno;
  NJS_POINT2 unused; // unused
  NJS_VECTOR scl;

  if (fn_800167E0(twp, ewp) == 0) {
    return;
  }

  twp->mode = MD_DEAD;
  twp->cwp->info->attr |= 0x10;
  twp->wtimer = 0;
  fn_80017BE4(twp, ewp);
  fn_80016D10(twp, e_gold_broken_models, &_rename_e_gold_texlist);

  score = 1000;
  if (fn_80035C54(tp) != 0) {
    score <<= 1;
  }
  fn_80062E0C(score);
  fn_800164FC(twp, ewp);
  if (!(ewp->flag & 0x400)) {
    return;
  }

  pno = ewp->pno;
  scl.x = scl.y = scl.z = 0.15f;
  fn_80015730(pno, &_rename_e_gold_object, &_rename_e_gold_texlist, &scl, 0);
}

static void EnemyGoldSearchPlayer(taskwk *twp, enemywk *ewp) {
  if (fn_80018A4C(twp)) {
    twp->flag |= 0x1000;
  } else {
    twp->flag &= ~0x1000;
  }
}

// uses its own frame count, not the global one
static void EnemyGoldFloat(taskwk *twp, enemywk *ewp) {
  Float f = njSin(twp->ang.x + ewp->count * twp->ang.z);

  ewp->count++;
  twp->pos.y = ewp->home_pos.y + f * twp->scl.y;
  ewp->float_ang += twp->ang.z;
}

static void EnemyGoldDestructor(task *tp) { fn_80018B88(tp); }

static void EnemyGoldDeadOut(task *tp) { DeadOut(tp); }

static void EnemyGoldNormal(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyGoldSearchPlayer(twp, ewp);
  EnemyGoldTurn(twp, ewp);
  EnemyGoldFloat(twp, ewp);
  EnemyGoldCheckDamage(tp, twp, ewp);
  if (twp->mode == MD_DEAD) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }
  if (twp->wtimer > ewp->wait) {
    twp->mode = MD_VANISH;
    twp->wtimer = 0;
    twp->flag &= ~0x1000;
  }
}

static void EnemyGoldHide(taskwk *twp, enemywk *ewp) {
  STACK_PAD_VAR(2);

  EnemyGoldSearchPlayer(twp, ewp);
  if (twp->flag & 0x1000) {
    twp->mode = MD_APPEAR;
    ewp->argb.a = -1.0f;
    ewp->argb.r = 1.0f;
    ewp->argb.g = 1.0f;
    ewp->argb.b = 1.0f;
    twp->flag &= ~0x1000;
    ewp->shadow_scl = 0.0f;
    fn_8006B7EC(0x400F, NULL, 0, 0x7F, &twp->pos);
  }
  EnemyGoldTurn(twp, ewp);
}

// spins and grows while the white flash fades in, then the flash fades out
static void EnemyGoldAppear(taskwk *twp, enemywk *ewp) {
  twp->ang.y += 8192.0f * -ewp->argb.a;
  ewp->scl = fn_8002AE5C(ewp->scl, 1.0f, 0.1f);
  ewp->shadow_scl = 12.0f * ewp->scl;
  if (ewp->argb.a < 0.0f) {
    ewp->argb.a = fn_8002AE5C(ewp->argb.a, 0.0f, 0.05f);
  } else if (ewp->argb.r > 0.0f) {
    ewp->argb.r = fn_8002AE5C(ewp->argb.r, 0.0f, 0.1f);
    ewp->argb.b = ewp->argb.g = ewp->argb.r;
  } else {
    twp->mode = MD_NORMAL;
    twp->wtimer = 0;
    twp->cwp->info->attr &= ~0x10;
  }
}

static void EnemyGoldVanish(taskwk *twp, enemywk *ewp) {
  twp->ang.y -= 8192.0f * -ewp->argb.a;
  if (ewp->argb.a <= -0.5f) {
    ewp->scl = fn_8002AE5C(ewp->scl, 0.0f, 0.1f);
    ewp->shadow_scl = 12.0f * ewp->scl;
  }

  if (ewp->argb.r < 1.0f) {
    ewp->argb.r = fn_8002AE5C(ewp->argb.r, 1.0f, 0.1f);
    ewp->argb.b = ewp->argb.g = ewp->argb.r;
  } else if (ewp->argb.a > -1.0f) {
    ewp->argb.a = fn_8002AE5C(ewp->argb.a, -1.0f, 0.05f);
  } else {
    twp->mode = MD_DEAD;
    twp->cwp->info->attr |= 0x10;
  }
}

static void EnemyGoldDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;
  STACK_PAD_VAR(6);

  fn_80024CB8(8);
  njSetTexture(&_rename_e_gold_texlist);
  if (twp->mode != MD_HIDE) {
    if (InRange(twp->mode, MD_APPEAR, MD_VANISH)) {
      fn_800156FC(ewp->argb.a, ewp->argb.r, ewp->argb.g, ewp->argb.b);
      _rename_e_gold_plist_0[46] |= 0x800;
      _rename_e_gold_plist_0[138] |= 0x800;
      _rename_e_gold_plist_1[10] |= 0x800;
      _rename_e_gold_plist_2[10] |= 0x800;
      _rename_e_gold_plist_3[10] |= 0x800;
      _rename_e_gold_plist_3[264] |= 0x800;
      _rename_e_gold_plist_4[8] |= 0x800;
    }

    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y + 0x4000);
    njScale(NULL, ewp->scl, 1.0f, ewp->scl);
    if (ewp->mtn.mtnmode != 2) {
      fn_8011E214(ewp->mtn.object, ewp->mtn.actptr[ewp->mtn.action].motion,
                  ewp->mtn.nframe);
    } else {
      fn_8011E19C(ewp->mtn.object, &ewp->mtn.link, ewp->mtn.nframe);
    }
    njPopMatrixEx();

    if (njFraction(twp->scl.x) == 0.0f && ewp->shadow_scl > 0.0f) {
      fn_80017CA4(twp, ewp);
    }

    if (twp->mode == MD_APPEAR || twp->mode == MD_VANISH) {
      fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
      _rename_e_gold_plist_0[46] &= ~0x800;
      _rename_e_gold_plist_0[138] &= ~0x800;
      _rename_e_gold_plist_1[10] &= ~0x800;
      _rename_e_gold_plist_2[10] &= ~0x800;
      _rename_e_gold_plist_3[10] &= ~0x800;
      _rename_e_gold_plist_3[264] &= ~0x800;
      _rename_e_gold_plist_4[8] &= ~0x800;
    }
  }

  fn_80024CB8(lbl_803AD926);
}

void EnemyGold(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;
  NJS_POINT3 pos;

  if (twp->mode != MD_INIT && CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case MD_INIT:
    EnemyGoldInit(tp, twp);
    return;
  case MD_NORMAL:
    EnemyGoldNormal(tp, twp, ewp);
    break;
  case MD_DEAD:
    EnemyGoldSetEnd(twp, ewp);
  case MD_END:
    EnemyGoldDeadOut(tp);
    return;
  case MD_HIDE:
    EnemyGoldHide(twp, ewp);
    break;
  case MD_APPEAR:
    EnemyGoldAppear(twp, ewp);
    break;
  case MD_VANISH:
    EnemyGoldVanish(twp, ewp);
    if (twp->mode != MD_VANISH) {
      tp->disp = NULL;
    }
    break;
  }

  if (lbl_801CC168._37 == 0) {
    fn_80017B94(twp, ewp);
    switch (twp->mode) {
    case MD_NORMAL:
      fn_80069D60(0x400D, twp, 1, 10, 5, &twp->pos);
      break;
    case MD_APPEAR:
    case MD_VANISH:
      fn_80069D60(0x400D, twp, 1, 10.0f + 127.0f * ewp->argb.a, 5, &twp->pos);
      break;
    }
    fn_80015C40(&ewp->mtn);

    if (twp->flag & 0x1000) {
      njPushMatrix(&nj_unit_matrix);
      njTranslateEx(&twp->pos);
      njRotateY(NULL, twp->ang.y);
      pos.x = 10.0f;
      pos.y = 4.6f;
      pos.z = 0.0f;
      njCalcPoint(NULL, &pos, &pos);
      CreateEnemyLight(tp, 1, &pos);
      njPopMatrixEx();
    }
  }

  if (twp->mode != MD_DEAD) {
    CCL_Entry(tp);
  }
}
