#include "OBJECT/o_dynamite.h"

#include "CCL.h"
#include "EFFECT/ef_explosion.h"
#include "OBJECT/o_itembox.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "set.h"

// samt/sonic/set.h does not compile under mwcc; OBJ_CONDITION is completed here
typedef struct _OBJ_EDITENTRY OBJ_EDITENTRY;

struct _OBJ_CONDITION {
  /* 0x00 */ Uint8 scCount;
  /* 0x01 */ Uint8 scUserFlag;
  /* 0x02 */ Sint16 ssCondition;
  /* 0x04 */ task *ptask;
  /* 0x08 */ OBJ_EDITENTRY *pObjEditEntry;
  /* 0x0C */ Float fRangeOut;
};

extern void fn_80018D28(task *tp);
extern void fn_80018B88(task *tp);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void fn_800156FC(Float, Float, Float, Float);
extern void fn_801218C8(Sint32, Sint32);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern Uint32 fn_800334B0(Uint32 col0, Uint32 col1, Float ratio);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);
extern void fn_80119FD8(Sint32);
extern void fn_8011A3D4(Sint32, Uint32 color);
extern void fn_8011A280(Float *rect, Float z);
extern void fn_8011A27C(void);
extern void AddScore(int);
extern void ds_DrawModelClip(NJS_MODEL *model);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern void __njColorBlendingMode(Int, Int);
extern Float _rename_GetBlinkRatio(Sint32 cycle, Sint32 on, Sint32 fade);
extern void _rename_CnkDrawModelColor(NJS_CNK_MODEL *model, Uint32 color);
// world point -> screen point, 0 when it is behind the camera
// out->z is the perspective divisor for the 2D draw
extern Sint32 _rename_CalcScreenPos(NJS_POINT3 *pos, NJS_POINT3 *out);

// bit 0: the dynamite list has been cleared once this stage load
extern Sint32 lbl_803ADC10;

typedef struct dsmodel // sizeof=0x10
{
  /* 0x00 */ Sint32 flag;
  /* 0x04 */ NJS_TEXLIST *texlist;
  /* 0x08 */ void *unk_8;
  /* 0x0C */ NJS_MODEL *model;
} dsmodel;

extern NJS_TEXLIST _rename_dynamite_texlist;
extern NJS_CNK_OBJECT _rename_dynamite_object;
extern NJS_TEXLIST _rename_dynamite_lamp_texlist;
extern NJS_CNK_MODEL _rename_dynamite_lamp_model;
extern NJS_TEXLIST _rename_dynamite_mark_texlist;
extern dsmodel _rename_dynamite_ObjArr[1];
extern CCL_INFO _rename_dynamite_colli_info[1];
extern NJS_POINT3 _rename_dynamite_mark_ofs;
extern Float _rename_dynamite_mark_size;
extern Float _rename_dynamite_mark_bob;

// ^ extern
// v in this file

static void DynamiteListInit(void);
static void DynamiteListEntry(task *tp);
static void DynamiteListFree(task *tp);
static void ObjectDynamiteDest(task *tp);
static void ObjectDynamiteExec(task *tp);
static void ObjectDynamiteDisp(task *tp);
static void ObjectDynamiteDispSort(task *tp);

// work allocated by fn_80018D28, sizeof=0x210
typedef struct dynamitewk {
  /* 0x00 */ Uint8 unk0[0x44];
  /* 0x44 */ Sint8 pno; // player that attacked the dynamite, -1 if none
} dynamitewk;

// where each set-placed dynamite is and whether it is still standing
typedef struct dynamiteinfo // sizeof=0x14
{
  /* 0x00 */ Sint32 entry; // the OBJ_EDITENTRY it was placed from, as a key
  /* 0x04 */ Sint32 id;
  /* 0x08 */ NJS_POINT3 pos;
} dynamiteinfo;

#define DYNAMITE_MAX 0x50

// never set in this build; the DS model table it picks is empty too
static BOOL dynamite_dsdraw;
static Sint32 dynamite_break_count;
static dynamiteinfo dynamite_list[DYNAMITE_MAX];

static void DynamiteListInit(void) {
  Sint32 i;

  for (i = 0; i < DYNAMITE_MAX; i++) {
    dynamite_list[i].entry = 0;
    dynamite_list[i].id = 0;
    dynamite_list[i].pos.x = 0.0f;
    dynamite_list[i].pos.y = 0.0f;
    dynamite_list[i].pos.z = 0.0f;
  }
}

static void DynamiteListEntry(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 i;
  Sint32 entry;
  dynamiteinfo *p;
  NJS_POINT2 pos; // unused

  if (!(lbl_803ADC10 & 1)) {
    lbl_803ADC10 |= 1;
    DynamiteListInit();
  }
  if (tp == NULL) {
    return;
  }
  // tested twice, as in the original
  if (tp->ocp == NULL) {
    return;
  }
  if (tp->ocp == NULL) {
    return;
  }
  if (tp->ocp->pObjEditEntry == NULL) {
    return;
  }
  entry = (Sint32)tp->ocp->pObjEditEntry;
  p = dynamite_list;
  if (entry == 0) {
    return;
  }
  for (i = 0; i < DYNAMITE_MAX; i++, p++) {
    if (p->entry == entry) {
      return;
    }
  }
  p = dynamite_list;
  for (i = 0; i < DYNAMITE_MAX; i++) {
    if (p->entry == 0) {
      p->entry = entry;
      p->id = (Uint8)twp->ang.x;
      p->pos = twp->pos;
      return;
    }
    p++;
  }
}

static void DynamiteListFree(task *tp) {
  dynamiteinfo *p;
  Sint32 entry;
  Sint32 i;

  if (!(lbl_803ADC10 & 1)) {
    return;
  }
  if (tp == NULL) {
    return;
  }
  // tested twice, as in the original
  if (tp->ocp == NULL) {
    return;
  }
  if (tp->ocp == NULL) {
    return;
  }
  if (tp->ocp->pObjEditEntry == NULL) {
    return;
  }
  entry = (Sint32)tp->ocp->pObjEditEntry;
  p = dynamite_list;
  if (entry == 0) {
    return;
  }
  for (i = 0; i < DYNAMITE_MAX; i++) {
    if (p->entry == entry) {
      p->entry = 0;
      return;
    }
    p++;
  }
}

void ObjectDynamite(task *tp) {
  LoadExplosionTexture();
  DynamiteListEntry(tp);
  if (CheckRangeOut(tp)) {
    return;
  }

  tp->fwp = syCalloc(1, 8);
  if (tp->fwp == NULL) {
    return;
  }

  tp->disp = ObjectDynamiteDisp;
  tp->disp_sort = ObjectDynamiteDispSort;
  tp->exec = ObjectDynamiteExec;
  tp->dest = ObjectDynamiteDest;
  CCL_Init(tp, _rename_dynamite_colli_info,
           ARYLEN(_rename_dynamite_colli_info), CID_ENEMY);
  fn_80018D28(tp);
}

static void ObjectDynamiteDest(task *tp) {
  taskwk *twp = tp->twp;

  fn_80018B88(tp);
  // a dynamite that was not blown up still scores
  if (tp->ocp != NULL && twp->btimer == 0 &&
      !(tp->ocp->ssCondition & 0x8001)) {
    AddScore(100);
    DynamiteListFree(tp);
  }
  syFree(tp->fwp);
  tp->fwp = NULL;
}

static void ObjectDynamiteExec(task *tp) {
  taskwk *twp = tp->twp;
  dynamitewk *wk;

  if (CheckRangeOut(tp)) {
    return;
  }

  // btimer runs the explosion out, then the task goes
  if (twp->btimer != 0) {
    if (twp->btimer > 20) {
      if (tp->ocp != NULL) {
        DeadOut(tp);
      } else {
        FreeTask(tp);
      }
    }
    twp->btimer++;
    return;
  }

  wk = (dynamitewk *)tp->mwp;
  if (((twp->flag & 4) && (wk->pno == 0 || wk->pno == 1)) ||
      ((twp->cwp->flag & 1) && twp->cwp->hit_cwp->mytask->twp->id == 7)) {
    twp->mode = 1;
    twp->btimer = 1;
    AddScore(100);
    ItemBombSetRange(&twp->pos, 100.0f);
    CreateExplosion(6, &twp->pos);
    fn_8006B7EC(0x100C, NULL, 0, 0x7F, &twp->pos);
    DynamiteListFree(tp);
    dynamite_break_count++;
    fn_8002FB2C(wk->pno, 2, 20, 0);
    if (tp->ocp != NULL) {
      Dead(tp);
    }
  } else {
    CCL_Entry(tp);
  }
}

static void ObjectDynamiteDisp(task *tp) {
  taskwk *twp = tp->twp;
  NJS_CNK_OBJECT *obj;

  njSetTexture(&_rename_dynamite_texlist);
  // assigned after the call, not initialised, to match
  obj = &_rename_dynamite_object;
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateY(NULL, twp->ang.y);
  // the low byte of ang.x is the list id, bits 8-15 are the tilt
  njRotateX(NULL, twp->ang.x & 0xFF00);
  if (twp->btimer == 0) {
    if (dynamite_dsdraw) {
      ds_DrawModelClip(_rename_dynamite_ObjArr[0].model);
    } else {
      fn_8011E158(obj->model);
    }
  }
  njDisableFog();
  gjSetFog();
  njSetTexture(&_rename_dynamite_lamp_texlist);
  _rename_CnkDrawModelColor(
      &_rename_dynamite_lamp_model,
      fn_800334B0(0xFF202020, 0xFFFFFFFF, _rename_GetBlinkRatio(0x12, 6, 5)));
  njPopMatrixEx();
  njEnableFog();
  gjSetFog();
}

static void ObjectDynamiteDispSort(task *tp) {
  taskwk *twp = tp->twp;
  NJS_CNK_OBJECT *obj;
  NJS_POINT3 pos;
  NJS_POINT3 spos;
  Float rect[8]; // x0 y0 x1 y1 u0 v0 u1 v1
  Float fade;
  Float size;
  Float ratio;
  Float bob;
  Uint32 color;

  njSetTexture(&_rename_dynamite_texlist);
  obj = &_rename_dynamite_object;
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, twp->ang.x & 0xFF00);

  // the blast: the body fades out white over the 20 frames btimer counts
  if (twp->btimer != 0) {
    fn_8002B304();
    fn_8002B35C();
    fade = 1.0f - (Float)twp->btimer / 20.0f;
    OffControl3D(0x220);
    OnControl3D(0x10);
    OnControl3D(0x800);
    fn_801218C8(0xFF, 0x900);
    fn_800156FC(0.5f + fade, 1.0f, fade, fade);
    njCnkCacheDrawModel(obj->model);
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
    fn_8002B348();
    fn_8002B2F8();
  }

  njDisableFog();
  gjSetFog();

  // the marker sprite above it, blinking red and bobbing with the blink
  ratio = _rename_GetBlinkRatio(0x16, 8, 5);
  color = fn_800334B0(0xFFFF0000, 0xFFFFFFFF, ratio);
  pos = _rename_dynamite_mark_ofs;
  size = _rename_dynamite_mark_size;
  if (_rename_CalcScreenPos(&pos, &spos)) {
    if (ratio > 0.5f) {
      bob = 0.5f * _rename_dynamite_mark_bob;
    } else {
      bob = 0.5f * -_rename_dynamite_mark_bob;
    }
    rect[0] = spos.x - size;
    rect[1] = bob + (spos.y - size);
    rect[2] = spos.x + size;
    rect[3] = bob + (spos.y + size);
    rect[4] = 0.0f;
    rect[5] = 0.0f;
    rect[6] = 1.0f;
    rect[7] = 1.0f;
    __njColorBlendingMode(0, 8);
    __njColorBlendingMode(1, 0xA);
    njSetTexture(&_rename_dynamite_mark_texlist);
    fn_80119FD8(1);
    fn_8011A3D4(0, color);
    fn_8011A280(rect, spos.z);
    fn_8011A27C();
    __njColorBlendingMode(0, 8);
    __njColorBlendingMode(1, 6);
  }

  njPopMatrixEx();
  njEnableFog();
  gjSetFog();
}
