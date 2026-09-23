#include "ENEMY/e_bomb.h"

#include "CCL.h"
#include "EFFECT/ef_explosion.h"
#include "OBJECT/o_ring.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "fabsf.h"

// enemy work allocated by fn_80018D28, sizeof=0x210
// field names modelled on SADX's enemywk
typedef struct enemywk {
  /* 0x000 */ NJS_VECTOR spd;
  /* 0x00C */ NJS_VECTOR acc;
  /* 0x018 */ Uint8 unk_18[0x34];
  /* 0x04C */ Sint16 flag;
  /* 0x04E */ Sint16 timer;
  /* 0x050 */ Uint8 unk_50[4];
  /* 0x054 */ Float unk_54;
  /* 0x058 */ NJS_POINT3 pre_pos;
  /* 0x064 */ Uint8 unk_64[0x30];
  /* 0x094 */ Float scl;
  /* 0x098 */ Uint8 unk_98[0x24];
  /* 0x0BC */ NJS_POINT3 colli_center;
  /* 0x0C8 */ Float colli_top;
  /* 0x0CC */ Float colli_radius;
  /* 0x0D0 */ Float colli_bottom;
  /* 0x0D4 */ Float unk_D4;
  /* 0x0D8 */ Float unk_D8;
  /* 0x0DC */ Float unk_DC;
  /* 0x0E0 */ Float unk_E0;
  /* 0x0E4 */ Float unk_E4;
  /* 0x0E8 */ Float unk_E8;
  /* 0x0EC */ Float unk_EC;
  /* 0x0F0 */ Float unk_F0;
  /* 0x0F4 */ Uint8 unk_F4[0xCC];
  /* 0x1C0 */ Float unk_1C0;
  /* 0x1C4 */ Uint8 unk_1C4[4];
  /* 0x1C8 */ Sint32 unk_1C8;
  /* 0x1CC */ Uint8 unk_1CC[4];
  /* 0x1D0 */ Angle unk_1D0;
  /* 0x1D4 */ Uint8 unk_1D4[0xC];
  /* 0x1E0 */ Float unk_1E0;
} enemywk;

// read by fn_80073028, which rewrites the UVs of a model every few frames
typedef struct uvanim_info // sizeof=0x24
{
  /* 0x00 */ Uint32 flag;
  /* 0x04 */ Uint32 frame_num;
  /* 0x08 */ Uint32 frame_time;
  /* 0x0C */ void *unkC;
  /* 0x10 */ Sint16 *uv;
  /* 0x14 */ void *unk14;
  /* 0x18 */ void *unk18;
  /* 0x1C */ Uint32 unk1C;
  /* 0x20 */ Uint32 unk20;
} uvanim_info;

typedef struct lbl_803AD860_t {
  Uint8 unk_0[0x10];
  /* 0x10 */ Sint16 _10;
} lbl_803AD860_t;

extern void fn_800068BC(task *tp, Float range);
extern void fn_800156FC(Float, Float, Float, Float);
extern void fn_800169A4(taskwk *twp, enemywk *ewp);
extern void fn_80016A1C(Sint32 num, NJS_POINT3 *pos, Angle3 *ang);
extern Float fn_800177EC(taskwk *twp, Sint32 pno);
extern void fn_80017BE4(taskwk *twp, enemywk *ewp);
extern void fn_80017C00(taskwk *twp, enemywk *ewp);
extern void fn_80018B88(task *tp);
extern enemywk *fn_80018D28(task *tp);
extern void fn_80024CB8(Sint32);
extern Float fn_8002AE5C(Float val, Float target, Float step);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B32C(Sint32, Sint32);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern Sint32 fn_80037C84(NJS_POINT3 *pos);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);
extern void fn_80072820(const char *name, NJS_TEXLIST *texlist);
extern void fn_80073028(NJS_CNK_MODEL *model, void *info, Uint32 frame);
extern void fn_800E29BC(NJS_POINT3 *pos, NJS_VECTOR *spd, Float r);
extern void fn_8011610C(NJS_VECTOR *);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_8011E17C(NJS_CNK_MODEL *model);
extern void ObjectMovableInitialize(taskwk *twp, motionwk *mwp, Sint32);
extern Float atan2f(Float y, Float x);
extern Float sqrtf(Float x);
extern void njEnableFog(void);
extern void gjSetFog(void);

extern Sint8 lbl_803AD926;
extern lbl_803AD860_t *lbl_803AD860;
extern NJS_CNK_MODEL *lbl_801857F0[8];
extern NJS_MATRIX nj_unit_matrix;

extern void o_ring_3(void);
extern void _rename_CreateSpark(Sint32 num, NJS_POINT3 *pos, NJS_VECTOR *spd);

// ^ extern
// v in this file

static void EnemyBombExecutor(task *tp);
static void ManTex(task *tp);

enum {
  MD_NORMAL,
  MD_COUNT,
  MD_EXPLODE,
  MD_PILLAR,
  MD_HOLD,
  MD_END,
};

#define RadAng(n) ((Angle)(10430.38043493439 * (n)))

#define SetExplosion(twp, ewp)                                                 \
  do {                                                                         \
    (twp)->scl.x = 5.0f;                                                       \
    (twp)->scl.y = 0.0f;                                                       \
    (twp)->cwp->info[1].attr &= ~0x10;                                         \
    (twp)->cwp->info[0].attr |= 0x10;                                          \
    EnemyBombCreateExplosion(twp, ewp);                                        \
  } while (0)

#define SetExplode(twp, ewp)                                                   \
  do {                                                                         \
    SetExplosion(twp, ewp);                                                    \
    if ((twp)->smode == ENEMYBOMB_PILLAR) {                                    \
      (twp)->cwp->info[3].attr &= ~0x10;                                       \
      (twp)->mode = MD_PILLAR;                                                 \
    } else {                                                                   \
      (twp)->mode = MD_EXPLODE;                                                \
    }                                                                          \
    (twp)->wtimer = 0;                                                         \
  } while (0)

static NJS_TEXNAME e_bomb_texname[] = {
    {"enemy2"},
    {"enemy3"},
    {"weapon03"},
};

NJS_TEXLIST e_bomb_texlist = {
    e_bomb_texname,
    ARYLEN(e_bomb_texname),
};

static Sint16 e_bomb_big_plist[] = {
#include "assets/e_bomb_big_plist.inc"
};

static Sint32 e_bomb_big_vlist[] = {
#include "assets/e_bomb_big_vlist.inc"
};

static NJS_CNK_MODEL e_bomb_big_model = {
    e_bomb_big_vlist,
    e_bomb_big_plist,
    {0.0f, 0.0f, 0.0f},
    5.0f,
};

static Sint16 e_bomb_plist[] = {
#include "assets/e_bomb_plist.inc"
};

static Sint32 e_bomb_vlist[] = {
#include "assets/e_bomb_vlist.inc"
};

NJS_CNK_MODEL e_bomb_model = {
    e_bomb_vlist,
    e_bomb_plist,
    {0.0f, 0.0f, 0.0f},
    2.5f,
};

static Sint16 e_bomb_touch_plist[] = {
#include "assets/e_bomb_touch_plist.inc"
};

static Sint32 e_bomb_touch_vlist[] = {
#include "assets/e_bomb_touch_vlist.inc"
};

static NJS_CNK_MODEL e_bomb_touch_model = {
    e_bomb_touch_vlist,
    e_bomb_touch_plist,
    {0.0f, 0.0f, 0.0f},
    2.5f,
};

// one strip of 18 UVs at plist offset 0xE1
static Sint16 e_bomb_big_uv[] = {
    1,    1,   0xE1, 18,
    -63,  0,   -63,  255,
    192,  0,   192,  255,
    448,  0,   448,  255,
    704,  0,   704,  255,
    960,  0,   960,  255,
    1216, 0,   1216, 255,
    1472, 0,   1472, 255,
    1728, 0,   1728, 255,
    1984, 0,   1984, 255,
};

// with flag 2 the second word is a (u, v) step of (-20, 0) per frame
static uvanim_info e_bomb_big_uvanim = {2, 0xFFEC0000, 1, NULL, e_bomb_big_uv};

static CCL_INFO e_bomb_colli_info[3] = {
    {0, CI_FORM_CYLINDER, 0x70, 0x2C, 0x0038A000,
     {0.0f, 0.0f, 0.0f}, 7.2f, 3.0f, 0.0f, 0.0f, 0, 0, 0},
    {CI_KIND_BOMB_EXPLOSION, CI_FORM_SPHERE, 0x70, (Sint8)0xE2, 0x00008010,
     {0.0f, 0.0f, 0.0f}, 5.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
    {0, CI_FORM_CYLINDER, 0x07, 0x60, 0x00808400,
     {0.0f, 0.0f, 0.0f}, 2.0f, 3.0f, 0.0f, 0.0f, 0, 0, 0},
};

static CCL_INFO e_bomb_big_colli_info[3] = {
    {0, CI_FORM_CYLINDER, 0x70, 0x2C, 0x0038A010,
     {0.0f, 0.0f, 0.0f}, 22.0f, 15.0f, 0.0f, 0.0f, 0, 0, 0},
    {CI_KIND_BOMB_EXPLOSION, CI_FORM_SPHERE, 0x70, (Sint8)0xE2, 0x00008010,
     {0.0f, 0.0f, 0.0f}, 5.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
    // grows with the bomb
    {0, CI_FORM_CYLINDER, 0x07, 0x60, 0x00808400,
     {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
};

static CCL_INFO e_bomb_pillar_colli_info[4] = {
    {0, CI_FORM_CYLINDER, 0x70, 0x2C, 0x0000A010,
     {0.0f, 0.0f, 0.0f}, 7.2f, 3.0f, 0.0f, 0.0f, 0, 0, 0},
    {CI_KIND_BOMB_EXPLOSION, CI_FORM_SPHERE, 0x70, (Sint8)0xE2, 0x00008010,
     {0.0f, 0.0f, 0.0f}, 5.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
    {0, CI_FORM_CYLINDER, 0x07, 0x60, 0x00808400,
     {0.0f, 0.0f, 0.0f}, 3.0f, 3.0f, 0.0f, 0.0f, 0, 0, 0},
    {0, CI_FORM_CYLINDER, 0x70, (Sint8)0xE2, 0x00008010,
     {0.0f, 0.0f, 0.0f}, 5.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
};

// task that owns the textures
static task *mantex_tp;

static void EnemyBombMove(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37) {
    return;
  }

  ewp->spd.y += ewp->acc.y - 0.06f;
  njAddVector(&twp->pos, &ewp->spd);
  ewp->acc.x = 0.0f;
  ewp->acc.y = 0.0f;
  ewp->acc.z = 0.0f;
}

static void EnemyBombCreateExplosion(taskwk *twp, enemywk *ewp) {
  NJS_POINT3 pos;
  NJS_VECTOR big_spd;
  NJS_VECTOR spd;

  if (twp->smode == ENEMYBOMB_BIG) {
    if (ewp->flag & 0x80) {
      pos = twp->pos;
      pos.y -= 10.0f;
      CreateExplosion(EXPLOSION_LL_GROUND, &pos);
    } else {
      CreateExplosion(EXPLOSION_LL, &twp->pos);
    }
    big_spd.x = 0.0f;
    big_spd.y = 0.0f;
    big_spd.z = 0.0f;
    fn_800E29BC(&twp->pos, &big_spd, 25.0f);
    _rename_CreateSpark(80, &twp->pos, NULL);
    fn_80016A1C(48, &twp->pos, &twp->ang);
    fn_8006B7EC(0x401A, NULL, 0, 0x7F, &twp->pos);
    if (!lbl_801CC168.TWO_PLAYER) {
      fn_8002FB2C(0, 5, 15, 0);
    }
  } else {
    // nested to match
    if (ewp->flag & 0x80) {
      CreateExplosion(EXPLOSION_L_GROUND, &twp->pos);
    } else {
      CreateExplosion(EXPLOSION_L, &twp->pos);
    }
    spd.x = 0.0f;
    spd.y = 0.0f;
    spd.z = 0.0f;
    fn_800E29BC(&twp->pos, &spd, 15.0f);
    _rename_CreateSpark(48, &twp->pos, NULL);
    fn_80016A1C(16, &twp->pos, &twp->ang);
    fn_8006B7EC(0x4019, NULL, 0, 0x5A, &twp->pos);
    if (twp->smode == ENEMYBOMB_PILLAR) {
      fn_8006B7EC(0x401B, NULL, 0, 10, &twp->pos);
    }
    if (!lbl_801CC168.TWO_PLAYER) {
      fn_8002FB2C(0, 3, 7, 0);
    }
  }
}

static void EnemyBombDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  Float sx = twp->scl.x;
  Float sy = twp->scl.y;

  fn_80024CB8(8);
  njSetTexture(&e_bomb_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  if (twp->smode == ENEMYBOMB_BIG) {
    enemywk *ewp = (enemywk *)tp->mwp;

    sx *= ewp->scl;
    sy *= ewp->scl;
    fn_80073028(&e_bomb_big_model, &e_bomb_big_uvanim, ewp->timer);
    njScale(NULL, sx, sy, sx);
    njCnkCacheDrawModel(&e_bomb_big_model);
  } else {
    njScale(NULL, sx, sy, sx);
    njCnkCacheDrawModel(&e_bomb_model);
  }
  njPopMatrixEx();
  fn_80024CB8(lbl_803AD926);
}

static void EnemyBombTouchDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  Float sx = twp->scl.x;
  Float sy = twp->scl.y;

  fn_80024CB8(8);
  njSetTexture(&e_bomb_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njScale(NULL, sx, sy, sx);
  njCnkCacheDrawModel(&e_bomb_touch_model);
  njPopMatrixEx();
  fn_80024CB8(lbl_803AD926);
}

static void EnemyBombRingDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  Float sx = twp->scl.x;
  Float sy = twp->scl.y;

  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njScale(NULL, sx, sy, sx);
  o_ring_3();
  njPopMatrixEx();
}

static void EnemyBombCheckHit(taskwk *twp, enemywk *ewp) {
  colliwk *cwp = twp->cwp;
  BOOL hit = FALSE;

  if (twp->smode == ENEMYBOMB_TOUCH || twp->smode == ENEMYBOMB_PILLAR) {
    if (twp->flag & 0xD) {
      hit = TRUE;
    }
  } else if (twp->flag & 4) {
    hit = TRUE;
  }
  if (hit == TRUE) {
    SetExplode(twp, ewp);
  }

  // held by the parent and hit by another enemy
  if ((twp->flag & 0x8000) && (twp->flag & 8) &&
      (cwp->hit_cwp->id == CID_ENEMY || cwp->hit_cwp->id == CID_ENEMY2)) {
    SetExplosion(twp, ewp);
    twp->mode = MD_EXPLODE;
    twp->wtimer = 0;
  }
}

static void EnemyBombNormal(taskwk *twp, enemywk *ewp) {
  // unused apart from the rand() calls below
  NJS_POINT3 pos;
  NJS_VECTOR spd;
  NJS_VECTOR v;

  if (twp->wtimer > 20) {
    EnemyBombCheckHit(twp, ewp);
  }
  EnemyBombMove(twp, ewp);
  fn_80017C00(twp, ewp);
  fn_800169A4(twp, ewp);

  if (ewp->flag & 0x80) {
    if (twp->smode == ENEMYBOMB_TOUCH || twp->smode == ENEMYBOMB_PILLAR) {
      SetExplode(twp, ewp);
    } else if (fabsf(ewp->spd.y) > 0.3f) {
      ewp->unk_1C0 = 0.4f;
    }
  }

  if ((twp->smode == ENEMYBOMB_NORMAL || twp->smode == ENEMYBOMB_BIG) &&
      !lbl_801CC168._37) {
    twp->scl.z -= 0.003f;
    v.x = njRandom();
    v.y = njRandom();
    v.z = njRandom();
  }

  if (fabsf(ewp->spd.y) <= 0.1f && (ewp->flag & 0x80)) {
    twp->mode = MD_COUNT;
    twp->wtimer = 0;
    twp->btimer = 15;
  }
}

static void EnemyBombCountDisplayer(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&e_bomb_texlist);
  fn_8002B304();
  fn_8002B35C();
  if (twp->wtimer <= 2) {
    OffControl3D(0x204);
    OnControl3D(0x810);
    fn_8002B32C(0, 0x300);
    fn_800156FC(1.0f, 2.0f, -2.0f, -2.0f);
  }
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  if (twp->smode == ENEMYBOMB_BIG) {
    enemywk *ewp = (enemywk *)tp->mwp;

    fn_80073028(&e_bomb_big_model, &e_bomb_big_uvanim, ewp->timer);
    njScale(NULL, ewp->scl, ewp->scl, ewp->scl);
    njCnkCacheDrawModel(&e_bomb_big_model);
  } else {
    njCnkCacheDrawModel(&e_bomb_model);
  }
  njPopMatrixEx();
  if (twp->wtimer <= 2) {
    njEnableFog();
    gjSetFog();
  }
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  fn_8002B2F8();
  fn_8002B348();
}

static void EnemyBombPillarDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  NJS_VECTOR v; // unused
  NJS_VECTOR scl;
  NJS_POINT3 pos;

  pos = twp->pos;
  pos.y += 5.0f;
  scl.z = scl.x = twp->scl.x;
  scl.y = 10.0f;
  njPushMatrixEx();
  njTranslateEx(&pos);
  fn_8011610C(&scl);
  fn_8011E17C(lbl_801857F0[2]);
  njPopMatrixEx();
}

static void EnemyBombCount(taskwk *twp, enemywk *ewp) {
  EnemyBombCheckHit(twp, ewp);
  // keeps moving until it lies still on the ground
  if (!(njScalor(&ewp->spd) < 0.05f && (ewp->flag & 0x80)) &&
      !(twp->flag & 0x8000)) {
    EnemyBombMove(twp, ewp);
    fn_80017C00(twp, ewp);
  }

  // blink faster and faster
  if (twp->wtimer > twp->btimer) {
    twp->wtimer = 0;
    if (twp->btimer != 0 && !lbl_801CC168._37) {
      twp->btimer--;
    }
  }
  if (!lbl_801CC168._37) {
    twp->scl.z -= 0.003f;
  }
  if (twp->scl.z < 0.0f) {
    twp->scl.z = 0.0f;
  }

  if (twp->btimer < 1) {
    twp->mode = MD_EXPLODE;
    twp->wtimer = 0;
    SetExplosion(twp, ewp);
  }
}

static void EnemyBombExplode(taskwk *twp, enemywk *ewp) {
  CCL_INFO *info = &twp->cwp->info[1];

  if (lbl_801CC168._37) {
    return;
  }

  if (twp->smode == ENEMYBOMB_BIG && twp->scl.x < 37.5f) {
    twp->scl.x += 1.0f;
    info->a += 1.0f;
  } else if (twp->smode != ENEMYBOMB_BIG && twp->scl.x < 25.0f) {
    twp->scl.x += 1.0f;
    info->a += 1.0f;
  } else {
    twp->scl.y -= 0.05f;
    if (twp->scl.y < -1.0f) {
      twp->mode = MD_END;
      twp->wtimer = 0;
    }
  }
  twp->ang.y += 0x100;
}

static void EnemyBombPillarExplode(taskwk *twp, enemywk *ewp) {
  if (lbl_801CC168._37) {
    return;
  }

  if (twp->scl.x < 25.0f) {
    CCL_INFO *info = &twp->cwp->info[1];

    twp->scl.x += 1.0f;
    info[0].a += 1.0f;
    info[2].a += 1.0f;
  } else {
    twp->scl.y -= 0.05f;
    if (twp->scl.y < -1.0f) {
      twp->mode = MD_END;
      twp->wtimer = 0;
    }
  }
  twp->ang.y += 0x100;
}

static void EnemyBombDestructor(task *tp) {
  fn_80018B88(tp);
}

static void EnemyBombExecutor(task *tp) {
  taskwk *twp = tp->twp;
  enemywk *ewp = (enemywk *)tp->mwp;

  if (CheckRangeOut(tp)) {
    return;
  }

  // the enemy that threw it is being destroyed
  if (tp->ptp != NULL && tp->ptp->exec == DestroyTask) {
    FreeTask(tp);
    return;
  }

  switch (twp->mode) {
  case MD_NORMAL:
    EnemyBombNormal(twp, ewp);
    if (twp->mode == MD_COUNT) {
      tp->disp = EnemyBombCountDisplayer;
    } else if (twp->mode == MD_EXPLODE) {
      tp->disp = NULL;
    } else if (twp->mode == MD_PILLAR) {
      tp->disp = EnemyBombPillarDisplayer;
    }
    break;
  case MD_COUNT:
    EnemyBombCount(twp, ewp);
    if (twp->mode == MD_EXPLODE) {
      tp->disp = NULL;
    }
    break;
  case MD_EXPLODE:
    EnemyBombExplode(twp, ewp);
    if (CCL_IsHitPlayer(tp) != NULL) {
      tp->work.l |= 0x200;
      if (tp->ptp != NULL) {
        tp->ptp->twp->flag |= 0x200;
      }
    }
    if (twp->mode != MD_EXPLODE) {
      FreeTask(tp);
      return;
    }
    break;
  case MD_PILLAR:
    EnemyBombPillarExplode(twp, ewp);
    if (CCL_IsHitPlayer(tp) != NULL) {
      tp->work.l |= 0x200;
      if (tp->ptp != NULL) {
        tp->ptp->twp->flag |= 0x200;
      }
    }
    if (twp->mode != MD_PILLAR) {
      FreeTask(tp);
      return;
    }
    break;
  case MD_HOLD:
    if (!(twp->flag & 0x8000)) {
      twp->mode = MD_NORMAL;
    }
    if (twp->smode == ENEMYBOMB_BIG) {
      ewp->scl = fn_8002AE5C(ewp->scl, 2.0f, 0.2f);
    }
    twp->scl.x = fn_8002AE5C(twp->scl.x, 1.0f, 0.1f);
    twp->scl.y = fn_8002AE5C(twp->scl.y, 1.0f, 0.1f);
    return;
  }

  if (!lbl_801CC168._37 && twp->smode == ENEMYBOMB_BIG) {
    ewp->timer++;
    ewp->scl = fn_8002AE5C(ewp->scl, 2.0f, 0.2f);
    twp->cwp->info[2].a = fn_8002AE5C(twp->cwp->info[2].a, 9.0f, 0.45f);
    twp->cwp->info[2].b = fn_8002AE5C(twp->cwp->info[2].b, 10.0f, 0.5f);
  }
  if (twp->flag & 0x8000) {
    return;
  }

  CCL_Entry(tp);
  if (!lbl_801CC168._37) {
    twp->wtimer++;
  }
}

static void EnemyBombInit(task *tp, Sint32 type) {
  taskwk *twp = tp->twp;
  enemywk *ewp = NULL;
  Sint32 pno;
  NJS_VECTOR spd;

  if (twp->smode == ENEMYBOMB_BIG) {
    CCL_Init(tp, e_bomb_big_colli_info, ARYLEN(e_bomb_big_colli_info),
             CID_ENEMY2);
  } else if (twp->smode == ENEMYBOMB_PILLAR) {
    CCL_Init(tp, e_bomb_pillar_colli_info, ARYLEN(e_bomb_pillar_colli_info),
             CID_ENEMY2);
  } else if (twp->smode != ENEMYBOMB_RING) {
    CCL_Init(tp, e_bomb_colli_info, ARYLEN(e_bomb_colli_info), CID_ENEMY2);
  }
  if (tp->twp->cwp != NULL) {
    tp->twp->cwp->flag &= ~0x40;
    fn_800068BC(tp, 40.0f);
  }

  if (twp->smode != ENEMYBOMB_RING) {
    ewp = fn_80018D28(tp);
    if (twp->smode == ENEMYBOMB_BIG) {
      ewp->unk_EC = 14.0f;
      ewp->unk_F0 = 1.0f;
    } else {
      ewp->unk_EC = 2.0f;
      ewp->unk_F0 = 1.0f;
    }
  }

  if (type == ENEMYBOMB_THROW) {
    pno = fn_80037C84(&twp->pos);
    if (pno != -1) {
      twp->ang.y = RadAng(atan2f(playertwp[pno]->pos.x - twp->pos.x,
                                 playertwp[pno]->pos.z - twp->pos.z));
    }
    if (pno != -1) {
      spd.z = 0.013f * sqrtf(fn_800177EC(twp, pno));
    } else {
      spd.z = 1.2f;
    }
    spd.y = 2.0f;
    spd.x = 0.0f;
    if (spd.z > 1.2f) {
      spd.z = 1.2f;
    }
    njPushMatrix(&nj_unit_matrix);
    njRotateY(NULL, twp->ang.y);
    njCalcVector(NULL, &spd, &ewp->spd);
    njPopMatrixEx();
  } else if (ewp != NULL) {
    ewp->spd.x = 0.0f;
    ewp->spd.y = 0.0f;
    ewp->spd.z = 0.0f;
    fn_8006B7EC(0x4004, NULL, 0, 0, &twp->pos);
  }

  if (twp->smode == ENEMYBOMB_NORMAL) {
    ObjectMovableInitialize(twp, tp->mwp, 10);
    twp->flag &= ~0x8000;
  } else if (twp->smode == ENEMYBOMB_BIG) {
    ObjectMovableInitialize(twp, tp->mwp, 12);
    twp->flag &= ~0x8000;
  }
  twp->wtimer = 0;
  tp->work.l = 0;
  if (twp->smode == ENEMYBOMB_BIG) {
    ewp->scl = 0.5f;
  } else if (twp->smode != ENEMYBOMB_RING) {
    ewp->scl = 1.0f;
  }
  twp->ang.x = NJM_DEG_ANG(njRandom() * 360.0f);
  twp->ang.z = NJM_DEG_ANG(njRandom() * 360.0f);

  if (twp->smode != ENEMYBOMB_RING) {
    ewp->unk_1C8 = 0xE0;
    ewp->unk_1D0 = twp->ang.y;
    ewp->unk_1E0 = 0.0f;
    ewp->colli_center.x = 0.0f;
    ewp->colli_center.y = 0.0f;
    ewp->colli_center.z = 0.0f;
    if (twp->smode == ENEMYBOMB_BIG) {
      ewp->colli_top = 12.0f;
      ewp->colli_radius = 10.0f;
      ewp->colli_bottom = -10.0f;
    } else {
      ewp->colli_top = 3.0f;
      ewp->colli_radius = 3.0f;
      ewp->colli_bottom = -3.0f;
    }
    ewp->unk_D8 = 0.7f;
    ewp->unk_DC = 0.5f;
    ewp->unk_E0 = 0.5f;
    ewp->unk_E4 = 0.7f;
    ewp->unk_54 = 2.0f;
    twp->scl.z = 1.0f;
    fn_80017BE4(twp, ewp);
  }

  if (type == ENEMYBOMB_HOLD) {
    twp->mode = MD_HOLD;
    twp->flag |= 0x8000;
    twp->scl.x = 0.0f;
    twp->scl.y = 0.0f;
  } else {
    twp->mode = MD_NORMAL;
  }

  if (twp->smode != ENEMYBOMB_RING) {
    tp->dest = EnemyBombDestructor;
    if (twp->smode == ENEMYBOMB_NORMAL || twp->smode == ENEMYBOMB_BIG) {
      tp->disp = EnemyBombDisplayer;
    } else {
      tp->disp = EnemyBombTouchDisplayer;
    }
  } else {
    tp->disp = EnemyBombRingDisplayer;
  }

  if (type == ENEMYBOMB_EXPLODE) {
    SetExplosion(twp, ewp);
    if (twp->smode == ENEMYBOMB_PILLAR) {
      twp->cwp->info[3].attr &= ~0x10;
      twp->mode = MD_PILLAR;
      tp->disp = EnemyBombPillarDisplayer;
    } else {
      twp->mode = MD_EXPLODE;
      tp->disp = NULL;
    }
    twp->wtimer = 0;
  }
}

static void ManTexDestructor(task *tp) {
  fn_801166F0(&e_bomb_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

void EnemyBombLoadTexture(void) {
  if (mantex_tp == NULL) {
    fn_80072820("E_BOMTEX", &e_bomb_texlist);
    mantex_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
    mantex_tp->dest = ManTexDestructor;
  }
  LoadExplosionTexture();
}

task *CreateEnemyBomb(task *ptp, Sint32 kind, Sint32 type, NJS_POINT3 *pos) {
  task *tp;
  taskwk *twp;
  taskwk *ptwp;

  if (kind == ENEMYBOMB_RING) {
    if (njRandom() >= 0.95f) {
      if (type != ENEMYBOMB_HOLD) {
        tp = CreateElementalTask(10, LEV_2, Tobitiri, "Tobitiri");
        if (tp != NULL) {
          twp = tp->twp;
          twp->pos = *pos;
          twp->pos.y -= 10.0f;
          lbl_801CC168._6E++;
          lbl_803AD860->_10++;
          ptwp = playertwp[fn_80037C84(pos)];
          if (ptwp != NULL) {
            twp->ang.y =
                RadAng(atan2f(ptwp->pos.x - pos->x, ptwp->pos.z - pos->z));
          }
        }
        return tp;
      }
    } else {
      kind = ENEMYBOMB_NORMAL;
    }
  }

  if (ptp != NULL) {
    tp = CreateChildTask(IM_TWK, EnemyBombExecutor, ptp);
  } else {
    tp = CreateFundamentalTask(IM_TWK, LEV_3, EnemyBombExecutor);
  }
  if (tp != NULL) {
    tp->twp->smode = kind;
    tp->twp->pos = *pos;
    EnemyBombInit(tp, type);
    EnemyBombLoadTexture();
  }
  return tp;
}
