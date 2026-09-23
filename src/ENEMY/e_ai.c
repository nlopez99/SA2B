#include "sa2b_types.h"

#include "samt/ninja/ninja.h"
#include "samt/sonic/task.h"
#include "samt/sonic/misc.h"
#include "samt/sonic/player.h"
#include "samt/sonic/c_colli.h"

#include "CCL.h"
#include "fabsf.h"

#include "ENEMY/e_ai.h"
#include "ENEMY/e_bullet.h"

#define RadAng(n) ((Angle)(10430.38043493439 * (n)))
#define InRange(n, lo, hi) ((Uint8)((n) - (lo)) <= (hi) - (lo))

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
typedef struct enemywk {
  /* 0x000 */ NJS_VECTOR spd;
  /* 0x00C */ NJS_VECTOR acc;
  /* 0x018 */ Sint32 turn_timer;
  /* 0x01C */ Angle home_ang;
  /* 0x020 */ Uint8 unk_20[0x20];
  /* 0x040 */ void *unk_40;
  /* 0x044 */ Sint8 pno; // player that destroyed it
  /* 0x045 */ Sint8 unk_45;
  /* 0x046 */ Sint8 spark_tex;
  /* 0x047 */ Sint8 shot_num;
  /* 0x048 */ Sint16 wait;
  /* 0x04A */ Uint8 unk_4A[2];
  /* 0x04C */ Sint16 flag;
  /* 0x04E */ Sint16 spark_frame;
  /* 0x050 */ Uint8 unk_50[8];
  /* 0x058 */ NJS_POINT3 home_pos;
  /* 0x064 */ NJS_POINT3 target_pos;
  /* 0x070 */ Uint8 unk_70[0x18];
  /* 0x088 */ NJS_POINT3 eye_pos;
  /* 0x094 */ Float shot_spd;
  /* 0x098 */ Uint8 unk_98[0x14];
  /* 0x0AC */ NJS_ARGB argb;
  /* 0x0BC */ NJS_POINT3 colli_center;
  /* 0x0C8 */ Float colli_top;
  /* 0x0CC */ Float colli_radius;
  /* 0x0D0 */ Float colli_bottom;
  /* 0x0D4 */ Uint8 unk_D4[8];
  /* 0x0DC */ Float unk_DC;
  /* 0x0E0 */ Float unk_E0;
  /* 0x0E4 */ Uint8 unk_E4[8];
  /* 0x0EC */ Float shadow_scl;
  /* 0x0F0 */ Float unk_F0;
  /* 0x0F4 */ Uint8 unk_F4[4];
  /* 0x0F8 */ Float ground_y;
  /* 0x0FC */ Uint8 unk_FC[0x54];
  /* 0x150 */ Float unk_150;
  /* 0x154 */ Uint8 unk_154[0x70];
  /* 0x1C4 */ Angle float_ang;
  /* 0x1C8 */ Angle turn_spd;
  /* 0x1CC */ Sint32 count;
  /* 0x1D0 */ Angle target_ang;
  /* 0x1D4 */ Angle view_ang;
  /* 0x1D8 */ Float view_range;
  /* 0x1DC */ Float home_range;
  /* 0x1E0 */ enemy_mtnwk mtn;
} enemywk;

extern Float sqrtf(Float x);
extern Float asinf(Float x);
extern Float atan2f(Float y, Float x);
extern int rand(void);

extern NJS_MATRIX nj_unit_matrix;
extern Sint8 lbl_803AD926;

extern NJS_TEXLIST _rename_e_ai_texlist;
extern NJS_CNK_OBJECT _rename_e_ai_object;
extern NJS_CNK_OBJECT _rename_e_ai_eye_object;
extern NJS_CNK_OBJECT _rename_e_ai2_object;
extern NJS_CNK_OBJECT _rename_e_ai2_eye_object;
extern NJS_CNK_OBJECT _rename_e_ai3_object;
extern NJS_CNK_OBJECT _rename_e_ai3_eye_object;
extern CCL_INFO _rename_e_ai_colli_info[4];
extern NJS_CNK_MODEL *_rename_e_ai_broken_models[];
extern enemy_action _rename_e_ai_actions[];

void _rename_PutDustCircle(NJS_POINT3 *pos, Float rad, Float spd, Sint32 num);
void _rename_EnemyCapturingBulletLoadTexture(void);
void _rename_CreateEnemyCapturingBullet(task *tp, NJS_VECTOR *spd,
                                        NJS_POINT3 *pos);
void _rename_EnemyJetLoadTexture(void);
void _rename_CreateEnemyJet(task *tp, Sint32 type, NJS_POINT3 *pos,
                            NJS_VECTOR *spd);
void _rename_EnemyLaserLoadTexture(void);
void _rename_CreateEnemyLaser(task *tp, Float pow, NJS_VECTOR *spd,
                              NJS_POINT3 *pos);
void _rename_EnemyLightLoadTexture(void);
void _rename_CreateEnemyLight(task *tp, Sint32 type, NJS_POINT3 *pos);
Sint32 _rename_CheckFlag0x20(task *tp);
void _rename_SetFlag0x20(task *tp);

void fn_801166F0(NJS_TEXLIST *txl);
void fn_80072820(const char *name, NJS_TEXLIST *txl);
enemywk *fn_80018C10(task *tp);
enemywk *fn_80018C88(task *tp);
void fn_80015B18(taskwk *twp, enemywk *ewp);
void fn_80015C40(enemy_mtnwk *mwp);
void fn_80017BE4(taskwk *twp, enemywk *ewp);
void fn_8006A5F4(Sint32 id, task *tp, Sint32 a3, Sint32 a4);
Float fn_80017A90(taskwk *twp, enemywk *ewp);
void fn_800176AC(taskwk *twp, enemywk *ewp, Uint32 pno);
Sint32 fn_800167E0(taskwk *twp, enemywk *ewp);
void fn_80015730(Sint32 pno, NJS_CNK_OBJECT *object, NJS_TEXLIST *txl,
                 NJS_VECTOR *scl, NJS_POINT3 *pos);
Sint32 fn_80006728(task *tp);
void fn_8000F748(task *tp);
Sint32 fn_8003679C(task *tp1, task *tp2);
void fn_80039EB0(Sint32 pno, NJS_POINT3 *pos, Angle3 *ang, Sint32 a4);
void fn_8006B7EC(Sint32 id, void *tp, Sint32 a3, Sint32 a4, NJS_POINT3 *pos);
void fn_8002FB2C(Sint32 pno, Sint32 a2, Sint32 a3, Sint32 a4);
void fn_80016D10(taskwk *twp, NJS_CNK_MODEL **models, NJS_TEXLIST *txl);
Sint32 fn_80035C54(task *tp);
void fn_80062E0C(Sint32 score);
void fn_800164FC(taskwk *twp, enemywk *ewp);
Sint32 fn_800188B0(taskwk *twp, enemywk *ewp);
Sint32 fn_800187C4(taskwk *twp, enemywk *ewp);
Sint32 fn_8001897C(taskwk *twp, enemywk *ewp);
void fn_80017C00(taskwk *twp, enemywk *ewp);
void fn_80017918(taskwk *twp, enemywk *ewp, Float rad);
Uint32 fn_80037C84(NJS_POINT3 *pos);
void fn_80018B88(task *tp);
void fn_80069CB8(task *tp);
Float fn_8002AE5C(Float now, Float dst, Float rate);
void fn_80024CB8(Sint32 mode);
void fn_8011C3A0(void (*func)(NJS_CNK_OBJECT *object));
void fn_8011E19C(NJS_CNK_OBJECT *object, enemy_mtnlink *link, Float frame);
void fn_8011E214(NJS_CNK_OBJECT *object, NJS_MOTION *motion, Float frame);
void fn_80017B94(taskwk *twp, enemywk *ewp);
void fn_80017CA4(taskwk *twp, enemywk *ewp);

static void EnemyAiDestructor(task *tp);
static void EnemyAiDisplayer(task *tp);

static task *mantex_tp;
static NJS_MATRIX e_ai_matrix;
static enemywk *e_ai_ewp;

static void ManTexDestructor(task *tp) {
  fn_801166F0(&_rename_e_ai_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

static void EnemyAiInit(task *tp, taskwk *twp) {
  enemywk *ewp;

  twp->smode = njRoundOff(twp->scl.x);
  if (twp->smode >= AI_NUM || twp->smode < 0) {
    twp->smode = AI_GUN;
  }

  if (twp->smode == AI_STAND) {
    ewp = fn_80018C10(tp);
  } else {
    ewp = fn_80018C88(tp);
  }

  if (twp->smode == AI_BIG) {
    CCL_Init(tp, &_rename_e_ai_colli_info[2], 2, CID_ENEMY2);
  } else {
    CCL_Init(tp, _rename_e_ai_colli_info, 2, CID_ENEMY2);
  }

  if (twp->smode == AI_GUN || twp->smode == AI_GUN_CHASE ||
      twp->smode == AI_HIDE_GUN) {
    ewp->shot_num = twp->ang.x & 0xFF;
    ewp->wait = twp->ang.z;
    if (ewp->wait < 60) {
      ewp->wait = 60;
    }
    ewp->shot_spd = twp->scl.y;
    ewp->mtn.object = &_rename_e_ai_object;
    EnemyBulletLoadTexture();
  } else if (twp->smode == AI_CAPTURE || twp->smode == AI_HIDE_CAPTURE) {
    ewp->shot_num = twp->ang.x & 0xFF;
    ewp->wait = twp->ang.z;
    if (ewp->wait < 60) {
      ewp->wait = 60;
    }
    ewp->shot_spd = twp->scl.y;
    ewp->mtn.object = &_rename_e_ai_object;
    EnemyBulletLoadTexture();
    _rename_EnemyCapturingBulletLoadTexture();
  } else if (InRange(twp->smode, AI_LASER, AI_LASER_CHASE) ||
             twp->smode == AI_BIG || twp->smode == AI_HIDE_LASER) {
    ewp->shot_num = twp->ang.x & 0xFF;
    ewp->wait = twp->ang.z;
    ewp->shot_spd = twp->scl.y;
    _rename_EnemyLaserLoadTexture();
    if (twp->smode == AI_BIG) {
      ewp->mtn.object = &_rename_e_ai3_object;
    } else {
      ewp->mtn.object = &_rename_e_ai2_object;
    }
  } else if (twp->smode == AI_STAND) {
    ewp->shot_num = 0;
    ewp->wait = 0x7FFF;
    ewp->shot_spd = 0.0f;
    ewp->mtn.object = &_rename_e_ai_object;
  }

  twp->ang.x = 0;
  twp->ang.z = 0;
  twp->scl.y = 2.0f;

  fn_80015B18(twp, ewp);

  ewp->view_range = 6400.0f;
  ewp->view_ang = 0x71C7;
  ewp->home_range = (60.0f + twp->scl.z) * (60.0f + twp->scl.z);
  ewp->spd.x = 1.0f;
  ewp->turn_spd = 0x800;

  _rename_EnemyLightLoadTexture();
  fn_80017BE4(twp, ewp);

  ewp->mtn.actptr = _rename_e_ai_actions;

  if (twp->smode == AI_GUN_CHASE || twp->smode == AI_LASER_CHASE) {
    twp->mode = 3;
    ewp->mtn.reqaction = 8;
    ewp->colli_bottom = -12.0f;
    ewp->unk_E0 = 0.9999f;
    fn_8006A5F4(0x401C, tp, 0, -15);
  } else if (twp->smode == AI_BIG) {
    twp->mode = 2;
    ewp->mtn.reqaction = 15;
    ewp->colli_bottom = -6.0f;
    twp->id = 8;
  } else if (InRange(twp->smode, AI_HIDE_GUN, AI_HIDE_CAPTURE)) {
    twp->mode = 6;
    ewp->mtn.reqaction = 8;
    ewp->colli_bottom = -6.0f;
    ewp->unk_DC = 0.0f;
  } else if (twp->smode == AI_STAND) {
    twp->mode = 9;
    ewp->mtn.reqaction = 3;
    ewp->colli_bottom = -6.0f;
    ewp->unk_DC = 0.0f;
  } else {
    twp->mode = 1;
    ewp->mtn.reqaction = 1;
    twp->cwp->info->center.y = 10.0f;
  }

  fn_80015C40(&ewp->mtn);

  ewp->shadow_scl = 6.0f;
  ewp->unk_F0 = 1.5f;
  ewp->flag |= 0x10;
  ewp->flag |= 0x4;

  if (_rename_CheckFlag0x20(tp)) {
    twp->mode = 0xb;
  } else {
    tp->disp = EnemyAiDisplayer;
  }
  tp->dest = EnemyAiDestructor;

  _rename_EnemyJetLoadTexture();

  if (mantex_tp == NULL) {
    fn_80072820("E_AITEX", &_rename_e_ai_texlist);
    mantex_tp = CreateFundamentalTask(0, LEV_0, ManTex);
    mantex_tp->dest = ManTexDestructor;
  }
}

static void EnemyAiSetTarget(taskwk *twp, enemywk *ewp) {
  Float rad;

  rad = sqrtf(ewp->home_range);

  ewp->target_pos.x =
      ewp->home_pos.x + 0.2f * (rad * (0.000030517578f * (Float)rand() - 0.5f));
  ewp->target_pos.y = ewp->home_pos.y;
  ewp->target_pos.z =
      ewp->home_pos.z + 0.2f * (rad * (0.000030517578f * (Float)rand() - 0.5f));

  ewp->target_ang = RadAng(atan2f(ewp->target_pos.x - twp->pos.x,
                                  ewp->target_pos.z - twp->pos.z)) -
                    0x4000;
}

static void EnemyAiTurn(taskwk *twp, enemywk *ewp) {
  Sint32 pno;

  if (lbl_801CC168._37 != 0) {
    return;
  }

  if (twp->flag & 0x1000) {
    if (ewp->turn_timer-- < 0) {
      EnemyAiSetTarget(twp, ewp);
      ewp->turn_timer = 60.0f + 60.0f * (3.0f * (0.000030517578f * (Float)rand()));
    } else {
      twp->ang.y = AdjustAngle(twp->ang.y, ewp->target_ang, ewp->turn_spd);
    }
  } else if (twp->flag & 0x2000) {
    pno = fn_80037C84(&twp->pos);
    if (!pno || pno == 1) {
      fn_800176AC(twp, ewp, pno);
    }
  } else if (twp->smode == AI_GUN_CHASE || twp->smode == AI_LASER_CHASE) {
    if (ewp->turn_timer-- < 0 ||
        ((twp->cwp->flag & 1) && twp->cwp->hit_cwp->id != 0)) {
      EnemyAiSetTarget(twp, ewp);
      ewp->turn_timer = 60.0f + 60.0f * (3.0f * (0.000030517578f * (Float)rand()));
    } else {
      twp->ang.y = AdjustAngle(twp->ang.y, ewp->target_ang, ewp->turn_spd);
    }
  }
}

static void EnemyAiCheckHome(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37 != 0) {
    return;
  }

  if (fn_80017A90(twp, ewp) > ewp->home_range) {
    twp->flag |= 0x1000;
  } else {
    twp->flag &= ~0x1000;
  }
}

static void EnemyAiSetEnd(taskwk *twp, enemywk *ewp) { twp->mode = 0xb; }

static void EnemyAiCheckDamage(task *tp, taskwk *twp, enemywk *ewp) {
  Sint32 score;

  if (!fn_800167E0(twp, ewp)) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    if (ewp->flag & 0x400) {
      NJS_VECTOR pos;
      NJS_VECTOR scl;
      Sint32 pno = ewp->pno;

      pos.z = -5.0f;
      pos.y = -17.0f;
      pos.x = 0.0f;
      scl.x = scl.y = scl.z = 0.12f;

      switch (twp->smode) {
      case AI_GUN:
      case AI_GUN_CHASE:
      case AI_CAPTURE:
      case AI_HIDE_GUN:
      case AI_HIDE_CAPTURE:
      case AI_STAND:
        fn_80015730(pno, &_rename_e_ai_object, &_rename_e_ai_texlist, &scl,
                    &pos);
        break;
      case AI_BIG:
        fn_80015730(pno, &_rename_e_ai3_object, &_rename_e_ai_texlist, &scl,
                    &pos);
        break;
      case AI_LASER:
      case AI_LASER_CHASE:
      case AI_HIDE_LASER:
        fn_80015730(pno, &_rename_e_ai2_object, &_rename_e_ai_texlist, &scl,
                    &pos);
        break;
      }
    } else if (twp->id == 8) {
      if (ewp->unk_40 != NULL && !fn_80006728(tp)) {
        ewp->mtn.reqaction = 14;
        twp->wtimer = 0;
        twp->btimer = 0;
        twp->mode = 8;
        twp->flag &= ~4;
        ewp->flag &= ~0x1000;
        fn_8000F748(tp);
        return;
      }

      if (fn_80006728(tp) &&
          !fn_8003679C(twp->cwp->mytask, twp->cwp->hit_cwp->mytask)) {
        Uint32 pno = IsThisTaskPlayer(twp->cwp->hit_cwp->mytask);

        if (pno <= 1) {
          if (CCL_IsHitPlayer(tp) != NULL) {
            NJS_POINT3 pos;
            Angle3 ang;

            pos.x = -2.5f;
            pos.y = 1.0f;
            pos.z = 0.0f;
            ang.x = 0;
            ang.y = twp->ang.y + 0xC000;
            ang.z = 0;
            fn_80039EB0(pno, &pos, &ang, 0);
          }

          ewp->mtn.reqaction = 14;
          twp->wtimer = 0;
          twp->btimer = 0;
          twp->mode = 8;
          twp->flag &= ~4;
          ewp->flag &= ~0x1000;
          fn_8006B7EC(0x401F, NULL, 0, 10, &twp->pos);
          fn_8002FB2C(pno, 3, 7, 0);
          fn_8000F748(tp);
          return;
        }
      }
    }
  }

  twp->mode = 10;
  twp->cwp->info->attr |= 0x10;
  twp->wtimer = 0;
  fn_80017BE4(twp, ewp);
  fn_80016D10(twp, _rename_e_ai_broken_models, &_rename_e_ai_texlist);

  if (twp->smode == AI_BIG) {
    score = 200;
  } else {
    score = 100;
  }
  if (fn_80035C54(tp)) {
    score *= 2;
  }
  fn_80062E0C(score);
  fn_800164FC(twp, ewp);
}

static void EnemyAiSearchNear(taskwk *twp, enemywk *ewp) {
  if (fn_800188B0(twp, ewp)) {
    twp->flag |= 0x2000;
  } else if (twp->smode == AI_BIG && ewp->unk_40 != NULL) {
    twp->flag |= 0x2000;
  } else {
    twp->flag &= ~0x2000;
  }
}

static void EnemyAiSearchFront(taskwk *twp, enemywk *ewp) {
  if (fn_800187C4(twp, ewp)) {
    twp->flag |= 0x2000;
  } else if (twp->smode == AI_BIG && ewp->unk_40 != NULL) {
    twp->flag |= 0x2000;
  } else {
    twp->flag &= ~0x2000;
  }
}

static void EnemyAiMove(task *tp, taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37 != 0) {
    return;
  }

  if (ewp->spd.x < -0.5f) {
    ewp->acc.x = 0.01f;
  } else {
    ewp->acc.x = 0.1f;
  }

  if (CCL_IsHitPlayer(tp) != NULL) {
    ewp->spd.x = -1.0f;
  } else {
    ewp->spd.x += ewp->acc.x;
    if (ewp->acc.x <= 0.0f && fabsf(ewp->spd.x) <= fabsf(ewp->acc.x)) {
      ewp->spd.x = 0.0f;
    }
    ewp->spd.z += ewp->acc.z;
  }

  if (ewp->spd.x > 1.0f) {
    ewp->spd.x = 1.0f;
  } else if (ewp->spd.x < -1.0f) {
    ewp->spd.x = -1.0f;
  }

  twp->pos.x += ewp->spd.x * njCos(twp->ang.y) + ewp->spd.z * njSin(twp->ang.y);
  twp->pos.y += ewp->spd.y;
  twp->pos.z +=
      -(ewp->spd.x * njSin(twp->ang.y)) + ewp->spd.z * njCos(twp->ang.y);

  twp->pos.y -= njSin(ewp->float_ang);
  ewp->float_ang += 0x300;
  fn_80017C00(twp, ewp);
  twp->pos.y += njSin(ewp->float_ang);

  if (ewp->unk_150 < ewp->ground_y + ewp->colli_bottom) {
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

static void EnemyAiFloat(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37 != 0) {
    return;
  }

  twp->pos.y += ewp->spd.y;
  fn_80017C00(twp, ewp);

  if (ewp->mtn.action != 8) {
    ewp->spd.y = 0.0f;
    return;
  }

  ewp->spd.y -= 0.1f;
  if (ewp->spd.y < -3.0f) {
    ewp->spd.y = -3.0f;
  } else if (ewp->spd.y > -1.0f) {
    ewp->spd.y = -1.0f;
  }
}

static void EnemyAiNoMove(taskwk *twp, enemywk *ewp) {}

static void EnemyAiCalcPoint(taskwk *twp, NJS_POINT3 *pos) {
  njPushMatrix(&nj_unit_matrix);
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njCalcPoint(NULL, pos, pos);
  njPopMatrixEx();
}

static void EnemyAiCalcVector(taskwk *twp, NJS_VECTOR *vec) {
  njPushMatrix(&nj_unit_matrix);
  njRotateY(NULL, twp->ang.y);
  njCalcVector(NULL, vec, vec);
  njPopMatrixEx();
}

static void EnemyAiPutJet(taskwk *twp) {
  NJS_POINT3 pos;
  NJS_VECTOR spd;

  if (lbl_801CC168._37 != 0) {
    return;
  }

  pos.x = -7.0f;
  pos.y = 25.0f;
  pos.z = -0.0f;
  EnemyAiCalcPoint(twp, &pos);
  spd.x = -1.2f;
  spd.y = -0.3f;
  spd.z = 0.0f;
  EnemyAiCalcVector(twp, &spd);
  _rename_CreateEnemyJet(NULL, 1, &pos, &spd);

  pos.x = -6.5f;
  pos.y = 23.0f;
  pos.z = -5.0f;
  EnemyAiCalcPoint(twp, &pos);
  spd.x = -1.0f;
  spd.y = -0.7f;
  spd.z = -0.5f;
  EnemyAiCalcVector(twp, &spd);
  _rename_CreateEnemyJet(NULL, 1, &pos, &spd);

  pos.x = -6.5f;
  pos.y = 23.0f;
  pos.z = 5.0f;
  EnemyAiCalcPoint(twp, &pos);
  spd.x = -1.0f;
  spd.y = -0.7f;
  spd.z = 0.5f;
  EnemyAiCalcVector(twp, &spd);
  _rename_CreateEnemyJet(NULL, 1, &pos, &spd);
}

static void EnemyAiAim(taskwk *twp, NJS_POINT3 *pos, NJS_VECTOR *spd) {
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
  njCalcVector(NULL, spd, spd);
  njPopMatrixEx();
}

static void EnemyAiShotBullet(task *tp, taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;
  NJS_VECTOR spd;

  if (twp->smode == AI_GUN_CHASE) {
    pos.x = 22.6f;
    pos.y = 23.1f;
    pos.z = 3.0f;
  } else {
    pos.x = 20.0f;
    pos.y = 26.5f;
    pos.z = 4.7f;
  }
  EnemyAiCalcPoint(twp, &pos);

  spd.x = ewp->shot_spd;
  spd.y = 0.0f;
  spd.z = 0.0f;
  EnemyAiAim(twp, &pos, &spd);

  CreateEnemyBullet(tp, 1, &spd, &pos);
}

static void EnemyAiShotLaser(task *tp, taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;
  NJS_VECTOR spd;

  if (twp->smode == AI_LASER_CHASE) {
    pos.x = 20.4f;
    pos.y = 20.4f;
    pos.z = 3.6f;
  } else {
    pos.x = 20.4f;
    pos.y = 22.4f;
    pos.z = 4.8f;
  }
  EnemyAiCalcPoint(twp, &pos);

  spd.x = ewp->shot_spd;
  spd.y = 0.0f;
  spd.z = 0.0f;
  EnemyAiAim(twp, &pos, &spd);

  _rename_CreateEnemyLaser(tp, (Float)ewp->shot_num, &spd, &pos);
}

static void EnemyAiShotCapture(task *tp, taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;
  NJS_VECTOR spd;

  pos.x = 20.0f;
  pos.y = 26.5f;
  pos.z = 4.7f;
  EnemyAiCalcPoint(twp, &pos);

  spd.x = ewp->shot_spd;
  spd.y = 0.0f;
  spd.z = 0.0f;
  EnemyAiAim(twp, &pos, &spd);

  _rename_CreateEnemyCapturingBullet(tp, &spd, &pos);
}

static void EnemyAiDestructor(task *tp) {
  fn_80018B88(tp);
  fn_80069CB8(tp);
}

static void EnemyAiDeadOut(task *tp) {
  DeadOut(tp);

  if (tp->twp->smode == AI_STAND) {
    _rename_SetFlag0x20(tp);
  }
}

static void EnemyAiWait(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyAiSearchFront(twp, ewp);

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }

  if (twp->flag & 0x2000) {
    ewp->mtn.reqaction = 2;
    twp->mode = 2;
    twp->wtimer = ewp->wait - 60;
    fn_8006B7EC(0x400C, NULL, 0, 110, &twp->pos);
  }

  EnemyAiCheckDamage(tp, twp, ewp);
}

static void EnemyAiAttack(task *tp, taskwk *twp, enemywk *ewp) {
  Angle ang;

  EnemyAiSearchFront(twp, ewp);

  if (ewp->mtn.reqaction == 2) {
    twp->cwp->info->center.y =
        fn_8002AE5C(twp->cwp->info->center.y, 25.0f, 0.4f);
  }

  if (ewp->mtn.action == 6 || ewp->mtn.action == 3 ||
      (ewp->mtn.action == 5 && ewp->mtn.nframe > 1.15f)) {
    ang = twp->ang.y;
    EnemyAiTurn(twp, ewp);
    if (ewp->mtn.action == 6 || ewp->mtn.action == 3) {
      if (DiffAngle(ang, twp->ang.y) >= 0x20) {
        ewp->mtn.reqaction = 6;
      } else {
        ewp->mtn.reqaction = 3;
      }
    }
  } else if (InRange(ewp->mtn.action, 15, 16) || ewp->mtn.action == 13) {
    ang = twp->ang.y;
    EnemyAiTurn(twp, ewp);
    if (ewp->mtn.action == 16 || ewp->mtn.action == 15) {
      if (DiffAngle(ang, twp->ang.y) >= 0x20) {
        ewp->mtn.reqaction = 16;
      } else {
        ewp->mtn.reqaction = 15;
      }
    }
  }

  EnemyAiNoMove(twp, ewp);
  EnemyAiCheckDamage(tp, twp, ewp);

  if (twp->mode == 10 || twp->mode == 8) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }

  if (twp->smode == AI_BIG) {
    if (ewp->mtn.action == 13 || (twp->flag & 0x800)) {
      twp->id = 0;
      ewp->flag |= 0x800;
    } else {
      twp->id = 8;
      ewp->flag &= ~0x800;
    }
  }

  if ((twp->smode == AI_GUN && ewp->shot_num > 0) || twp->smode == AI_CAPTURE) {
    if (twp->flag & 0x400) {
      twp->flag &= ~0x400;
      twp->wtimer = 0;
    }
    if (!(twp->flag & 0x2000)) {
      twp->wtimer = 0;
      twp->btimer = 0;
      return;
    }
    if (twp->wtimer <= ewp->wait) {
      return;
    }
    if (ewp->mtn.action == 3 || ewp->mtn.action == 6) {
      ewp->mtn.reqaction = 5;
      return;
    }
    if (ewp->mtn.action != 5) {
      return;
    }
    if (ewp->mtn.nframe >= 1.0f && ewp->mtn.nframe < 1.15f) {
      if (twp->smode == AI_GUN ||
          playertwp[fn_80037C84(&twp->pos)]->mode == 0x26) {
        EnemyAiShotBullet(tp, twp, ewp);
        twp->btimer++;
      } else {
        EnemyAiShotCapture(tp, twp, ewp);
        twp->btimer = ewp->shot_num;
      }
      if (twp->btimer >= ewp->shot_num) {
        twp->btimer = 0;
        twp->wtimer = 0;
      }
    }
  } else if (twp->smode == AI_LASER || twp->smode == AI_BIG) {
    if (!(twp->flag & 0x2000)) {
      twp->wtimer = 0;
      twp->flag &= ~0x800;
      return;
    }
    if (twp->flag & 0x800) {
      twp->flag &= ~0x800;
      return;
    }
    if (twp->wtimer <= ewp->wait) {
      return;
    }
    if (ewp->mtn.action == 3 || ewp->mtn.action == 6) {
      ewp->mtn.reqaction = 4;
      EnemyAiShotLaser(tp, twp, ewp);
      return;
    }
    if (InRange(ewp->mtn.action, 15, 16)) {
      ewp->mtn.reqaction = 11;
      return;
    }
    if (ewp->mtn.action == 11 && (ewp->mtn.unk_11 & 2)) {
      EnemyAiShotLaser(tp, twp, ewp);
      return;
    }
    if (ewp->mtn.action == 12) {
      ewp->mtn.reqaction = 13;
      twp->wtimer = 0;
      return;
    }
    if (ewp->mtn.action == 4) {
      ewp->mtn.reqaction = 5;
      twp->wtimer = 0;
      return;
    }
  }
}

static void EnemyAiWaitNear(task *tp, taskwk *twp, enemywk *ewp) {
  if (fn_8001897C(twp, ewp)) {
    twp->flag |= 0x2000;
    twp->mode = 7;
  } else {
    twp->flag &= ~0x2000;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }

  EnemyAiCheckDamage(tp, twp, ewp);
}

static void EnemyAiAppear(task *tp, taskwk *twp, enemywk *ewp) {
  if (fn_8001897C(twp, ewp)) {
    twp->flag |= 0x2000;
  } else {
    twp->flag &= ~0x2000;
  }

  EnemyAiTurn(twp, ewp);
  EnemyAiFloat(twp, ewp);

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }

  if (ewp->mtn.action == 3) {
    twp->mode = 2;
    if (twp->smode == AI_HIDE_GUN) {
      twp->smode = AI_GUN;
    } else if (twp->smode == AI_HIDE_LASER) {
      twp->smode = AI_LASER;
    } else if (twp->smode == AI_HIDE_CAPTURE) {
      twp->smode = AI_CAPTURE;
    }
    twp->wtimer = ewp->wait - 60;
  } else if (ewp->mtn.action == 8) {
    if (ewp->flag & 0x80) {
      ewp->mtn.reqaction = 0;
      _rename_PutDustCircle(&twp->pos, 8.0f, 4.0f, 8);
      fn_8006B7EC(0x400B, NULL, 0, 100, &twp->pos);
    }
  }

  EnemyAiCheckDamage(tp, twp, ewp);
}

static void EnemyAiPatrol(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyAiCheckHome(twp, ewp);
  EnemyAiSearchNear(twp, ewp);
  EnemyAiTurn(twp, ewp);
  EnemyAiMove(tp, twp, ewp);
  EnemyAiPutJet(twp);
  EnemyAiCheckDamage(tp, twp, ewp);

  if (twp->mode == 10 || twp->mode == 8) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }

  if (twp->flag & 0x2000) {
    twp->wtimer = 0;
    twp->btimer = 0;
    twp->mode = 5;
  }
}

static void EnemyAiChase(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyAiCheckHome(twp, ewp);
  EnemyAiSearchNear(twp, ewp);

  if (twp->smode == AI_GUN_CHASE ||
      (twp->smode == AI_LASER_CHASE && !(twp->flag & 0x800))) {
    EnemyAiTurn(twp, ewp);
    EnemyAiMove(tp, twp, ewp);
  } else {
    EnemyAiNoMove(twp, ewp);
  }

  EnemyAiPutJet(twp);
  EnemyAiCheckDamage(tp, twp, ewp);

  if (twp->mode == 10 || twp->mode == 8) {
    return;
  }

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }

  if (twp->smode == AI_GUN_CHASE && ewp->shot_num > 0) {
    if (!(twp->flag & 0x2000)) {
      twp->wtimer = 0;
      twp->btimer = 0;
      twp->mode = 3;
      return;
    }
    if (twp->wtimer <= ewp->wait) {
      return;
    }
    if (ewp->mtn.action == 8) {
      ewp->mtn.reqaction = 10;
      return;
    }
    if (ewp->mtn.action != 10) {
      return;
    }
    if (ewp->mtn.nframe >= 1.0f && ewp->mtn.nframe < 1.16f) {
      EnemyAiShotBullet(tp, twp, ewp);
      twp->btimer++;
    }
    if (twp->btimer >= ewp->shot_num) {
      twp->wtimer = 0;
      twp->btimer = 0;
    }
  } else if (twp->smode == AI_LASER_CHASE) {
    if (!(twp->flag & 0x2000)) {
      twp->wtimer = 0;
      twp->flag &= ~0x800;
      twp->mode = 3;
      return;
    }
    if (twp->flag & 0x800) {
      twp->flag &= ~0x800;
      return;
    }
    if (twp->wtimer <= ewp->wait) {
      return;
    }
    if (ewp->mtn.action == 8) {
      ewp->mtn.reqaction = 9;
      EnemyAiShotLaser(tp, twp, ewp);
      return;
    }
    if (ewp->mtn.action == 9) {
      ewp->mtn.reqaction = 10;
      twp->wtimer = 0;
      return;
    }
  }
}

static void EnemyAiDamage(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyAiSearchFront(twp, ewp);
  EnemyAiTurn(twp, ewp);

  if (lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }

  if (twp->wtimer > 6) {
    twp->mode = 2;
    twp->wtimer = 0;
  }

  EnemyAiCheckDamage(tp, twp, ewp);
}

static void EnemyAiStand(task *tp, taskwk *twp, enemywk *ewp) {
  EnemyAiNoMove(twp, ewp);
  EnemyAiCheckDamage(tp, twp, ewp);

  if (twp->mode != 10 && lbl_801CC168._37 == 0) {
    twp->wtimer++;
  }
}

static void EnemyAiNodeCallBack(NJS_CNK_OBJECT *object) {
  NJS_POINT3 pos;

  if (object == &_rename_e_ai_eye_object) {
    pos.x = -3.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    njCalcPoint(NULL, &pos, &pos);
    njCalcPoint(&e_ai_matrix, &pos, &e_ai_ewp->eye_pos);
  } else if (object == &_rename_e_ai2_eye_object) {
    pos.x = -3.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    njCalcPoint(NULL, &pos, &pos);
    njCalcPoint(&e_ai_matrix, &pos, &e_ai_ewp->eye_pos);
  } else if (object == &_rename_e_ai3_eye_object) {
    pos.x = -3.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    njCalcPoint(NULL, &pos, &pos);
    njCalcPoint(&e_ai_matrix, &pos, &e_ai_ewp->eye_pos);
  }
}

static void EnemyAiDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;
  Float unused[16]; // unused

  fn_80024CB8(8);

  if (twp->mode != 6) {
    e_ai_ewp = ewp;
    fn_8011C3A0(EnemyAiNodeCallBack);

    njGetMatrix(&e_ai_matrix);
    njInvertMatrix(&e_ai_matrix);

    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y + 0x4000);
    njSetTexture(&_rename_e_ai_texlist);

    if (ewp->mtn.mtnmode != 2) {
      fn_8011E214(ewp->mtn.object, ewp->mtn.actptr[ewp->mtn.action].motion,
                  ewp->mtn.nframe);
    } else {
      fn_8011E19C(ewp->mtn.object, &ewp->mtn.link, ewp->mtn.nframe);
    }

    njPopMatrixEx();
    fn_8011C3A0(NULL);
    fn_80017CA4(twp, ewp);
  }

  fn_80024CB8(lbl_803AD926);
}

void EnemyAi(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;
  Float frame;

  if (twp->mode != 0 && CheckRangeOut(tp)) {
    return;
  }

  if (_rename_CheckFlag0x20(tp)) {
    FreeTask(tp);
    return;
  }

  switch (twp->mode) {
  case 0:
    EnemyAiInit(tp, twp);
    return;
  case 1:
    EnemyAiWait(tp, twp, ewp);
    break;
  case 2:
    EnemyAiAttack(tp, twp, ewp);
    break;
  case 6:
    EnemyAiWaitNear(tp, twp, ewp);
    break;
  case 7:
    EnemyAiAppear(tp, twp, ewp);
    break;
  case 3:
    EnemyAiPatrol(tp, twp, ewp);
    break;
  case 5:
    EnemyAiChase(tp, twp, ewp);
    break;
  case 8:
    EnemyAiDamage(tp, twp, ewp);
    fn_80017B94(twp, ewp);
    fn_80015C40(&ewp->mtn);
    return;
  case 9:
    EnemyAiStand(tp, twp, ewp);
    break;
  case 10:
    EnemyAiSetEnd(twp, ewp);
  case 11:
    EnemyAiDeadOut(tp);
    return;
  }

  if (lbl_801CC168._37 == 0) {
    fn_80017B94(twp, ewp);
    frame = ewp->mtn.nframe;
    fn_80015C40(&ewp->mtn);

    if (ewp->mtn.mtnmode != 2) {
      switch (ewp->mtn.action) {
      case 6:
        if ((ewp->mtn.nframe >= 1.0f && frame < 1.0f) ||
            (ewp->mtn.nframe >= 3.0f && frame < 3.0f)) {
          fn_8006B7EC(0x400B, NULL, 0, -10, &twp->pos);
        }
        break;
      case 7:
      case 16:
        if ((ewp->mtn.nframe >= 2.0f && frame < 2.0f) ||
            (ewp->mtn.nframe >= 6.0f && frame < 6.0f)) {
          fn_8006B7EC(0x400B, NULL, 0, -10, &twp->pos);
        }
        break;
      }
    }

    if (twp->flag & 0x2000) {
      if (twp->mode == 7 || ewp->mtn.action == 2) {
        _rename_CreateEnemyLight(tp, 4, &ewp->eye_pos);
      } else {
        _rename_CreateEnemyLight(tp, 3, &ewp->eye_pos);
      }
    }
  }

  if (twp->mode != 6 && twp->mode != 10) {
    CCL_Entry(tp);
  }
}
