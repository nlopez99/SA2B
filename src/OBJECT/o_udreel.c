#include "OBJECT/o_udreel.h"

#include "CCL.h"
#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njbasic.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/shadow.h"
#include "set.h"
#include "fabsf.h"

// mobile land collision entry: register / withdraw
extern void fn_800232A4(Uint32 attr, task *tp, NJS_OBJECT *object);
extern void fn_800231CC(task *tp, NJS_OBJECT *object);
extern s32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, float);
extern void _rename_RingDrawShadowModel(void);
extern void ds_DrawModelClip(NJS_MODEL *);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern void fn_800068E4(colliwk *cwp);
extern void fn_800399BC(Sint32 pno, Float x, Float y, Float z);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);

extern BOOL DisableObjectFog;

extern NJS_TEXLIST _rename_udreel_texlist;
extern GJS_OBJECT  _rename_udreel_object; // reel, child: rope, child: handle
extern NJS_OBJECT  _rename_udreel_col_object;

// ^ extern
// v in this file

static void ObjectUDReelDest(task *tp);
static void ObjectUDReelExec(task *tp);
static void ObjectUDReelDisp(task *tp);
static void ObjectUDReelDispDely(task *tp);
static void ObjectUDReelDispDS(task *tp);

typedef struct udreelwk // sizeof=0x4C
{
  /* 0x00 */ NJS_OBJECT object; // collision of the reel
  /* 0x38 */ Float pos;         // how far the rope is let out
  /* 0x3C */ Float spd;
  /* 0x40 */ Float handle_y;
  /* 0x44 */ Float rope_scl;
  /* 0x48 */ Float shadow_y;
} udreelwk;

typedef struct dsmodel // sizeof=0x10
{
  /* 0x00 */ Sint32 flag;
  /* 0x04 */ NJS_TEXLIST *texlist;
  /* 0x08 */ void *unk_8;
  /* 0x0C */ NJS_MODEL *model;
} dsmodel;

#define GetWork(task) ((udreelwk *)task->mwp)
#define GetObject(task) (&GetWork(task)->object)
#define GetPlayerNum(twp) ((Sint16)twp->wtimer)
// twp->scl.x / twp->scl.y: upper and lower stop, in units of 20
#define GetTopY(twp) (-20.0f * twp->scl.x)
#define GetBottomY(twp) (-20.0f * (1.0f + twp->scl.y))
#define GetLength(task)                                                      \
  ((GetWork(task)->pos > -7.0f) ? 8.0f + GetWork(task)->pos : 1.0f)
#define GetHandleY(task) (-3.5f - GetLength(task))

enum {
  FLAG_HOLD = 1,    // twp->mode: a player is hanging on the handle
  FLAG_RELEASE = 2, // twp->mode: throw the player off
  FLAG_TOP = 4,     // twp->mode: the handle passed the upper stop
  FLAG_BOTTOM = 8,  // twp->mode: the handle passed the lower stop
};

enum {
  MD_BOTTOM,
  MD_START,
  MD_UP,
  MD_TOP,
  MD_DOWN,
};

enum {
  ARG_RETURN = 1, // twp->ang.x: goes back down, otherwise it waits at the top
};

static BOOL udreel_dsdraw = FALSE;

// how far below the handle each character hangs
Float udreel_hold_ofs_sonic = 16.3f;
Float udreel_hold_ofs_shadow = 16.3f;
Float udreel_hold_ofs_knuckles = 17.7f;
Float udreel_hold_ofs_rouge = 15.4f;
Float udreel_hold_ofs_eggwalker = 34.8f;
Float udreel_hold_ofs_tailswalker = 35.0f;
Float udreel_hold_ofs_amy = 16.2f;
Float udreel_hold_ofs_metalsonic = 16.8f;
Float udreel_hold_ofs_tical = 15.8f;
Float udreel_hold_ofs_chaos0 = 17.5f;
Float udreel_hold_ofs_darkwalker = 34.9f;
Float udreel_hold_ofs_chaowalker = 34.5f;

NJS_TEXLIST *udreel_texlists[] = {&_rename_udreel_texlist, NULL};

dsmodel udreel_ObjArr[] = {
    {1, &_rename_udreel_texlist, NULL, NULL},
    {0, &_rename_udreel_texlist, NULL, NULL},
    {0, &_rename_udreel_texlist, NULL, NULL},
};

CCL_INFO udreel_colli_info[] = {
    {0, CI_FORM_SPHERE, 0xF0, 0, 0x8000, {0.0f, 0.0f, 0.0f}, 8.0f, 0.0f, 0.0f,
     0.0f, 0, 0, 0},
};

Float udreel_spd_max = 2.5f;
Float udreel_acc = 0.1f;

void ObjectUDReel(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object;
  Uint32 attr;
  Float r;
  Float top;
  Float bottom;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(udreelwk));
  if (tp->mwp == NULL) {
    return;
  }

  if (udreel_dsdraw) {
    tp->disp = ObjectUDReelDispDS;
  } else {
    tp->disp = ObjectUDReelDisp;
    tp->disp_dely = ObjectUDReelDispDely;
  }
  tp->exec = ObjectUDReelExec;
  tp->dest = ObjectUDReelDest;

  top = twp->scl.x;
  bottom = twp->scl.y;
  if (bottom < top) {
    twp->scl.x = bottom;
    twp->scl.y = top;
  } else {
    twp->scl.x = top;
    twp->scl.y = bottom;
  }

  twp->smode = MD_BOTTOM;
  GetWork(tp)->pos = -GetBottomY(twp);
  GetWork(tp)->spd = 0.0f;
  twp->mode = 0;
  CCL_Init(tp, udreel_colli_info, ARYLEN(udreel_colli_info), CID_OBJECT);

  object = GetObject(tp);
  *object = _rename_udreel_col_object;
  object->evalflags &= ~3;
  object->ang.y = twp->ang.y;
  object->ang.z = 0;
  object->ang.x = 0;
  object->pos.x = twp->pos.x;
  object->pos.y = twp->pos.y;
  object->pos.z = twp->pos.z;

  attr = 1;
  r = 5.0f + object->model->r;
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

  if (!(twp->ang.x & ARG_RETURN)) {
    GetWork(tp)->spd = 0.0f;
    GetWork(tp)->pos = -GetTopY(twp);
    twp->smode = MD_TOP;
  }
  GetWork(tp)->shadow_y = -1000000.0f;
}

static void ObjectUDReelDest(task *tp) {
  if (GetObject(tp)->model != NULL) {
    fn_800231CC(tp, GetObject(tp));
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectUDReelExec(task *tp) {
  taskwk *twp = tp->twp;
  Angle3 ang;
  NJS_VECTOR spd;
  NJS_VECTOR unused; // unused
  Float d;
  Float posy;

  if (!(twp->mode & FLAG_HOLD) && CheckRangeOut(tp)) {
    return;
  }

  if (lbl_801CC168._42 == 1) {
    twp->mode &= ~(FLAG_HOLD | FLAG_RELEASE);
  }

  // look for the ground every 16 frames until there is one
  if (-1000000.0f == GetWork(tp)->shadow_y && tp->work.l > 15) {
    posy = GetShadowPos(twp->pos.x, twp->pos.y + GetHandleY(tp), twp->pos.z,
                        &ang);
    if (-1000000.0f != posy) {
      GetWork(tp)->shadow_y = posy;
    }
    tp->work.l = 0;
  } else {
    tp->work.l++;
  }

  if (twp->mode & FLAG_RELEASE) {
    twp->mode &= ~(FLAG_HOLD | FLAG_RELEASE);
    spd.y = 1.1f + twp->scl.z;
    spd.x = 0.0f;
    spd.z = 0.0f;
    njPushMatrixEx();
    njUnitMatrix(NULL);
    njRotateY(NULL, twp->ang.y);
    njRotateZ(NULL, twp->ang.z + 0x3400);
    njCalcVector(NULL, &spd, &spd);
    njPopMatrixEx();
    playertwp[GetPlayerNum(twp)]->ang.z = 0;
    playertwp[GetPlayerNum(twp)]->ang.x = 0;
    playertwp[GetPlayerNum(twp)]->ang.y = 0x8000 - twp->ang.y;
    SetVelocityP(GetPlayerNum(twp), spd.x, spd.y, spd.z);
  }

  GetWork(tp)->handle_y = GetHandleY(tp);
  GetWork(tp)->rope_scl = GetLength(tp) / 20.0f;

  // case order as in the original
  switch (twp->smode) {
  case MD_UP:
    if (twp->mode & FLAG_TOP) {
      // swing around the upper stop
      GetWork(tp)->pos += GetWork(tp)->spd;
      d = GetWork(tp)->pos + GetTopY(twp);
      if (GetWork(tp)->pos < -5.0f) {
        GetWork(tp)->spd = fabsf(GetWork(tp)->spd);
        GetWork(tp)->pos = -5.0f;
      }
      GetWork(tp)->spd = 0.92f * GetWork(tp)->spd - 0.1f * d;
      if (!(twp->mode & FLAG_HOLD)) {
        GetWork(tp)->spd = 0.0f;
        GetWork(tp)->pos = -GetTopY(twp);
        twp->smode = (twp->ang.x & ARG_RETURN) ? MD_DOWN : MD_TOP;
        twp->mode &= ~FLAG_TOP;
      }
    } else {
      NJS_POINT3 pos;
      STACK_PAD_VAR(1);

      GetWork(tp)->spd += -udreel_spd_max * udreel_acc;
      if (GetWork(tp)->spd < -udreel_spd_max) {
        GetWork(tp)->spd = -udreel_spd_max;
      }
      GetWork(tp)->pos += GetWork(tp)->spd;
      if (GetWork(tp)->pos < -GetTopY(twp)) {
        twp->mode |= FLAG_TOP;
      }
      pos = twp->pos;
      pos.y += GetHandleY(tp);
      fn_8006AFFC(0x101D, twp, 1, 0, 10, &pos);
    }
    if ((twp->mode & FLAG_HOLD) &&
        playertwp[GetPlayerNum(twp)]->mode != 51) {
      twp->mode |= FLAG_RELEASE;
      fn_8002FB2C(GetPlayerNum(twp), 4, 15, 0);
    }
    break;
  case MD_TOP:
    GetWork(tp)->spd = 0.0f;
    GetWork(tp)->pos = -GetTopY(twp);
    if (twp->mode & FLAG_HOLD) {
      GetWork(tp)->spd = 0.0f;
      twp->smode = MD_DOWN;
    }
    break;
  case MD_DOWN:
    if (twp->mode & FLAG_BOTTOM) {
      // swing around the lower stop
      GetWork(tp)->pos += GetWork(tp)->spd;
      GetWork(tp)->spd = 0.92f * GetWork(tp)->spd -
                         0.1f * (GetWork(tp)->pos + GetBottomY(twp));
      if (!(twp->mode & FLAG_HOLD)) {
        GetWork(tp)->spd = 0.0f;
        GetWork(tp)->pos = -GetBottomY(twp);
        twp->smode = MD_BOTTOM;
        twp->mode &= ~FLAG_BOTTOM;
      }
    } else {
      NJS_POINT3 pos;
      STACK_PAD_VAR(2);

      GetWork(tp)->pos += GetWork(tp)->spd;
      GetWork(tp)->spd += udreel_spd_max * udreel_acc;
      if (GetWork(tp)->spd > udreel_spd_max) {
        GetWork(tp)->spd = udreel_spd_max;
      }
      if (GetWork(tp)->pos >= -GetBottomY(twp)) {
        twp->mode |= FLAG_BOTTOM;
      }
      pos = twp->pos;
      pos.y += GetHandleY(tp);
      fn_8006AFFC(0x101D, twp, 1, 0, 10, &pos);
    }
    if ((twp->mode & FLAG_HOLD) &&
        playertwp[GetPlayerNum(twp)]->mode != 51) {
      twp->mode |= FLAG_RELEASE;
      fn_8002FB2C(GetPlayerNum(twp), 4, 15, 0);
    }
    break;
  case MD_START:
    // the grab pulls the handle down a little first
    GetWork(tp)->pos += GetWork(tp)->spd;
    GetWork(tp)->spd =
        GetWork(tp)->spd - 0.02f * (GetWork(tp)->pos + GetBottomY(twp));
    if (GetWork(tp)->spd < 0.0f) {
      GetWork(tp)->spd += -udreel_spd_max * udreel_acc;
      if (GetWork(tp)->spd < -udreel_spd_max) {
        GetWork(tp)->spd = -udreel_spd_max;
      }
      twp->smode = MD_UP;
      twp->mode &= ~FLAG_TOP;
    }
    break;
  case MD_BOTTOM:
    if (twp->mode & FLAG_HOLD) {
      twp->smode = MD_START;
    }
    break;
  }

  if (GetObject(tp)->model != NULL) {
    if (_rename_EitherPlayerWithinSphere(&GetObject(tp)->pos,
                                         32.0f + GetObject(tp)->model->r)) {
      twp->flag |= 0x100;
    } else {
      twp->flag &= ~0x100;
    }
  }

  if (!(twp->mode & FLAG_HOLD)) {
    Sint32 pno;

    if ((twp->cwp->flag & 1) && twp->btimer > 60 &&
        twp->cwp->hit_cwp->id == CID_PLAYER &&
        (pno = IsThisTaskPlayer(twp->cwp->hit_cwp->mytask)) >= 0) {
      twp->mode |= FLAG_HOLD;
      twp->wtimer = (Sint16)pno;
      GetWork(tp)->spd = 1.0f;
      SetInputP(GetPlayerNum(twp), 20, 75);
      fn_8002FB2C(GetPlayerNum(twp), 5, 11, 0);
      return;
    }

    twp->cwp->info->center.y = GetHandleY(tp) - 1.0f;
    fn_800068E4(twp->cwp);
    CCL_Entry(tp);
    // no grabbing again for 60 frames after letting go
    if (twp->btimer <= 60) {
      twp->cwp->info->attr |= 0x10;
      twp->btimer++;
      return;
    }
    twp->cwp->info->attr &= ~0x10;
    return;
  }

  {
    NJS_POINT3 pos;
    Float dang;
    STACK_PAD_VAR(8);

    pos = twp->pos;
    pos.y += GetHandleY(tp);
    switch (playerpwp[GetPlayerNum(twp)]->character) {
    case PLNO_SONIC:
      pos.y -= udreel_hold_ofs_sonic;
      break;
    case PLNO_SHADOW:
      pos.y -= udreel_hold_ofs_shadow;
      break;
    case PLNO_KNUCKLES:
      pos.y -= udreel_hold_ofs_knuckles;
      break;
    case PLNO_ROUGE:
      pos.y -= udreel_hold_ofs_rouge;
      break;
    case PLNO_EGG_WALKER:
      pos.y -= udreel_hold_ofs_eggwalker;
      break;
    case PLNO_TAILS_WALKER:
      pos.y -= udreel_hold_ofs_tailswalker;
      break;
    case PLNO_AMY:
      pos.y -= udreel_hold_ofs_amy;
      break;
    case PLNO_METAL_SONIC:
      pos.y -= udreel_hold_ofs_metalsonic;
      break;
    case PLNO_TICAL:
      pos.y -= udreel_hold_ofs_tical;
      break;
    case PLNO_CHAOS0:
      pos.y -= udreel_hold_ofs_chaos0;
      break;
    case PLNO_DARK_WALKER:
      pos.y -= udreel_hold_ofs_darkwalker;
      break;
    case PLNO_CHAO_WALKER:
      pos.y -= udreel_hold_ofs_chaowalker;
      break;
    default:
      pos.y -= 16.5f;
      break;
    }

    // pull the player under the handle and turn them to face the reel
    pos.x += 0.35f * (playertwp[GetPlayerNum(twp)]->pos.x - pos.x);
    pos.z += 0.35f * (playertwp[GetPlayerNum(twp)]->pos.z - pos.z);
    dang = (Sint16)((0x8000 - twp->ang.y -
                     playertwp[GetPlayerNum(twp)]->ang.y) &
                    0xFFFF);
    dang *= 0.35f;
    fn_800399BC(GetPlayerNum(twp), pos.x, pos.y, pos.z);
    playertwp[GetPlayerNum(twp)]->ang.y += (Sint16)dang;
    playertwp[GetPlayerNum(twp)]->ang.z = 0;
    playertwp[GetPlayerNum(twp)]->ang.x = 0;
    twp->btimer = 0;
  }
}

static void ObjectUDReelDisp(task *tp) {
  taskwk *twp = tp->twp;
  GJS_OBJECT *object;
  GJS_OBJECT *child;
  Float r;

  njSetTexture(&_rename_udreel_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  object = &_rename_udreel_object;
  OnControl3D(0x2400);
  gjDrawModel(object->model);
  OffControl3D(0x2400);

  // the rope is stretched, so it must not be clipped by its radius
  child = object->child;
  njPushMatrixEx();
  r = child->model->r;
  child->model->r = 0.0f;
  njScale(NULL, 1.0f, GetWork(tp)->rope_scl, 1.0f);
  gjDrawModel(child->model);
  child->model->r = r;
  njPopMatrixEx();

  child = child->child;
  njTranslate(NULL, 0.0f, GetWork(tp)->handle_y, 0.0f);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  gjDrawModel(child->model);
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
  njPopMatrixEx();
}

static void ObjectUDReelDispDely(task *tp) {
  taskwk *twp = tp->twp;

  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x, 0.5f + GetWork(tp)->shadow_y, twp->pos.z);
  njScale(NULL, 3.6f, 1.0f, 3.6f);
  _rename_RingDrawShadowModel();
  njPopMatrixEx();
}

static void ObjectUDReelDispDS(task *tp) {
  taskwk *twp = tp->twp;
  GJS_OBJECT *child;
  Float r;

  njSetTexture(&_rename_udreel_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  ds_DrawModelClip(udreel_ObjArr[0].model);
  OffControl3D(0x2400);

  child = _rename_udreel_object.child;
  njPushMatrixEx();
  r = child->model->r;
  child->model->r = 0.0f;
  njScale(NULL, 1.0f, GetWork(tp)->rope_scl, 1.0f);
  gjDrawModel(child->model);
  child->model->r = r;
  njPopMatrixEx();

  child = child->child;
  njTranslate(NULL, 0.0f, GetWork(tp)->handle_y, 0.0f);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  // the handle blinks while nobody holds it
  if (lbl_801CC168._7C % 46 < 23 || (twp->mode & FLAG_HOLD)) {
    ds_DrawModelClip(udreel_ObjArr[2].model);
  } else {
    gjDrawModel(child->model);
  }
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
  njPopMatrixEx();

  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x, 0.5f + GetWork(tp)->shadow_y, twp->pos.z);
  njScale(NULL, 3.6f, 1.0f, 3.6f);
  _rename_RingDrawShadowModel();
  njPopMatrixEx();
}
