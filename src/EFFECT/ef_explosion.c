#include "EFFECT/ef_explosion.h"

#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"

extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void fn_800156FC(Float, Float, Float, Float);
extern Float fn_8002AE5C(Float val, Float target, Float step);
extern void fn_80072820(const char *name, NJS_TEXLIST *texlist);
extern void fn_80073028(NJS_CNK_MODEL *model, void *info, Uint32 frame);
extern void fn_801166F0(NJS_TEXLIST *texlist);

extern void CreateDustRing(NJS_POINT3 *pos, NJS_VECTOR *spd, Float r, Float scl,
                           Sint32 num);

extern NJS_TEXLIST   _rename_explosion_texlist;
extern NJS_CNK_MODEL _rename_explosion_model_0;
extern NJS_CNK_MODEL _rename_explosion_model_1;
extern NJS_CNK_MODEL _rename_explosion_model_2;
extern Sint16        _rename_explosion_uv[];

// ^ extern
// v in this file

static void ExplosionDisp(task *tp);
static void ExplosionDest(task *tp);
static void ExplosionExec(task *tp);
static void ManTex(task *tp);
static void ManTexDest(task *tp);

enum {
  MD_NORMAL,
  MD_END,
};

#define LAYER_NUM 3

typedef struct explosionwk // sizeof=0x4C
{
  /* 0x00 */ Sint8 mode;
  /* 0x01 */ Sint8 kind;
  /* 0x02 */ Sint16 timer;
  /* 0x04 */ NJS_POINT3 pos;
  /* 0x10 */ Float scl[5]; // one per layer, only LAYER_NUM are used
  /* 0x24 */ Float spd[5];
  /* 0x38 */ Float alpha[5];
} explosionwk;

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

#define GetWork(task) ((explosionwk *)task->awp)

static task *mantex_tp;

uvanim_info explosion_uvanim = {2, 5, 1, NULL, _rename_explosion_uv};

static task *ExplosionCreateTask(task_exec exec) {
  task *tp = CreateFundamentalTask(IM_NONE, LEV_3, exec);
  explosionwk *wk;

  if (tp == NULL) {
    return tp;
  }

  wk = syCalloc(1, sizeof(explosionwk));
  if (wk == NULL) {
    DestroyTask(tp);
    return NULL;
  }

  tp->awp = (anywk *)wk;
  return tp;
}

static NJS_CNK_MODEL *explosion_models[LAYER_NUM] = {
    &_rename_explosion_model_0,
    &_rename_explosion_model_1,
    &_rename_explosion_model_2,
};

static void ExplosionDisp(task *tp) {
  explosionwk *wk = GetWork(tp);
  Sint32 i;
  Float scl;

  fn_8002B304();
  fn_8002B35C();
  OffControl3D(0x204);
  OnControl3D(0x10);
  njSetTexture(&_rename_explosion_texlist);
  njPushMatrixEx();
  njTranslateEx(&wk->pos);
  fn_80073028(&_rename_explosion_model_1, &explosion_uvanim, wk->timer);
  for (i = 0; i < LAYER_NUM; i++) {
    if (!(wk->scl[i] > 0.0f && wk->alpha[i] > 0.0f)) {
      continue;
    }

    fn_800156FC(wk->alpha[i], 1.0f, 1.0f, 1.0f);
    njPushMatrixEx();
    scl = wk->scl[i];
    if (i == 2) {
      // the last layer is never raised
    } else if (wk->kind == EXPLOSION_L_GROUND ||
               wk->kind == EXPLOSION_LL_GROUND) {
      njTranslate(NULL, 0.0f, 10.0f, 0.0f);
    } else if (wk->kind == EXPLOSION_M_GROUND) {
      njTranslate(NULL, 0.0f, 5.0f, 0.0f);
    } else if (wk->kind == EXPLOSION_S_GROUND ||
               wk->kind == EXPLOSION_SS_GROUND) {
      njTranslate(NULL, 0.0f, 3.0f, 0.0f);
    }
    if (i == 1) {
      njRotateX(NULL, 0xF000);
    }
    njScale(NULL, scl, scl, scl);
    njCnkCacheDrawModel(explosion_models[i]);
    njPopMatrixEx();
  }
  njPopMatrixEx();
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  fn_8002B2F8();
  fn_8002B348();
}

// layer n grows from scl0 by spd0 a frame, braking towards spd_min, and fades
// out after fade_time
#define ExplosionLayer(wk, n, scl0, spd0, alpha0, spd_min, brake, fade_time,   \
                       fade)                                                   \
  {                                                                            \
    if (wk->timer == 0) {                                                      \
      wk->scl[n] = scl0;                                                       \
      wk->spd[n] = spd0;                                                       \
      wk->alpha[n] = alpha0;                                                   \
    }                                                                          \
    wk->spd[n] = fn_8002AE5C(wk->spd[n], spd_min, brake * (Float)wk->timer);   \
    wk->scl[n] += wk->spd[n];                                                  \
    if (wk->timer > fade_time) {                                               \
      wk->alpha[n] -= fade;                                                    \
    }                                                                          \
  }

#define ExplosionDustRing(wk, up, r, scl, num)                                 \
  {                                                                            \
    NJS_VECTOR spd;                                                            \
    spd.x = 0.0f;                                                              \
    spd.y = up;                                                                \
    spd.z = 0.0f;                                                              \
    CreateDustRing(&wk->pos, &spd, r, scl, num);                               \
  }

static void ExplosionExecS(task *tp) {
  explosionwk *wk = GetWork(tp);

  if (wk->timer > 60) {
    wk->mode = MD_END;
    return;
  }

  ExplosionLayer(wk, 0, 0.5f, 0.3f, 0.7f, 0.02f, 0.015f, 5, 0.1f);
  ExplosionLayer(wk, 1, 0.0f, 0.2f, 0.6f, 0.02f, 0.012f, 20, 0.022f);
  ExplosionLayer(wk, 2, 0.4f, 0.25f, 0.7f, 0.015f, 0.01f, 0, 0.035f);

  if (wk->timer == 6) {
    ExplosionDustRing(wk, 0.1f, 3.0f, 3.5f, 3);
  } else if (wk->timer == 30) {
    ExplosionDustRing(wk, 0.15f, 4.0f, 5.0f, 4);
  }
}

static void ExplosionExecSGround(task *tp) { ExplosionExecS(tp); }

static void ExplosionExecM(task *tp) {
  explosionwk *wk = GetWork(tp);

  if (wk->timer > 60) {
    wk->mode = MD_END;
    return;
  }

  ExplosionLayer(wk, 0, 1.0f, 0.4f, 0.7f, 0.04f, 0.01f, 5, 0.1f);
  ExplosionLayer(wk, 1, 0.65f, 0.28f, 0.6f, 0.025f, 0.004f, 20, 0.022f);
  ExplosionLayer(wk, 2, 0.5f, 0.25f, 0.7f, 0.015f, 0.01f, 0, 0.035f);

  if (wk->timer == 30) {
    ExplosionDustRing(wk, 0.2f, 14.0f, 5.5f, 9);
  }
}

static void ExplosionExecMGround(task *tp) { ExplosionExecM(tp); }

static void ExplosionExecSS(task *tp) {
  explosionwk *wk = GetWork(tp);

  if (wk->timer > 45) {
    wk->mode = MD_END;
    return;
  }

  ExplosionLayer(wk, 0, 0.5f, 0.3f, 0.7f, 0.02f, 0.015f, 5, 0.1f);
  ExplosionLayer(wk, 1, 0.0f, 0.1f, 0.5f, 0.015f, 0.005f, 20, 0.025f);
  ExplosionLayer(wk, 2, 0.4f, 0.25f, 0.7f, 0.015f, 0.01f, 0, 0.035f);
}

static void ExplosionExecSSGround(task *tp) { ExplosionExecSS(tp); }

static void ExplosionExecL(task *tp) {
  explosionwk *wk = GetWork(tp);

  if (wk->timer > 180) {
    wk->mode = MD_END;
    return;
  }

  ExplosionLayer(wk, 0, 1.5f, 0.5f, 0.7f, 0.05f, 0.012f, 10, 0.05f);
  ExplosionLayer(wk, 1, 0.0f, 0.5f, 0.7f, 0.024f, 0.01f, 30, 0.01f);
  ExplosionLayer(wk, 2, 1.0f, 0.4f, 0.7f, 0.04f, 0.003f, 0, 0.02f);

  if (wk->timer == 10) {
    ExplosionDustRing(wk, 0.15f, 10.0f, 8.0f, 3);
  } else if (wk->timer == 30) {
    ExplosionDustRing(wk, 0.2f, 13.0f, 10.0f, 4);
  }
  if (wk->timer == 50) {
    ExplosionDustRing(wk, 0.25f, 14.0f, 12.0f, 5);
  }
}

static void ExplosionExecLGround(task *tp) { ExplosionExecL(tp); }

static void ExplosionExecLL(task *tp) {
  explosionwk *wk = GetWork(tp);

  if (wk->timer > 180) {
    wk->mode = MD_END;
    return;
  }

  ExplosionLayer(wk, 0, 1.5f, 0.6f, 0.7f, 0.05f, 0.012f, 12, 0.05f);
  ExplosionLayer(wk, 1, 0.0f, 0.6f, 0.7f, 0.04f, 0.0025f, 30, 0.007f);
  ExplosionLayer(wk, 2, 1.0f, 0.5f, 0.7f, 0.05f, 0.003f, 0, 0.014f);

  if (wk->timer == 10) {
    ExplosionDustRing(wk, 0.15f, 11.0f, 10.0f, 3);
  } else if (wk->timer == 30) {
    ExplosionDustRing(wk, 0.2f, 15.0f, 12.0f, 4);
  }
  if (wk->timer == 50) {
    ExplosionDustRing(wk, 0.25f, 18.0f, 14.0f, 5);
  }
  if (wk->timer == 70) {
    ExplosionDustRing(wk, 0.3f, 20.0f, 15.0f, 7);
  }
}

static void ExplosionExecLLGround(task *tp) { ExplosionExecLL(tp); }

static void ExplosionDest(task *tp) {}

static void ExplosionExec(task *tp) {
  explosionwk *wk = GetWork(tp);

  if (lbl_801CC168._37 != 0) {
    return;
  }

  switch (wk->mode) {
  case MD_NORMAL:
    switch (wk->kind) {
    case EXPLOSION_L:
      ExplosionExecL(tp);
      break;
    case EXPLOSION_L_GROUND:
      ExplosionExecLGround(tp);
      break;
    case EXPLOSION_LL_GROUND:
      ExplosionExecLLGround(tp);
      break;
    case EXPLOSION_LL:
      ExplosionExecLL(tp);
      break;
    case EXPLOSION_S:
      ExplosionExecS(tp);
      break;
    case EXPLOSION_S_GROUND:
      ExplosionExecSGround(tp);
      break;
    case EXPLOSION_M:
      ExplosionExecM(tp);
      break;
    case EXPLOSION_M_GROUND:
      ExplosionExecMGround(tp);
      break;
    case EXPLOSION_SS:
      ExplosionExecSS(tp);
      break;
    case EXPLOSION_SS_GROUND:
      ExplosionExecSSGround(tp);
      break;
    }
    break;
  case MD_END:
  default:
    FreeTask(tp);
    break;
  }

  wk->timer++;
}

static void ExplosionInit(task *tp) {
  explosionwk *wk = GetWork(tp);

  wk->alpha[0] = 1.0f;
  wk->scl[0] = 0.0f;
  wk->spd[0] = 0.34f;
  tp->dest = ExplosionDest;
  tp->disp = ExplosionDisp;
}

static void ManTexDest(task *tp) {
  fn_801166F0(&_rename_explosion_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

// loads the textures once; ManTexDest releases them
void LoadExplosionTexture(void) {
  if (mantex_tp != NULL) {
    return;
  }

  fn_80072820("E_EXPTEX", &_rename_explosion_texlist);
  mantex_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
  mantex_tp->dest = ManTexDest;
}

void CreateExplosion(Sint32 kind, NJS_POINT3 *pos) {
  task *tp = ExplosionCreateTask(ExplosionExec);
  explosionwk *wk;

  if (tp == NULL) {
    return;
  }

  wk = GetWork(tp);
  wk->kind = kind;
  wk->pos = *pos;
  ExplosionInit(tp);
  LoadExplosionTexture();
}
