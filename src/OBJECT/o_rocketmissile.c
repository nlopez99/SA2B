#include "OBJECT/o_rocketmissile.h"

#include "CCL.h"
#include "EFFECT/ef_rocketthrust.h"
#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njbasic.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njcollision.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njmotion.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "stdlib.h"

// mobile land collision entry: register / withdraw
extern void fn_800232A4(Uint32 attr, task *tp, NJS_OBJECT *object);
extern void fn_800231CC(task *tp, NJS_OBJECT *object);
extern s32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, float);
extern void _rename_SetConditionFlag(task *tp, u8 smode);
extern void _rename_gjSetTexMtx(NJS_POINT3 *pos, Angle3 *ang);
extern void SetSwitchOnOff(Sint32 id, Sint32 flag);
extern Sint32 GetSwitchOnOff(Sint32 id);
extern Sint8 fn_80065388(task *tp);
extern Sint32 fn_80018E30(Sint32 id);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_800156FC(Float, Float, Float, Float);
extern void fn_80066820(Sint32, NJS_POINT3 *, Float);
extern void fn_8006648C(void);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);
extern void fn_800E29BC(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void fn_8011E1EC(NJS_CNK_OBJECT *, NJS_MOTION *, Float);
extern void fn_80123088(GJS_OBJECT *, NJS_MOTION *, Float);
extern void FreeTaskC(task *tp);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern Float atan2f(Float y, Float x);
inline float sqrtf(float f);

extern BOOL DisableObjectFog;

extern NJS_TEXLIST    _rename_rocketmissile_texlist;
extern NJS_CNK_OBJECT _rename_rocketmissile_object;
extern NJS_MOTION     _rename_rocketmissile_motion;
extern NJS_TEXLIST    _rename_rocketmissile_fire_texlist;
extern NJS_CNK_MODEL  _rename_rocketmissile_fire_model;
extern NJS_TEXLIST    _rename_rocketmissile_switch_texlist;
extern NJS_CNK_OBJECT _rename_rocketmissile_switch_object;
extern NJS_MOTION     _rename_rocketmissile_base_motion;
extern NJS_TEXLIST    _rename_rocketmissile_base_texlist;
extern GJS_OBJECT     _rename_rocketmissile_base_object;
extern NJS_OBJECT     _rename_rocketmissile_col_object;

#define TARGET_MAX 16

extern rocketmissiletarget _rename_rocketmissile_target[TARGET_MAX];
extern NJS_VECTOR          _rename_rocketmissile_smoke_vec;
extern Float               _rename_rocketmissile_smoke_spd_rand;
extern Float               _rename_rocketmissile_smoke_ground_ofs;
extern Float               _rename_rocketmissile_smoke_scl;
extern Float               _rename_rocketmissile_smoke_scl_rand;
extern CCL_INFO            _rename_rocketmissile_colli_info[1];
extern CCL_INFO            _rename_rocketmissile_colli_info_bomb[1];

// ^ extern
// v in this file

static void ObjectRocketMissileDest(task *tp);
static void ObjectRocketMissileExec(task *tp);
static void ObjectRocketMissileDisp(task *tp);
static void ObjectRocketMissileColli(task *tp);
static void ObjectRocketMissileColliExec(task *tp);
static void ObjectRocketMissileColliDest(task *tp);

typedef struct rocketmissilewk // sizeof=0x64
{
  /* 0x00 */ NJS_OBJECT object;
  /* 0x38 */ Float frame;
  /* 0x3C */ Float base_frame;
  /* 0x40 */ Angle3 ang;
  /* 0x4C */ NJS_POINT3 pos;
  /* 0x58 */ Float fire_scl;
  /* 0x5C */ Float switch_ofs;
  /* 0x60 */ Sint32 pno;
} rocketmissilewk;

enum {
  MD_WAIT = 2,   // closed, waiting for its trigger
  MD_APPEAR = 3, // the hatch opens, the missile rises and unfolds
  MD_AIM = 4,    // switch pressed: turn towards the target
  MD_END = 5,    // one-shot missile that has been fired
  MD_FLY = 6,
  MD_HIT = 7,
};

#define GetWork(task) ((rocketmissilewk *)task->mwp)
#define GetObject(task) (&GetWork(task)->object)
#define GetId(twp) (twp->ang.z & 0x1F)
#define GetType(twp) ((twp->ang.x & 0xF) % 4)
#define IsReverse(twp) (twp->ang.x & 0x10)
#define RadAng(n) ((Angle)(10430.38043493439 * (n)))
#define DegAng(n) ((Angle)(182.04445f * (n)))
#define RandF() (0.000030517578f * (Float)rand())

// last step of MD_APPEAR: the missile unfolds, then waits for the switch
#define UnfoldAndWaitSwitch(tp, twp)                                           \
  GetWork(tp)->frame += 0.3f;                                                  \
  if (GetWork(tp)->frame >= 7.0f) {                                            \
    GetWork(tp)->frame = 7.0f;                                                 \
    if (0.0f == GetWork(tp)->switch_ofs) {                                     \
      GetWork(tp)->fire_scl = 0.0f;                                            \
      twp->mode = MD_AIM;                                                      \
      twp->wtimer = 0;                                                         \
    }                                                                          \
  }

// the missile turns at most 0x140 a frame
#define LimitTurn(d)                                                           \
  if (d > 0) {                                                                 \
    if (d > 0x140) {                                                           \
      d = 0x140;                                                               \
    }                                                                          \
  } else if (d < -0x140) {                                                     \
    d = -0x140;                                                                \
  }

rocketmissiletarget *RocketMissileSetTarget(Sint32 id, NJS_POINT3 **pos) {
  Sint32 i;
  rocketmissiletarget *target = _rename_rocketmissile_target;

  for (i = 0; i < TARGET_MAX; i++, target++) {
    if (target->pos == NULL) {
      target->pos = pos;
      target->id = id;
      *pos = NULL;
      return target;
    }
  }
  return NULL;
}

rocketmissiletarget *RocketMissileFreeTarget(NJS_POINT3 **pos) {
  Sint32 i;
  rocketmissiletarget *target = _rename_rocketmissile_target;

  for (i = 0; i < TARGET_MAX; i++, target++) {
    if (target->pos == pos) {
      *pos = NULL;
      target->pos = NULL;
      target->id = -1;
      return target;
    }
  }
  return NULL;
}

void ObjectRocketMissile(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object;
  Uint32 attr;
  Float r;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(rocketmissilewk));
  if (tp->mwp == NULL) {
    return;
  }

  tp->disp = ObjectRocketMissileDisp;
  tp->exec = ObjectRocketMissileExec;
  tp->dest = ObjectRocketMissileDest;
  twp->mode = MD_WAIT;
  GetWork(tp)->pos = twp->pos;
  GetWork(tp)->ang.z = 0;
  GetWork(tp)->ang.x = 0;
  GetWork(tp)->ang.y = twp->ang.y;
  GetWork(tp)->frame = 0.0f;
  GetWork(tp)->base_frame = 0.0f;
  GetWork(tp)->fire_scl = 0.0f;
  GetWork(tp)->switch_ofs = 0.0f;

  object = GetObject(tp);
  *object = _rename_rocketmissile_col_object;
  object->evalflags &= ~3;
  object->ang.y = twp->ang.y;
  object->ang.z = IsReverse(twp) ? 0x8000 : 0;
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

  twp->ang.x &= 0x1F;
  twp->ang.z &= 0x1F;
  twp->ang.z += IsReverse(twp) ? 0x8000 : 0;
  CreateChildTask(IM_TWK, ObjectRocketMissileColli, tp);
  if (tp->ocp != NULL && fn_80065388(tp)) {
    twp->mode = MD_END;
  }
}

static void ObjectRocketMissileDest(task *tp) {
  if (tp->twp->mode == MD_END && tp->ocp != NULL) {
    _rename_SetConditionFlag(tp, 1);
  }
  if (GetObject(tp)->model != NULL) {
    fn_800231CC(tp, GetObject(tp));
  }
  FreeTaskC(tp);
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectRocketMissileExec(task *tp) {
  Angle angz;
  Angle angy;
  taskwk *twp = tp->twp;
  NJS_POINT3 sw_pos;
  NJS_VECTOR vec;
  NJS_POINT3 unused;
  NJS_POINT3 smoke_pos;
  NJS_VECTOR spd;
  NJS_VECTOR bomb_spd;
  NJS_POINT3 fire_pos;
  NJS_OBJECT *object;
  Sint16 dy;
  Sint16 dz;
  // one index and walker per loop, as in the original
  Sint32 i;
  Sint32 n0;
  Sint32 n1;
  rocketmissiletarget *target;
  Sint32 j;
  rocketmissiletarget *target2;
  Sint32 k;
  rocketmissiletarget *target3;
  Float dist;
  Float scl;
  STACK_PAD_VAR(3); // unused

  if (twp->mode < MD_FLY && CheckRangeOut(tp)) {
    return;
  }

  // the launch switch sits beside the hatch
  sw_pos.x = IsReverse(twp) ? 15.1f : -15.1f;
  sw_pos.y = IsReverse(twp) ? -2.6f : 2.6f;
  sw_pos.z = 0.0f;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateV(NULL, &twp->pos);
  njRotateY(NULL, twp->ang.y);
  njCalcPoint(NULL, &sw_pos, &sw_pos);
  njPopMatrix(1);

  // pno is left at 1 when nobody stands on the switch
  GetWork(tp)->pno = 0;
  if ((playertwp[0] != NULL &&
       njDistanceP2P(&playertwp[0]->pos, &sw_pos) < 6.5f &&
       (playertwp[0]->flag & 3)) ||
      ((GetWork(tp)->pno = 1, playertwp[1] != NULL) &&
       njDistanceP2P(&playertwp[1]->pos, &sw_pos) < 6.5f &&
       (playertwp[1]->flag & 3))) {
    if (twp->smode == 0 && GetWork(tp)->switch_ofs > 0.0f) {
      twp->smode = 1;
    }
    GetWork(tp)->switch_ofs = 0.0f;
  } else {
    GetWork(tp)->switch_ofs = 1.0f;
  }

  if (!lbl_801CC168._37 || twp->smode != 0) {
    switch (twp->mode) {
    case MD_WAIT:
      GetWork(tp)->pos = twp->pos;
      GetWork(tp)->ang.z = 0;
      GetWork(tp)->ang.y = twp->ang.y;
      GetWork(tp)->ang.x = 0;
      GetWork(tp)->base_frame = 0.0f;
      GetWork(tp)->frame = 0.0f;
      twp->mode = MD_APPEAR;
      switch (GetType(twp)) {
      case 2:
        if (!GetSwitchOnOff(GetId(twp))) {
          twp->mode = MD_WAIT;
        }
        break;
      case 3:
        if (!_rename_EitherPlayerWithinSphere(&twp->pos, 100.0f) ||
            !fn_80018E30(GetId(twp))) {
          twp->mode = MD_WAIT;
        }
        break;
      }
      // stay closed while nothing has registered as a target for this id
      if (!(twp->ang.z & 0x80)) {
        n0 = 0;
        n1 = 0;
        for (i = 0, target = _rename_rocketmissile_target; i < TARGET_MAX;
             i++, target++) {
          if ((target->id & 0xFFFF) != GetId(twp)) {
            continue;
          }
          if (target->id & 0x80000000) {
            n1++;
          } else {
            n0++;
          }
        }
        if (n0 == 0 && n1 == 0) {
          twp->mode = MD_WAIT;
        }
      }
      if (twp->mode == MD_APPEAR) {
        fn_8006B7EC(0x1005, NULL, 0, 0, &twp->pos);
      }
      break;
    case MD_APPEAR:
      GetWork(tp)->base_frame += 0.05f;
      if (!(GetWork(tp)->base_frame >= 1.0f)) {
        break;
      }
      GetWork(tp)->base_frame = 1.0f;
      if (IsReverse(twp)) {
        GetWork(tp)->pos.y -= 0.05f;
        if (!(GetWork(tp)->pos.y <= twp->pos.y - 9.0f)) {
          break;
        }
        GetWork(tp)->pos.y = twp->pos.y - 9.0f;
        UnfoldAndWaitSwitch(tp, twp);
      } else {
        GetWork(tp)->pos.y += 0.05f;
        if (!(GetWork(tp)->pos.y >= 9.0f + twp->pos.y)) {
          break;
        }
        GetWork(tp)->pos.y = 9.0f + twp->pos.y;
        UnfoldAndWaitSwitch(tp, twp);
      }
      break;
    case MD_AIM:
      // twp->scl holds the point to fly to
      vec.x = -GetWork(tp)->pos.x + twp->scl.x;
      vec.y = -GetWork(tp)->pos.y + twp->scl.y;
      vec.z = -GetWork(tp)->pos.z + twp->scl.z;
      if (IsReverse(twp)) {
        angz = -RadAng(atan2f(vec.y, sqrtf(vec.z * vec.z + vec.x * vec.x)));
        angy = 0x8000 + -RadAng(atan2f(vec.z, vec.x));
      } else {
        angz = RadAng(atan2f(vec.y, sqrtf(vec.z * vec.z + vec.x * vec.x)));
        angy = -RadAng(atan2f(vec.z, vec.x));
      }
      // turn around y first, then around z
      dz = angz - GetWork(tp)->ang.z;
      dy = angy - GetWork(tp)->ang.y;
      LimitTurn(dy);
      if (dy == 0) {
        LimitTurn(dz);
      } else {
        dz = 0;
      }
      GetWork(tp)->ang.y += dy;
      GetWork(tp)->ang.z += dz;
      GetWork(tp)->ang.x = DegAng(10.0f * njSin(twp->wtimer << 10));
      if (IsReverse(twp)) {
        GetWork(tp)->pos.y =
            twp->pos.y - 9.0f + 0.3f * njSin(twp->wtimer++ * 0x600);
      } else {
        GetWork(tp)->pos.y =
            9.0f + twp->pos.y + 0.3f * njSin(twp->wtimer++ * 0x600);
      }

      smoke_pos.x = -0.2f;
      smoke_pos.y = 0.0f;
      smoke_pos.z = 0.0f;
      njPushMatrixEx();
      njUnitMatrix(NULL);
      njTranslateV(NULL, &GetWork(tp)->pos);
      njRotateY(NULL, GetWork(tp)->ang.y);
      njRotateZ(NULL, GetWork(tp)->ang.z);
      njCalcPoint(NULL, &smoke_pos, &smoke_pos);
      njCalcVector(NULL, &_rename_rocketmissile_smoke_vec, &vec);
      njPopMatrix(1);
      fn_8006AFFC(0x1019, twp, 1, 0, 30, &smoke_pos);
      vec.x += _rename_rocketmissile_smoke_spd_rand * (RandF() - 0.5f);
      vec.y += _rename_rocketmissile_smoke_spd_rand * (RandF() - 0.5f);
      vec.z += _rename_rocketmissile_smoke_spd_rand * (RandF() - 0.5f);
      scl = _rename_rocketmissile_smoke_scl +
            _rename_rocketmissile_smoke_scl_rand * RandF();
      CreateRocketThrustGround(
          &smoke_pos, &vec, scl,
          scl + (twp->pos.y + _rename_rocketmissile_smoke_ground_ofs));

      // on target: close the hatch, light the engine, launch
      if ((GetWork(tp)->ang.y & 0xFFFF) != (angy & 0xFFFF) ||
          (GetWork(tp)->ang.z & 0xFFFF) != (angz & 0xFFFF)) {
        break;
      }
      GetWork(tp)->base_frame -= 0.05f;
      if (!(GetWork(tp)->base_frame <= 0.0f)) {
        break;
      }
      GetWork(tp)->fire_scl += 0.07f;
      GetWork(tp)->base_frame = 0.0f;
      if (GetWork(tp)->fire_scl >= 1.0f) {
        GetWork(tp)->fire_scl = 1.0f;
        twp->wtimer = 0;
        twp->mode = MD_FLY;
        fn_8002FB2C(GetWork(tp)->pno, 5, 50, 0);
      }
      break;
    case MD_END:
      break;
    case MD_FLY:
      spd.x = -GetWork(tp)->pos.x + twp->scl.x;
      spd.y = -GetWork(tp)->pos.y + twp->scl.y;
      spd.z = -GetWork(tp)->pos.z + twp->scl.z;
      dist = njScalor(&spd);
      // the colli child counts in this task's work.l, but this reads the child's
      if (dist < 5.1f || (tp->ctp != NULL && tp->ctp->work.l > 15 &&
                          (tp->ctp->twp->cwp->flag & 1))) {
        if (tp->ctp == NULL || !(tp->ctp->twp->cwp->flag & 1)) {
          // arrived: hand the position to everything waiting for this id
          GetWork(tp)->pos.x += spd.x;
          GetWork(tp)->pos.y += spd.y;
          GetWork(tp)->pos.z += spd.z;
          for (j = 0, target2 = _rename_rocketmissile_target; j < TARGET_MAX;
               j++, target2++) {
            if ((target2->id & 0xFFFF) == GetId(twp) && target2->pos != NULL) {
              *target2->pos = &GetWork(tp)->pos;
            }
          }
        }
        twp->mode = MD_HIT;
      } else {
        scl = 5.1f / dist;
        spd.x *= scl;
        spd.y *= scl;
        spd.z *= scl;
        GetWork(tp)->pos.x += spd.x;
        GetWork(tp)->pos.y += spd.y;
        GetWork(tp)->pos.z += spd.z;
      }
      if (twp->mode == MD_HIT) {
        bomb_spd.x = 0.2f * spd.x;
        bomb_spd.y = 0.2f * spd.y;
        bomb_spd.z = 0.2f * spd.z;
        fn_800E29BC(&GetWork(tp)->pos, &bomb_spd, 15.0f);
      }

      fire_pos.x = -0.2f;
      fire_pos.y = 0.0f;
      fire_pos.z = 0.0f;
      spd.x *= 0.5f;
      spd.y *= 0.5f;
      spd.z *= 0.5f;
      njPushMatrixEx();
      njUnitMatrix(NULL);
      njTranslateV(NULL, &GetWork(tp)->pos);
      njRotateY(NULL, GetWork(tp)->ang.y);
      njRotateZ(NULL, GetWork(tp)->ang.z);
      njCalcPoint(NULL, &fire_pos, &fire_pos);
      njPopMatrix(1);
      fn_8006AFFC(0x1019, twp, 1, 0, 30, &fire_pos);
      if (dist > 10.2f) {
        spd.x += 0.3f * (RandF() - 0.5f);
        spd.y += 0.3f * (RandF() - 0.5f);
        spd.z += 0.3f * (RandF() - 0.5f);
        CreateRocketThrust(&fire_pos, &spd, 3.0f + 2.0f * RandF());
        njPopMatrix(1); // the original pops twice
      }
      break;
    case MD_HIT:
      switch (GetType(twp)) {
      case 0:
        twp->mode = MD_WAIT;
        break;
      case 2:
        twp->mode = MD_WAIT;
        SetSwitchOnOff(GetId(twp), 0);
        break;
      case 1:
        twp->mode = MD_END;
        break;
      }
      GetWork(tp)->pos = twp->pos;
      GetWork(tp)->ang.z = 0;
      GetWork(tp)->ang.y = twp->ang.y;
      GetWork(tp)->ang.x = 0;
      GetWork(tp)->frame = 0.0f;
      GetWork(tp)->base_frame = 0.0f;
      GetWork(tp)->fire_scl = 0.0f;
      twp->smode = 0;
      for (k = 0, target3 = _rename_rocketmissile_target; k < TARGET_MAX;
           k++, target3++) {
        if (target3->pos != NULL && *target3->pos == &GetWork(tp)->pos) {
          *target3->pos = NULL;
        }
      }
      break;
    }
  }

  object = GetObject(tp);
  if (_rename_EitherPlayerWithinSphere(&object->pos,
                                       32.0f + object->model->r)) {
    twp->flag |= 0x100;
  } else {
    twp->flag &= ~0x100;
  }
}

static void ObjectRocketMissileDisp(task *tp) {
  taskwk *twp = tp->twp;
  Angle3 ang;
  Float scl;

  ang.x = 0;
  ang.y = twp->ang.y;
  ang.z = 0;

  OnControl3D(0x2400);
  njPushMatrix(NULL);
  njTranslateV(NULL, &twp->pos);
  if (IsReverse(twp)) {
    njRotateY(NULL, twp->ang.y);
    njRotateZ(NULL, 0x8000);
  } else {
    njRotateY(NULL, twp->ang.y);
  }
  njSetTexture(&_rename_rocketmissile_base_texlist);
  fn_80066820(0xFF, &twp->pos, _rename_rocketmissile_base_object.model->r);
  _rename_gjSetTexMtx(&twp->pos, &ang);
  fn_80123088(&_rename_rocketmissile_base_object,
              &_rename_rocketmissile_base_motion, GetWork(tp)->base_frame);
  fn_8006648C();

  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  njSetTexture(&_rename_rocketmissile_switch_texlist);
  njTranslate(NULL, _rename_rocketmissile_switch_object.pos.x,
              1.6f + GetWork(tp)->switch_ofs,
              _rename_rocketmissile_switch_object.pos.z);
  njCnkCacheDrawModel(_rename_rocketmissile_switch_object.model);
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
  njPopMatrix(1);
  OffControl3D(0x2400);

  njPushMatrix(NULL);
  njTranslateV(NULL, &GetWork(tp)->pos);
  njRotateY(NULL, GetWork(tp)->ang.y);
  njRotateZ(NULL, GetWork(tp)->ang.z);
  njRotateX(NULL, GetWork(tp)->ang.x);
  if (IsReverse(twp)) {
    njRotateZ(NULL, 0x8000);
  }
  if (twp->mode != MD_WAIT && twp->mode != MD_END && twp->mode != MD_HIT) {
    njSetTexture(&_rename_rocketmissile_texlist);
    fn_8011E1EC(&_rename_rocketmissile_object, &_rename_rocketmissile_motion,
                GetWork(tp)->frame);
    fn_8002B304();
    OffControl3D(0x220);
    OnControl3D(0x10);
    scl = (lbl_801CC168._7C & 1) ? 0.6f * GetWork(tp)->fire_scl
                                 : GetWork(tp)->fire_scl;
    fn_800156FC(1.0f, scl, scl, scl);
    njSetTexture(&_rename_rocketmissile_fire_texlist);
    njCnkCacheDrawModel(&_rename_rocketmissile_fire_model);
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
    fn_8002B2F8();
  }
  njPopMatrix(1);
}

static void ObjectRocketMissileColli(task *tp) {
  taskwk *twp = tp->twp;
  task *ptp = tp->ptp;
  taskwk *ptwp = ptp->twp;

  if (GetWork(ptp) == NULL) {
    return;
  }

  if (ptwp->ang.z & 0x80) {
    CCL_Init(tp, _rename_rocketmissile_colli_info_bomb,
             ARYLEN(_rename_rocketmissile_colli_info_bomb), CID_ENEMY2);
  } else {
    CCL_Init(tp, _rename_rocketmissile_colli_info,
             ARYLEN(_rename_rocketmissile_colli_info), CID_OBJECT);
  }
  tp->exec = ObjectRocketMissileColliExec;
  tp->dest = ObjectRocketMissileColliDest;
  twp->pos = GetWork(ptp)->pos;
  twp->ang.x = 0;
  twp->ang.y = GetWork(ptp)->ang.y + (IsReverse(ptwp) ? 0x8000 : 0);
  twp->ang.z = GetWork(ptp)->ang.z;
}

static void ObjectRocketMissileColliExec(task *tp) {
  taskwk *twp = tp->twp;
  task *ptp = tp->ptp;
  taskwk *ptwp = ptp->twp;

  if (GetWork(ptp) == NULL) {
    return;
  }

  twp->pos = GetWork(ptp)->pos;
  twp->ang.x = 0;
  twp->ang.y = GetWork(ptp)->ang.y + (IsReverse(ptwp) ? 0x8000 : 0);
  twp->ang.z = GetWork(ptp)->ang.z;
  if (ptwp->mode == MD_FLY) {
    if (ptp->work.l > 15) {
      twp->cwp->info->damage = (twp->cwp->info->damage & ~3) | 2;
    } else {
      ptp->work.l++;
    }
  } else {
    twp->cwp->info->damage = twp->cwp->info->damage & ~3;
    ptp->work.l = 0;
  }
  CCL_Entry(tp);
}

static void ObjectRocketMissileColliDest(task *tp) {}
