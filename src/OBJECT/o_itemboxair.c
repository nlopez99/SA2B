#include "OBJECT/o_itemboxair.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/sound.h"
#include "set.h"

extern BOOL _rename_CheckFlag0x20(task *tp);
extern void _rename_SetFlag0x20(task *tp);
extern void fn_80006908(task *tp);
extern task *fn_80006574(task *tp);
extern void fn_80018D28(task *tp);
extern void fn_80018B88(task *tp);
extern void fn_800168C8(Sint32 pno);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void fn_801218C8(Sint32, Sint32);
extern void fn_800156FC(Float, Float, Float, Float);
extern void fn_800641B8(Sint32 pno, Sint32 num);
extern void fn_800373B8(Sint32 pno);
extern void fn_800374E0(Sint32 pno);
extern void fn_800375FC(Sint32 pno);
extern void fn_80037718(Sint32 pno);
extern void AddScore(int);
extern void AddNumRing(int, s16);
extern void AddMechHP(s32, f32);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);

extern void _rename_ItemGetDisplaySet(Sint32 pno, Sint32 kind);
extern Sint32 _rename_ItemIconDraw(Sint32 kind, NJS_POINT3 *pos, Angle ang,
                                   Float scl);
extern void *CreateSnowPuff(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void _rename_ItemBombSet(NJS_POINT3 *pos);

extern NJS_TEXLIST   _rename_itemboxair_texlist;
extern NJS_CNK_MODEL _rename_itemboxair_model_0;
extern NJS_CNK_MODEL _rename_itemboxair_model_1;
extern NJS_CNK_MODEL _rename_itemboxair_model_2;
extern CCL_INFO      _rename_itemboxair_colli_info[1];

// ^ extern
// v in this file

static void ObjectItemBoxAirInit(task *tp);
static void ObjectItemBoxAirAppear(task *tp);
static void ObjectItemBoxAirNormal(task *tp);
static void ObjectItemBoxAirBroken(task *tp);
static void ObjectItemBoxAirVanish(task *tp);
static void ObjectItemBoxAirDisp(task *tp);
static void ObjectItemBoxAirDispSort(task *tp);
static void ObjectItemBoxAirDest(task *tp);

static void ItemBoxAirRing10(task *tp, Sint32 pno);
static void ItemBoxAirExtraLife(task *tp, Sint32 pno);
static void ItemBoxAirRing5(task *tp, Sint32 pno);
static void ItemBoxAirSpeedUp(task *tp, Sint32 pno);
static void ItemBoxAirRing20(task *tp, Sint32 pno);
static void ItemBoxAirBarrier(task *tp, Sint32 pno);
static void ItemBoxAirBomb(task *tp, Sint32 pno);
static void ItemBoxAirHealth(task *tp, Sint32 pno);
static void ItemBoxAirMagnet(task *tp, Sint32 pno);
static void ItemBoxAirNothing(task *tp, Sint32 pno);
static void ItemBoxAirInvincible(task *tp, Sint32 pno);

enum {
  MD_INIT,
  MD_APPEAR,
  MD_NORMAL,
  MD_BROKEN,
  MD_VANISH,
  MD_COLLI,
  MD_DEST,
};

enum {
  ITEM_RING10,
  ITEM_RING20,
  ITEM_RING5,
  ITEM_BARRIER,
  ITEM_BOMB,
  ITEM_HEALTH,
  ITEM_MAGNET,
  ITEM_NOTHING,
  ITEM_SPEEDUP,
  ITEM_INVINCIBLE,
  ITEM_EXTRALIFE,
};

typedef struct iteminfo // sizeof=0x8
{
  /* 0x00 */ Sint32 kind;
  /* 0x04 */ void (*func)(task *tp, Sint32 pno);
} iteminfo;

// work allocated by fn_80018D28, sizeof=0x210
typedef struct itemboxairwk {
  /* 0x00 */ Uint8 unk0[0x44];
  /* 0x44 */ Sint8 pno; // player that attacked the box, -1 if none
} itemboxairwk;

static iteminfo itemboxair_item_info[] = {
    {ITEM_SPEEDUP, ItemBoxAirSpeedUp},
    {ITEM_RING5, ItemBoxAirRing5},
    {ITEM_EXTRALIFE, ItemBoxAirExtraLife},
    {ITEM_RING10, ItemBoxAirRing10},
    {ITEM_RING20, ItemBoxAirRing20},
    {ITEM_BARRIER, ItemBoxAirBarrier},
    {ITEM_BOMB, ItemBoxAirBomb},
    {ITEM_HEALTH, ItemBoxAirHealth},
    {ITEM_MAGNET, ItemBoxAirMagnet},
    {ITEM_NOTHING, ItemBoxAirNothing},
    {ITEM_INVINCIBLE, ItemBoxAirInvincible},
};

void ObjectItemBoxAir(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 pos; // unused

  if (twp->smode == 0 && CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case MD_INIT:
    ObjectItemBoxAirInit(tp);
    break;
  case MD_APPEAR:
    ObjectItemBoxAirAppear(tp);
    break;
  case MD_NORMAL:
    ObjectItemBoxAirNormal(tp);
    break;
  case MD_BROKEN:
    ObjectItemBoxAirBroken(tp);
    break;
  case MD_VANISH:
    ObjectItemBoxAirVanish(tp);
    break;
  case MD_COLLI:
    CCL_Entry(tp);
    break;
  case MD_DEST:
    ObjectItemBoxAirDest(tp);
  default:
    twp->mode = MD_INIT;
    break;
  }
}

static NJS_POINT3 itemboxair_colli_center = {0.0f, 14.0f, 0.0f};

static void ObjectItemBoxAirInit(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 i;

  if (tp->ocp != NULL && CheckBroken(tp)) {
    DeadOut(tp);
    return;
  }
  if (twp->smode != 0 && tp->ptp->ocp != NULL && CheckBroken(tp->ptp)) {
    DeadOut(tp->ptp);
    return;
  }

  twp->mode = MD_APPEAR;
  twp->scl.z = 0.0f;
  twp->scl.y = 0.0f;
  twp->btimer = (Uint8)(Sint32)twp->scl.x;
  if (twp->btimer >= ARYLEN(itemboxair_item_info)) {
    twp->btimer = ARYLEN(itemboxair_item_info) - 1;
  } else if (twp->btimer < 0) { // never true, as in the original
    twp->btimer = 0;
  }

  // an extra life that was already taken becomes an empty box
  if (((tp->ocp != NULL && !CheckBroken(tp) && _rename_CheckFlag0x20(tp)) ||
       (twp->smode != 0 && tp->ptp->ocp != NULL && !CheckBroken(tp->ptp) &&
        _rename_CheckFlag0x20(tp->ptp))) &&
      itemboxair_item_info[twp->btimer].kind == ITEM_EXTRALIFE) {
    for (i = 0; i < ARYLEN(itemboxair_item_info); i++) {
      if (itemboxair_item_info[i].kind == ITEM_NOTHING) {
        twp->btimer = i;
        break;
      }
    }
  }

  CCL_Init(tp, _rename_itemboxair_colli_info,
           ARYLEN(_rename_itemboxair_colli_info), CID_ENEMY);
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njCalcPoint(NULL, &itemboxair_colli_center, &twp->cwp->info->center);
  njPopMatrixEx();
  fn_80006908(tp);
  fn_80018D28(tp);
  tp->dest = ObjectItemBoxAirDest;
  tp->disp = ObjectItemBoxAirDisp;
  tp->disp_sort = ObjectItemBoxAirDispSort;
}

static void ObjectItemBoxAirAppear(task *tp) {
  taskwk *twp = tp->twp;

  if (lbl_801CC168._37) {
    twp->scl.z = 1.0f;
    twp->mode = MD_NORMAL;
    return;
  }

  twp->scl.z += 0.05f;
  if (twp->scl.z >= 1.0f) {
    twp->mode = MD_NORMAL;
    twp->scl.z = 1.0f;
  }
  twp->scl.y += 5.0f;
}

static void ObjectItemBoxAirNormal(task *tp) {
  task *hit_tp = NULL;
  taskwk *twp = tp->twp;
  itemboxairwk *wk = (itemboxairwk *)tp->mwp;
  Sint32 pno;
  NJS_VECTOR spd;
  NJS_POINT3 pos; // unused

  if ((twp->flag & 4 && wk->pno != -1) ||
      (hit_tp = CCL_IsHitPlayer(tp)) != NULL ||
      (hit_tp = fn_80006574(tp)) != NULL) {
    if (hit_tp == NULL || (pno = IsThisTaskPlayer(hit_tp)) < 0) {
      pno = wk->pno;
    }
    if (pno >= 0) {
      if (!(playerpwp[pno]->item & 0x4000)) {
        _rename_ItemGetDisplaySet(pno, itemboxair_item_info[twp->btimer].kind);
        itemboxair_item_info[twp->btimer].func(tp, pno);
        fn_800168C8(pno);
      }
      twp->mode = MD_BROKEN;
      spd.x = 0.0f;
      spd.y = 0.1f;
      spd.z = 0.0f;
      CreateSnowPuff(&twp->pos, &spd, 6.0f);
      SE_Call(0x800D, NULL, 0, 0);
      fn_8002FB2C(pno, 1, 0x30, 0);
      if (tp->ocp != NULL) {
        SetBroken(tp);
      }
      if (twp->smode != 0 && tp->ptp->ocp != NULL) {
        SetBroken(tp->ptp);
      }
    }
  }

  if (twp->mode != MD_BROKEN) {
    CCL_Entry(tp);
  }
  if (!lbl_801CC168._37) {
    twp->scl.y += 5.0f;
  }
}

static void ObjectItemBoxAirBroken(task *tp) {
  tp->twp->mode = MD_VANISH;
}

static void ObjectItemBoxAirVanish(task *tp) {
  taskwk *twp = tp->twp;

  twp->scl.z += 0.2;
  if (!(twp->scl.z > 5.0f)) {
    return;
  }

  twp->scl.z = 5.0f;
  if (twp->smode != 0) {
    if (tp->ptp->ocp != NULL) {
      DeadOut(tp->ptp);
    } else {
      FreeTask(tp->ptp);
    }
  } else if (tp->ocp != NULL) {
    DeadOut(tp);
  } else {
    FreeTask(tp);
  }
}

static void ObjectItemBoxAirDisp(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->scl.z > 1.0f) {
    return;
  }

  if (twp->mode == MD_VANISH) {
    njDisableFog();
    gjSetFog();
  }
  njSetTexture(&_rename_itemboxair_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njScale(NULL, twp->scl.z, twp->scl.z, twp->scl.z);
  njCnkCacheDrawModel(&_rename_itemboxair_model_2);
  njCnkCacheDrawModel(&_rename_itemboxair_model_1);
  njPopMatrixEx();
  if (twp->mode == MD_VANISH) {
    njEnableFog();
    gjSetFog();
  }
}

static void ObjectItemBoxAirDispSort(task *tp) {
  taskwk *twp = tp->twp;
  Float alpha;
  NJS_POINT3 pos;

  if (twp->mode == MD_VANISH) {
    njDisableFog();
    gjSetFog();
  }

  if (twp->scl.z > 1.0f) {
    alpha = 1.0f - (twp->scl.z - 1.0f) / 4.0f;
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
  }

  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njScale(NULL, twp->scl.z, twp->scl.z, twp->scl.z);
  if (itemboxair_item_info[twp->btimer].kind != ITEM_NOTHING) {
    pos.x = 0.0f;
    pos.y = 14.5f;
    pos.z = 0.0f;
    _rename_ItemIconDraw(itemboxair_item_info[twp->btimer].kind, &pos,
                         182.04445f * twp->scl.y, 9.0f);
  }
  njSetTexture(&_rename_itemboxair_texlist);
  if (twp->scl.z > 1.0f) {
    njCnkCacheDrawModel(&_rename_itemboxair_model_2);
  }
  njCnkCacheDrawModel(&_rename_itemboxair_model_0);
  if (twp->scl.z > 1.0f) {
    njCnkCacheDrawModel(&_rename_itemboxair_model_1);
  }
  njPopMatrixEx();

  if (twp->scl.z > 1.0f) {
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
    fn_8002B348();
    fn_8002B2F8();
  }

  if (twp->mode == MD_VANISH) {
    njEnableFog();
    gjSetFog();
  }
}

static void ObjectItemBoxAirDest(task *tp) {
  fn_80018B88(tp);
  tp->fwp = NULL;
}

static void ItemBoxAirRing10(task *tp, Sint32 pno) {
  AddScore(100);
  AddNumRing(pno, 10);
  AddMechHP(pno, 0.5f);
}

static void ItemBoxAirExtraLife(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800641B8(pno, 1);
  if (tp->twp->smode != 0) {
    _rename_SetFlag0x20(tp->ptp);
  } else {
    _rename_SetFlag0x20(tp);
  }
}

static void ItemBoxAirRing5(task *tp, Sint32 pno) {
  AddScore(50);
  AddNumRing(pno, 5);
  AddMechHP(pno, 0.25f);
}

static void ItemBoxAirSpeedUp(task *tp, Sint32 pno) {
  AddScore(200);
  fn_80037718(pno);
}

static void ItemBoxAirRing20(task *tp, Sint32 pno) {
  AddScore(200);
  AddNumRing(pno, 20);
  AddMechHP(pno, 1.0f);
}

static void ItemBoxAirBarrier(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800374E0(pno);
}

static void ItemBoxAirBomb(task *tp, Sint32 pno) {
  AddScore(200);
  _rename_ItemBombSet(&tp->twp->pos);
}

static void ItemBoxAirHealth(task *tp, Sint32 pno) {
  AddScore(200);
  AddMechHP(pno, 12.0f);
}

static void ItemBoxAirMagnet(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800375FC(pno);
}

static void ItemBoxAirNothing(task *tp, Sint32 pno) {}

static void ItemBoxAirInvincible(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800373B8(pno);
}
