#include "OBJECT/o_bunchin.h"

#include "CCL.h"
#include "samt/ninja/gjdraw.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njcollision.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "fabsf.h"

inline float sqrtf(float f);

// mobile land collision entry: register / withdraw
extern void fn_800232A4(Uint32 attr, task *tp, NJS_OBJECT *object);
extern void fn_800231CC(task *tp, NJS_OBJECT *object);
extern s32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, float);
extern BOOL _rename_IsSwitchOn(Uint8 id);
extern void _rename_CreateBoxDust(NJS_POINT3 *pos, Float x, Float z,
                                  Angle3 *ang, Float scl, Float interval,
                                  Float spd);
extern void _rename_CnkDrawModelColor(NJS_CNK_MODEL *model, Uint32 color);
extern Float _rename_GetBlinkRatio(Sint32 cycle, Sint32 on, Sint32 fade);
extern void _rename_ShadowTexInit(void *, Sint32);
extern void _rename_ShadowTexFree(void *);
extern void _rename_ShadowTexBegin(void *, Float, Float, void *, void *,
                                   void *);
extern void _rename_ShadowTexEnd(void *);
extern void _rename_ShadowTexDraw(Sint32, NJS_VECTOR *, Float, void *);
extern void ds_DrawModelClip(NJS_MODEL *);
extern task *fn_80005F88(task *tp, Sint32 num);
extern void fn_800068E4(colliwk *cwp);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern Uint32 fn_800334B0(Uint32 col0, Uint32 col1, Float ratio);
// player position history: set / get
extern void fn_80037F64(Sint32 pno, Sint32 num, NJS_POINT3 *pos, Angle3 *ang);
extern void fn_80038008(Sint32 pno, Sint32 num, NJS_POINT3 *pos, Angle3 *ang);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);

extern NJS_TEXLIST   _rename_bunchin_top_texlist;
extern NJS_CNK_MODEL _rename_bunchin_top_model;
extern NJS_TEXLIST   _rename_bunchin_texlist;
extern NJS_CNK_MODEL _rename_bunchin_model;
extern GJS_MODEL     _rename_bunchin_shadow_model;
extern NJS_OBJECT    _rename_bunchin_col_object;
extern NJS_TEXLIST   _rename_bunchin_lamp_texlist;
extern NJS_CNK_MODEL _rename_bunchin_lamp2_model;
extern NJS_CNK_MODEL _rename_bunchin_lamp_model;
extern NJS_MODEL    *_rename_bunchin_ObjArr[4];

extern Float    _rename_bunchin_rise_spd;
extern Float    _rename_bunchin_gravity;
extern Float    _rename_bunchin_fall_ratio;
extern Float    _rename_bunchin_bound_ratio;
extern Float    _rename_bunchin_bound_spring;
extern CCL_INFO _rename_bunchin_colli_info[3];
extern Float    _rename_bunchin_dust_interval;
extern Float    _rename_bunchin_dust_spd;
extern Float    _rename_bunchin_dust_scl;
extern Uint32   _rename_bunchin_wait_time;
extern Sint32   _rename_bunchin_se_vol;
extern Angle    _rename_bunchin_lamp_spd;

// ^ extern
// v in this file

static void ObjectBunchinDest(task *tp);
static void ObjectBunchinExec(task *tp);
static void ObjectBunchinDisp(task *tp);
static void ObjectBunchinDispShad(task *tp);

typedef struct bunchinwk // sizeof=0x98
{
  /* 0x00 */ Float posy; // height above the ground
  /* 0x04 */ NJS_OBJECT object;
  /* 0x3C */ Float fall; // height of the player it falls on, 0 or below
  /* 0x40 */ Uint8 shadow[0x58];
} bunchinwk;

#define GetWork(task) ((bunchinwk *)task->mwp)
#define GetObject(task) (&GetWork(task)->object)
#define GetType(twp) (twp->ang.x & 3)
#define GetSwitch(twp) ((Uint8)twp->ang.z)
#define GetTop(twp) (30.0f + fabsf(twp->scl.y))
#define GetWidth(s) (40.0f * (1.0f + (s)))
#define GetHalfWidth(s) (0.5f * GetWidth(s))
// is the player below the weight, whatever the height
#define IsPlayerBelow(twp, pno)                                               \
  BunchinCheckInBox(&playertwp[pno]->pos, &twp->pos, twp->ang.y,              \
                    20.0f + GetHalfWidth(twp->scl.x), 1000.0f,                \
                    20.0f + GetHalfWidth(twp->scl.z))
#define IsNoPlayerBelow(twp)                                                  \
  ((playertwp[0] == NULL || !IsPlayerBelow(twp, 0)) &&                        \
   (playertwp[1] == NULL || !IsPlayerBelow(twp, 1)))

enum {
  TYPE_ALWAYS = 0,        // falls again as soon as it is back up
  TYPE_PLAYER = 1,        // falls on a player below it
  TYPE_PLAYER_SWITCH = 2, // same, and stays up while the switch is on
  TYPE_SWITCH = 3,        // falls when the switch is off
};

enum {
  MD_WAIT = 0,
  MD_UP = 1,
  MD_FALL = 2,
  MD_GROUND = 3,
};

static BOOL bunchin_dsdraw = FALSE;

static BOOL BunchinCheckInBox(NJS_POINT3 *pos, NJS_POINT3 *center, Angle ang,
                              Float x, Float y, Float z) {
  NJS_VECTOR v;

  v.x = center->x - pos->x;
  v.y = center->y - pos->y;
  v.z = center->z - pos->z;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njRotateY(NULL, -ang);
  njCalcVector(NULL, &v, &v);
  njPopMatrixEx();
  if (v.x < x && v.x > -x && v.y < y && v.y > -y && v.z < z && v.z > -z) {
    return TRUE;
  }
  return FALSE;
}

void ObjectBunchin(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object;
  Uint32 attr;
  Float r;

  if (CheckRangeOut(tp)) {
    return;
  }
  tp->mwp = syCalloc(1, sizeof(bunchinwk));
  if (tp->mwp == NULL) {
    return;
  }

  tp->disp = ObjectBunchinDisp;
  tp->exec = ObjectBunchinExec;
  tp->dest = ObjectBunchinDest;
  tp->disp_shad = ObjectBunchinDispShad;
  twp->smode = MD_WAIT;
  GetWork(tp)->posy = GetTop(twp);
  if (twp->scl.x > 300.0f) {
    twp->scl.x = 300.0f;
  }
  if (twp->scl.x < 0.0f) {
    twp->scl.x = 0.0f;
  }
  if (twp->scl.z > 300.0f) {
    twp->scl.z = 300.0f;
  }
  if (twp->scl.z < 0.0f) {
    twp->scl.z = 0.0f;
  }
  _rename_ShadowTexInit(GetWork(tp)->shadow, 0x20);

  object = GetObject(tp);
  *object = _rename_bunchin_col_object;
  object->evalflags &= ~7;
  object->scl[0] = 1.0f + twp->scl.x;
  object->scl[1] = 1.0f;
  object->scl[2] = 1.0f + twp->scl.z;
  object->ang.y = twp->ang.y;
  object->pos.x = twp->pos.x;
  object->pos.y = 20.0f + twp->pos.y + GetWork(tp)->posy;
  object->pos.z = twp->pos.z;

  r = 10.0f + object->model->r * (1.0f + MAX(twp->scl.x, twp->scl.z));
  attr = 0x08000001;
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

  CCL_Init(tp, _rename_bunchin_colli_info, ARYLEN(_rename_bunchin_colli_info),
           CID_OBJECT);
  twp->cwp->info[1].a = GetHalfWidth(twp->scl.x);
  twp->cwp->info[1].c = GetHalfWidth(twp->scl.z);
  twp->cwp->info[2].a = GetHalfWidth(twp->scl.x);
  twp->cwp->info[2].c = GetHalfWidth(twp->scl.z);
  twp->cwp->info[1].attr |= 0x10;
  twp->cwp->info[2].attr |= 0x10;
  twp->ang.x &= 0x103;
  twp->ang.z &= 0x10FF;
}

static void ObjectBunchinDest(task *tp) {
  _rename_ShadowTexFree(GetWork(tp)->shadow);
  if (GetObject(tp)->model != NULL) {
    fn_800231CC(tp, GetObject(tp));
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectBunchinExec(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object;
  task *hit_tp;
  Sint32 pno;
  Float posy;
  Float r;
  NJS_POINT3 pos;
  STACK_PAD_VAR(2);

  if (CheckRangeOut(tp)) {
    return;
  }

  // nested to match
  if (!lbl_801CC168._37) {
    switch (twp->smode) {
    case MD_WAIT:
      switch (GetType(twp)) {
      case TYPE_PLAYER_SWITCH:
        if (_rename_IsSwitchOn(GetSwitch(twp))) {
          GetWork(tp)->fall = 0.0f;
          twp->smode = MD_UP;
          break;
        }
        // fallthrough
      case TYPE_PLAYER:
        if (IsPlayerBelow(twp, 0) && (playertwp[0]->flag & 3)) {
          GetWork(tp)->fall = playertwp[0]->pos.y - twp->pos.y;
          if (GetWork(tp)->fall > 0.0f) {
            GetWork(tp)->fall = 0.0f;
          }
          twp->smode = MD_FALL;
        } else if (playertwp[1] != NULL && IsPlayerBelow(twp, 1) &&
                   (playertwp[1]->flag & 3)) {
          GetWork(tp)->fall = playertwp[1]->pos.y - twp->pos.y;
          if (GetWork(tp)->fall > 0.0f) {
            GetWork(tp)->fall = 0.0f;
          }
          twp->smode = MD_FALL;
        }
        break;
      case TYPE_ALWAYS:
        GetWork(tp)->fall = 0.0f;
        twp->smode = MD_FALL;
        break;
      case TYPE_SWITCH:
        if (!_rename_IsSwitchOn(GetSwitch(twp))) {
          GetWork(tp)->fall = 0.0f;
          twp->smode = MD_FALL;
        }
        break;
      }
      twp->smode != MD_FALL; // no effect, as in the original
      break;

    case MD_UP:
      GetWork(tp)->posy += _rename_bunchin_rise_spd;
      if (GetType(twp) != TYPE_PLAYER_SWITCH ||
          !_rename_IsSwitchOn(GetSwitch(twp))) {
        if ((GetType(twp) != TYPE_PLAYER_SWITCH &&
             GetType(twp) != TYPE_PLAYER) ||
            IsNoPlayerBelow(twp)) {
          if (GetWork(tp)->posy > GetTop(twp)) {
            GetWork(tp)->posy = GetTop(twp);
            twp->smode = MD_WAIT;
          }
        } else {
          twp->smode = MD_FALL;
        }
      } else if (GetWork(tp)->posy > GetTop(twp)) {
        GetWork(tp)->posy = GetTop(twp);
      }
      break;

    case MD_FALL:
      GetWork(tp)->posy += tp->work.f;
      tp->work.f =
          -_rename_bunchin_gravity + tp->work.f * _rename_bunchin_fall_ratio;
      if (GetWork(tp)->posy < 0.0f) {
        GetWork(tp)->posy = 0.0f;
        twp->smode = MD_GROUND;
        _rename_CreateBoxDust(&twp->pos, GetHalfWidth(twp->scl.x) - 5.0f,
                              GetHalfWidth(twp->scl.z) - 5.0f, &twp->ang,
                              _rename_bunchin_dust_scl,
                              _rename_bunchin_dust_interval,
                              _rename_bunchin_dust_spd);
        if (playertwp[0] != NULL) {
          r = 1.5f * sqrtf(GetWidth(twp->scl.x) * GetWidth(twp->scl.x) +
                           GetWidth(twp->scl.z) * GetWidth(twp->scl.z));
          if (njDistanceP2P(&playertwp[0]->pos, &twp->pos) < r) {
            fn_8002FB2C(0, 5, 20, 0);
          }
        }
        if (playertwp[1] != NULL) {
          r = 1.5f * sqrtf(GetWidth(twp->scl.x) * GetWidth(twp->scl.x) +
                           GetWidth(twp->scl.z) * GetWidth(twp->scl.z));
          if (njDistanceP2P(&playertwp[1]->pos, &twp->pos) < r) {
            fn_8002FB2C(1, 5, 20, 0);
          }
        }
        twp->wtimer = 0;
        if (twp->cwp != NULL) {
          twp->cwp->info[1].attr |= 0x10;
          twp->cwp->info[2].attr |= 0x10;
        }
        fn_8006B7EC(0x1004, NULL, 0, (Sint16)_rename_bunchin_se_vol,
                    &twp->pos);
      } else if (twp->cwp != NULL) {
        twp->cwp->info[1].attr &= ~0x10;
        twp->cwp->info[2].attr &= ~0x10;
        twp->cwp->info[1].center.y = 7.5f + GetWork(tp)->posy;
        twp->cwp->info[2].center.y = GetWork(tp)->fall - 7.5f - 1.0f;
      }
      break;

    case MD_GROUND:
      tp->work.f = -_rename_bunchin_gravity +
                   tp->work.f * _rename_bunchin_bound_ratio -
                   GetWork(tp)->posy * _rename_bunchin_bound_spring;
      GetWork(tp)->posy += tp->work.f;
      if (GetWork(tp)->posy < 0.0f) {
        GetWork(tp)->posy = 0.0f;
        twp->smode = MD_GROUND;
        tp->work.f = 0.3f * fabsf(tp->work.f);
      }
      if (twp->wtimer > _rename_bunchin_wait_time) {
        if ((GetType(twp) == TYPE_PLAYER_SWITCH &&
             _rename_IsSwitchOn(GetSwitch(twp))) ||
            (GetType(twp) != TYPE_PLAYER_SWITCH &&
             (GetType(twp) != TYPE_PLAYER || IsNoPlayerBelow(twp)))) {
          tp->work.f = 0.0f;
          GetWork(tp)->posy = 0.0f;
          twp->smode = MD_UP;
        }
      } else {
        twp->wtimer++;
      }
      break;
    }
  } else if (twp->cwp != NULL) {
    twp->cwp->info[1].attr |= 0x10;
  }

  object = GetObject(tp);
  posy = 20.0f + twp->pos.y + GetWork(tp)->posy;
  tp->fwp->pos_spd.y = posy - object->pos.y;
  object->pos.y = posy;
  if (_rename_EitherPlayerWithinSphere(
          &object->pos,
          32.0f + object->model->r * (1.0f + MAX(twp->scl.x, twp->scl.z)))) {
    twp->flag |= 0x100;
  } else {
    twp->flag &= ~0x100;
  }
  twp->cwp->info[0].center.y = 20.0f + GetWork(tp)->posy;

  if (!((twp->ang.x & 0x100) >> 8)) {
    // carry the players standing on it
    while ((hit_tp = fn_80005F88(tp, 1)) != NULL) {
      pno = IsThisTaskPlayer(hit_tp);
      if (pno != -1 &&
          BunchinCheckInBox(&playertwp[pno]->pos, &twp->pos, twp->ang.y,
                            10.0f + GetHalfWidth(twp->scl.x), 20.0f,
                            10.0f + GetHalfWidth(twp->scl.z))) {
        fn_80038008(pno, 1, &pos, NULL);
        pos.y = 20.0f + twp->pos.y + GetWork(tp)->posy;
        fn_80037F64(pno, 0, &pos, NULL);
        playertwp[pno]->pos = pos;
      }
    }
  }
  fn_800068E4(twp->cwp);
  CCL_Entry(tp);
}

static void ObjectBunchinDisp(task *tp) {
  taskwk *twp = tp->twp;
  Uint32 color;
  STACK_PAD_VAR(4);

  njPushMatrix(NULL);
  njTranslate(NULL, twp->pos.x, 20.0f + (twp->pos.y + GetWork(tp)->posy),
              twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  njSetTexture(&_rename_bunchin_top_texlist);
  if (bunchin_dsdraw) {
    ds_DrawModelClip(_rename_bunchin_ObjArr[3]);
  } else {
    njCnkCacheDrawModel(&_rename_bunchin_top_model);
  }
  OffControl3D(0x2400);

  njScale(NULL, 1.0f + twp->scl.x, 1.0f, 1.0f + twp->scl.z);
  if ((GetType(twp) != TYPE_SWITCH && GetType(twp) != TYPE_PLAYER_SWITCH) ||
      !_rename_IsSwitchOn(GetSwitch(twp))) {
    // working: the lamps blink
    color = fn_800334B0(
        0xFF101010, 0xFFFFFFFF,
        0.5f + 0.5f * njSin(lbl_801CC168._7C * _rename_bunchin_lamp_spd));
    njSetTexture(&_rename_bunchin_lamp_texlist);
    _rename_CnkDrawModelColor(&_rename_bunchin_lamp_model, color);
    _rename_CnkDrawModelColor(
        &_rename_bunchin_lamp2_model,
        fn_800334B0(0xFF202020, 0xFFFFFFFF, _rename_GetBlinkRatio(30, 15, 5)));
  } else {
    njSetTexture(&_rename_bunchin_lamp_texlist);
    _rename_CnkDrawModelColor(&_rename_bunchin_lamp_model, 0xFF101010);
    _rename_CnkDrawModelColor(&_rename_bunchin_lamp2_model, 0xFF202020);
  }
  OnControl3D(0x2400);
  njSetTexture(&_rename_bunchin_texlist);
  njCnkCacheDrawModel(&_rename_bunchin_model);
  njPopMatrix(1);
  OffControl3D(0x2400);
}

static void ObjectBunchinDispShad(task *tp) {
  taskwk *twp = tp->twp;
  Float r;
  NJS_POINT3 pos;

  r = sqrtf((1.0f + twp->scl.x) * (1.0f + twp->scl.x) +
            (1.0f + twp->scl.z) * (1.0f + twp->scl.z));
  pos.x = twp->pos.x;
  pos.y = 20.0f + (twp->pos.y + GetWork(tp)->posy);
  pos.z = twp->pos.z;
  _rename_ShadowTexBegin(GetWork(tp)->shadow, 1.05f * r, 0.0f, &twp->pos, NULL,
                         NULL);
  njPushMatrix(NULL);
  njTranslateEx(&pos);
  njRotateY(NULL, twp->ang.y);
  njSetTexture(&_rename_bunchin_top_texlist);
  njScale(NULL, 1.0f + twp->scl.x, 1.0f, 1.0f + twp->scl.z);
  njSetTexture(&_rename_bunchin_texlist);
  gjDrawModel(&_rename_bunchin_shadow_model);
  njPopMatrix(1);
  _rename_ShadowTexEnd(GetWork(tp)->shadow);
  _rename_ShadowTexDraw(2, &twp->pos, 20.0f * r, GetWork(tp)->shadow);
}
