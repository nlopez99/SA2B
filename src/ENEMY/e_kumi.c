#include "ENEMY/e_kumi.h"

#include "ENEMY/e_light.h"

#include "CCL.h"
#include "ENEMY/e_bullet.h"
#include "fabsf.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njmotion.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "samt/sonic/shadow.h"
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

// enemy work allocated by fn_80018C88, sizeof=0x210
// field names modelled on SADX's enemywk
typedef struct enemywk {
  /* 0x000 */ NJS_VECTOR spd;
  /* 0x00C */ NJS_VECTOR acc;
  /* 0x018 */ Uint8 unk_18[4];
  /* 0x01C */ Angle home_ang;
  /* 0x020 */ Uint8 unk_20[0x24];
  /* 0x044 */ Sint8 pno; // player that destroyed it
  /* 0x045 */ Sint8 unk_45;
  /* 0x046 */ Sint8 spark_tex;
  /* 0x047 */ Sint8 shot_num; // bullets in a burst, or the kind of bomb
  /* 0x048 */ Sint16 wait;
  /* 0x04A */ Uint8 unk_4A[2];
  /* 0x04C */ Sint16 flag;
  /* 0x04E */ Sint16 spark_frame;
  /* 0x050 */ Uint8 unk_50[8];
  /* 0x058 */ NJS_POINT3 home_pos;
  /* 0x064 */ Uint8 unk_64[0x30];
  /* 0x094 */ Float shot_spd;
  /* 0x098 */ Float scl; // spring power of a KUMI_SPRING
  /* 0x09C */ Uint8 unk_9C[0x10];
  /* 0x0AC */ NJS_ARGB argb;
  /* 0x0BC */ NJS_POINT3 colli_center;
  /* 0x0C8 */ Float colli_top;
  /* 0x0CC */ Float colli_radius;
  /* 0x0D0 */ Float colli_bottom;
  /* 0x0D4 */ Uint8 unk_D4[0x18];
  /* 0x0EC */ Float shadow_scl;
  /* 0x0F0 */ Float unk_F0;
  /* 0x0F4 */ Uint8 unk_F4[4];
  /* 0x0F8 */ Float ground_y;
  /* 0x0FC */ Uint8 unk_FC[0x54];
  /* 0x150 */ Float unk_150;
  /* 0x154 */ Uint8 unk_154[0x70];
  /* 0x1C4 */ Angle float_ang;
  /* 0x1C8 */ Angle unk_1C8;
  /* 0x1CC */ Sint32 count;
  /* 0x1D0 */ Uint8 unk_1D0[4];
  /* 0x1D4 */ Angle view_ang;
  /* 0x1D8 */ Float view_range;
  /* 0x1DC */ Float home_range;
  /* 0x1E0 */ enemy_mtnwk mtn;
} enemywk;

extern task *fn_80005F88(task *tp, Sint32);
extern void fn_800068BC(task *tp, Float range);
extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_80015730(Sint32 pno, NJS_CNK_OBJECT *object,
                        NJS_TEXLIST *texlist, NJS_VECTOR *scl, Sint32);
extern void fn_80015B18(taskwk *twp, enemywk *ewp);
extern void fn_80015C40(enemy_mtnwk *mtn);
extern void fn_800164FC(taskwk *twp, enemywk *ewp);
extern Sint32 fn_800167E0(taskwk *twp, enemywk *ewp);
extern void fn_80016D10(taskwk *twp, NJS_CNK_MODEL **models,
                        NJS_TEXLIST *texlist);
extern void fn_800176AC(taskwk *twp, enemywk *ewp, Uint32 pno);
extern void fn_80017918(taskwk *twp, enemywk *ewp, Float range);
extern void fn_800179F0(taskwk *twp, enemywk *ewp);
extern Float fn_80017AB8(taskwk *twp, enemywk *ewp);
extern void fn_80017B94(taskwk *twp, enemywk *ewp);
extern void fn_80017BE4(taskwk *twp, enemywk *ewp);
extern void fn_80017C00(taskwk *twp, enemywk *ewp);
extern Sint32 fn_800187C4(taskwk *twp, enemywk *ewp);
extern Sint32 fn_8001897C(taskwk *twp, enemywk *ewp);
extern void fn_80018B88(task *tp);
extern enemywk *fn_80018C88(task *tp);
extern void fn_80024CB8(Sint32);
extern Float fn_8002AE5C(Float val, Float target, Float step);
extern Sint32 fn_8002B958(NJS_POINT3 *pos, Float range);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern Sint32 fn_80035C54(task *tp);
extern Uint32 fn_80037C84(NJS_POINT3 *pos);
extern void fn_80062E0C(Sint32 score);
extern void fn_80069D60(Sint32 tone, void *id, Sint32 pri, Sint32 volume,
                        Sint32 timer, NJS_POINT3 *pos);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);
extern void fn_80072820(const char *name, NJS_TEXLIST *texlist);
extern void fn_80073028(NJS_CNK_MODEL *model, void *info, Uint32 frame);
extern void fn_800BD92C(task *tp, NJS_POINT3 *pos, Float range);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_8011E17C(NJS_CNK_OBJECT *object);
extern void fn_8011E19C(NJS_CNK_OBJECT *object, enemy_mtnlink *link,
                        Float frame);
extern void fn_8011E214(NJS_CNK_OBJECT *object, NJS_MOTION *motion,
                        Float frame);
extern void *fn_8011F504(NJS_CNK_OBJECT *object, void *buf);
extern Sint32 fn_8011F50C(NJS_CNK_OBJECT *object);
extern void fn_8011F520_nop(void);
extern void SE_Call(int, int, int, int);
extern Float asinf(Float x);
extern Float atan2f(Float y, Float x);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);

extern NJS_MATRIX nj_unit_matrix;
extern Sint8 lbl_803AD926;

extern void EnemyBombLoadTexture(void);
extern task *CreateEnemyBomb(task *ptp, Sint32 kind, Sint32 type,
                                     NJS_POINT3 *pos);



extern NJS_TEXLIST _rename_e_kumi_texlist;
extern NJS_TEXLIST _rename_e_g_kumi_texlist;
extern NJS_TEXLIST _rename_e_b_kumi_texlist;
extern NJS_TEXLIST _rename_e_s_kumi_texlist;
extern NJS_TEXLIST _rename_e_e_kumi_texlist;
extern NJS_CNK_OBJECT _rename_e_kumi_object;
extern NJS_CNK_OBJECT _rename_e_g_kumi_object;
extern NJS_CNK_OBJECT _rename_e_b_kumi_object;
extern NJS_CNK_OBJECT _rename_e_s_kumi_object;
extern Sint16 _rename_e_kumi_plist_0[];
extern Sint16 _rename_e_kumi_plist_1[];
extern Sint16 _rename_e_kumi_plist_2[];
extern Sint16 _rename_e_kumi_plist_3[];
extern Sint16 _rename_e_kumi_plist_4[];
extern NJS_CNK_MODEL *_rename_e_kumi_broken_models[];
extern NJS_CNK_MODEL *_rename_e_g_kumi_broken_models[];
extern NJS_CNK_MODEL *_rename_e_b_kumi_broken_models[];
extern NJS_CNK_MODEL _rename_e_s_kumi_broken_model_0;
extern NJS_CNK_MODEL _rename_e_s_kumi_broken_model_1;
extern NJS_MOTION _rename_e_kumi_motion;
extern NJS_MOTION _rename_e_g_kumi_motion_0;
extern NJS_MOTION _rename_e_g_kumi_motion_1;
extern NJS_MOTION _rename_e_b_kumi_motion;
extern NJS_MOTION _rename_e_s_kumi_motion;
extern NJS_CNK_MODEL _rename_e_e_kumi_spark_model;
extern NJS_CNK_OBJECT _rename_e_e_kumi_spark_object;
extern Uint32 _rename_e_e_kumi_spark_uvanim[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_0[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_1[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_2[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_3[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_4[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_5[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_6[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_7[];
extern NJS_TEXNAME _rename_e_e_kumi_spark_texname_8[];
extern CCL_INFO _rename_e_kumi_colli_info[];

// ^ extern
// v in this file

static void EnemyKumiDisplayer(task *tp);
static void EnemyKumiDisplayerSort(task *tp);
static void EnemyKumiDestructor(task *tp);

enum {
  MD_INIT,
  MD_NORMAL,
  MD_HIDE,
  MD_APPEAR,
  MD_VANISH,
  MD_ELEC,
  MD_BOMB,
  MD_GUN,
  MD_DEAD,
  MD_END,
};

// enemy_action index
enum {
  ACT_KUMI,
  ACT_BOMB,
  ACT_GUN,
  ACT_GUN_SHOT,
  ACT_SPRING,
  ACT_SPRING_JUMP,
};

#define SPARK_TEX_NUM 9

#define RadAng(n) ((Angle)(10430.38043493439 * (n)))

// lo <= n <= hi, as one unsigned byte compare
#define InRange(n, lo, hi) ((Uint8)((n) - (lo)) <= (hi) - (lo))

static NJS_CNK_MODEL *e_s_kumi_broken_models[] = {
    &_rename_e_s_kumi_broken_model_0,
    &_rename_e_s_kumi_broken_model_1,
    NULL,
};

static NJS_TEXLIST e_e_kumi_spark_texlists[SPARK_TEX_NUM] = {
    {_rename_e_e_kumi_spark_texname_0, 1},
    {_rename_e_e_kumi_spark_texname_1, 1},
    {_rename_e_e_kumi_spark_texname_2, 1},
    {_rename_e_e_kumi_spark_texname_3, 1},
    {_rename_e_e_kumi_spark_texname_4, 1},
    {_rename_e_e_kumi_spark_texname_5, 1},
    {_rename_e_e_kumi_spark_texname_6, 1},
    {_rename_e_e_kumi_spark_texname_7, 1},
    {_rename_e_e_kumi_spark_texname_8, 1},
};

static enemy_action e_kumi_actions[] = {
    {&_rename_e_kumi_motion, 3, ACT_KUMI, 1.0f, 0.08f},
    {&_rename_e_b_kumi_motion, 3, ACT_BOMB, 1.0f, 0.08f},
    {&_rename_e_g_kumi_motion_0, 3, ACT_GUN, 1.0f, 0.08f},
    {&_rename_e_g_kumi_motion_1, 4, ACT_GUN, 1.0f, 0.2f},
    {&_rename_e_s_kumi_motion, 10, ACT_SPRING, 1.0f, 0.08f},
    {&_rename_e_s_kumi_motion, 4, ACT_SPRING, 1.0f, 0.08f},
};

// tasks that own the textures; awp holds the prebuilt model
static task *mantex_tp;
static task *mantex_g_tp;
static task *mantex_b_tp;
static task *mantex_s_tp;
static task *mantex_e_tp;

static void ManTexDestructor(task *tp) {
  fn_801166F0(&_rename_e_kumi_texlist);
  mantex_tp = NULL;
}

static void ManTexDestructorG(task *tp) {
  fn_801166F0(&_rename_e_g_kumi_texlist);
  mantex_g_tp = NULL;
}

static void ManTexDestructorB(task *tp) {
  fn_801166F0(&_rename_e_b_kumi_texlist);
  mantex_b_tp = NULL;
}

static void ManTexDestructorS(task *tp) {
  fn_801166F0(&_rename_e_s_kumi_texlist);
  mantex_s_tp = NULL;
}

static void ManTexDestructorE(task *tp) {
  fn_801166F0(&_rename_e_e_kumi_texlist);
  mantex_e_tp = NULL;
}

static void ManTex(task *tp) {}

static void EnemyKumiInit(task *tp, taskwk *twp) {
  Uint32 count = lbl_801CC168._7C;
  enemywk *ewp = fn_80018C88(tp);
  Angle3 ang;
  Float y;
  Float f;
  Angle a;
  void *buf;

  twp->smode = njRoundOff(twp->scl.x);
  if (twp->smode >= KUMI_NUM || twp->smode < 0) {
    twp->smode = KUMI_FLOAT;
  }

  if (twp->smode == KUMI_ELEC || twp->smode == KUMI_ELEC_MOVE) {
    CCL_Init(tp, _rename_e_kumi_colli_info, 2, CID_ENEMY2);
    twp->cwp->info[1].attr |= 0x10;
    fn_800068BC(tp, 15.0f);
  } else if (twp->smode == KUMI_SPRING) {
    CCL_Init(tp, &_rename_e_kumi_colli_info[2], 2, CID_ENEMY2);
    fn_800068BC(tp, 18.0f);
  } else {
    CCL_Init(tp, _rename_e_kumi_colli_info, 1, CID_ENEMY2);
    fn_800068BC(tp, 9.0f);
  }

  if (twp->smode == KUMI_ELEC || twp->smode == KUMI_ELEC_MOVE) {
    ewp->wait = twp->scl.y;
    twp->wtimer = (count + (twp->ang.x & 0xFF)) % (ewp->wait + 302);
    twp->scl.y = 2.0f;
  } else if (twp->smode == KUMI_SPRING) {
    ewp->scl = twp->ang.x & 0xFF;
  } else if (twp->smode == KUMI_GUN) {
    ewp->shot_num = twp->ang.x & 0xF;
    if (twp->ang.x & 0xF0) {
      twp->flag |= 0x8000;
    }
    ewp->wait = twp->ang.z & 0xFF;
    if (ewp->wait < 60) {
      ewp->wait = 60;
    }
    twp->ang.z &= 0xFF00;
    ewp->shot_spd = twp->scl.y;
    twp->scl.y = 2.0f;
    EnemyBulletLoadTexture();
    fn_80015B18(twp, ewp);
  } else if (twp->smode == KUMI_BOMB) {
    ewp->shot_num = twp->ang.x & 0xFF;
    if (ewp->shot_num >= 5 || ewp->shot_num < 0) {
      ewp->shot_num = 0;
    }
    ewp->home_ang = twp->ang.y & 0xFF00;
    ewp->wait = twp->scl.y;
    twp->scl.y = 2.0f;
    EnemyBombLoadTexture();
  } else if (twp->smode == KUMI_HIDE) {
    ewp->scl = 0.0f;
    twp->ang.z &= ~1;
  }

  twp->ang.x &= 0xFF00;
  fn_80015C40(&ewp->mtn);
  EnemyLightLoadTexture();
  ewp->view_range = (60.0f + twp->scl.z) * (60.0f + twp->scl.z);
  ewp->view_ang = 0x4000;
  ewp->home_range = (60.0f + twp->scl.z) * (60.0f + twp->scl.z);
  ewp->spd.x = 0.4f;
  ewp->unk_1C8 = 0x180;
  if (twp->smode == KUMI_MOVE || twp->smode == KUMI_ELEC_MOVE) {
    y = GetShadowPos(twp->pos.x, twp->pos.y, twp->pos.z, &ang);
    if (y == -1000000.0f) {
      ewp->colli_bottom = -20.0f;
    } else {
      ewp->colli_bottom = y - twp->pos.y;
    }
  }

  ewp->float_ang = twp->ang.x + count * twp->ang.z;
  fn_80017BE4(twp, ewp);
  switch (twp->smode) {
  case KUMI_FLOAT:
  case KUMI_ELEC:
  case KUMI_GUN:
  case KUMI_SPRING:
    f = njSin(twp->ang.x + count * twp->ang.z);
    twp->pos.y = ewp->home_pos.y + f * twp->scl.y;
    break;
  case KUMI_HIDE:
    f = njSin(twp->ang.x);
    twp->pos.y = ewp->home_pos.y + f * twp->scl.y;
    break;
  case KUMI_BOMB:
    twp->ang.y = ewp->home_ang + twp->ang.z * count;
    a = twp->ang.y + 0x4000;
    twp->pos.x = ewp->home_pos.x + twp->scl.z * njCos(a);
    twp->pos.z = ewp->home_pos.z - twp->scl.z * njSin(a);
  case KUMI_MOVE:
  case KUMI_ELEC_MOVE:
    f = njSin(count * twp->ang.z);
    twp->pos.y = ewp->home_pos.y + f * twp->scl.y;
    break;
  }

  ewp->mtn.actptr = e_kumi_actions;
  if (twp->smode == KUMI_HIDE) {
    twp->mode = MD_HIDE;
    ewp->mtn.object = &_rename_e_kumi_object;
    ewp->mtn.reqaction = ACT_KUMI;
    twp->cwp->info->attr |= 0x10;
  } else if (twp->smode == KUMI_GUN) {
    twp->mode = MD_GUN;
    ewp->mtn.object = &_rename_e_g_kumi_object;
    ewp->mtn.reqaction = ACT_GUN;
  } else if (twp->smode == KUMI_BOMB) {
    twp->mode = MD_BOMB;
    ewp->mtn.object = &_rename_e_b_kumi_object;
    ewp->mtn.reqaction = ACT_BOMB;
  } else if (twp->smode == KUMI_SPRING) {
    twp->mode = MD_NORMAL;
    ewp->mtn.object = &_rename_e_s_kumi_object;
    ewp->mtn.reqaction = ACT_SPRING;
  } else if (twp->smode == KUMI_ELEC) {
    if (twp->wtimer > ewp->wait) {
      twp->cwp->info[1].attr &= ~0x10;
      twp->mode = MD_ELEC;
      twp->wtimer -= ewp->wait + 1;
    } else {
      twp->mode = MD_NORMAL;
    }
    ewp->mtn.object = &_rename_e_kumi_object;
    ewp->mtn.reqaction = ACT_KUMI;
  } else {
    twp->mode = MD_NORMAL;
    ewp->mtn.object = &_rename_e_kumi_object;
    ewp->mtn.reqaction = ACT_KUMI;
  }

  fn_80015C40(&ewp->mtn);
  ewp->shadow_scl = 12.0f;
  ewp->unk_F0 = 1.2f;
  ewp->flag |= 0x10;
  ewp->flag |= 0x4;
  tp->disp = EnemyKumiDisplayer;
  tp->disp_sort = EnemyKumiDisplayerSort;
  tp->dest = EnemyKumiDestructor;

  if (twp->smode == KUMI_GUN) {
    if (mantex_g_tp == NULL) {
      fn_80072820("E_G_KUMITEX", &_rename_e_g_kumi_texlist);
      mantex_g_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
      mantex_g_tp->dest = ManTexDestructorG;
      if (lbl_801CC168._3C == 1) {
        njSetTexture(&_rename_e_g_kumi_texlist);
        fn_80024CB8(8);
        njEnableFog();
        gjSetFog();
        fn_8011F520_nop();
        if ((mantex_g_tp->awp =
                 syCalloc(1, fn_8011F50C(&_rename_e_g_kumi_object))) != NULL) {
          buf = mantex_g_tp->awp;
          fn_8011F504(&_rename_e_g_kumi_object, buf);
        }
        fn_80024CB8(lbl_803AD926);
      }
    }
  } else if (twp->smode == KUMI_BOMB) {
    if (mantex_b_tp == NULL) {
      fn_80072820("E_B_KUMITEX", &_rename_e_b_kumi_texlist);
      mantex_b_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
      mantex_b_tp->dest = ManTexDestructorB;
    }
  } else if (twp->smode == KUMI_SPRING) {
    if (mantex_s_tp == NULL) {
      fn_80072820("E_S_KUMITEX", &_rename_e_s_kumi_texlist);
      mantex_s_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
      mantex_s_tp->dest = ManTexDestructorS;
    }
  } else {
    if (InRange(twp->smode, KUMI_ELEC, KUMI_ELEC_MOVE) &&
        mantex_e_tp == NULL) {
      fn_80072820("E_E_KUMITEX", &_rename_e_e_kumi_texlist);
      mantex_e_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
      mantex_e_tp->dest = ManTexDestructorE;
    }
    if (mantex_tp == NULL) {
      fn_80072820("E_KUMITEX", &_rename_e_kumi_texlist);
      mantex_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
      mantex_tp->dest = ManTexDestructor;
      if (lbl_801CC168._3C == 1) {
        njSetTexture(&_rename_e_kumi_texlist);
        fn_80024CB8(8);
        njEnableFog();
        gjSetFog();
        fn_8011F520_nop();
        if ((mantex_tp->awp =
                 syCalloc(1, fn_8011F50C(&_rename_e_kumi_object))) != NULL) {
          buf = mantex_tp->awp;
          fn_8011F504(&_rename_e_kumi_object, buf);
        }
        fn_80024CB8(lbl_803AD926);
      }
    }
  }

  // the prebuilt model can be drawn instead of the animated one
  if (lbl_801CC168._3C != 1) {
    return;
  }
  if (ewp->mtn.object == &_rename_e_kumi_object && (twp->ang.z & 1) &&
      mantex_tp != NULL && mantex_tp->awp != NULL) {
    twp->flag |= 0x4000;
  }
  if (ewp->mtn.object == &_rename_e_g_kumi_object && mantex_g_tp != NULL &&
      mantex_g_tp->awp != NULL) {
    twp->flag |= 0x4000;
  }
}

// turns back home when it strayed too far, else towards the player it found
static void EnemyKumiTurn(taskwk *twp, enemywk *ewp) {
  Uint32 pno;

  if (lbl_801CC168._37 != 0) {
    return;
  }

  if (twp->flag & 0x1000) {
    fn_800179F0(twp, ewp);
  } else if (twp->flag & 0x2000) {
    pno = fn_80037C84(&twp->pos);
    if (pno <= 1) {
      fn_800176AC(twp, ewp, pno);
    }
  }
}

static void EnemyKumiCheckHome(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37 != 0) {
    return;
  }

  if (fn_80017AB8(twp, ewp) > ewp->home_range) {
    twp->flag |= 0x1000;
  } else {
    twp->flag &= ~0x1000;
  }
}

static void EnemyKumiSetEnd(taskwk *twp, enemywk *ewp) { twp->mode = MD_END; }

static void EnemyKumiCheckDamage(task *tp, taskwk *twp, enemywk *ewp) {
  Sint32 hit = 0;
  task *hit_tp;
  Sint32 pno;
  NJS_VECTOR scl;

  if (twp->smode == KUMI_MOVE && CCL_IsHitPlayer(tp) != NULL) {
    hit = 1;
  }

  if (twp->smode == KUMI_SPRING && (hit_tp = fn_80005F88(tp, 1)) != NULL) {
    hit = IsThisTaskPlayer(hit_tp);
    SetSpringVelocityP(hit, 0.0f, ewp->scl, 0.0f);
    ewp->mtn.reqaction = ACT_SPRING_JUMP;
    SE_Call(0x1000, 0, 0, 0);
    fn_8002FB2C(hit, 4, 15, 0);
    return;
  }

  if (fn_800167E0(twp, ewp) == 0 && hit != 1) {
    return;
  }

  twp->mode = MD_DEAD;
  twp->cwp->info->attr |= 0x10;
  twp->wtimer = 0;
  fn_80017BE4(twp, ewp);
  if (twp->smode == KUMI_GUN) {
    fn_80016D10(twp, _rename_e_g_kumi_broken_models,
                &_rename_e_g_kumi_texlist);
  } else if (twp->smode == KUMI_BOMB) {
    fn_80016D10(twp, _rename_e_b_kumi_broken_models,
                &_rename_e_b_kumi_texlist);
  } else if (twp->smode == KUMI_SPRING) {
    fn_80016D10(twp, e_s_kumi_broken_models, &_rename_e_s_kumi_texlist);
  } else {
    fn_80016D10(twp, _rename_e_kumi_broken_models, &_rename_e_kumi_texlist);
  }

  hit = 100;
  if (fn_80035C54(tp) != 0) {
    hit <<= 1;
  }
  fn_80062E0C(hit);
  fn_800164FC(twp, ewp);
  if (!(ewp->flag & 0x400)) {
    return;
  }

  pno = ewp->pno;
  scl.x = scl.y = scl.z = 0.15f;
  if (twp->smode == KUMI_GUN) {
    fn_80015730(pno, &_rename_e_g_kumi_object, &_rename_e_g_kumi_texlist, &scl,
                0);
  } else if (twp->smode == KUMI_BOMB) {
    fn_80015730(pno, &_rename_e_b_kumi_object, &_rename_e_b_kumi_texlist, &scl,
                0);
  } else if (twp->smode == KUMI_SPRING) {
    fn_80015730(pno, &_rename_e_s_kumi_object, &_rename_e_s_kumi_texlist, &scl,
                0);
  } else {
    fn_80015730(pno, &_rename_e_kumi_object, &_rename_e_kumi_texlist, &scl, 0);
  }
}

static void EnemyKumiSearchPlayer(taskwk *twp, enemywk *ewp) {
  if (twp->smode == KUMI_GUN) {
    if (fn_800187C4(twp, ewp)) {
      twp->flag |= 0x2000;
    } else {
      twp->flag &= ~0x2000;
    }
  } else {
    if (fn_8001897C(twp, ewp)) {
      twp->flag |= 0x2000;
    } else {
      twp->flag &= ~0x2000;
    }
  }
}

static void EnemyKumiMove(taskwk *twp, enemywk *ewp) {
  Uint32 count;
  Float f;

  if (lbl_801CC168._37 != 0) {
    return;
  }

  ewp->spd.x += ewp->acc.x;
  ewp->spd.z += ewp->acc.z;
  if (ewp->spd.x > 0.4f) {
    ewp->spd.x = 0.4f;
  } else if (ewp->spd.x < 0.1f) {
    ewp->spd.x = 0.1f;
  }

  count = lbl_801CC168._7C;
  f = njSin((count - 1) * twp->ang.z);
  f = njSin(count * twp->ang.z) - f;
  twp->pos.y += f * twp->scl.y;
  twp->pos.x +=
      ewp->spd.x * njCos(twp->ang.y) + ewp->spd.z * njSin(twp->ang.y);
  twp->pos.y += ewp->spd.y;
  twp->pos.z +=
      -(ewp->spd.x * njSin(twp->ang.y)) + ewp->spd.z * njCos(twp->ang.y);
  fn_80017C00(twp, ewp);

  // keeps 2 amplitudes above the ground
  if (ewp->unk_150 < ewp->ground_y + ewp->colli_bottom - 2.0f * twp->scl.y) {
    ewp->spd.y -= 0.05f;
    if (ewp->spd.y < -2.0f) {
      ewp->spd.y = -2.0f;
    }
  } else {
    ewp->spd.y *= 0.95f;
    if (fabsf(ewp->spd.y) < 0.1f) {
      ewp->spd.y = 0.0f;
    }
  }
  fn_80017918(twp, ewp, 60.0f + twp->scl.z);
}

static void EnemyKumiFloat(taskwk *twp, enemywk *ewp) {
  Float f = njSin(twp->ang.x + lbl_801CC168._7C * twp->ang.z);

  twp->pos.y = ewp->home_pos.y + f * twp->scl.y;
  ewp->float_ang += twp->ang.z;
}

// counts its own frames instead of the global frame count
static void EnemyKumiFloatCount(taskwk *twp, enemywk *ewp) {
  Float f = njSin(twp->ang.x + ewp->count * twp->ang.z);

  ewp->count++;
  twp->pos.y = ewp->home_pos.y + f * twp->scl.y;
  ewp->float_ang += twp->ang.z;
}

static void EnemyKumiDestructor(task *tp) { fn_80018B88(tp); }

static void EnemyKumiDeadOut(task *tp) { DeadOut(tp); }

// turns the shot velocity v towards the nearest player, a bit at a time
static void EnemyKumiAim(taskwk *twp, NJS_POINT3 *pos, NJS_VECTOR *v) {
  NJS_POINT3 ppos;
  Float dist;
  Float dy;
  Angle angx;
  Angle angy;

  ppos = playertwp[fn_80037C84(&twp->pos)]->cwp->info->center;
  dist = njDistanceP2P(pos, &ppos);
  dy = ppos.y - pos->y;
  if (dist != 0.0f) {
    angx = RadAng(asinf(dy / dist));
    angy = RadAng(atan2f(pos->z - ppos.z, ppos.x - pos->x));
  } else {
    angx = 0;
    angy = twp->ang.y;
  }

  angx = AdjustAngle(-0xC00, angx, 0x1800);
  angy = AdjustAngle(twp->ang.y, angy, 0x800);
  njPushMatrix(&nj_unit_matrix);
  njRotateY(NULL, angy);
  njRotateZ(NULL, angx);
  njCalcVector(NULL, v, v);
  njPopMatrixEx();
}

static void EnemyKumiNormal(task *tp, taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;
  NJS_VECTOR spd;

  EnemyKumiCheckHome(twp, ewp);
  EnemyKumiSearchPlayer(twp, ewp);
  EnemyKumiTurn(twp, ewp);
  if (twp->smode == KUMI_MOVE || twp->smode == KUMI_ELEC_MOVE ||
      twp->smode == KUMI_BOMB) {
    EnemyKumiMove(twp, ewp);
  } else if (twp->smode == KUMI_HIDE) {
    EnemyKumiFloatCount(twp, ewp);
  } else {
    EnemyKumiFloat(twp, ewp);
  }

  EnemyKumiCheckDamage(tp, twp, ewp);
  if (twp->mode == MD_DEAD) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }
  if (twp->smode == KUMI_ELEC || twp->smode == KUMI_ELEC_MOVE) {
    if (twp->wtimer <= ewp->wait) {
      return;
    }
    twp->mode = MD_ELEC;
    twp->wtimer = 0;
    twp->cwp->info[1].attr &= ~0x10;
    return;
  }
  if (twp->smode != KUMI_GUN) {
    return;
  }
  if (ewp->shot_num <= 0) {
    return;
  }

  if (!(twp->flag & 0x2000)) {
    twp->wtimer = 0;
    twp->btimer = 0;
    return;
  }
  if (twp->wtimer <= ewp->wait) {
    return;
  }
  if (ewp->mtn.action != ACT_GUN_SHOT) {
    ewp->mtn.reqaction = ACT_GUN_SHOT;
    return;
  }

  if (ewp->mtn.nframe >= 1.0f && ewp->mtn.nframe < 1.2f) {
    pos.x = 7.5f;
    pos.y = -9.2f;
    pos.z = 0.0f;
    njPushMatrix(&nj_unit_matrix);
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    njCalcPoint(NULL, &pos, &pos);
    njPopMatrixEx();
    spd.x = ewp->shot_spd;
    spd.y = 0.0f;
    spd.z = 0.0f;
    EnemyKumiAim(twp, &pos, &spd);
    CreateEnemyBullet(tp, !(twp->flag & 0x8000), &spd, &pos);
    twp->btimer++;
  }
  if (twp->btimer >= ewp->shot_num) {
    twp->wtimer = 0;
    twp->btimer = 0;
  }
}

static void EnemyKumiElec(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyKumiFloat(twp, ewp);
  EnemyKumiCheckDamage(tp, twp, ewp);
  if (twp->mode == MD_DEAD) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
    ewp->spark_tex++;
    if (ewp->spark_tex >= SPARK_TEX_NUM) {
      ewp->spark_tex = 0;
    }
    ewp->spark_frame++;
    if (twp->wtimer & 1) {
      fn_800BD92C(tp, &twp->pos, 15.0f);
    }
    fn_80069D60(0x400E, twp, 1, 15, 60, &twp->pos);
  }

  if (ewp->wait != 0 && twp->wtimer > 300) {
    twp->mode = MD_NORMAL;
    twp->wtimer = 0;
    twp->cwp->info[1].attr |= 0x10;
  }
}

static void EnemyKumiBomb(task *tp, taskwk *twp, enemywk *ewp) {
  Sint32 unused[2]; // unused
  NJS_POINT3 pos;

  EnemyKumiFloat(twp, ewp);
  if (lbl_801CC168._37 == 0) {
    twp->pos.x = ewp->home_pos.x + twp->scl.z * njCos(twp->ang.y + 0x4000);
    twp->pos.z = ewp->home_pos.z - twp->scl.z * njSin(twp->ang.y + 0x4000);
    twp->ang.y = ewp->home_ang + twp->ang.z * lbl_801CC168._7C;
  }

  EnemyKumiCheckDamage(tp, twp, ewp);
  if (twp->mode == MD_DEAD) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }
  if (twp->wtimer > ewp->wait) {
    pos.x = 0.0f;
    pos.y = -6.0f;
    pos.z = 0.0f;
    njPushMatrix(&nj_unit_matrix);
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    njCalcPoint(NULL, &pos, &pos);
    njPopMatrixEx();
    CreateEnemyBomb(NULL, ewp->shot_num, 1, &pos); // type 1: dropped
    twp->wtimer = 0;
  }
}

static void EnemyKumiHide(taskwk *twp, enemywk *ewp) {
  STACK_PAD_VAR(2);

  EnemyKumiSearchPlayer(twp, ewp);
  EnemyKumiCheckHome(twp, ewp);
  if (twp->flag & 0x2000) {
    twp->mode = MD_APPEAR;
    ewp->argb.a = -1.0f;
    ewp->argb.r = 1.0f;
    ewp->argb.g = 1.0f;
    ewp->argb.b = 1.0f;
    twp->flag &= ~0x2000;
    fn_8006B7EC(0x400F, NULL, 0, 0x7F, &twp->pos);
  }
  EnemyKumiTurn(twp, ewp);
}

// spins and grows while the white flash fades in, then the flash fades out
static void EnemyKumiAppear(taskwk *twp, enemywk *ewp) {
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

static void EnemyKumiVanish(taskwk *twp, enemywk *ewp) {
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

static Sint8 e_b_kumi_disp = 1;

static void EnemyKumiDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;
  NJS_POINT3 unused[6]; // unused

  if (fn_8002B958(&twp->pos, 40.0f) != 0) {
    return;
  }

  fn_80024CB8(8);
  if (twp->smode == KUMI_GUN) {
    njSetTexture(&_rename_e_g_kumi_texlist);
  } else if (twp->smode == KUMI_BOMB) {
    njSetTexture(&_rename_e_b_kumi_texlist);
  } else if (twp->smode == KUMI_SPRING) {
    njSetTexture(&_rename_e_s_kumi_texlist);
  } else {
    njSetTexture(&_rename_e_kumi_texlist);
  }

  if (twp->mode != MD_HIDE) {
    if (InRange(twp->mode, MD_APPEAR, MD_VANISH)) {
      fn_800156FC(ewp->argb.a, ewp->argb.r, ewp->argb.g, ewp->argb.b);
      _rename_e_kumi_plist_0[50] |= 0x800;
      _rename_e_kumi_plist_0[144] |= 0x800;
      _rename_e_kumi_plist_1[10] |= 0x800;
      _rename_e_kumi_plist_2[10] |= 0x800;
      _rename_e_kumi_plist_3[10] |= 0x800;
      _rename_e_kumi_plist_3[264] |= 0x800;
      _rename_e_kumi_plist_4[8] |= 0x800;
    }

    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y + 0x4000);
    if (twp->smode == KUMI_HIDE) {
      njScale(NULL, ewp->scl, 1.0f, ewp->scl);
    }
    if (twp->smode == KUMI_BOMB) {
      while (e_b_kumi_disp == 0) {
      }
    }

    if (ewp->mtn.mtnmode != 2) {
      if (twp->smode == KUMI_BOMB) {
        if (lbl_801CC168._7C > 2) {
          fn_8011E214(ewp->mtn.object,
                      ewp->mtn.actptr[ewp->mtn.action].motion,
                      ewp->mtn.nframe);
        }
      } else {
        fn_8011E214(ewp->mtn.object, ewp->mtn.actptr[ewp->mtn.action].motion,
                    ewp->mtn.nframe);
      }
    } else {
      fn_8011E19C(ewp->mtn.object, &ewp->mtn.link, ewp->mtn.nframe);
    }
    njPopMatrixEx();

    if (twp->mode == MD_APPEAR || twp->mode == MD_VANISH) {
      fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
      _rename_e_kumi_plist_0[50] &= ~0x800;
      _rename_e_kumi_plist_0[144] &= ~0x800;
      _rename_e_kumi_plist_1[10] &= ~0x800;
      _rename_e_kumi_plist_2[10] &= ~0x800;
      _rename_e_kumi_plist_3[10] &= ~0x800;
      _rename_e_kumi_plist_3[264] &= ~0x800;
      _rename_e_kumi_plist_4[8] &= ~0x800;
    }
  }

  fn_80024CB8(lbl_803AD926);
}

static void EnemyKumiDisplayerSort(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;

  if (fn_8002B958(&twp->pos, 40.0f) != 0) {
    return;
  }
  if (twp->mode != MD_ELEC) {
    return;
  }

  njDisableFog();
  gjSetFog();
  njPushMatrixEx();
  fn_80073028(&_rename_e_e_kumi_spark_model, _rename_e_e_kumi_spark_uvanim,
              ewp->spark_frame);
  njSetTexture(&e_e_kumi_spark_texlists[ewp->spark_tex]);
  njTranslateEx(&twp->pos);
  fn_8011E17C(&_rename_e_e_kumi_spark_object);
  njPopMatrixEx();
  njEnableFog();
  gjSetFog();
}

void EnemyKumi(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;
  NJS_POINT3 lpos;
  NJS_POINT3 pos;

  if (twp->mode != MD_INIT && CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case MD_INIT:
    EnemyKumiInit(tp, twp);
    return;
  case MD_NORMAL:
    EnemyKumiNormal(tp, twp, ewp);
    break;
  case MD_DEAD:
    EnemyKumiSetEnd(twp, ewp);
  case MD_END:
    EnemyKumiDeadOut(tp);
    return;
  case MD_HIDE:
    EnemyKumiHide(twp, ewp);
    break;
  case MD_APPEAR:
    EnemyKumiAppear(twp, ewp);
    break;
  case MD_VANISH:
    EnemyKumiVanish(twp, ewp);
    if (twp->mode != MD_VANISH) {
      tp->disp = NULL;
    }
    break;
  case MD_ELEC:
    EnemyKumiElec(tp, twp, ewp);
    break;
  case MD_BOMB:
    EnemyKumiBomb(tp, twp, ewp);
    break;
  case MD_GUN:
    EnemyKumiNormal(tp, twp, ewp);
    break;
  }

  if (lbl_801CC168._37 == 0) {
    fn_80017B94(twp, ewp);
    switch (twp->mode) {
    case MD_NORMAL:
    case MD_ELEC:
    case MD_BOMB:
    case MD_GUN:
      fn_80069D60(0x400D, twp, 1, 10, 164, &twp->pos);
      break;
    case MD_APPEAR:
    case MD_VANISH:
      fn_80069D60(0x400D, twp, 1, 10.0f + 127.0f * ewp->argb.a, 164,
                  &twp->pos);
      break;
    }
    fn_80015C40(&ewp->mtn);

    if (twp->smode == KUMI_SPRING && (twp->wtimer & 0x10)) {
      njPushMatrix(&nj_unit_matrix);
      njTranslateEx(&twp->pos);
      njRotateY(NULL, twp->ang.y);
      lpos.x = -3.35f;
      lpos.y = -8.32f;
      lpos.z = -6.78f;
      njCalcPoint(NULL, &lpos, &lpos);
      CreateEnemyLight(tp, 0, &lpos);
      lpos.x = -3.35f;
      lpos.y = -8.32f;
      lpos.z = 6.78f;
      njCalcPoint(NULL, &lpos, &lpos);
      CreateEnemyLight(tp, 0, &lpos);
      njPopMatrixEx();
    }

    if (twp->flag & 0x2000) {
      njPushMatrix(&nj_unit_matrix);
      njTranslateEx(&twp->pos);
      njRotateY(NULL, twp->ang.y);
      if (twp->smode == KUMI_SPRING) {
        pos.x = 11.0f;
        pos.y = 2.5f;
        pos.z = 0.0f;
      } else {
        pos.x = 10.0f;
        pos.y = 4.6f;
        pos.z = 0.0f;
      }
      njCalcPoint(NULL, &pos, &pos);
      CreateEnemyLight(tp, 1, &pos);
      njPopMatrixEx();
    }
  }

  if (twp->mode != MD_DEAD) {
    CCL_Entry(tp);
  }
}
