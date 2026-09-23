#include "ENEMY/e_bullet.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "set.h"

// enemy work allocated by fn_80018D28, sizeof=0x210
// field names modelled on SADX's enemywk
typedef struct enemywk {
  /* 0x00 */ NJS_VECTOR spd;
  /* 0x0C */ NJS_VECTOR acc;
  /* 0x18 */ Uint8 unk_18[0x34];
  /* 0x4C */ Sint16 flag;
  /* 0x4E */ Uint8 unk_4E[6];
  /* 0x54 */ Float unk_54;
  /* 0x58 */ NJS_POINT3 pre_pos;
  /* 0x64 */ Uint8 unk_64[0x58];
  /* 0xBC */ NJS_POINT3 colli_center;
  /* 0xC8 */ Float colli_top;
  /* 0xCC */ Float colli_radius;
  /* 0xD0 */ Float colli_bottom;
} enemywk;

extern void fn_800068BC(task *tp, Float range);
extern void fn_80014650(NJS_POINT3 *pos, Angle3 *ang, NJS_VECTOR *scl, Sint32);
extern void fn_800156FC(Float, Float, Float, Float);
extern void fn_80017BE4(taskwk *twp, enemywk *ewp);
extern void fn_80017C00(taskwk *twp, enemywk *ewp);
extern void fn_80018B88(task *tp);
extern enemywk *fn_80018D28(task *tp);
extern void fn_80024CB8(Sint32);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);
extern void fn_80072820(const char *name, NJS_TEXLIST *texlist);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern void *fn_8011F504(NJS_CNK_OBJECT *object, void *buf);
extern Sint32 fn_8011F50C(NJS_CNK_OBJECT *object);
extern void fn_8011F520_nop(void);
extern void ds_DrawModelClip(void *model);
extern Float asinf(Float x);
extern Float atan2f(Float y, Float x);
extern void njEnableFog(void);
extern void gjSetFog(void);

extern void _rename_CreateSmoke(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void CreateSpark(Sint32 num, NJS_POINT3 *pos, NJS_VECTOR *spd);

// ^ extern
// v in this file

static void EnemyBulletExecutor(task *tp);
static void ManTex(task *tp);

enum {
  MD_NORMAL,
  MD_EXPLODE,
  MD_END,
};

#define TRAIL_NUM 10

#define RadAng(n) ((Angle)(10430.38043493439 * (n)))

#define SetExplode(twp)                                                        \
  do {                                                                         \
    (twp)->mode = MD_EXPLODE;                                                  \
    (twp)->wtimer = 0;                                                         \
    (twp)->scl.x = 5.0f;                                                       \
    (twp)->scl.y = 0.0f;                                                       \
    (twp)->cwp->info->attr |= 0x10;                                            \
  } while (0)

static NJS_TEXNAME e_bullet_texname[] = {
    {"ENEMY5"},
};

static NJS_TEXLIST e_bullet_texlist = {
    e_bullet_texname,
    ARYLEN(e_bullet_texname),
};

static Sint16 e_bullet_plist[] = {
#include "assets/e_bullet_plist.inc"
};

static Sint32 e_bullet_vlist[] = {
#include "assets/e_bullet_vlist.inc"
};

static NJS_CNK_MODEL e_bullet_model = {
    e_bullet_vlist,
    e_bullet_plist,
    {0.0f, -0.0f, 4.266667f},
    4.3351f,
};

static NJS_CNK_OBJECT e_bullet_object = {
    0x17, &e_bullet_model, {0.0f, 0.0f, 0.0f}, {0, 0, 0}, {1.0f, 1.0f, 1.0f},
    NULL, NULL,
};

static Sint16 e_bullet_trail_plist[] = {
#include "assets/e_bullet_trail_plist.inc"
};

static Sint32 e_bullet_trail_vlist[] = {
#include "assets/e_bullet_trail_vlist.inc"
};

static NJS_CNK_MODEL e_bullet_trail_model = {
    e_bullet_trail_vlist,
    e_bullet_trail_plist,
    {0.0f, 0.0f, -2.888884f},
    5.348093f,
};

static NJS_CNK_OBJECT e_bullet_trail_object = {
    0x17, &e_bullet_trail_model, {0.0f, 0.0f, 0.0f}, {0, 0, 0},
    {1.0f, 1.0f, 1.0f}, NULL, NULL,
};

static CCL_INFO e_bullet_colli_info[1] = {
    {CI_KIND_NO_PUNCH, CI_FORM_SPHERE, 0x70, (Sint8)0xE2, 0x00808400,
     {0.0f, 0.0f, 0.0f}, 2.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
};

// task that owns the textures; its fwp/awp hold the prebuilt models
static task *mantex_tp;

static void EnemyBulletTrailDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  Uint32 ofs;

  if (tp->ptp->ctp == tp) {
    if (lbl_801CC168._3C != 1) {
      njSetTexture(&e_bullet_texlist);
    }
    njPushMatrixEx();
  }

  if (twp->scl.x > -1.0f) {
    if (lbl_801CC168._3C != 1) {
      fn_800156FC(twp->scl.x, 0.0f, 0.0f, 0.0f);
    } else {
      // ofs is unset on the other path, where fwp is never allocated;
      // clamp nested to match
      ofs = twp->wtimer;
      if (ofs >= TRAIL_NUM) {
        ofs = TRAIL_NUM - 1;
      }
      ofs *= mantex_tp->work.l;
    }
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    njRotateX(NULL, twp->ang.x);
    njRotateZ(NULL, twp->ang.z);
    if (mantex_tp->fwp != NULL) {
      ds_DrawModelClip((Uint8 *)mantex_tp->fwp + ofs);
    } else {
      fn_8011E158(&e_bullet_trail_model);
    }
  }

  if (tp->ptp->ctp == tp->next) {
    njPopMatrixEx();
    njEnableFog();
    gjSetFog();
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  } else {
    njPopMatrixEx();
    njPushMatrixEx();
  }
}

static void EnemyBulletTrailExecutor(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }
  if (lbl_801CC168._37) {
    return;
  }

  if (twp->wtimer++ >= TRAIL_NUM) {
    FreeTask(tp);
    return;
  }
  twp->scl.x -= 0.04f;
}

static void EnemyBulletTrailCreate(task *tp, NJS_POINT3 *pos, Angle3 *ang,
                                   NJS_VECTOR *spd) {
  task *ctp;
  taskwk *ctwp;

  if (lbl_801CC168._7C & 1) {
    return;
  }
  ctp = CreateChildTask(IM_TWK, EnemyBulletTrailExecutor, tp);
  if (ctp == NULL) {
    return;
  }

  ctwp = ctp->twp;
  ctwp->pos = *pos;
  ctwp->ang = *ang;
  ctwp->scl.x = -1.0f + 0.3f * njScalor(spd);
  if (ctwp->scl.x > -0.5f) {
    ctwp->scl.x = -0.5f;
  }
  ctp->disp_dely = EnemyBulletTrailDisplayer;
}

static void EnemyBulletMove(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37) {
    return;
  }

  twp->pos.x += ewp->spd.x;
  twp->pos.y += ewp->spd.y;
  twp->pos.z += ewp->spd.z;
  ewp->acc.x = 0.0f;
  ewp->acc.y = 0.0f;
  ewp->acc.z = 0.0f;
}

static void EnemyBulletDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  Angle3 ang;
  NJS_VECTOR scl;
  void *model;

  if (twp->mode == MD_EXPLODE) {
    ang.x = ang.y = ang.z = 0;
    scl.z = scl.y = scl.x = twp->scl.x;
    fn_80014650(&twp->pos, &ang, &scl, 4);
    return;
  }

  njPushMatrixEx();
  if (lbl_801CC168._3C != 1) {
    njSetTexture(&e_bullet_texlist);
  }
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, twp->ang.x);
  njRotateZ(NULL, twp->ang.z);
  if (lbl_801CC168._3C == 1 && mantex_tp != NULL && mantex_tp->awp != NULL) {
    model = mantex_tp->awp;
    ds_DrawModelClip(model);
  } else {
    njCnkCacheDrawModel(&e_bullet_model);
  }
  njPopMatrixEx();
}

static void EnemyBulletCheckHit(taskwk *twp, enemywk *ewp) {
  BOOL hit = FALSE;

  if (twp->flag & 0xD) {
    hit = TRUE;
  }
  if (hit == TRUE) {
    SetExplode(twp);
  }
}

static void EnemyBulletNormal(taskwk *twp, enemywk *ewp) {
  NJS_VECTOR spd;

  if (twp->wtimer > 20) {
    EnemyBulletCheckHit(twp, ewp);
  }
  EnemyBulletMove(twp, ewp);
  if (twp->smode != 0) {
    fn_80017C00(twp, ewp);
  }

  if (ewp->flag & 0x1C0) {
    spd.x = 0.1f * ewp->spd.x;
    spd.y = 0.1f * ewp->spd.y;
    spd.z = 0.1f * ewp->spd.z;
    CreateSpark(16, &twp->pos, &spd);
    SetExplode(twp);
  }
  if (!lbl_801CC168._37) {
    twp->ang.z += 0x1400;
  }
}

static void EnemyBulletExplode(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37) {
    return;
  }

  if (twp->scl.x < 25.0f) {
    twp->scl.x += 1.0f;
  } else {
    twp->scl.y -= 0.05f;
    if (twp->scl.y < -1.0f) {
      twp->mode = MD_END;
      twp->wtimer = 0;
    }
  }
  twp->ang.y += 0x100;
}

static void EnemyBulletDestructor(task *tp) {
  fn_80018B88(tp);
}

static void EnemyBulletExecutor(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;

  if (CheckRangeOut(tp)) {
    return;
  }

  // the enemy that fired it is being destroyed
  if (tp->ptp != NULL && tp->ptp->exec == DestroyTask) {
    tp->disp_dely = NULL;
    twp->cwp->info->attr |= 0x10;
    FreeTask(tp);
    return;
  }

  switch (twp->mode) {
  case MD_NORMAL:
    EnemyBulletNormal(twp, ewp);
    if (twp->mode == MD_EXPLODE) {
      tp->disp_dely = NULL;
      break;
    }
    if (!lbl_801CC168._37) {
      EnemyBulletTrailCreate(tp, &twp->pos, &twp->ang, &ewp->spd);
    }
    CCL_Entry(tp);
    break;
  case MD_EXPLODE:
    EnemyBulletExplode(twp, ewp);
    if (twp->mode != MD_EXPLODE) {
      FreeTask(tp);
      return;
    }
    break;
  }

  if (!lbl_801CC168._37) {
    twp->wtimer++;
  }
}

static void EnemyBulletInit(task *tp, NJS_VECTOR *spd) {
  taskwk *twp = tp->twp;
  enemywk *ewp;
  NJS_VECTOR v; // unused

  CCL_Init(tp, e_bullet_colli_info, ARYLEN(e_bullet_colli_info), CID_ENEMY2);
  tp->twp->cwp->flag &= ~0x40;
  fn_800068BC(tp, 2.0f + njScalor(spd));
  ewp = fn_80018D28(tp);
  ewp->spd = *spd;
  twp->scl.x = 1.0f;
  twp->scl.y = 1.0f;
  twp->ang.x = RadAng(asinf(-ewp->spd.y / njScalor(&ewp->spd)));
  twp->ang.y = RadAng(atan2f(ewp->spd.x, ewp->spd.z));
  ewp->colli_center.x = 0.0f;
  ewp->colli_center.y = 0.0f;
  ewp->colli_center.z = 0.0f;
  ewp->colli_top = 3.0f;
  ewp->colli_radius = 3.0f;
  ewp->colli_bottom = -3.0f;
  ewp->unk_54 = 2.0f;
  fn_80017BE4(twp, ewp);
  tp->dest = EnemyBulletDestructor;
  tp->disp_dely = EnemyBulletDisplayer;
}

static void ManTexDestructor(task *tp) {
  fn_801166F0(&e_bullet_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

void EnemyBulletLoadTexture(void) {
  Sint32 size;
  Sint32 i;
  Uint8 *buf;
  Float alpha;

  if (mantex_tp != NULL) {
    return;
  }

  fn_80072820("E_BULTEX", &e_bullet_texlist);
  mantex_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
  mantex_tp->dest = ManTexDestructor;
  if (lbl_801CC168._3C != 1) {
    return;
  }
  if (mantex_tp == NULL) {
    return;
  }

  // one prebuilt trail model per alpha step
  njSetTexture(&e_bullet_texlist);
  fn_80024CB8(8);
  njEnableFog();
  gjSetFog();
  fn_8011F520_nop();
  size = fn_8011F50C(&e_bullet_trail_object);
  mantex_tp->work.l = size;
  if ((mantex_tp->fwp = syCalloc(TRAIL_NUM, size)) != NULL) {
    alpha = -0.5f;
    buf = (Uint8 *)mantex_tp->fwp;
    for (i = 0; i < TRAIL_NUM; i++) {
      fn_800156FC(alpha, 0.0f, 0.0f, 0.0f);
      fn_8011F504(&e_bullet_trail_object, buf);
      buf += size;
      alpha -= 0.04f;
    }
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  }

  if ((mantex_tp->awp = syCalloc(1, fn_8011F50C(&e_bullet_object))) !=
      NULL) {
    buf = (Uint8 *)mantex_tp->awp;
    fn_8011F504(&e_bullet_object, buf);
  }
}

void CreateEnemyBullet(task *ptp, Sint32 smode, NJS_VECTOR *spd,
                       NJS_POINT3 *pos) {
  task *tp;
  NJS_VECTOR v;

  if (ptp != NULL) {
    tp = CreateChildTask(IM_TWK, EnemyBulletExecutor, ptp);
  } else {
    tp = CreateFundamentalTask(IM_TWK, LEV_3, EnemyBulletExecutor);
  }
  if (tp == NULL) {
    return;
  }

  tp->twp->smode = smode;
  tp->twp->pos = *pos;
  EnemyBulletInit(tp, spd);
  EnemyBulletLoadTexture();
  v.x = 0.2f * spd->x;
  v.y = 0.2f * spd->y;
  v.z = 0.2f * spd->z;
  _rename_CreateSmoke(pos, &v, 2.0f);
  v.x = 0.1f * spd->x;
  v.y = 0.1f * spd->y;
  v.z = 0.1f * spd->z;
  CreateSpark(8, pos, &v);
  fn_8006B7EC(0x4000, NULL, 0, 0x7F, pos);
}
