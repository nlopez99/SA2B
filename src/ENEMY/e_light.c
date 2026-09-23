#include "ENEMY/e_light.h"

#include "samt/ninja/njdraw.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/player.h"
#include "set.h"

// per-screen camera work
typedef struct camwk {
  /* 0x00 */ Uint8 unk_00[0xC];
  /* 0x0C */ Angle3 ang;
} camwk;

extern Sint32 lbl_803ADAD0; // screen being drawn
extern camwk *lbl_80175378[4];

extern void gjSetFog(void);
extern void njDisableFog(void);
extern void njEnableFog(void);
extern void __njColorBlendingMode(Int, Int);
extern void njDrawTexture3DEx(NJS_TEXTURE_VTX *polygon, Int count, Int trans);
extern void fn_800156FC(Float, Float, Float, Float);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_8011ED60(NJS_SPRITE *sp, Int n, Uint32 attr);
extern void fn_80118B5C(NJS_TEXLIST *texlist);

// ^ extern
// v in this file

static void EnemyLightDisplayer(task *tp);
static void ManTex(task *tp);

#define LIGHTPTCL_MAX 512

// one entry per light drawn this frame; the low byte of col is the kind
typedef struct lightptcl // sizeof=0x10
{
  /* 0x00 */ Uint32 col;
  /* 0x04 */ NJS_POINT3 pos;
} lightptcl;

static NJS_TEXNAME e_light_texname[] = {
    {"st_c20"},
};

static NJS_TEXLIST e_light_texlist = {
    e_light_texname,
    ARYLEN(e_light_texname),
};

// particle pass shared by every light
static task *lightptcl_tp;
static Uint32 lightptcl_num;
static lightptcl lightptcl_buf[LIGHTPTCL_MAX];

// task that owns the texture
static task *mantex_tp;

static void EnemyLightPtclDraw(task *tp) {
  lightptcl *p;
  Uint32 col;
  Float scl;
  Sint32 num;

  p = lightptcl_buf;
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 10);
  njSetTexture(&e_light_texlist);
  njDisableFog();
  gjSetFog();

  while (lightptcl_num != 0) {
    NJS_TEXTURE_VTX vtx[4] = {
        {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF},
        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF},
        {1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF},
        {1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF},
    };

    col = p->col;
    switch (col & 0xFF) {
    case 1:
      num = 0;
      scl = 2.08f;
      break;
    case 2:
      num = 0;
      scl = 2.08f;
      break;
    case 3:
      num = 0;
      scl = 0.96f;
      break;
    case 4:
      num = 0;
      scl = 3.2f;
      break;
    case 0:
    default:
      num = 0;
      scl = 2.4f;
      break;
    }
    njSetTextureNum(num);

    njPushMatrixEx();
    njTranslateEx(&p->pos);
    njScale(NULL, scl, scl, scl);
    njRotateEx(&lbl_80175378[lbl_803ADAD0]->ang, 0);
    vtx[3].col = vtx[2].col = vtx[1].col = vtx[0].col =
        (col & ~0xFFFFFFU) | 0xFFFFFF;
    njDrawTexture3DEx(vtx, 4, TRUE);
    njPopMatrixEx();

    p++;
    lightptcl_num--;
  }

  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  njEnableFog();
  gjSetFog();
}

static void EnemyLightPtclDest(task *tp) {
  lightptcl_tp = NULL;
}

static void EnemyLightPtclDrawExec(task *tp) {}

static Sint32 EnemyLightPtclEntry(NJS_POINT3 *pos, Sint32 kind, Uint32 col) {
  if (lightptcl_tp == NULL) {
    task *ntp;

    ntp = CreateFundamentalTask(IM_NONE, LEV_5, EnemyLightPtclDrawExec);
    lightptcl_tp = ntp;
    if (ntp == NULL) {
      return FALSE;
    }
    ntp->disp = EnemyLightPtclDraw;
    ntp->dest = EnemyLightPtclDest;
  }

  if (lightptcl_num < LIGHTPTCL_MAX) {
    lightptcl_buf[lightptcl_num].col = (col & ~0xFFU) | (Uint8)kind;
    lightptcl_buf[lightptcl_num].pos = *pos;
    lightptcl_num++;
    return TRUE;
  }
  return FALSE;
}

static NJS_TEXANIM e_light_texanim[] = {
    {16, 16, 8, 8, 0, 0, 256, 256, 0, 0},
};

static NJS_SPRITE e_light_sprite = {
    {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0, &e_light_texlist, e_light_texanim,
};

static void EnemyLightDisplayer(task *tp) {
  taskwk *twp = tp->twp;
  Float scl = twp->scl.z;

  if (EnemyLightPtclEntry(&twp->pos, twp->smode,
                          (Uint32)(255.0f * twp->scl.x) << 24) == TRUE) {
    return;
  }

  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 10);
  fn_800156FC(twp->scl.x, 1.0f, 1.0f, 1.0f);
  njSetTexture(&e_light_texlist);
  njDisableFog();
  gjSetFog();

  e_light_sprite.sx = e_light_sprite.sy = scl;
  e_light_sprite.p = twp->pos;
  fn_8011ED60(&e_light_sprite, 0,
              NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);

  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  njEnableFog();
  gjSetFog();
}

static void EnemyLightDestructor(task *tp) {}

static void EnemyLightExecutor(task *tp) {
  taskwk *twp = tp->twp;

  if (tp->ptp == NULL && CheckRangeOut(tp)) {
    return;
  }
  if (lbl_801CC168._37) {
    return;
  }
  if (twp->scl.x <= 0.0f) {
    FreeTask(tp);
    return;
  }

  twp->scl.x -= twp->scl.y;
  if (twp->scl.x < 0.0f) {
    twp->scl.x = 0.0f;
  }
  twp->wtimer++;
}

static void EnemyLightInit(task *tp) {
  taskwk *twp = tp->twp;

  twp->scl.x = 1.0f;
  switch (twp->smode) {
  case 1:
    twp->scl.y = 0.05f;
    twp->scl.z = 0.26f;
    break;
  case 2:
    twp->scl.y = 0.3f;
    twp->scl.z = 0.26f;
    break;
  case 3:
    twp->scl.y = 0.05f;
    twp->scl.z = 0.12f;
    break;
  case 4:
    twp->scl.y = 0.05f;
    twp->scl.z = 0.4f;
    break;
  case 0:
  default:
    twp->scl.y = 0.1f;
    twp->scl.z = 0.3f;
    break;
  }
  tp->dest = EnemyLightDestructor;
  tp->disp = EnemyLightDisplayer;
}

static void ManTexDestructor(task *tp) {
  fn_801166F0(&e_light_texlist);
  mantex_tp = NULL;
}

static void ManTex(task *tp) {}

void EnemyLightLoadTexture(void) {
  if (mantex_tp != NULL) {
    return;
  }

  fn_80118B5C(&e_light_texlist);
  mantex_tp = CreateFundamentalTask(IM_NONE, LEV_0, ManTex);
  mantex_tp->dest = ManTexDestructor;
}

void CreateEnemyLight(task *ptp, Sint32 smode, NJS_POINT3 *pos) {
  task *tp;

  if (ptp != NULL) {
    tp = CreateChildTask(IM_TWK, EnemyLightExecutor, ptp);
  } else {
    tp = CreateFundamentalTask(IM_TWK, LEV_3, EnemyLightExecutor);
  }
  if (tp == NULL) {
    return;
  }

  tp->twp->smode = smode;
  tp->twp->pos = *pos;
  EnemyLightInit(tp);
  EnemyLightLoadTexture();
}
