#include "OBJECT/o_bigjump.h"

#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "types.h"

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

// mobile land object: allocate / free, register / withdraw
extern NJS_OBJECT *fn_8002314C(void);
extern void fn_80023108(NJS_OBJECT *object);
extern void fn_800232A4(Uint32 attr, task *tp, NJS_OBJECT *object);
extern void fn_800231CC(task *tp, NJS_OBJECT *object);

extern Sint32 _rename_EitherPlayerWithinSphere(NJS_POINT3 *pos, Float r);
// launch the player: negative and positive spin counts
extern void fn_80039E48(Sint32 pno, NJS_POINT3 *spd, Angle3 *ang, Sint32 num);
extern void fn_80039EB0(Sint32 pno, NJS_POINT3 *spd, Angle3 *ang, Sint32 num);
extern void fn_8002FB2C(Sint32 pno, Sint32 a2, Sint32 a3, Sint32 a4);
extern void fn_80069D60(Sint32 tone, void *id, Sint32 pri, Sint32 volume,
                        Sint32 timer, NJS_POINT3 *pos);
extern void fn_80073028(NJS_CNK_MODEL *model, void *info, Uint32 frame);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern Float ceilf(Float f);

extern NJS_OBJECT    _rename_bigjump_col_object;
extern NJS_OBJECT    _rename_bigjump_col_object_child;
extern NJS_TEXLIST   _rename_bigjump_texlist;
extern NJS_CNK_MODEL _rename_bigjump_model;
extern NJS_TEXLIST   _rename_bigjump_arrow_texlist;
extern NJS_CNK_MODEL _rename_bigjump_arrow_model;
extern uvanim_info   _rename_bigjump_arrow_uvanim;

// ^ extern
// v in this file

static void ObjectBigJumpInit(task *tp);
static void ObjectBigJumpExec(task *tp);
static void ObjectBigJumpDisp(task *tp);
static void ObjectBigJumpDest(task *tp);

// the two per-player retrigger timers live in the unused mwp
#define GetTimer(task) ((Sint8 *)&(task)->mwp)
#define GetObject(task) ((NJS_OBJECT *)(task)->awp)

enum {
  MD_INIT = 0,
  MD_RUN = 1,
};

// launches the player standing on the panel, then keeps launching for 20 frames
#define BigJumpPlayer(tp, pno)                                                \
  if (GetTimer(tp)[pno] <= 0) {                                               \
    if (playertwp[pno] != NULL && (playertwp[pno]->flag & 3) &&               \
        playerpwp[pno]->mlotp == (tp)) {                                      \
      GetTimer(tp)[pno] = 20;                                                 \
      ObjectBigJumpJump(tp, pno);                                             \
    }                                                                         \
  } else {                                                                    \
    ObjectBigJumpJump(tp, pno);                                               \
    GetTimer(tp)[pno]--;                                                      \
  }

void ObjectBigJump(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case MD_INIT:
    tp->disp = ObjectBigJumpDisp;
    tp->dest = ObjectBigJumpDest;
    ObjectBigJumpInit(tp);
    break;
  case MD_RUN:
    ObjectBigJumpExec(tp);
    break;
  default:
    twp->mode = MD_INIT;
    break;
  }
}

static void ObjectBigJumpInit(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object;
  Uint32 attr;
  Float r;

  twp->mode = MD_RUN;
  object = fn_8002314C();
  tp->awp = (anywk *)object;

  *object = _rename_bigjump_col_object;
  object->evalflags &= ~3;
  object->scl[0] = 1.0f;
  object->scl[1] = 1.0f;
  object->scl[2] = 1.0f;
  object->ang.x = twp->ang.x;
  object->ang.y = twp->ang.y;
  object->ang.z = twp->ang.z;
  object->pos.x = twp->pos.x;
  object->pos.y = twp->pos.y;
  object->pos.z = twp->pos.z;
  object->model = _rename_bigjump_col_object.model;
  object->child = &_rename_bigjump_col_object_child;
  object->sibling = NULL;

  r = 32.0f + object->model->r;
  attr = 0x00000101;
  if (r > 200.0f) {
    attr |= 0x00000000;
  } else if (r > 100.0f) {
    attr |= 0x20000000;
  } else if (r > 30.0f) {
    attr |= 0x40000000;
  } else {
    attr |= 0x60000000;
  }
  fn_800232A4(attr, tp, object);

  GetTimer(tp)[0] = 0;
  GetTimer(tp)[1] = 0;
}

static void ObjectBigJumpJump(task *tp, Sint32 pno) {
  taskwk *twp = tp->twp;
  Sint32 num;
  Angle3 ang;
  NJS_POINT3 spd;
  STACK_PAD_VAR(3);

  num = (Sint32)ceilf(twp->scl.y);
  spd.x = twp->scl.x;
  spd.y = 3.2f + twp->scl.z;
  spd.z = 0.0f;
  ang.x = 0;
  ang.y = twp->ang.y + 0x8000;
  ang.z = 0;

  if (num < 0) {
    fn_80039EB0(pno, &spd, &ang, -num);
  } else {
    fn_80039E48(pno, &spd, &ang, num);
  }

  fn_80069D60(0x1003, tp, 1, 0, 0x78, &tp->twp->pos);
  if (GetTimer(tp)[pno] == 20) {
    fn_8002FB2C(pno, 4, 15, 0);
  }
}

static void ObjectBigJumpExec(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object = GetObject(tp);

  object->pos.x = twp->pos.x;
  object->pos.y = twp->pos.y;
  object->pos.z = twp->pos.z;
  object->ang.x = twp->ang.x;
  object->ang.y = twp->ang.y;
  object->ang.z = twp->ang.z;

  if (_rename_EitherPlayerWithinSphere(&twp->pos, 32.0f + object->model->r)) {
    twp->flag |= 0x100;
    BigJumpPlayer(tp, 0);
    BigJumpPlayer(tp, 1);
  } else {
    twp->flag &= ~0x100;
    GetTimer(tp)[0] = 0;
    GetTimer(tp)[1] = 0;
  }
}

static void ObjectBigJumpDisp(task *tp) {
  taskwk *twp = tp->twp;

  njPushMatrix(NULL);
  njTranslateV(NULL, &twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateY(NULL, twp->ang.y);
  njRotateX(NULL, twp->ang.x);

  njSetTexture(&_rename_bigjump_texlist);
  fn_8011E158(&_rename_bigjump_model);

  njSetTexture(&_rename_bigjump_arrow_texlist);
  fn_80073028(&_rename_bigjump_arrow_model, &_rename_bigjump_arrow_uvanim,
              lbl_801CC168._7C);
  fn_8011E158(&_rename_bigjump_arrow_model);

  njPopMatrix(1);
}

static void ObjectBigJumpDest(task *tp) {
  fn_800231CC(tp, GetObject(tp));
  fn_80023108(GetObject(tp));
  tp->awp = NULL;
  tp->mwp = NULL;
}
