#include "ENEMY/e_path.h"

#include "ENEMY/e_bomb.h"
#include "ENEMY/e_bullet.h"
#include "ENEMY/e_jet.h"
#include "ENEMY/e_light.h"
#include "CCL.h"
#include "fabsf.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njcollision.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njmotion.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
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
  /* 0x00C */ Float unk_0C;
  /* 0x010 */ Float unk_10;
  /* 0x014 */ Uint8 unk_14[8];
  /* 0x01C */ Angle home_ang;
  /* 0x020 */ Uint8 unk_20[4];
  /* 0x024 */ Angle bob_ang; // bobs the hull up and down
  /* 0x028 */ Uint8 unk_28[0x1C];
  /* 0x044 */ Sint8 pno; // player that destroyed it
  /* 0x045 */ Sint8 unk_45;
  /* 0x046 */ Sint8 path_no; // which path of e_path_paths it rides
  /* 0x047 */ Sint8 shot_num;
  /* 0x048 */ Sint16 wait;
  /* 0x04A */ Uint8 unk_4A[2];
  /* 0x04C */ Sint16 flag;
  /* 0x04E */ Sint16 spark_frame;
  /* 0x050 */ Uint8 unk_50[8];
  /* 0x058 */ NJS_POINT3 home_pos;
  /* 0x064 */ Uint8 unk_64[0xC];
  /* 0x070 */ NJS_POINT3 set_pos; // the SET scale, kept for the path offset
  /* 0x07C */ NJS_POINT3 last_pos;
  /* 0x088 */ Uint8 unk_88[0xC];
  /* 0x094 */ Float shot_spd;
  /* 0x098 */ Float scl;
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

// the path the enemy rides, laid out like the loop/rail tables
typedef struct pathhead {
  /* 0x00 */ Sint16 unk_00;
  /* 0x02 */ Sint16 count;
  /* 0x04 */ Float totaldist;
  /* 0x08 */ void *points;
  /* 0x0C */ void *object;
} pathhead;

// fn_8005C910 reads .dist and fills in where on the path that lands
typedef struct pathpos {
  /* 0x00 */ Sint32 unk_00;
  /* 0x04 */ Sint32 unk_04;
  /* 0x08 */ Angle angx;
  /* 0x0C */ Angle angz;
  /* 0x10 */ Float dist;
  /* 0x14 */ NJS_POINT3 pos;
} pathpos;

// the 0x3C block syCalloc'd per enemy and hung off tp->awp
typedef struct pathwk {
  /* 0x00 */ Uint8 unk_00[0x38];
  /* 0x38 */ pathhead *path;
} pathwk;

extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_80015C40(enemy_mtnwk *mtn);
extern void fn_800164FC(taskwk *twp, enemywk *ewp);
extern Sint32 fn_800167E0(taskwk *twp, enemywk *ewp);
extern void fn_80016D10(taskwk *twp, NJS_CNK_MODEL **models,
                        NJS_TEXLIST *texlist);
extern void fn_80017BE4(taskwk *twp, enemywk *ewp);
extern enemywk *fn_80018D28(task *tp);
extern Sint32 fn_80035C54(task *tp);
extern void fn_80062E0C(Sint32 score);
extern void fn_80072820(const char *name, NJS_TEXLIST *texlist);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_80017B94(taskwk *twp, enemywk *ewp);
extern void fn_80017CA4(taskwk *twp, enemywk *ewp);
extern void fn_80024CB8(Sint32 mode);
extern void fn_8011E19C(NJS_CNK_OBJECT *object, enemy_mtnlink *link,
                        Float frame);
extern void fn_8011E214(NJS_CNK_OBJECT *object, NJS_MOTION *motion,
                        Float frame);
extern void fn_8011E244(NJS_CNK_OBJECT *object);
extern void fn_8011F520_nop(void);
extern void ds_DrawModelClip(void *model);
extern Sint8 lbl_803AD926;
extern Sint32 fn_8005C6F8(pathhead *path, Sint32 id, Float *dst);
extern Sint32 fn_8005C848(pathhead *path, Float dist, Uint32 *idx);
extern Sint32 fn_8005C910(pathhead *path, pathpos *pp);

extern Uint32 fn_80037C84(NJS_POINT3 *pos);
extern Float asinf(Float x);
extern Float atan2f(Float y, Float x);

extern void EnemyBombLoadTexture(void);
extern void EnemyJetLoadTexture(void);

extern NJS_MATRIX nj_unit_matrix;

extern NJS_TEXLIST _rename_e_pathkumi_texlist;
extern NJS_TEXLIST _rename_e_pathkyoko_texlist;
extern CCL_INFO _rename_e_path_colli_info[];
extern NJS_CNK_OBJECT _rename_e_path_searchlight_object;

extern NJS_CNK_OBJECT _rename_e_pathkumi_object;
extern NJS_CNK_OBJECT _rename_e_pathkumi_bomb_object;
extern NJS_CNK_OBJECT _rename_e_pathkyoko_object;
extern NJS_CNK_OBJECT _rename_e_pathkyoko_beam_object;

extern NJS_MOTION _rename_e_pathkumi_motion;
extern NJS_MOTION _rename_e_pathkumi_bomb_motion;
extern NJS_MOTION _rename_e_pathkyoko_motion;
extern NJS_MOTION _rename_e_pathkyoko_beam_motion;
extern NJS_MOTION _rename_e_pathkyoko_shot_motion;

extern NJS_CNK_MODEL _rename_e_pathkumi_broken_model_0;
extern NJS_CNK_MODEL _rename_e_pathkumi_broken_model_1;
extern NJS_CNK_MODEL _rename_e_pathkumi_broken_model_2;
extern NJS_CNK_MODEL _rename_e_pathkumi_broken_model_3;
extern NJS_CNK_MODEL _rename_e_pathkumi_bomb_broken_model_0;
extern NJS_CNK_MODEL _rename_e_pathkumi_bomb_broken_model_1;
extern NJS_CNK_MODEL _rename_e_pathkumi_bomb_broken_model_2;

extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_0;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_1;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_2;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_3;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_4;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_5;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_6;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_7;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_8;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_9;
extern NJS_CNK_MODEL _rename_e_pathkyoko_broken_model_10;

extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_0;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_1;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_2;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_3;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_4;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_5;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_6;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_7;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_8;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_9;
extern NJS_CNK_MODEL _rename_e_pathkyoko_beam_broken_model_10;

extern pathhead _rename_e_path_path_0;
extern pathhead _rename_e_path_path_1;
extern pathhead _rename_e_path_path_2;
extern pathhead _rename_e_path_path_3;
extern pathhead _rename_e_path_path_4;
extern pathhead _rename_e_path_path_5;
extern pathhead _rename_e_path_path_6;
extern pathhead _rename_e_path_path_7;
extern pathhead _rename_e_path_path_8;
extern pathhead _rename_e_path_path_9;
extern pathhead _rename_e_path_path_10;
extern pathhead _rename_e_path_path_11;

// ^ extern
// v in this file

enum {
  MD_INIT,
  MD_WAIT,
  MD_MOVE,
  MD_ATTACK,
  MD_RETURN,
  MD_DEAD,
  MD_END,
};

// enemy_action index
enum {
  ACT_KUMI,
  ACT_KUMI_BOMB,
  ACT_KYOKO,
  ACT_KYOKO_BEAM,
  ACT_KYOKO_SHOT,
};

#define E_PATH_NUM 12

#define RadAng(n) ((Angle)(10430.38043493439 * (n)))
#define InRange(n, lo, hi) ((Uint8)((n) - (lo)) <= (hi) - (lo))

static void EnemyPathRun(task *tp, taskwk *twp, enemywk *ewp);
static void EnemyPathDestructor(task *tp);
static void EnemyPathDisplayer(task *tp);

static NJS_CNK_MODEL *e_pathkumi_broken_models[] = {
    &_rename_e_pathkumi_broken_model_0,
    &_rename_e_pathkumi_broken_model_1,
    &_rename_e_pathkumi_broken_model_2,
    &_rename_e_pathkumi_broken_model_3,
    NULL,
};

static NJS_CNK_MODEL *e_pathkumi_bomb_broken_models[] = {
    &_rename_e_pathkumi_bomb_broken_model_0,
    &_rename_e_pathkumi_bomb_broken_model_1,
    &_rename_e_pathkumi_bomb_broken_model_2,
    NULL,
};

static NJS_CNK_MODEL *e_pathkyoko_broken_models[] = {
    &_rename_e_pathkyoko_broken_model_0,
    &_rename_e_pathkyoko_broken_model_1,
    &_rename_e_pathkyoko_broken_model_2,
    &_rename_e_pathkyoko_broken_model_3,
    &_rename_e_pathkyoko_broken_model_4,
    &_rename_e_pathkyoko_broken_model_5,
    &_rename_e_pathkyoko_broken_model_6,
    &_rename_e_pathkyoko_broken_model_7,
    &_rename_e_pathkyoko_broken_model_8,
    &_rename_e_pathkyoko_broken_model_9,
    &_rename_e_pathkyoko_broken_model_10,
    NULL,
};

static NJS_CNK_MODEL *e_pathkyoko_beam_broken_models[] = {
    &_rename_e_pathkyoko_beam_broken_model_0,
    &_rename_e_pathkyoko_beam_broken_model_1,
    &_rename_e_pathkyoko_beam_broken_model_2,
    &_rename_e_pathkyoko_beam_broken_model_3,
    &_rename_e_pathkyoko_beam_broken_model_4,
    &_rename_e_pathkyoko_beam_broken_model_5,
    &_rename_e_pathkyoko_beam_broken_model_6,
    &_rename_e_pathkyoko_beam_broken_model_7,
    &_rename_e_pathkyoko_beam_broken_model_8,
    &_rename_e_pathkyoko_beam_broken_model_9,
    &_rename_e_pathkyoko_beam_broken_model_10,
    NULL,
};

static pathhead *e_path_paths[E_PATH_NUM] = {
    &_rename_e_path_path_0, &_rename_e_path_path_1,  &_rename_e_path_path_2,
    &_rename_e_path_path_3, &_rename_e_path_path_4,  &_rename_e_path_path_5,
    &_rename_e_path_path_6, &_rename_e_path_path_7,  &_rename_e_path_path_8,
    &_rename_e_path_path_9, &_rename_e_path_path_10, &_rename_e_path_path_11,
};

static enemy_action e_path_actions[] = {
    {&_rename_e_pathkumi_motion, 3, ACT_KUMI, 1.0f, 0.5f},
    {&_rename_e_pathkumi_bomb_motion, 4, ACT_KUMI, 1.0f, 0.2f},
    {&_rename_e_pathkyoko_motion, 3, ACT_KYOKO, 1.0f, 0.5f},
    {&_rename_e_pathkyoko_beam_motion, 3, ACT_KYOKO_BEAM, 1.0f, 0.5f},
    {&_rename_e_pathkyoko_shot_motion, 3, ACT_KYOKO_SHOT, 1.0f, 0.5f},
};

// tasks that own the two skins' textures
static task *mantex_tp;
static task *mantex_k_tp;

static void ManTexDestructor(task *tp) {
  fn_801166F0(&_rename_e_pathkumi_texlist);
  mantex_tp = NULL;
}

static void ManTexDestructorK(task *tp) {
  fn_801166F0(&_rename_e_pathkyoko_texlist);
  mantex_k_tp = NULL;
}

static void ManTex(task *tp) {}

static void EnemyPathSetParam(taskwk *twp, enemywk *ewp, pathwk *wk) {
  Sint32 id;

  ewp->path_no = (twp->ang.x & 0xF000) >> 12;
  if ((Uint32)ewp->path_no >= E_PATH_NUM || ewp->path_no < 0) {
    ewp->path_no = 0;
  }
  wk->path = e_path_paths[ewp->path_no];

  twp->btimer = (twp->ang.x & 0x0FF0) >> 4;

  id = (twp->ang.z & 0xFF00) >> 8;
  if (fn_8005C6F8(wk->path, id, &ewp->unk_0C) == 0) {
    ewp->unk_0C = 0.0f;
  }
  id = twp->ang.z & 0xFF;
  if (fn_8005C6F8(wk->path, id, &ewp->unk_10) == 0) {
    ewp->unk_10 = 0.0f;
  }
  if (fn_8005C6F8(wk->path, (Uint32)fabsf(100.0f * njFraction(twp->scl.y)),
                  &ewp->shot_spd) == 0) {
    ewp->shot_spd = 0.0f;
  }
}

static void EnemyPathInit(task *tp, taskwk *twp) {
  enemywk *ewp = fn_80018D28(tp);
  pathwk *wk;
  Sint32 unused[4]; // unused

  twp->smode = twp->ang.x & 0xF;
  if (twp->smode >= PATH_NUM || twp->smode < 0) {
    twp->smode = PATH_KUMI;
  }

  wk = syCalloc(1, sizeof(pathwk));
  tp->awp = (anywk *)wk;
  EnemyPathSetParam(twp, ewp, wk);

  switch (twp->smode) {
  case PATH_KUMI:
  case PATH_KUMI_BOMB:
    CCL_Init(tp, _rename_e_path_colli_info, 1, CID_ENEMY2);
    break;
  case PATH_KYOKO:
  case PATH_KYOKO_BEAM:
    CCL_Init(tp, &_rename_e_path_colli_info[1], 5, CID_ENEMY2);
    break;
  }

  ewp->set_pos = twp->scl;
  ewp->scl = 10.0f + fabsf(400.0f * njFraction(twp->scl.x));
  ewp->home_ang = twp->ang.y & 0xFF00;
  twp->ang.z = 0;
  twp->ang.x = 0;
  ewp->unk_1C8 = 0x400;
  ewp->spark_frame = fabsf(100.0f * njFraction(twp->scl.z));
  fn_80017BE4(twp, ewp);
  twp->mode = MD_WAIT;
  ewp->mtn.actptr = e_path_actions;

  switch (twp->smode) {
  case PATH_KUMI:
    ewp->mtn.object = &_rename_e_pathkumi_object;
    ewp->mtn.reqaction = 0;
    ewp->shadow_scl = 12.0f;
    ewp->unk_F0 = 1.0f;
    EnemyBulletLoadTexture();
    break;
  case PATH_KUMI_BOMB:
    ewp->mtn.object = &_rename_e_pathkumi_bomb_object;
    ewp->mtn.reqaction = 2;
    ewp->shadow_scl = 12.0f;
    ewp->unk_F0 = 1.0f;
    EnemyBombLoadTexture();
    break;
  case PATH_KYOKO:
    ewp->mtn.object = &_rename_e_pathkyoko_object;
    ewp->mtn.reqaction = 3;
    ewp->shadow_scl = 12.0f;
    ewp->unk_F0 = 1.0f;
    EnemyBulletLoadTexture();
    break;
  case PATH_KYOKO_BEAM:
    ewp->mtn.object = &_rename_e_pathkyoko_beam_object;
    ewp->mtn.reqaction = 4;
    ewp->shadow_scl = 12.0f;
    ewp->unk_F0 = 1.0f;
    EnemyBulletLoadTexture();
    break;
  }

  fn_80015C40(&ewp->mtn);
  ewp->flag |= 0x10;
  ewp->flag |= 0x4;
  tp->disp = EnemyPathDisplayer;
  tp->dest = EnemyPathDestructor;
  EnemyJetLoadTexture();
  EnemyLightLoadTexture();

  if (twp->smode == PATH_KUMI || twp->smode == PATH_KUMI_BOMB) {
    if (mantex_tp == NULL) {
      fn_80072820("E_PATHKUMITEX", &_rename_e_pathkumi_texlist);
      mantex_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
      mantex_tp->dest = ManTexDestructor;
    }
  }
  if (twp->smode == PATH_KYOKO || twp->smode == PATH_KYOKO_BEAM) {
    if (mantex_k_tp == NULL) {
      fn_80072820("E_PATHKYOKOTEX", &_rename_e_pathkyoko_texlist);
      mantex_k_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
      mantex_k_tp->dest = ManTexDestructorK;
    }
  }

  // the texture manager doubles as a store for the prebuilt model
  switch (twp->smode) {
  case PATH_KUMI:
    if (mantex_tp != NULL && mantex_tp->awp != NULL) {
      twp->flag |= 0x1000;
    }
    break;
  case PATH_KUMI_BOMB:
    if (mantex_tp != NULL && mantex_tp->fwp != NULL) {
      twp->flag |= 0x1000;
    }
    break;
  case PATH_KYOKO:
    if (mantex_k_tp != NULL && mantex_k_tp->awp != NULL) {
      twp->flag |= 0x1000;
    }
    break;
  case PATH_KYOKO_BEAM:
    if (mantex_k_tp != NULL && mantex_k_tp->fwp != NULL) {
      twp->flag |= 0x1000;
    }
    break;
  }
}

static void EnemyPathSetModeEnd(taskwk *twp, enemywk *ewp) { twp->mode = MD_END; }

static void EnemyPathCheckDamage(task *tp, taskwk *twp, enemywk *ewp) {
  Sint32 score;

  if (fn_800167E0(twp, ewp) == 0) {
    return;
  }

  twp->mode = MD_DEAD;
  twp->cwp->info->attr |= 0x10;
  twp->wtimer = 0;
  fn_80017BE4(twp, ewp);
  switch (twp->smode) {
  case PATH_KUMI:
    fn_80016D10(twp, e_pathkumi_broken_models, &_rename_e_pathkumi_texlist);
    break;
  case PATH_KUMI_BOMB:
    fn_80016D10(twp, e_pathkumi_bomb_broken_models,
                &_rename_e_pathkumi_texlist);
    break;
  case PATH_KYOKO:
    fn_80016D10(twp, e_pathkyoko_broken_models, &_rename_e_pathkyoko_texlist);
    break;
  case PATH_KYOKO_BEAM:
    fn_80016D10(twp, e_pathkyoko_beam_broken_models,
                &_rename_e_pathkyoko_texlist);
    break;
  }

  score = 500;
  if (fn_80035C54(tp) != 0) {
    score <<= 1;
  }
  fn_80062E0C(score);
  fn_800164FC(twp, ewp);
}

// unreferenced; reconstructed from the .rodata order, stripped by the linker
static Angle EnemyPathRadToAng(Float rad) { return RadAng(rad); }

// walks the enemy one step along its path, and points it where it is going
static void EnemyPathRun(task *tp, taskwk *twp, enemywk *ewp) {
  pathwk *wk;
  pathhead *path;
  Sint32 unused[8]; // unused
  pathpos pp;
  Sint32 unused2[13];
  Uint32 idx;
  Sint32 unused3;
  NJS_VECTOR v;
  Float spd;
  Sint32 pno;

  if (lbl_801CC168._37 != 0) {
    return;
  }

  if (twp->mode == MD_MOVE) {
    wk = (pathwk *)tp->awp;
    path = wk->path;

    switch (twp->smode) {
    case PATH_KUMI:
    case PATH_KUMI_BOMB:
      spd = 2.5f;
      break;
    case PATH_KYOKO:
    case PATH_KYOKO_BEAM:
      spd = 5.0f;
      break;
    }

    // it eases off over the last 50 units of the path
    if ((twp->btimer & 1) && ewp->shot_spd >= path->totaldist - 50.0f) {
      spd = 0.2f * (spd * (path->totaldist - ewp->shot_spd));
      if (spd < 0.3f) {
        spd = 0.3f;
      }
    }

    ewp->shot_spd += spd;
    pp.dist = ewp->shot_spd;
    if (fn_8005C848(path, ewp->shot_spd, &idx) == 0 ||
        idx >= path->count - 2 || fn_8005C910(path, &pp) == 0) {
      if (twp->btimer & 1) {
        twp->mode = MD_RETURN;
        njAddVector(&twp->pos, &ewp->spd);
      } else {
        twp->mode = MD_ATTACK;
        njAddVector(&twp->pos, &ewp->spd);
      }
      return;
    }

    if (twp->btimer & 0x10) {
      pp.pos.x = -pp.pos.x;
    }

    njPushMatrixEx();
    njUnitMatrix(NULL);
    njTranslateEx(&ewp->home_pos);
    njRotateY(NULL, ewp->home_ang);
    njCalcPoint(NULL, &pp.pos, &twp->pos);
    njPopMatrixEx();

    ewp->spd = twp->pos;
    njSubVector(&ewp->spd, &ewp->last_pos);

    v.x = 0.0f;
    v.y = 1.0f;
    v.z = 0.0f;
    njPushMatrixEx();
    njUnitMatrix(NULL);
    njRotateY(NULL, ewp->home_ang);
    njRotateZ(NULL, pp.angz);
    njRotateX(NULL, pp.angx);
    njCalcVector(NULL, &v, &v);

    if (ewp->path_no == 4 || twp->smode == PATH_KUMI_BOMB) {
      twp->ang.x = 0;
      twp->ang.z = 0;
    } else {
      twp->ang.x = RadAng(asinf(v.z));
      twp->ang.z = -RadAng(atan2f(v.x, v.y));
    }

    if (twp->smode == PATH_KUMI_BOMB || twp->smode == PATH_KYOKO) {
      v = ewp->spd;
    } else if (twp->smode == PATH_KYOKO_BEAM) {
      v.x = -ewp->spd.x;
      v.y = -ewp->spd.y;
      v.z = -ewp->spd.z;
    } else {
      pno = fn_80037C84(&twp->pos);
      if (pno != -1) {
        v = playertwp[pno]->pos;
        njSubVector(&v, &twp->pos);
      } else {
        v = ewp->spd;
      }
    }

    njUnitMatrix(NULL);
    njRotateZ(NULL, twp->ang.z);
    njRotateX(NULL, twp->ang.x);
    njInvertMatrix(NULL);
    njCalcVector(NULL, &v, &v);
    twp->ang.y = -RadAng(atan2f(v.z, v.x));
    njPopMatrixEx();
    return;
  }

  if (twp->mode == MD_ATTACK) {
    njAddVector(&twp->pos, &ewp->spd);
    return;
  }
}

static void EnemyPathDestructor(task *tp) { fn_80018B88(tp); }

static void EnemyPathDeadOut(task *tp) { DeadOut(tp); }

// takes a point given in the enemy's own frame out into world space
static void EnemyPathCalcPoint(taskwk *twp, NJS_POINT3 *p) {
  njPushMatrix(&nj_unit_matrix);
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  njCalcPoint(NULL, p, p);
  njPopMatrixEx();
}

static void EnemyPathCalcVector(taskwk *twp, NJS_VECTOR *v) {
  njPushMatrix(&nj_unit_matrix);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  njCalcVector(NULL, v, v);
  njPopMatrixEx();
}

// turns the point p towards the nearest player, a bit at a time
static void EnemyPathAim(taskwk *twp, NJS_POINT3 *pos, NJS_POINT3 *p) {
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

  angx = AdjustAngle(0, angx, 0x1800);
  angy = AdjustAngle(twp->ang.y, angy, 0x800);
  njPushMatrix(&nj_unit_matrix);
  njRotateY(NULL, angy);
  njRotateZ(NULL, angx);
  njCalcPoint(NULL, p, p);
  njPopMatrixEx();
}

static void EnemyPathShot(task *tp, taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;
  NJS_VECTOR spd;

  if (twp->smode == PATH_KUMI) {
    pos.x = 7.5f;
    pos.y = -9.2f;
    pos.z = 0.0f;
  } else if (twp->smode == PATH_KYOKO) {
    pos.x = 15.6f;
    pos.y = 11.5f;
    pos.z = 0.0f;
  } else if (twp->smode == PATH_KYOKO_BEAM) {
    pos.x = 15.6f;
    pos.y = 11.5f;
    pos.z = 0.0f;
  }
  EnemyPathCalcPoint(twp, &pos);

  if (twp->smode == PATH_KUMI) {
    spd.x = 3.2f;
  } else if (twp->smode == PATH_KYOKO) {
    spd.x = 5.5f;
  } else if (twp->smode == PATH_KYOKO_BEAM) {
    spd.x = 5.5f;
  }
  spd.y = 0.0f;
  spd.z = 0.0f;
  EnemyPathAim(twp, &pos, &spd);

  CreateEnemyBullet(tp, !(twp->btimer & 4), &spd, &pos);
}

static void EnemyPathDropBomb(task *tp, taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;

  pos.x = -1.2f;
  pos.y = -8.1f;
  pos.z = 0.0f;
  EnemyPathCalcPoint(twp, &pos);

  CreateEnemyBomb(tp, 1, 1, &pos);
}

// the two thruster flames, mirrored about the hull
static void EnemyPathJet(taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;
  NJS_VECTOR v;

  if (lbl_801CC168._37 != 0) {
    return;
  }

  if (twp->smode == PATH_KYOKO) {
    pos.x = -18.6f;
    pos.y = 13.2f;
    pos.z = 13.5f;
  } else if (twp->smode == PATH_KYOKO_BEAM) {
    pos.x = 16.6f;
    pos.y = 13.2f;
    pos.z = 13.5f;
  }
  EnemyPathCalcPoint(twp, &pos);

  if (twp->smode == PATH_KYOKO) {
    v.x = njScalor(&ewp->spd) - 1.8f;
  } else if (twp->smode == PATH_KYOKO_BEAM) {
    v.x = 1.8f - njScalor(&ewp->spd);
  }
  v.y = 0.0f;
  v.z = 0.0f;
  EnemyPathCalcVector(twp, &v);
  CreateEnemyJet(NULL, 0, &pos, &v);

  if (twp->smode == PATH_KYOKO) {
    pos.x = -18.6f;
    pos.y = 13.2f;
    pos.z = -13.5f;
  } else if (twp->smode == PATH_KYOKO_BEAM) {
    pos.x = 16.6f;
    pos.y = 13.2f;
    pos.z = -13.5f;
  }
  EnemyPathCalcPoint(twp, &pos);
  CreateEnemyJet(NULL, 0, &pos, &v);
}

// the running lights: one on each wing, plus a nose light on the kyoko skins
static void EnemyPathLight(task *tp, taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;

  if (twp->smode == PATH_KUMI || twp->smode == PATH_KUMI_BOMB) {
    pos.x = -3.0f;
    pos.y = 1.9f;
    pos.z = 17.8f;
  } else if (twp->smode == PATH_KYOKO) {
    pos.x = -19.8f;
    pos.y = 23.1f;
    pos.z = 34.0f;
  } else if (twp->smode == PATH_KYOKO_BEAM) {
    pos.x = -19.8f;
    pos.y = 23.1f;
    pos.z = 34.0f;
  }
  EnemyPathCalcPoint(twp, &pos);
  CreateEnemyLight(tp, 0, &pos);

  if (twp->smode == PATH_KUMI || twp->smode == PATH_KUMI_BOMB) {
    pos.x = -3.0f;
    pos.y = 1.9f;
    pos.z = -17.8f;
  } else if (twp->smode == PATH_KYOKO) {
    pos.x = -19.8f;
    pos.y = 23.1f;
    pos.z = -34.0f;
  } else if (twp->smode == PATH_KYOKO_BEAM) {
    pos.x = -19.8f;
    pos.y = 23.1f;
    pos.z = -34.0f;
  }
  EnemyPathCalcPoint(twp, &pos);
  CreateEnemyLight(tp, 0, &pos);

  if (twp->smode != PATH_KYOKO && twp->smode != PATH_KYOKO_BEAM) {
    return;
  }

  if (twp->smode == PATH_KYOKO) {
    pos.x = -24.8f;
    pos.y = 23.1f;
    pos.z = 0.0f;
  } else if (twp->smode == PATH_KYOKO_BEAM) {
    pos.x = -24.8f;
    pos.y = 23.1f;
    pos.z = 0.0f;
  }
  EnemyPathCalcPoint(twp, &pos);
  CreateEnemyLight(tp, 0, &pos);
}

static void EnemyPathMove(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyPathRun(tp, twp, ewp);
  EnemyPathCheckDamage(tp, twp, ewp);
  if (twp->mode == MD_DEAD) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }
  if (twp->smode == PATH_KYOKO || twp->smode == PATH_KYOKO_BEAM) {
    EnemyPathJet(twp, ewp);
  }
  if (twp->wtimer & 0x10) {
    EnemyPathLight(tp, twp, ewp);
  }

  // it only fires over the stretch of the path between unk_0C and unk_10
  if (ewp->shot_spd >= ewp->unk_0C && ewp->shot_spd <= ewp->unk_10) {
    // reload counter in the else arm to match
    if (ewp->wait <= 0) {
      if (twp->smode == PATH_KUMI) {
        if (ewp->mtn.action != 1) {
          ewp->mtn.reqaction = 1;
        } else if (ewp->mtn.nframe >= 1.0f && ewp->mtn.nframe < 1.2f) {
          EnemyPathShot(tp, twp, ewp);
          ewp->wait = ewp->spark_frame + 6;
        }
      } else if (twp->smode == PATH_KUMI_BOMB) {
        EnemyPathDropBomb(tp, twp, ewp);
        ewp->wait = ewp->spark_frame + 30;
      } else if (InRange(twp->smode, PATH_KYOKO, PATH_KYOKO_BEAM)) {
        EnemyPathShot(tp, twp, ewp);
        ewp->wait = ewp->spark_frame + 15;
      }
    } else {
      ewp->wait--;
    }
  }
}

// bobs the hull and swings it round to face the player it is chasing
static void EnemyPathTurn(taskwk *twp, enemywk *ewp) {
  NJS_VECTOR v;
  Float c;
  Sint32 pno;

  c = njCos(ewp->bob_ang);
  ewp->bob_ang += 0x400;
  twp->pos.y += 0.2f * c;

  pno = fn_80037C84(&twp->pos);
  twp->ang.x = AdjustAngle(twp->ang.x, 0, 0x100);
  twp->ang.z = AdjustAngle(twp->ang.z, 0, 0x100);

  if (pno != -1) {
    v = playertwp[pno]->pos;
    njSubVector(&v, &twp->pos);
  } else {
    v = ewp->spd;
  }

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njInvertMatrix(NULL);
  njCalcVector(NULL, &v, &v);
  twp->ang.y = -RadAng(atan2f(v.z, v.x));
  njPopMatrixEx();
}

static void EnemyPathReturn(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyPathTurn(twp, ewp);
  EnemyPathCheckDamage(tp, twp, ewp);
  if (twp->mode == MD_DEAD) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }
  if (twp->smode == PATH_KYOKO || twp->smode == PATH_KYOKO_BEAM) {
    EnemyPathJet(twp, ewp);
  }
  if (twp->wtimer & 0x10) {
    EnemyPathLight(tp, twp, ewp);
  }

  if (ewp->wait <= 0) {
    if (twp->smode == PATH_KUMI) {
      if (ewp->mtn.action != 1) {
        ewp->mtn.reqaction = 1;
      } else if (ewp->mtn.nframe >= 1.0f && ewp->mtn.nframe < 1.2f) {
        EnemyPathShot(tp, twp, ewp);
        ewp->wait = ewp->spark_frame + 6;
      }
    } else if (twp->smode == PATH_KUMI_BOMB) {
      EnemyPathDropBomb(tp, twp, ewp);
      ewp->wait = ewp->spark_frame + 30;
    } else if (InRange(twp->smode, PATH_KYOKO, PATH_KYOKO_BEAM)) {
      EnemyPathShot(tp, twp, ewp);
      ewp->wait = ewp->spark_frame + 15;
    }
  } else {
    ewp->wait--;
  }
}

// parked at the start of its path until a player comes within scl
static void EnemyPathWait(task *tp, taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;
  Sint32 i;

  pos = ewp->home_pos;
  njAddVector(&pos, &ewp->set_pos);

  for (i = 0; i < 2; i++) {
    if (playertwp[i] != NULL &&
        njDistanceP2P(&playertwp[i]->pos, &pos) < ewp->scl) {
      twp->mode = MD_MOVE;
      Dead(tp);
      EnemyPathRun(tp, twp, ewp);
    }
  }
}

static void EnemyPathDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;
  void *buf;
  NJS_POINT3 unused[4]; // unused

  fn_80024CB8(8);
  if (twp->mode != MD_WAIT) {
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateZ(NULL, twp->ang.z);
    njRotateX(NULL, twp->ang.x);
    njRotateY(NULL, twp->ang.y + 0x4000);
    switch (twp->smode) {
    case PATH_KUMI:
      njSetTexture(&_rename_e_pathkumi_texlist);
      break;
    case PATH_KUMI_BOMB:
      njSetTexture(&_rename_e_pathkumi_texlist);
      break;
    case PATH_KYOKO:
      njSetTexture(&_rename_e_pathkyoko_texlist);
      break;
    case PATH_KYOKO_BEAM:
      njSetTexture(&_rename_e_pathkyoko_texlist);
      break;
    }

    if (twp->flag & 0x1000) {
      fn_8011F520_nop();
      switch (twp->smode) {
      case PATH_KUMI:
        buf = mantex_tp->awp;
        break;
      case PATH_KUMI_BOMB:
        buf = mantex_tp->fwp;
        break;
      case PATH_KYOKO:
        buf = mantex_k_tp->awp;
        break;
      case PATH_KYOKO_BEAM:
        buf = mantex_k_tp->fwp;
        break;
      }
      ds_DrawModelClip(buf);
    } else if (ewp->mtn.mtnmode != 2) {
      fn_8011E214(ewp->mtn.object, ewp->mtn.actptr[ewp->mtn.action].motion,
                  ewp->mtn.nframe);
    } else {
      fn_8011E19C(ewp->mtn.object, &ewp->mtn.link, ewp->mtn.nframe);
    }
    njPopMatrixEx();

    if (twp->btimer & 2) {
      switch (twp->smode) {
      case PATH_KUMI:
      case PATH_KUMI_BOMB:
        fn_80017CA4(twp, ewp);
        break;
      case PATH_KYOKO:
      case PATH_KYOKO_BEAM:
        OnControl3D(0x2400);
        njPushMatrixEx();
        njTranslateEx(&twp->pos);
        njRotateY(NULL, twp->ang.y + 0x4000);
        fn_8011E244(&_rename_e_path_searchlight_object);
        njPopMatrixEx();
        OffControl3D(0x2400);
        break;
      }
    }
  }
  fn_80024CB8(lbl_803AD926);
}

void EnemyPath(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;
  Sint32 unused[2]; // unused

  if (twp->mode != MD_INIT && twp->mode != MD_MOVE && CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case MD_INIT:
    EnemyPathInit(tp, twp);
    return;
  case MD_MOVE:
    EnemyPathMove(tp, twp, ewp);
    break;
  case MD_ATTACK:
    EnemyPathMove(tp, twp, ewp);
    break;
  case MD_RETURN:
    EnemyPathReturn(tp, twp, ewp);
    break;
  case MD_DEAD:
    EnemyPathSetModeEnd(twp, ewp);
  case MD_END:
    EnemyPathDeadOut(tp);
    return;
  case MD_WAIT:
    EnemyPathWait(tp, twp, ewp);
    return;
  }

  if (lbl_801CC168._37 == 0) {
    fn_80017B94(twp, ewp);
    fn_80015C40(&ewp->mtn);
  }
  if (twp->mode != MD_DEAD) {
    CCL_Entry(tp);
  }
}
