#include "OBJECT/o_itemboxballoon.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/sound.h"
#include "set.h"
#include "stl/stdlib.h"

extern BOOL _rename_CheckFlag0x20(task *tp);
extern void _rename_SetFlag0x20(task *tp);
extern void _rename_ItemGetDisplaySet(Sint32 pno, Sint32 kind);
extern void _rename_ItemBombSet(NJS_POINT3 *pos);
extern void *CreateSnowPuff(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern void fn_800373B8(Sint32 pno); // invincibility
extern void fn_800374E0(Sint32 pno); // barrier
extern void fn_800375FC(Sint32 pno); // magnetic barrier
extern void fn_80037718(Sint32 pno); // speed up
extern void fn_800641B8(Sint32 pno, Sint32 num);
extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_801218C8(Sint32, Sint32);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern void AddScore(int);
extern void AddNumRing(int, s16);
extern void AddMechHP(s32, f32);

extern BOOL DisableObjectFog;

// ^ extern
// v in this file

enum {
  MD_INIT,
  MD_APPEAR,
  MD_NORMAL,
  MD_BROKEN,
  MD_BURST,
  MD_COLLI_ONLY,
  MD_RESET,
};

// item kinds as _rename_ItemGetDisplaySet takes them
enum {
  ITEM_KIND_NOTHING = 7,
  ITEM_KIND_EXTRALIFE = 10,
};

typedef struct {
  /* 0x00 */ Sint32 kind;
  /* 0x04 */ void (*func)(task *tp, Sint32 pno);
} BALLOON_ITEM_INFO;

static void ObjectItemBoxBalloonInit(task *tp);
static void ObjectItemBoxBalloonAppear(task *tp);
static void ObjectItemBoxBalloonNormal(task *tp);
static void ObjectItemBoxBalloonBroken(task *tp);
static void ObjectItemBoxBalloonBurst(task *tp);
static void ObjectItemBoxBalloonDisp(task *tp);
static void ObjectItemBoxBalloonDispSort(task *tp);
static void ObjectItemBoxBalloonDest(task *tp);
static void ItemBoxBalloon10Ring(task *tp, Sint32 pno);
static void ItemBoxBalloonExtraLife(task *tp, Sint32 pno);
static void ItemBoxBalloon5Ring(task *tp, Sint32 pno);
static void ItemBoxBalloonSpeedUp(task *tp, Sint32 pno);
static void ItemBoxBalloon20Ring(task *tp, Sint32 pno);
static void ItemBoxBalloonBarrier(task *tp, Sint32 pno);
static void ItemBoxBalloonBomb(task *tp, Sint32 pno);
static void ItemBoxBalloonHealth(task *tp, Sint32 pno);
static void ItemBoxBalloonMagneticBarrier(task *tp, Sint32 pno);
static void ItemBoxBalloonNothing(task *tp, Sint32 pno);
static void ItemBoxBalloonInvincible(task *tp, Sint32 pno);

static NJS_TEXNAME itemboxballoon_texname[] = {
    {"sikake_01_128"},
};

NJS_TEXLIST itemboxballoon_texlist = {
    itemboxballoon_texname,
    ARYLEN(itemboxballoon_texname),
};

static Sint16 itemboxballoon_plist[] = {
#include "assets/itemboxballoon_plist.inc"
};

static Sint32 itemboxballoon_vlist[] = {
#include "assets/itemboxballoon_vlist.inc"
};

static NJS_CNK_MODEL itemboxballoon_model = {
    itemboxballoon_vlist,
    itemboxballoon_plist,
    {0.0f, -4.316724f, 0.0f},
    37.561543f,
};

static CCL_INFO itemboxballoon_colli_info[1] = {
    {0, CI_FORM_SPHERE, (Sint8)0xB0, (Sint8)0xE0, 0x00800000,
     {0.0f, -4.13f, 0.0f}, 38.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
};

static BALLOON_ITEM_INFO itemboxballoon_item_info[11] = {
    {8, ItemBoxBalloonSpeedUp},
    {2, ItemBoxBalloon5Ring},
    {ITEM_KIND_EXTRALIFE, ItemBoxBalloonExtraLife},
    {0, ItemBoxBalloon10Ring},
    {1, ItemBoxBalloon20Ring},
    {3, ItemBoxBalloonBarrier},
    {4, ItemBoxBalloonBomb},
    {5, ItemBoxBalloonHealth},
    {6, ItemBoxBalloonMagneticBarrier},
    {ITEM_KIND_NOTHING, ItemBoxBalloonNothing},
    {9, ItemBoxBalloonInvincible},
};

// twp->scl.x: item index from the set file, twp->scl.z: balloon scale
#define GetItem(task) (*(Sint32 *)&task->fwp)

void ObjectItemBoxBalloon(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOut(tp)) {
    return;
  }

  if (twp->mode == MD_INIT || twp->mode == MD_APPEAR ||
      twp->mode == MD_NORMAL) {
    tp->disp_sort = ObjectItemBoxBalloonDispSort;
    tp->disp = ObjectItemBoxBalloonDisp;
  }

  switch (twp->mode) {
  case MD_INIT:
    tp->dest = ObjectItemBoxBalloonDest;
    tp->disp_sort = ObjectItemBoxBalloonDispSort;
    tp->disp = ObjectItemBoxBalloonDisp;
    ObjectItemBoxBalloonInit(tp);
    break;
  case MD_APPEAR:
    ObjectItemBoxBalloonAppear(tp);
    break;
  case MD_NORMAL:
    ObjectItemBoxBalloonNormal(tp);
    break;
  case MD_BROKEN:
    ObjectItemBoxBalloonBroken(tp);
    break;
  case MD_BURST:
    ObjectItemBoxBalloonBurst(tp);
    break;
  case MD_COLLI_ONLY:
    CCL_Entry(tp);
    break;
  case MD_RESET:
    ObjectItemBoxBalloonDest(tp);
  default:
    twp->mode = MD_INIT;
    break;
  }

  if (!lbl_801CC168._37) {
    twp->wtimer++;
  }
}

static void ObjectItemBoxBalloonInit(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 i;

  twp->mode = MD_APPEAR;
  twp->scl.z = 0.0f;
  twp->scl.y = 0.0f;
  GetItem(tp) = twp->scl.x;
  if (GetItem(tp) >= (Sint32)ARYLEN(itemboxballoon_item_info)) {
    GetItem(tp) = ARYLEN(itemboxballoon_item_info) - 1;
  } else if (GetItem(tp) < 0) {
    GetItem(tp) = 0;
  }

  // an extra life that was already taken turns into an empty balloon
  if (tp->ocp != NULL && !CheckBroken(tp) && _rename_CheckFlag0x20(tp) &&
      itemboxballoon_item_info[GetItem(tp)].kind == ITEM_KIND_EXTRALIFE) {
    for (i = 0; i < (Sint32)ARYLEN(itemboxballoon_item_info); i++) {
      if (itemboxballoon_item_info[i].kind == ITEM_KIND_NOTHING) {
        GetItem(tp) = i;
        break;
      }
    }
  }

  CCL_Init(tp, itemboxballoon_colli_info, ARYLEN(itemboxballoon_colli_info),
           CID_ENEMY);
  if (tp->ocp != NULL && CheckBroken(tp)) {
    twp->mode = MD_BROKEN;
  }
  twp->wtimer = 0;
}

static void ObjectItemBoxBalloonAppear(task *tp) {
  taskwk *twp = tp->twp;
  twp->scl.z += 0.11f;
  if (twp->scl.z >= 1.0f) {
    twp->mode = MD_NORMAL;
    twp->scl.z = 1.0f;
  }
  twp->scl.y += 5.0f;
}

static void ObjectItemBoxBalloonNormal(task *tp) {
  taskwk *twp = tp->twp;
  colliwk *cwp = twp->cwp;
  Sint32 pno;

  if ((cwp->flag & 1) &&
      (cwp->hit_cwp->id == CID_PLAYER || cwp->hit_cwp->id == CID_BULLET) &&
      (pno = IsThisTaskPlayer(cwp->hit_cwp->mytask)) != -1) {
    NJS_VECTOR spd;

    if (!(playerpwp[pno]->item & 0x4000)) {
      _rename_ItemGetDisplaySet(pno,
                                itemboxballoon_item_info[GetItem(tp)].kind);
      itemboxballoon_item_info[GetItem(tp)].func(tp, pno);
      if (!(playertwp[pno]->flag & 1)) {
        SetVelocityP(pno, 0.0f, 2.0f, 0.0f);
      }
    }
    twp->mode = MD_BROKEN;
    spd.x = 0.0f;
    spd.y = 0.3f;
    spd.z = 0.0f;
    CreateSnowPuff(&twp->pos, &spd, 26.0f);
    SE_Call(0x800D, NULL, 0, 0);
    fn_8002FB2C(pno, 1, 48, 0);
    if (tp->ocp != NULL) {
      SetBroken(tp);
    }
  }

  CCL_Entry(tp);
  twp->scl.y += 5.0f;
}

static void ObjectItemBoxBalloonBroken(task *tp) {
  tp->twp->mode = MD_BURST;
  SE_Call(0x1006, NULL, 0, 0);
}

static void ObjectItemBoxBalloonBurst(task *tp) {
  taskwk *twp = tp->twp;
  twp->scl.z += 0.05f;
  if (twp->scl.z > 3.0f) {
    twp->scl.z = 3.0f;
    if (tp->ocp != NULL) {
      DeadOut(tp);
    } else {
      FreeTask(tp);
    }
  }
}

// opaque until the balloon bursts
static void ObjectItemBoxBalloonDisp(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->scl.z > 1.0f) {
    return;
  }

  njSetTexture(&itemboxballoon_texlist);
  if (twp->mode == MD_BURST || DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  fn_8002B304();
  fn_8002B35C();
  OffControl3D(0x220);
  OnControl3D(0x810);
  fn_801218C8(~0x800, 0);
  fn_800156FC(1.0f, 1.0f, 1.0f, 1.0f);
  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x,
              twp->pos.y + 4.0f * njSin((twp->wtimer + 800) << 8), twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, NJM_DEG_ANG(3.5f * njSin(twp->wtimer * 0x280)));
  njScale(NULL, twp->scl.z + 0.01f * njCos(twp->wtimer * 0x900),
          twp->scl.z + 0.01f * njSin(twp->wtimer << 11),
          twp->scl.z + 0.01f * njCos((twp->wtimer + 0x1000) * 0x850));
  njCnkCacheDrawModel(&itemboxballoon_model);
  njPopMatrixEx();
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  fn_8002B348();
  fn_8002B2F8();
  if (twp->mode == MD_BURST || DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
}

// fades out while the burst balloon grows
static void ObjectItemBoxBalloonDispSort(task *tp) {
  taskwk *twp = tp->twp;
  Float alpha;

  if (twp->scl.z <= 1.0f) {
    return;
  }

  njSetTexture(&itemboxballoon_texlist);
  if (twp->mode == MD_BURST || DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  alpha = 1.0f - (twp->scl.z - 1.0f) / 2.0f;
  if (alpha > 1.0f) {
    alpha = 1.0f;
  }
  if (alpha < 0.0f) {
    alpha = 0.0f;
  }
  alpha *= 0.7f;
  fn_8002B304();
  fn_8002B35C();
  OffControl3D(0x220);
  OnControl3D(0x810);
  fn_801218C8(0xFF, 0x800);
  fn_800156FC(alpha, 1.0f, 1.0f, 1.0f);
  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x,
              twp->pos.y + 4.0f * njSin((twp->wtimer + 800) << 8), twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, NJM_DEG_ANG(3.5f * njSin(twp->wtimer * 0x280)));
  njScale(NULL, twp->scl.z + 0.01f * njCos(twp->wtimer * 0x900),
          twp->scl.z + 0.01f * njSin(twp->wtimer << 11),
          twp->scl.z + 0.01f * njCos((twp->wtimer + 0x1000) * 0x850));
  njCnkCacheDrawModel(&itemboxballoon_model);
  njPopMatrixEx();
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  fn_8002B348();
  fn_8002B2F8();
  if (twp->mode == MD_BURST || DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
}

static void ObjectItemBoxBalloonDest(task *tp) {
  GetItem(tp) = 0;
}

static void ItemBoxBalloon10Ring(task *tp, Sint32 pno) {
  AddScore(100);
  AddNumRing(pno, 10);
  AddMechHP(pno, 0.5f);
}

static void ItemBoxBalloonExtraLife(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800641B8(pno, 1);
  _rename_SetFlag0x20(tp);
}

static void ItemBoxBalloon5Ring(task *tp, Sint32 pno) {
  AddScore(50);
  AddNumRing(pno, 5);
  AddMechHP(pno, 0.25f);
}

static void ItemBoxBalloonSpeedUp(task *tp, Sint32 pno) {
  AddScore(200);
  fn_80037718(pno);
}

static void ItemBoxBalloon20Ring(task *tp, Sint32 pno) {
  AddScore(200);
  AddNumRing(pno, 20);
  AddMechHP(pno, 1.0f);
}

static void ItemBoxBalloonBarrier(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800374E0(pno);
}

static void ItemBoxBalloonBomb(task *tp, Sint32 pno) {
  AddScore(200);
  _rename_ItemBombSet(&tp->twp->pos);
}

static void ItemBoxBalloonHealth(task *tp, Sint32 pno) {
  AddScore(200);
  AddMechHP(pno, 12.0f);
}

static void ItemBoxBalloonMagneticBarrier(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800375FC(pno);
}

static void ItemBoxBalloonNothing(task *tp, Sint32 pno) {
}

static void ItemBoxBalloonInvincible(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800373B8(pno);
}

// indices into itemboxballoon_item_info; repeats weight the common items
static Sint8 itemboxballoon_generator_item[12] = {
    0, 1, 3, 3, 3, 3, 4, 4, 4, 5, 5, 1,
};

// keeps one balloon alive, replacing it twp->ang.x frames after it is gone
void ObjectItemBoxBalloonGenerator(task *tp) {
  taskwk *twp = tp->twp;
  task *ctp;
  taskwk *ctwp;

  if (CheckRangeOut(tp)) {
    return;
  }
  // wtimer reset in both branches to match
  if (tp->ctp == NULL) {
    if (twp->wtimer++ <= twp->ang.x) {
      return;
    }
    ctp = CreateChildTask(IM_TWK, ObjectItemBoxBalloon, tp);
    if (ctp != NULL) {
      ctwp = ctp->twp;
      ctwp->ang.y = twp->ang.y;
      ctwp->scl.x = itemboxballoon_generator_item[(Sint32)(
          12.0f * (0.000030517578f * (Float)rand()))];
    }
    twp->wtimer = 0;
  } else {
    twp->wtimer = 0;
  }
}
