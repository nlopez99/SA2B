#include "ENEMY/e_laser.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/player.h"

// enemy work allocated by fn_80018D28, sizeof=0x210
// field names modelled on SADX's enemywk
typedef struct enemywk {
  /* 0x00 */ NJS_VECTOR spd;
  /* 0x0C */ NJS_VECTOR acc;
  /* 0x18 */ Uint8 unk_18[0x34];
  /* 0x4C */ Sint16 flag;
  /* 0x4E */ Uint8 unk_4E[0x4E];
  /* 0x9C */ Float len_max;
  /* 0xA0 */ Uint8 unk_A0[0x1C];
  /* 0xBC */ NJS_POINT3 colli_center;
  /* 0xC8 */ Float colli_top;
  /* 0xCC */ Float colli_radius;
  /* 0xD0 */ Float colli_bottom;
} enemywk;

extern void fn_800068BC(task *tp, Float range);
extern void fn_800068E4(colliwk *cwp);
extern void fn_80014650(NJS_POINT3 *pos, Angle3 *ang, NJS_VECTOR *scl, Sint32);
extern void fn_80017BE4(taskwk *twp, enemywk *ewp);
extern void fn_80017C00(taskwk *twp, enemywk *ewp);
extern void fn_80018B88(task *tp);
extern enemywk *fn_80018D28(task *tp);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);
extern void fn_80072820(const char *name, NJS_TEXLIST *texlist);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_8011610C(NJS_VECTOR *scl);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern Float asinf(Float x);
extern Float atan2f(Float y, Float x);
extern Float sqrtf(Float x);
extern void njDisableFog(void);
extern void njEnableFog(void);
extern void gjSetFog(void);

// ^ extern
// v in this file

static void EnemyLaserExecutor(task *tp);
static void ManTex(task *tp);

enum {
  MD_APPEAR,
  MD_EXTEND,
  MD_NORMAL,
  MD_EXPLODE,
  MD_END,
};

#define RadAng(n) ((Angle)(10430.38043493439 * (n)))

#define SetExplode(twp)                                                        \
  do {                                                                         \
    (twp)->mode = MD_EXPLODE;                                                  \
    (twp)->wtimer = 0;                                                         \
    (twp)->scl.x = 5.0f;                                                       \
    (twp)->scl.y = 0.0f;                                                       \
    (twp)->cwp->info->attr |= 0x10;                                            \
  } while (0)

static Sint16 e_laser_plist[] = {
#include "assets/e_laser_plist.inc"
};

static Sint32 e_laser_vlist[] = {
#include "assets/e_laser_vlist.inc"
};

static NJS_CNK_MODEL e_laser_model = {
    e_laser_vlist,
    e_laser_plist,
    {-0.0f, -1e-06f, 0.083336f},
    5.149998f,
};

static Sint16 e_laser_flash_plist[] = {
#include "assets/e_laser_flash_plist.inc"
};

static Sint32 e_laser_flash_vlist[] = {
#include "assets/e_laser_flash_vlist.inc"
};

static NJS_CNK_MODEL e_laser_flash_model = {
    e_laser_flash_vlist,
    e_laser_flash_plist,
    {0.0f, 0.0f, -0.00357f},
    1.145347f,
};

static NJS_TEXNAME e_laser_texname[] = {
    {"ENEMY5"},
};

static NJS_TEXLIST e_laser_texlist = {
    e_laser_texname,
    ARYLEN(e_laser_texname),
};

static CCL_INFO e_laser_colli_info[1] = {
    {CI_KIND_NO_PUNCH, CI_FORM_CAPSULE, 0x70, (Sint8)0xEE, 0x00808600,
     {0.0f, 0.0f, 0.0f}, 0.25f, 0.5f, 0.0f, 0.0f, 0, 0, 0},
};

// task that owns the textures
static task *mantex_tp;

static void EnemyLaserMove(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37) {
    return;
  }

  njAddVector(&twp->pos, &ewp->spd);
  ewp->acc.x = 0.0f;
  ewp->acc.y = 0.0f;
  ewp->acc.z = 0.0f;
}

static void EnemyLaserDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  NJS_VECTOR flash_scl;
  Angle3 ang;
  NJS_VECTOR scl;

  njDisableFog();
  gjSetFog();
  if (twp->mode == MD_APPEAR) {
    // muzzle flash, shrinking as scl.x grows
    flash_scl.x = 2.0f * (1.0f / sqrtf(twp->scl.x));
    flash_scl.y = flash_scl.x;
    flash_scl.z = 5.0f;
    njSetTexture(&e_laser_texlist);
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    fn_8011610C(&flash_scl);
    njCnkCacheDrawModel(&e_laser_flash_model);
    njPopMatrixEx();
  } else if (twp->mode == MD_EXPLODE) {
    ang.x = ang.y = ang.z = 0;
    scl.z = scl.y = scl.x = twp->scl.x;
    fn_80014650(&twp->pos, &ang, &scl, 4);
  } else {
    // scl.z is the beam's length, grown a frame at a time in MD_EXTEND
    njSetTexture(&e_laser_texlist);
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    njRotateX(NULL, twp->ang.x);
    njScale(NULL, 1.0f, 1.0f, 0.2f * twp->scl.z);
    fn_8011E158(&e_laser_model);
    njPopMatrixEx();
  }
  njEnableFog();
  gjSetFog();
}

static void EnemyLaserCheckHit(taskwk *twp, enemywk *ewp) {
  BOOL hit = FALSE;

  if ((twp->flag & 0xD) || (twp->cwp->flag & 1)) {
    hit = TRUE;
  }
  if (hit == TRUE) {
    SetExplode(twp);
  }
}

static void EnemyLaserAppear(taskwk *twp, enemywk *ewp) {
  if (twp->scl.x >= 1.0f) {
    twp->mode = MD_EXTEND;
    twp->scl.x = 1.0f;
    fn_8006B7EC(0x4002, NULL, 0, 0, &twp->pos);
  } else if (!lbl_801CC168._37) {
    twp->scl.x += 0.02f;
  }
}

static void EnemyLaserExtend(taskwk *twp, enemywk *ewp) {
  if (twp->scl.z >= ewp->len_max) {
    twp->mode = MD_NORMAL;
    twp->scl.z = ewp->len_max;
  } else if (!lbl_801CC168._37) {
    // the head moves at half speed and the beam grows by the same step
    twp->pos.x += 0.5f * ewp->spd.x;
    twp->pos.y += 0.5f * ewp->spd.y;
    twp->pos.z += 0.5f * ewp->spd.z;
    twp->scl.z += 0.5f * njScalor(&ewp->spd);
  }
  twp->cwp->info->b = twp->scl.z;
  fn_800068E4(twp->cwp);
}

static void EnemyLaserNormal(taskwk *twp, enemywk *ewp) {
  if (twp->wtimer > 20) {
    EnemyLaserCheckHit(twp, ewp);
  }
  EnemyLaserMove(twp, ewp);
  fn_80017C00(twp, ewp);

  if (ewp->flag & 0x1C0) {
    SetExplode(twp);
  }
  if (!lbl_801CC168._37) {
    twp->ang.z += 0x1400;
  }
}

static void EnemyLaserExplode(taskwk *twp, enemywk *ewp) {
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

static void EnemyLaserDestructor(task *tp) {
  fn_80018B88(tp);
}

static void EnemyLaserExecutor(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;

  if (CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case MD_APPEAR:
    EnemyLaserAppear(twp, ewp);
    // flag the enemy that fired it
    if (tp->ptp != NULL) {
      tp->ptp->twp->flag |= 0x800;
    }
    break;
  case MD_EXTEND:
    EnemyLaserExtend(twp, ewp);
    break;
  case MD_NORMAL:
    EnemyLaserNormal(twp, ewp);
    if (twp->mode == MD_EXPLODE) {
      tp->disp_dely = NULL;
    }
    break;
  case MD_EXPLODE:
    EnemyLaserExplode(twp, ewp);
    if (twp->mode != MD_EXPLODE) {
      FreeTask(tp);
      return;
    }
    break;
  }

  twp->cwp->info->angx = twp->ang.x + 0x4000;
  twp->cwp->info->angy = twp->ang.y;
  CCL_Entry(tp);
  if (!lbl_801CC168._37) {
    twp->wtimer++;
  }
}

static void EnemyLaserInit(task *tp, Float pow, NJS_VECTOR *spd) {
  taskwk *twp = tp->twp;
  enemywk *ewp;
  NJS_VECTOR v; // unused

  CCL_Init(tp, e_laser_colli_info, ARYLEN(e_laser_colli_info), CID_ENEMY2);
  tp->twp->cwp->flag &= ~0x40;
  fn_800068BC(tp, pow);
  ewp = fn_80018D28(tp);
  ewp->spd = *spd;
  ewp->len_max = 0.5f * pow;
  twp->scl.z = 0.0f;
  twp->mode = MD_APPEAR;
  twp->scl.x = 0.02f;
  twp->ang.x = RadAng(asinf(-ewp->spd.y / njScalor(&ewp->spd)));
  twp->ang.y = RadAng(atan2f(ewp->spd.x, ewp->spd.z));
  ewp->colli_center.x = 0.0f;
  ewp->colli_center.y = 0.0f;
  ewp->colli_center.z = 0.0f;
  ewp->colli_top = 3.0f;
  ewp->colli_radius = 3.0f;
  ewp->colli_bottom = -3.0f;
  fn_80017BE4(twp, ewp);
  tp->dest = EnemyLaserDestructor;
  tp->disp_dely = EnemyLaserDisplayer;
}

static void ManTexDestructor(task *tp) {
  fn_801166F0(&e_laser_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

void EnemyLaserLoadTexture(void) {
  if (mantex_tp != NULL) {
    return;
  }

  fn_80072820("E_LASTEX", &e_laser_texlist);
  mantex_tp = CreateElementalTask(IM_NONE, LEV_0, ManTex, "ManTex");
  mantex_tp->dest = ManTexDestructor;
}

void CreateEnemyLaser(task *ptp, Float pow, NJS_VECTOR *spd, NJS_POINT3 *pos) {
  task *tp;

  if (ptp != NULL) {
    tp = CreateChildTask(IM_TWK, EnemyLaserExecutor, ptp);
  } else {
    tp = CreateElementalTask(IM_TWK, LEV_3, EnemyLaserExecutor,
                             "EnemyLaserExecutor");
  }
  if (tp == NULL) {
    return;
  }

  tp->twp->pos = *pos;
  EnemyLaserInit(tp, pow, spd);
  EnemyLaserLoadTexture();
  fn_8006B7EC(0x4001, NULL, 0, 0, pos);
}
