#include "OBJECT/o_itembox.h"

#include "CCL.h"
#include "samt/ninja/gjdraw.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njcollision.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "samt/sonic/sound.h"
#include "set.h"

extern BOOL _rename_CheckFlag0x20(task *tp);
extern void _rename_SetFlag0x20(task *tp);
extern task *fn_80006574(task *tp);
extern void fn_800067F8(task *tp);
extern void fn_80018D28(task *tp);
extern void fn_80018B88(task *tp);
extern void fn_800168C8(Sint32 pno);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern void fn_800641B8(Sint32 pno, Sint32 num);
extern void fn_800373B8(Sint32 pno); // invincibility
extern void fn_800374E0(Sint32 pno); // barrier
extern void fn_800375FC(Sint32 pno); // magnetic barrier
extern void fn_80037718(Sint32 pno); // speed up
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
extern void _rename_DrawExplosion(Float r, Float rate);

extern BOOL DisableObjectFog;
// asked by other tasks whether a point is inside an explosion
extern BOOL (*lbl_803AD7E8)(NJS_POINT3 *pos);

extern NJS_TEXLIST   _rename_itembox_texlist;
extern NJS_CNK_MODEL _rename_itembox_model;
extern GJS_MODEL     _rename_itembox_gjmodel_0;
extern GJS_MODEL     _rename_itembox_gjmodel_1;
extern CCL_INFO      _rename_itembox_colli_info[2];
extern CCL_INFO      _rename_itembox_colli_info_broken[2];

// ^ extern
// v in this file

static void ObjectItemBoxInit(task *tp);
static void ObjectItemBoxAppear(task *tp);
static void ObjectItemBoxNormal(task *tp);
static void ObjectItemBoxBroken(task *tp);
static void ObjectItemBoxDisp(task *tp);
static void ObjectItemBoxDispSort(task *tp);
static void ObjectItemBoxDispBroken(task *tp);
static void ObjectItemBoxDest(task *tp);

static void ItemBoxRing10(task *tp, Sint32 pno);
static void ItemBoxExtraLife(task *tp, Sint32 pno);
static void ItemBoxRing5(task *tp, Sint32 pno);
static void ItemBoxSpeedUp(task *tp, Sint32 pno);
static void ItemBoxRing20(task *tp, Sint32 pno);
static void ItemBoxBarrier(task *tp, Sint32 pno);
static void ItemBoxBomb(task *tp, Sint32 pno);
static void ItemBoxHealth(task *tp, Sint32 pno);
static void ItemBoxMagnet(task *tp, Sint32 pno);
static void ItemBoxNothing(task *tp, Sint32 pno);
static void ItemBoxInvincible(task *tp, Sint32 pno);

static void ExpManExec(task *tp);
static void ExpManDisp(task *tp);
static void ExpManDest(task *tp);
static BOOL ExpManCheckHit(NJS_POINT3 *pos);

enum {
  MD_INIT,
  MD_APPEAR,
  MD_NORMAL,
  MD_BROKEN,
  MD_REMAIN, // only the base is left
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
typedef struct itemboxwk {
  /* 0x00 */ Uint8 unk0[0x44];
  /* 0x44 */ Sint8 pno; // player that attacked the box, -1 if none
} itemboxwk;

static iteminfo itembox_item_info[] = {
    {ITEM_SPEEDUP, ItemBoxSpeedUp},
    {ITEM_RING5, ItemBoxRing5},
    {ITEM_EXTRALIFE, ItemBoxExtraLife},
    {ITEM_RING10, ItemBoxRing10},
    {ITEM_RING20, ItemBoxRing20},
    {ITEM_BARRIER, ItemBoxBarrier},
    {ITEM_BOMB, ItemBoxBomb},
    {ITEM_HEALTH, ItemBoxHealth},
    {ITEM_MAGNET, ItemBoxMagnet},
    {ITEM_NOTHING, ItemBoxNothing},
    {ITEM_INVINCIBLE, ItemBoxInvincible},
};

static task *expman_tp;

// twp->scl.x: item index from the set file, twp->scl.z: box scale,
// twp->smode: the box is the child of another object
#define GetItem(task) (*(Sint32 *)&task->fwp)
#define GetExpList(task) (*(expwk **)&task->fwp)

void _rename_GoalRingChildExec(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->smode == 0 && CheckRangeOut(tp)) {
    return;
  }

  if (twp->mode == MD_INIT || twp->mode == MD_APPEAR ||
      twp->mode == MD_NORMAL) {
    tp->disp = ObjectItemBoxDisp;
    tp->disp_sort = ObjectItemBoxDispSort;
  } else if (twp->mode == MD_BROKEN || twp->mode == MD_REMAIN) {
    tp->disp = ObjectItemBoxDispBroken;
    tp->disp_sort = NULL;
  }

  switch (twp->mode) {
  case MD_INIT:
    ObjectItemBoxInit(tp);
    break;
  case MD_APPEAR:
    ObjectItemBoxAppear(tp);
    break;
  case MD_NORMAL:
    ObjectItemBoxNormal(tp);
    break;
  case MD_BROKEN:
    ObjectItemBoxBroken(tp);
    break;
  case MD_REMAIN:
  case MD_COLLI:
    CCL_Entry(tp);
    break;
  case MD_DEST:
    ObjectItemBoxDest(tp);
  default:
    twp->mode = MD_INIT;
    break;
  }
}

static void ObjectItemBoxInit(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 i;

  twp->mode = MD_APPEAR;
  twp->scl.z = 0.0f;
  twp->scl.y = 0.0f;
  GetItem(tp) = twp->scl.x;
  if (GetItem(tp) >= (Sint32)ARYLEN(itembox_item_info)) {
    GetItem(tp) = ARYLEN(itembox_item_info) - 1;
  } else if (GetItem(tp) < 0) {
    GetItem(tp) = 0;
  }

  // an extra life that was already taken becomes an empty box
  if (((tp->ocp != NULL && !CheckBroken(tp) && _rename_CheckFlag0x20(tp)) ||
       (twp->smode != 0 && tp->ptp->ocp != NULL && !CheckBroken(tp->ptp) &&
        _rename_CheckFlag0x20(tp->ptp))) &&
      itembox_item_info[GetItem(tp)].kind == ITEM_EXTRALIFE) {
    for (i = 0; i < (Sint32)ARYLEN(itembox_item_info); i++) {
      if (itembox_item_info[i].kind == ITEM_NOTHING) {
        GetItem(tp) = i;
        break;
      }
    }
  }

  if ((tp->ocp != NULL && CheckBroken(tp)) ||
      (twp->smode != 0 && tp->ptp->ocp != NULL && CheckBroken(tp->ptp))) {
    CCL_InitShare(tp, _rename_itembox_colli_info_broken,
                  ARYLEN(_rename_itembox_colli_info_broken), CID_OBJECT);
    twp->mode = MD_REMAIN;
  } else {
    CCL_InitShare(tp, _rename_itembox_colli_info,
                  ARYLEN(_rename_itembox_colli_info), CID_ENEMY);
  }
  fn_80018D28(tp);
  tp->dest = ObjectItemBoxDest;
  tp->disp = ObjectItemBoxDisp;
  tp->disp_sort = ObjectItemBoxDispSort;
}

static void ObjectItemBoxAppear(task *tp) {
  taskwk *twp = tp->twp;

  if (lbl_801CC168._37) {
    twp->scl.z = 1.0f;
    twp->mode = MD_NORMAL;
    return;
  }

  twp->scl.z += 0.11f;
  if (twp->scl.z >= 1.0f) {
    twp->mode = MD_NORMAL;
    twp->scl.z = 1.0f;
  }
  twp->scl.y += 5.0f;
}

static void ObjectItemBoxNormal(task *tp) {
  task *hit_tp = NULL;
  taskwk *twp = tp->twp;
  itemboxwk *wk = (itemboxwk *)tp->mwp;
  Sint32 pno;
  NJS_VECTOR spd;
  NJS_POINT3 pos;

  if ((twp->flag & 4 && wk->pno != -1) ||
      (hit_tp = CCL_IsHitPlayer(tp)) != NULL ||
      (hit_tp = fn_80006574(tp)) != NULL) {
    if (hit_tp == NULL || (pno = IsThisTaskPlayer(hit_tp)) < 0) {
      pno = wk->pno;
    }
    if (pno >= 0) {
      if (!(playerpwp[pno]->item & 0x4000)) {
        _rename_ItemGetDisplaySet(pno, itembox_item_info[GetItem(tp)].kind);
        itembox_item_info[GetItem(tp)].func(tp, pno);
        fn_800168C8(pno);
      }
      twp->mode = MD_BROKEN;
      pos = twp->pos;
      pos.y += 7.5f;
      spd.x = 0.0f;
      spd.y = 0.1f;
      spd.z = 0.0f;
      CreateSnowPuff(&pos, &spd, 6.0f);
      SE_Call(0x800D, NULL, 0, 0);
      fn_8002FB2C(pno, 1, 0x30, 0);
      if (tp->ocp != NULL) {
        SetBroken(tp);
      } else if (twp->smode != 0 && tp->ptp->ocp != NULL) {
        SetBroken(tp->ptp);
      }
    }
  }

  CCL_Entry(tp);
  if (!lbl_801CC168._37) {
    twp->scl.y += 5.0f;
  }
}

static void ObjectItemBoxBroken(task *tp) {
  taskwk *twp = tp->twp;

  fn_800067F8(tp);
  CCL_InitShare(tp, _rename_itembox_colli_info_broken,
                ARYLEN(_rename_itembox_colli_info_broken), CID_OBJECT);
  twp->mode = MD_REMAIN;
}

static void ObjectItemBoxDisp(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_itembox_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njScale(NULL, twp->scl.z, twp->scl.z, twp->scl.z);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  gjDrawModel(&_rename_itembox_gjmodel_1);
  gjDrawModel(&_rename_itembox_gjmodel_0);
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
  njPopMatrixEx();
}

static void ObjectItemBoxDispSort(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 pos;

  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njScale(NULL, twp->scl.z, twp->scl.z, twp->scl.z);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  if (itembox_item_info[GetItem(tp)].kind != ITEM_NOTHING) {
    pos.x = 0.0f;
    pos.y = 7.5f;
    pos.z = 0.0f;
    _rename_ItemIconDraw(itembox_item_info[GetItem(tp)].kind, &pos,
                         182.04445f * twp->scl.y, 6.0f);
  }
  njSetTexture(&_rename_itembox_texlist);
  njCnkCacheDrawModel(&_rename_itembox_model);
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
  njPopMatrixEx();
}

static void ObjectItemBoxDispBroken(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_itembox_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  gjDrawModel(&_rename_itembox_gjmodel_1);
  njPopMatrixEx();
}

static void ObjectItemBoxDest(task *tp) {
  fn_80018B88(tp);
  GetItem(tp) = 0;
}

static void ItemBoxRing10(task *tp, Sint32 pno) {
  AddScore(100);
  AddNumRing(pno, 10);
  AddMechHP(pno, 0.5f);
}

static void ItemBoxExtraLife(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800641B8(pno, 1);
  if (tp->twp->smode != 0 && tp->ptp->ocp != NULL) {
    _rename_SetFlag0x20(tp->ptp);
  } else if (tp->ocp != NULL) {
    _rename_SetFlag0x20(tp);
  }
}

static void ItemBoxRing5(task *tp, Sint32 pno) {
  AddScore(50);
  AddNumRing(pno, 5);
  AddMechHP(pno, 0.25f);
}

static void ItemBoxSpeedUp(task *tp, Sint32 pno) {
  AddScore(200);
  fn_80037718(pno);
}

static void ItemBoxRing20(task *tp, Sint32 pno) {
  AddScore(200);
  AddNumRing(pno, 20);
  AddMechHP(pno, 1.0f);
}

static void ItemBoxBarrier(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800374E0(pno);
}

static void ItemBoxBomb(task *tp, Sint32 pno) {
  AddScore(200);
  _rename_ItemBombSet(&tp->twp->pos);
}

static void ItemBoxHealth(task *tp, Sint32 pno) {
  AddScore(200);
  AddMechHP(pno, 12.0f);
}

static void ItemBoxMagnet(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800375FC(pno);
}

static void ItemBoxNothing(task *tp, Sint32 pno) {}

static void ItemBoxInvincible(task *tp, Sint32 pno) {
  AddScore(200);
  fn_800373B8(pno);
}

// gives an item without a box (check points, ...)
void _rename_GiveItemP(Sint32 pno, Sint32 kind) {
  Sint32 i;

  if (playerpwp[pno]->item & 0x4000) {
    return;
  }
  if (kind == ITEM_NOTHING) {
    return;
  }

  for (i = 0; i < (Sint32)ARYLEN(itembox_item_info); i++) {
    if (kind == itembox_item_info[i].kind) {
      _rename_ItemGetDisplaySet(pno, itembox_item_info[i].kind);
      itembox_item_info[i].func(NULL, pno);
      break;
    }
  }
}

// ExpMan: one task that owns every live explosion sphere
expwk *_rename_ItemBombSet(NJS_POINT3 *pos) {
  expwk *ewp = NULL;
  task *tp;

  if (expman_tp == NULL) {
    expman_tp = CreateFundamentalTask(IM_TWK, 3, ExpManExec);
    if (expman_tp != NULL) {
      expman_tp->disp = ExpManDisp;
      expman_tp->dest = ExpManDest;
    }
  }

  if (expman_tp != NULL && (ewp = syCalloc(1, sizeof(expwk))) != NULL) {
    if (lbl_803AD7E8 == NULL) {
      lbl_803AD7E8 = ExpManCheckHit;
    }
    tp = expman_tp;
    ewp->pos = *pos;
    ewp->r = 0.0f;
    ewp->next = GetExpList(tp);
    ewp->r_max = 500.0f;
    GetExpList(tp) = ewp;
  }
  return ewp;
}

expwk *ItemBombSetRange(NJS_POINT3 *pos, Float range) {
  expwk *ewp = NULL;
  task *tp;

  if (expman_tp == NULL) {
    expman_tp = CreateFundamentalTask(IM_TWK, 3, ExpManExec);
    if (expman_tp != NULL) {
      expman_tp->disp = ExpManDisp;
      expman_tp->dest = ExpManDest;
    }
  }

  if (expman_tp != NULL && (ewp = syCalloc(1, sizeof(expwk))) != NULL) {
    if (lbl_803AD7E8 == NULL) {
      lbl_803AD7E8 = ExpManCheckHit;
    }
    tp = expman_tp;
    ewp->pos = *pos;
    ewp->r = 0.0f;
    ewp->next = GetExpList(tp);
    ewp->r_max = range;
    GetExpList(tp) = ewp;
  }
  return ewp;
}

// if/else instead of an early continue, to match
static void ExpManExec(task *tp) {
  expwk *prev = NULL;
  expwk *ewp = GetExpList(tp);
  expwk *next;

  while (ewp != NULL) {
    ewp->r += 10.0f;
    if (ewp->r > ewp->r_max) {
      ewp->r = ewp->r_max;
      next = ewp->next;
      if (GetExpList(tp) == ewp) {
        GetExpList(tp) = next;
      }
      if (prev != NULL) {
        prev->next = next;
      }
      syFree(ewp);
      ewp = next;
    } else {
      prev = ewp;
      ewp = ewp->next;
    }
  }

  if (GetExpList(tp) == NULL) {
    DestroyTask(tp);
  }
}

static void ExpManDisp(task *tp) {
  expwk *ewp;
  Float rate;
  NJS_POINT3 pos; // unused

  for (ewp = GetExpList(tp); ewp != NULL; ewp = ewp->next) {
    rate = ewp->r / ewp->r_max;
    njPushMatrixEx();
    njTranslateEx(&ewp->pos);
    njRotateY(NULL, lbl_801CC168._7C * 0x120);
    _rename_DrawExplosion(ewp->r, rate);
    njPopMatrixEx();
  }
}

static void ExpManDest(task *tp) {
  expwk *next;

  while (GetExpList(tp) != NULL) {
    next = GetExpList(tp)->next;
    syFree(GetExpList(tp));
    GetExpList(tp) = next;
  }

  if (tp == expman_tp) {
    expman_tp = NULL;
  }
  if (lbl_803AD7E8 == ExpManCheckHit) {
    lbl_803AD7E8 = NULL;
  }
}

// nested to match
static BOOL ExpManCheckHit(NJS_POINT3 *pos) {
  expwk *ewp;

  if (expman_tp != NULL) {
    for (ewp = GetExpList(expman_tp); ewp != NULL; ewp = ewp->next) {
      if (njDistanceP2P(&ewp->pos, pos) < ewp->r) {
        return TRUE;
      }
    }
    return FALSE;
  }
  return FALSE;
}
