#include "OBJECT/o_rocket.h"

#include "CCL.h"
#include "EFFECT/ef_rocketthrust.h"
#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njmotion.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
// player.h has mode as Sint8; this file passes it as Sint32
#define SetInputP SetInputP_Sint8
#include "samt/sonic/player.h"
#undef SetInputP
#include "samt/sonic/sound.h"
#include "set.h"

// mobile land collision entry: register / withdraw
extern void fn_800232A4(Uint32 attr, task *tp, NJS_OBJECT *object);
extern void fn_800231CC(task *tp, NJS_OBJECT *object);
extern s32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, float);
extern Sint32 GetSwitchOnOff(Sint32 no);
extern void _rename_gjSetTexMtx(NJS_POINT3 *pos, Angle3 *ang);
extern void FreeTaskC(task *tp);
extern void SetInputP(Sint32 pno, Sint32 mode, Sint32 unk);
extern void fn_800133BC(Angle *x, Angle *y, Angle *z);
extern void fn_8002FB2C(Sint32, Sint32, Sint32, Sint32);
extern void fn_800399BC(Sint32, Float, Float, Float);
extern void fn_80066820(Sint32, NJS_POINT3 *pos, Float r);
extern void fn_8006648C(void);
extern void fn_8006B1FC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        Sint32 timer);
extern void fn_800E29BC(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void fn_8011E214(NJS_CNK_OBJECT *object, NJS_MOTION *motion,
                        Float frame);
extern void fn_80123088(GJS_OBJECT *object, NJS_MOTION *motion, Float frame);
extern Float sqrtf(Float x);
extern Float atan2f(Float y, Float x);

extern NJS_TEXLIST    _rename_rocket_texlist;
extern NJS_CNK_OBJECT _rename_rocket_object;
extern NJS_MOTION     _rename_rocket_motion;
extern NJS_TEXLIST    _rename_rocket_base_texlist;
extern GJS_OBJECT     _rename_rocket_base_object;
extern NJS_MOTION     _rename_rocket_base_motion;
extern NJS_OBJECT     _rename_rocket_col_object;
extern Float          _rename_rocket_up;
extern NJS_POINT3     _rename_rocket_ofs;
extern NJS_POINT3     _rename_rocket_hang_pos;
extern NJS_VECTOR     _rename_rocket_thrust_spd;
extern Float          _rename_rocket_thrust_spread;
extern Float          _rename_rocket_thrust_ground;
extern Float          _rename_rocket_thrust_scl;
extern Float          _rename_rocket_thrust_scl_rand;
extern CCL_INFO       _rename_rocket_colli_info[2];

// ^ extern
// v in this file

static void ObjectRocketDest(task *tp);
static void ObjectRocketExec(task *tp);
static void ObjectRocketDisp(task *tp);
static void RocketBodyInit(task *tp);
static void RocketBodyDisp(task *tp);
static void RocketBodyExec(task *tp);
static void RocketBodyDest(task *tp);

typedef struct rocketwk // sizeof=0x48
{
  /* 0x00 */ NJS_OBJECT object; // land collision of the base
  /* 0x38 */ Float frame;       // rocket motion, 0..7
  /* 0x3C */ Float base_frame;  // base motion, 0..1
  /* 0x40 */ task *body;        // the rocket standing on the base
  /* 0x44 */ Sint32 ride;       // rockets in flight with a player
} rocketwk;

#define GetWork(task) ((rocketwk *)task->mwp)
#define GetObject(task) (&GetWork(task)->object)
#define GetBody(task) (GetWork(task)->body->twp)
#define RadAng(n) ((Angle)(10430.38043493439 * (n)))
// the low two bits of CCL_INFO.damage are the attack power
#define SetAttack(info, n) ((info)->damage = ((info)->damage & ~3) | (n))
#define ClearAttack(info) ((info)->damage = (info)->damage & ~3)

enum {
  MD_WAIT = 1,
  MD_SETUP = 2,
  MD_FLY = 3,
};

enum {
  MD_BODY_INIT = 0,
  MD_BODY_WAIT = 1,
  MD_BODY_FLY = 2,
};

enum {
  SMD_SIDE = 4, // the target is above: the rocket hangs on the side
};

void ObjectRocket(task *tp) {
  taskwk *twp = tp->twp;
  NJS_OBJECT *object;
  Uint32 attr;
  Float r;
  NJS_VECTOR v;
  Angle angx;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(rocketwk));
  if (tp->mwp == NULL) {
    return;
  }

  tp->disp = ObjectRocketDisp;
  tp->exec = ObjectRocketExec;
  tp->dest = ObjectRocketDest;
  twp->mode = MD_WAIT;
  GetWork(tp)->body = NULL;

  v.x = -twp->pos.x + twp->scl.x;
  v.y = -twp->pos.y + twp->scl.y;
  v.z = -twp->pos.z + twp->scl.z;
  angx = RadAng(atan2f(v.y, sqrtf(v.z * v.z + v.x * v.x)));
  twp->ang.y = -RadAng(atan2f(v.z, v.x));
  if ((angx & 0xFFFF) > 0x2000) {
    twp->smode = SMD_SIDE;
  }

  object = GetObject(tp);
  *object = _rename_rocket_col_object;
  object->evalflags &= ~3;
  object->ang.y = twp->ang.y;
  object->ang.z = 0;
  object->ang.x = 0;
  object->pos.x = twp->pos.x;
  object->pos.y = twp->pos.y;
  object->pos.z = twp->pos.z;

  attr = 0x101;
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

  twp->ang.x &= 1;
  twp->ang.z &= 0x1f;
  GetWork(tp)->frame = 0.0f;
  GetWork(tp)->base_frame = 0.0f;
  GetWork(tp)->ride = 0;
}

static void ObjectRocketDest(task *tp) {
  if (GetObject(tp)->model != NULL) {
    fn_800231CC(tp, GetObject(tp));
  }
  FreeTaskC(tp);
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectRocketExec(task *tp) {
  Angle angx;
  Angle angy;
  Sint8 *pause;
  Sint32 pno;
  taskwk *twp = tp->twp;
  Sint16 dy;
  Angle dz;
  Float scl;
  particle *p;
  NJS_VECTOR ofs;
  NJS_VECTOR v;
  NJS_VECTOR unused; // unused
  NJS_POINT3 pos;
  STACK_PAD_VAR(1);

  if (GetWork(tp)->ride == 0 && CheckRangeOut(tp)) {
    return;
  }

  if (twp->mode == MD_SETUP || twp->btimer != 0 || !lbl_801CC168._37) {
    if (!*(pause = &lbl_801CC168._37)) {
      twp->btimer = 0;
    }

    switch (twp->mode) {
    case MD_WAIT:
      if (GetWork(tp)->body == NULL &&
          (!(twp->ang.x & 1) || GetSwitchOnOff(twp->ang.z & 0x1f))) {
        GetWork(tp)->body = CreateChildTask(IM_TWK, RocketBodyInit, tp);
        twp->mode = MD_SETUP;
        SE_Call(0x1005, NULL, 0, 0);
      }
      if (GetWork(tp)->body != NULL) {
        GetBody(tp)->smode = -1;
        GetBody(tp)->pos = twp->pos;
        if (twp->smode == SMD_SIDE) {
          ofs.x = 10.0f;
          ofs.y = 0.0f;
          ofs.z = 0.0f;
          njPushMatrixEx();
          njUnitMatrix(NULL);
          njRotateY(NULL, twp->ang.y);
          njCalcVector(NULL, &ofs, &ofs);
          njPopMatrixEx();
          GetBody(tp)->pos.x += ofs.x;
          GetBody(tp)->pos.z += ofs.z;
        }
        GetBody(tp)->pos.y -= 1.5f;
        GetBody(tp)->ang.z = twp->smode == SMD_SIDE ? 0x4000 : 0;
        GetBody(tp)->ang.y = twp->ang.y;
        GetBody(tp)->ang.x = 0;
        GetWork(tp)->base_frame = 0.0f;
        GetWork(tp)->frame = 0.0f;
      }
      break;
    case MD_SETUP:
      // the base opens, the rocket rises, then it unfolds its handle
      if (!*pause) {
        GetWork(tp)->base_frame += 0.05f;
      }
      if (GetWork(tp)->base_frame >= 1.0f) {
        GetWork(tp)->base_frame = 1.0f;
        if (!*pause) {
          GetBody(tp)->pos.y += 0.15f;
        }
        if (GetBody(tp)->pos.y >= twp->pos.y + _rename_rocket_up) {
          GetBody(tp)->pos.y = twp->pos.y + _rename_rocket_up;
          if (!*pause) {
            GetWork(tp)->frame += 0.3f;
          }
          if (GetWork(tp)->frame >= 7.0f) {
            GetWork(tp)->frame = 7.0f;
            if (tp->ctp != NULL && (tp->ctp->twp->cwp->flag & 1) &&
                tp->ctp->twp->cwp->my_num == 1 &&
                (pno = IsThisTaskPlayer(
                     tp->ctp->twp->cwp->hit_cwp->mytask)) >= 0) {
              GetBody(tp)->scl = twp->scl;
              GetBody(tp)->smode = pno;
              GetBody(tp)->mode = MD_BODY_WAIT;
              GetBody(tp)->btimer = *pause;
              SetInputP(pno, 9, 0x4c);
              GetWork(tp)->ride++;
              twp->mode = MD_FLY;
              twp->wtimer = 0;
              playertwp[pno]->ang.x = 0;
              playertwp[pno]->ang.y = GetBody(tp)->ang.y;
              playertwp[pno]->ang.z = 0;
              twp->btimer = *pause;
            }
          }
        }
      }
      break;
    case MD_FLY:
      // turn the rocket towards the target, then let it go
      GetBody(tp)->btimer = *pause;
      v.x = -GetBody(tp)->pos.x + twp->scl.x;
      v.y = -GetBody(tp)->pos.y + twp->scl.y;
      v.z = -GetBody(tp)->pos.z + twp->scl.z;
      angx = RadAng(atan2f(v.y, sqrtf(v.z * v.z + v.x * v.x)));
      angy = -RadAng(atan2f(v.z, v.x));
      dz = angx - GetBody(tp)->ang.z;
      dy = angy - GetBody(tp)->ang.y;
      if (dy > 0) {
        if (dy > 0x100) {
          dy = 0x100;
        }
      } else if (dy < -0x100) {
        dy = -0x100;
      }
      if (dy == 0) {
        if ((Sint16)dz > 0) {
          if ((Sint16)dz > 0x100) {
            dz = 0x100;
          }
        } else if ((Sint16)dz < -0x100) {
          dz = -0x100;
        }
      } else {
        dz = 0;
      }
      GetBody(tp)->ang.y += dy;
      GetBody(tp)->ang.z += (Sint16)dz;
      GetBody(tp)->pos.y = twp->pos.y + _rename_rocket_up;

      pos.x = 0.0f;
      pos.y = -0.3f;
      pos.z = 0.0f;
      njPushMatrixEx();
      njUnitMatrix(NULL);
      njTranslateEx(&GetBody(tp)->pos);
      njRotateY(NULL, GetBody(tp)->ang.y);
      njRotateZ(NULL, GetBody(tp)->ang.z);
      njRotateX(NULL, GetBody(tp)->ang.x);
      njRotateY(NULL, 0x4000);
      njRotateZ(NULL, -0x4000);
      njRotateX(NULL, 0x4000);
      if (twp->smode == SMD_SIDE) {
        njTranslate(NULL, _rename_rocket_ofs.x, -_rename_rocket_ofs.y,
                    -_rename_rocket_ofs.z);
      } else {
        njTranslate(NULL, -_rename_rocket_ofs.x, -_rename_rocket_ofs.y,
                    -_rename_rocket_ofs.z);
      }
      njCalcPoint(NULL, &pos, &pos);
      njCalcVector(NULL, &_rename_rocket_thrust_spd, &v);
      njPopMatrixEx();
      v.x += _rename_rocket_thrust_spread * (ParticleRandom() - 0.5f);
      v.y += _rename_rocket_thrust_spread * (ParticleRandom() - 0.5f);
      v.z += _rename_rocket_thrust_spread * (ParticleRandom() - 0.5f);
      scl = _rename_rocket_thrust_scl +
            _rename_rocket_thrust_scl_rand * ParticleRandom();
      p = CreateRocketThrustGround(
          &pos, &v, scl, scl + (twp->pos.y + _rename_rocket_thrust_ground));
      if (p != NULL) {
        p->argb = 0x90FFFFFF;
      }

      twp->wtimer++;
      if ((GetBody(tp)->ang.y & 0xFFFF) == (angy & 0xFFFF) &&
          (GetBody(tp)->ang.z & 0xFFFF) == (angx & 0xFFFF) && twp->wtimer > 7) {
        GetWork(tp)->base_frame -= 0.05f;
        if (GetWork(tp)->base_frame <= 0.0f) {
          GetWork(tp)->base_frame = 0.0f;
          SetInputP(GetBody(tp)->smode, 9, 0x4d);
          twp->wtimer = 0;
          twp->mode = MD_WAIT;
          GetBody(tp)->mode = MD_BODY_FLY;
          GetWork(tp)->body = NULL;
        }
      }
      break;
    }
  }

  if (_rename_EitherPlayerWithinSphere(&GetObject(tp)->pos,
                                       32.0f + GetObject(tp)->model->r)) {
    twp->flag |= 0x100;
  } else {
    twp->flag &= ~0x100;
  }
}

static void ObjectRocketDisp(task *tp) {
  taskwk *twp = tp->twp;
  Angle3 ang;

  ang.x = 0;
  ang.y = twp->ang.y;
  ang.z = 0;
  OnControl3D(0x2400);
  njPushMatrix(NULL);
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njSetTexture(&_rename_rocket_base_texlist);
  fn_80066820(0xff, &twp->pos, _rename_rocket_base_object.model->r);
  _rename_gjSetTexMtx(&twp->pos, &ang);
  fn_80123088(&_rename_rocket_base_object, &_rename_rocket_base_motion,
              GetWork(tp)->base_frame);
  fn_8006648C();
  njPopMatrixEx();
  OffControl3D(0x2400);
}

static void RocketBodyInit(task *tp) {
  taskwk *twp = tp->twp;
  task *ptp = tp->ptp;
  taskwk *ptwp = ptp->twp;
  NJS_VECTOR side_ofs;
  NJS_VECTOR ofs;

  if (ptp->mwp == NULL) {
    return;
  }

  CCL_Init(tp, _rename_rocket_colli_info, ARYLEN(_rename_rocket_colli_info),
           CID_OBJECT);
  ClearAttack(twp->cwp->info);
  tp->exec = RocketBodyExec;
  tp->dest = RocketBodyDest;
  tp->disp = RocketBodyDisp;
  twp->pos = ptwp->pos;
  twp->ang.x = 0;
  twp->ang.y = ptwp->ang.y;
  twp->ang.z = ptwp->smode == SMD_SIDE ? 0x4000 : 0;

  if (ptwp->smode == SMD_SIDE) {
    side_ofs.x = -5.0f;
    side_ofs.y = -13.0f;
    side_ofs.z = 0.0f;
    njPushMatrixEx();
    njUnitMatrix(NULL);
    njRotateY(NULL, ptwp->ang.y);
    njCalcVector(NULL, &side_ofs, &side_ofs);
    njPopMatrixEx();
    twp->pos.x += side_ofs.x;
    twp->pos.y += side_ofs.y;
    twp->pos.z += side_ofs.z;
    twp->cwp->info->center.y = -_rename_rocket_colli_info[0].center.y;
  } else {
    ofs.x = -5.0f;
    ofs.y = -10.0f;
    ofs.z = 0.0f;
    njPushMatrixEx();
    njUnitMatrix(NULL);
    njRotateY(NULL, ptwp->ang.y);
    njCalcVector(NULL, &ofs, &ofs);
    njPopMatrixEx();
    twp->pos.x += ofs.x;
    twp->pos.y += ofs.y;
    twp->pos.z += ofs.z;
  }

  twp->smode = -1;
  twp->mode = MD_BODY_INIT;
  tp->work.l = 0;
}

static void RocketBodyDisp(task *tp) {
  taskwk *twp = tp->twp;
  task *ptp = tp->ptp;
  taskwk *ptwp = ptp->twp;

  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, 0x4000);
  njRotateZ(NULL, -0x4000);
  njRotateX(NULL, 0x4000);
  if (ptwp->smode == SMD_SIDE) {
    njTranslate(NULL, _rename_rocket_ofs.x, -_rename_rocket_ofs.y,
                -_rename_rocket_ofs.z);
  } else {
    njTranslate(NULL, -_rename_rocket_ofs.x, -_rename_rocket_ofs.y,
                -_rename_rocket_ofs.z);
  }
  njSetTexture(&_rename_rocket_texlist);
  if (ptwp->smode == SMD_SIDE) {
    njRotateY(NULL, 0x8000);
  }
  if (GetWork(ptp)->body == tp) {
    fn_8011E214(&_rename_rocket_object, &_rename_rocket_motion,
                GetWork(ptp)->frame);
  } else {
    fn_8011E214(&_rename_rocket_object, &_rename_rocket_motion, 7.0f);
  }
  njPopMatrixEx();
}

static void RocketBodyExec(task *tp) {
  taskwk *twp = tp->twp;
  task *ptp = tp->ptp;
  taskwk *ptwp = ptp->twp;
  Float r;
  Float scl;
  Float *vp;
  particle *p;
  BOOL side;
  NJS_VECTOR v;
  NJS_VECTOR spd;
  NJS_POINT3 pos;
  NJS_POINT3 ppos;
  Angle3 ang;
  NJS_POINT3 hang_pos;

  if (ptp->mwp == NULL) {
    return;
  }

  if (twp->smode >= 0) {
    if ((ptwp->btimer != 0 || !lbl_801CC168._37) &&
        !(lbl_801CC168._37 & (1 << twp->smode))) {
      if (!lbl_801CC168._37) {
        ptwp->btimer = 0;
      }
      tp->work.l++;
      v.x = -twp->pos.x + ptwp->scl.x;
      v.y = -twp->pos.y + ptwp->scl.y;
      v.z = -twp->pos.z + ptwp->scl.z;

      if (twp->mode > MD_BODY_WAIT) {
        r = njScalor(&v);
        if (r < 9.0f || lbl_801CC168._42) {
          // arrived
          twp->pos.x += v.x;
          twp->pos.y += v.y;
          twp->pos.z += v.z;
          spd.x = 0.0f;
          spd.y = 0.0f;
          spd.z = 0.0f;
          fn_800E29BC(&twp->pos, &spd, 15.0f);
          FreeTask(tp);
          SetInputP(twp->smode, 15, 0);
          return;
        }
        r = 9.0f / r;
        v.x *= r;
        v.y *= r;
        v.z *= r;
        twp->pos.x += v.x;
        twp->pos.y += v.y;
        twp->pos.z += v.z;
      }

      if (twp->mode > MD_BODY_WAIT && tp->work.l % 19 == 0) {
        if (tp->work.l < 2) {
          fn_8002FB2C(twp->smode, 3, 60, 1);
        } else {
          fn_8002FB2C(twp->smode, 1, 60, 1);
        }
      }

      if (twp->mode > MD_BODY_WAIT) {
        pos.x = 0.0f;
        pos.y = -0.2f;
        pos.z = 0.0f;
        vp = &v.x;
        *vp++ *= 0.5f + 0.5f * ParticleRandom();
        *vp++ *= 0.5f + 0.5f * ParticleRandom();
        *vp *= 0.5f + 0.5f * ParticleRandom();
        njPushMatrixEx();
        njUnitMatrix(NULL);
        njTranslateEx(&twp->pos);
        njRotateY(NULL, twp->ang.y);
        njRotateZ(NULL, twp->ang.z);
        njRotateX(NULL, twp->ang.x);
        njRotateY(NULL, 0x4000);
        njRotateZ(NULL, -0x4000);
        njRotateX(NULL, 0x4000);
        if (ptwp->smode == SMD_SIDE) {
          njTranslate(NULL, _rename_rocket_ofs.x, -_rename_rocket_ofs.y,
                      -_rename_rocket_ofs.z);
        } else {
          njTranslate(NULL, -_rename_rocket_ofs.x, -_rename_rocket_ofs.y,
                      -_rename_rocket_ofs.z);
        }
        v.x += 0.25f * (ParticleRandom() - 0.5f);
        v.y += 0.25f * (ParticleRandom() - 0.5f);
        v.z += 0.25f * (ParticleRandom() - 0.5f);
        njCalcPoint(NULL, &pos, &pos);
        scl = 3.5f + 3.0f * ParticleRandom();
        p = CreateRocketThrust(&pos, &v, scl);
        if (p != NULL) {
          p->argb = 0xA0FFFFFF;
        }
        njPopMatrixEx();
      }

      twp->cwp->info[1].attr |= 0x10;
      if (tp->work.l > 10) {
        SetAttack(twp->cwp->info, 2);
      }

      if (twp->smode >= 0) {
        // hang the player on the handle
        njPushMatrixEx();
        njUnitMatrix(NULL);
        njTranslateEx(&twp->pos);
        njRotateY(NULL, twp->ang.y);
        side = twp->mode != MD_BODY_WAIT && ptwp->smode == SMD_SIDE;
        njRotateZ(NULL, side ? 0x4000 : 0);
        fn_800133BC(&ang.x, &ang.y, &ang.z);
        if (twp->mode == MD_BODY_WAIT) {
          njCalcPoint(NULL, &_rename_rocket_hang_pos, &ppos);
        } else {
          hang_pos.z = 0.0f;
          switch (playerpwp[twp->smode]->character) {
          case PLNO_SONIC:
          case PLNO_SHADOW:
            hang_pos.x = -4.3f;
            hang_pos.y = -4.9f;
            break;
          case PLNO_METAL_SONIC:
            hang_pos.x = -4.0f;
            hang_pos.y = -6.0f;
            break;
          case PLNO_ROUGE:
            hang_pos.x = -3.5f;
            hang_pos.y = -5.6f;
            break;
          case PLNO_AMY:
            hang_pos.x = -4.0f;
            hang_pos.y = -5.4f;
            break;
          case PLNO_CHAOS0:
            hang_pos.x = -6.0f;
            hang_pos.y = -6.0f;
            break;
          case PLNO_TICAL:
            hang_pos.x = -4.1f;
            hang_pos.y = -5.3f;
            break;
          case PLNO_KNUCKLES:
            hang_pos.x = -5.6f;
            hang_pos.y = -6.0f;
            break;
          }
          njCalcPoint(NULL, &hang_pos, &ppos);
        }
        njPopMatrixEx();
        fn_800399BC(twp->smode, ppos.x, ppos.y, ppos.z);
        playertwp[twp->smode]->ang.x = ang.x;
        playertwp[twp->smode]->ang.y = -ang.y;
        playertwp[twp->smode]->ang.z = ang.z;
        fn_8006B1FC(0x1019, ptwp, 1, 0, 30);
      }
    }
  } else {
    twp->cwp->info[1].attr &= ~0x10;
    ClearAttack(twp->cwp->info);
  }

  CCL_Entry(tp);
}

static void RocketBodyDest(task *tp) {
  taskwk *twp = tp->twp;
  task *ptp = tp->ptp;

  if (twp->mode > MD_BODY_WAIT) {
    SetInputP(twp->smode, 15, 0);
    GetWork(ptp)->ride--;
  }
  if (ptp->mwp != NULL && GetWork(ptp)->body == tp) {
    GetWork(ptp)->body = NULL;
  }
  tp->fwp = NULL;
}
