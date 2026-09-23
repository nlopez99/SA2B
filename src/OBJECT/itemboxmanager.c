#include "OBJECT/itemboxmanager.h"

#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"

extern s32 _rename_GetPlayerCharacter(s32);
extern void __njColorBlendingMode(Int, Int);
extern void njDrawTexture3DEx(NJS_TEXTURE_VTX *polygon, Int count, Int trans);
extern Uint32 fn_801177FC(void);
extern void fn_8011C508(NJS_TEXTURE_VTX *polygon, Int count, Uint32 tex,
                        Int trans);
extern void fn_8001475C(void);
extern void fn_800148FC(void);
extern void fn_80014CD8(void);

extern Sint32 lbl_803ADAD0; // screen being drawn
extern Sint32 lbl_803ADAD4; // number of screens

// ^ extern
// v in this file

static task *itemBoxManagerCreate(void);
static void itemBoxManagerDest(task *tp);
static void itemBoxManager(task *tp);
static void itemBoxManagerAppear(task *tp);
static void itemBoxManagerWait(task *tp);
static void itemBoxManagerVanish(task *tp);
static void itemBoxManagerDisp(task *tp);

enum {
  MD_END,
  MD_APPEAR,
  MD_WAIT,
  MD_VANISH,
};

// item kinds, as the item boxes pass them; they index itemp_texname
#define ITEM_NOTHING 7
#define ITEM_EXTRALIFE 10 // + character number, see itemp_texname
#define ITEM_MAX 11

// an icon on the HUD
typedef struct itemicon // sizeof=0xC
{
  /* 0x00 */ Sint32 kind;
  /* 0x04 */ Float pos; // screen x
  /* 0x08 */ Float scl;
} itemicon;

typedef struct itemboxmanagerwk // sizeof=0xFC
{
  /* 0x00 */ Sint32 unk_0;
  /* 0x04 */ Sint32 num;
  /* 0x08 */ Uint32 timer;
  /* 0x0C */ itemicon item[20];
} itemboxmanagerwk;

#define GetWork(task) ((itemboxmanagerwk *)task->mwp)
// pointer cast on purpose, to match
#define GetItem(task) ((itemicon *)GetWork(task)->item)

static NJS_TEXNAME itemp_texname[] = {
    {"itemp_10ring", 0, 0},     {"itemp_20ring", 0, 0},
    {"itemp_5ring", 0, 0},      {"itemp_barrier", 0, 0},
    {"itemp_bomb", 0, 0},       {"itemp_life", 0, 0},
    {"itemp_magnet", 0, 0},     {"itemp_randomring", 0, 0},
    {"itemp_speed", 0, 0},      {"itemp_super", 0, 0},
    {"itemp_1up", 0, 0},        {"itemp_1up4", 0, 0},
    {"itemp_1up3", 0, 0},       {"itemp_1up6", 0, 0},
    {"itemp_1up2", 0, 0},       {"itemp_1up5", 0, 0},
    {"itemp_1up3", 0, 0},       {"itemp_1up6", 0, 0},
    {"itemp_1up", 0, 0},
};

NJS_TEXLIST itemp_texlist = {itemp_texname, ARYLEN(itemp_texname)};

static NJS_TEXTURE_VTX itemicon_poly[4] = {
    {0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF},
    {0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF},
    {-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF},
    {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF},
};

// one manager per player, created by the first item they get
static task *itemboxmanager_tp[2] = {NULL, NULL};

Sint32 _rename_ItemIconDraw(Sint32 kind, NJS_POINT3 *pos, Angle ang,
                            Float scl) {
  Sint32 ch;

  if (kind >= ITEM_MAX) {
    return FALSE;
  }
  if (kind == ITEM_EXTRALIFE &&
      (ch = _rename_GetPlayerCharacter(lbl_803ADAD0)) >= 0) {
    kind += ch;
  }

  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  njSetTexture(&itemp_texlist);
  njSetTextureNum(kind);
  njPushMatrix(NULL);
  njTranslateV(NULL, pos);
  njRotateY(NULL, ang);
  njScale(NULL, scl, scl, scl);
  njDrawTexture3DEx(itemicon_poly, 4, TRUE);
  njPopMatrix(1);
  return TRUE;
}

static Sint32 ItemIconDraw2D(Sint32 kind, NJS_POINT3 *pos, Float scl) {
  Sint32 ch;
  NJS_TEXTURE_VTX poly[4];
  STACK_PAD_VAR(2); // unused

  if (kind >= ITEM_MAX) {
    return FALSE;
  }

  scl = 0.5f * scl;
  poly[0].x = pos->x - scl;
  poly[0].y = pos->y - scl;
  poly[0].z = pos->z;
  poly[0].u = 0.0f;
  poly[0].v = 0.0f;
  poly[0].col = 0xFFFFFFFF;
  poly[1].x = pos->x - scl;
  poly[1].y = pos->y + scl;
  poly[1].z = pos->z;
  poly[1].u = 0.0f;
  poly[1].v = 1.0f;
  poly[1].col = 0xFFFFFFFF;
  poly[2].x = pos->x + scl;
  poly[2].y = pos->y - scl;
  poly[2].z = pos->z;
  poly[2].u = 1.0f;
  poly[2].v = 0.0f;
  poly[2].col = 0xFFFFFFFF;
  poly[3].x = pos->x + scl;
  poly[3].y = pos->y + scl;
  poly[3].z = pos->z;
  poly[3].u = 1.0f;
  poly[3].v = 1.0f;
  poly[3].col = 0xFFFFFFFF;

  if (kind == ITEM_EXTRALIFE &&
      (ch = _rename_GetPlayerCharacter(lbl_803ADAD0)) >= 0) {
    kind += ch;
  }

  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  njSetTexture(&itemp_texlist);
  njSetTextureNum(kind);
  fn_8011C508(poly, 4, fn_801177FC(), TRUE);
  return TRUE;
}

static task *itemBoxManagerCreate(void) {
  task *tp =
      CreateElementalTask(IM_TWK, LEV_3, itemBoxManager, "itemBoxManager");

  if (tp != NULL) {
    tp->mwp = syCalloc(1, sizeof(itemboxmanagerwk));
    if (tp->mwp != NULL) {
      tp->disp_late = itemBoxManagerDisp;
      tp->dest = itemBoxManagerDest;
      GetWork(tp)->num = 0;
      tp->twp->mode = MD_END;
    } else {
      DestroyTask(tp);
      tp = NULL;
    }
  }
  return tp;
}

static void itemBoxManagerDest(task *tp) {
  Sint32 i;

  for (i = 0; i < 2; i++) {
    if (tp == itemboxmanager_tp[i]) {
      itemboxmanager_tp[i] = NULL;
    }
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

void _rename_ItemGetDisplaySet(Sint32 pno, Sint32 kind) {
  task **tpp;
  task *tp;

  if (kind == ITEM_NOTHING) {
    return;
  }
  if (pno < 0 || pno >= 2 || lbl_801CC168._B == 0) {
    return;
  }

  tpp = &itemboxmanager_tp[pno];
  if (*tpp == NULL) {
    *tpp = itemBoxManagerCreate();
  }
  tp = *tpp;
  tp->twp->btimer = pno;
  GetItem(tp)[GetWork(tp)->num].kind = kind;
  GetItem(tp)[GetWork(tp)->num].pos = -60.0f;
  GetItem(tp)[GetWork(tp)->num].scl = 64.0f;
  GetWork(tp)->num++;
  tp->twp->mode = MD_APPEAR;
}

static void itemBoxManager(task *tp) {
  switch (tp->twp->mode) {
  case MD_END:
    DestroyTask(tp);
    break;
  case MD_APPEAR:
    itemBoxManagerAppear(tp);
    break;
  case MD_WAIT:
    itemBoxManagerWait(tp);
    break;
  case MD_VANISH:
    itemBoxManagerVanish(tp);
    break;
  }
}

// slide the icons in from the left until each reaches its place in the row
static void itemBoxManagerAppear(task *tp) {
  Sint32 i;
  Sint32 count = 0;
  Float pos;

  for (i = 0; i < GetWork(tp)->num; i++) {
    GetWork(tp)->item[i].pos += 30.0f;
    pos = 320.0f + 80.0f * (Float)(GetWork(tp)->num - 1) / 2.0f -
          80.0f * (Float)i;
    if (GetWork(tp)->item[i].pos >= pos) {
      GetWork(tp)->item[i].pos = pos;
      count++;
    }
    GetWork(tp)->item[i].scl += 0.2f;
    if (GetWork(tp)->item[i].scl > 64.0f) {
      GetWork(tp)->item[i].scl = 64.0f;
    }
  }

  if (count == GetWork(tp)->num) {
    tp->twp->mode = MD_WAIT;
    GetWork(tp)->timer = 0;
  }
}

static void itemBoxManagerWait(task *tp) {
  Sint32 i;

  for (i = 0; i < GetWork(tp)->num; i++) {
    GetWork(tp)->item[i].scl += 0.2f;
    if (GetWork(tp)->item[i].scl > 64.0f) {
      GetWork(tp)->item[i].scl = 64.0f;
    }
  }

  GetWork(tp)->timer++;
  if (GetWork(tp)->timer > 60) {
    GetWork(tp)->timer = 60;
    tp->twp->mode = MD_VANISH;
  }
}

// shrink the icons to nothing over the 60 frames counted by Wait
static void itemBoxManagerVanish(task *tp) {
  Sint32 i;

  for (i = 0; i < GetWork(tp)->num; i++) {
    GetWork(tp)->item[i].scl =
        0.016666668f * (64.0f * (Float)GetWork(tp)->timer);
  }

  GetWork(tp)->timer--;
  if (GetWork(tp)->timer == 0) {
    GetWork(tp)->num = 0;
    tp->twp->mode = MD_END;
  }
}

static void itemBoxManagerDisp(task *tp) {
  Sint32 i;
  NJS_POINT3 pos;

  if (lbl_803ADAD4 <= 0 ||
      (tp->twp->btimer != lbl_803ADAD0 && lbl_801CC168._20 != 0) ||
      (tp->twp->btimer != lbl_803ADAD0 && lbl_801CC168.TWO_PLAYER == 0)) {
    return;
  }

  pos.y = 370.0f;
  pos.z = 0.952381f;
  if (lbl_801CC168.TWO_PLAYER) {
    fn_800148FC();
    fn_80014CD8();
  }

  for (i = 0; i < GetWork(tp)->num; i++) {
    if (lbl_801CC168.TWO_PLAYER && lbl_801CC168._20 == 0) {
      pos.x = 0.5f * GetWork(tp)->item[i].pos + 320.0f * (Float)tp->twp->btimer;
      ItemIconDraw2D(GetWork(tp)->item[i].kind, &pos,
                     0.5f * GetWork(tp)->item[i].scl);
    } else {
      pos.x = 320.0f * (Float)lbl_803ADAD0 +
              GetWork(tp)->item[i].pos / (Float)lbl_803ADAD4;
      ItemIconDraw2D(GetWork(tp)->item[i].kind, &pos,
                     GetWork(tp)->item[i].scl / (Float)lbl_803ADAD4);
    }
  }

  if (lbl_801CC168.TWO_PLAYER) {
    fn_8001475C();
  }
}
