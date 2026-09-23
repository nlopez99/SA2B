#include "OBJECT/o_skull.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"

extern int rand(void);

// the stage's movable-object helper; it hangs a physics record off 'fwp'
extern void _rename_MovableObjInit(task *tp);
extern void _rename_MovableObjFree(task *tp);
extern void _rename_MovableObjExec(task *tp);

extern Sint32 _rename_EitherPlayerWithinSphere(NJS_POINT3 *pos, Float r);
extern void _rename_SetConditionFlag(task *tp, Uint8 smode);

extern void fn_8011E158(NJS_CNK_MODEL *model);
extern void fn_8002B304(void);
extern void fn_8002B35C(void);
extern void fn_8002B348(void);
extern void fn_8002B2F8(void);
extern void fn_801218C8(Sint32 a, Sint32 b);
extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern Sint8 fn_80065388(task *tp);
// spawns one of the three debris pieces at a point
extern void fn_800260FC(Float x, Float y, Float z, Sint32 kind);

extern NJS_TEXLIST   _rename_skull_texlist;
extern NJS_CNK_MODEL _rename_skull_model;
extern CCL_INFO      _rename_pskull_colli_info[3];
extern CCL_INFO      _rename_skull_colli_info[1];

// ^ extern
// v in this file

static void ObjectSkullDest(task *tp);
static void ObjectPSkullExec(task *tp);
static void ObjectSkullDisp(task *tp);
static void ObjectSkullExec(task *tp);

// the fade level, kept in the unused 'awp' slot; 1.0 is fully opaque
#define GetAlpha(tp) (*(Float *)&(tp)->awp)

// the physics record the movable-object helper hangs off 'fwp'
#define PhysFlag(tp)   (*(Uint32 *)((Uint8 *)(tp)->fwp + 0x60))
#define PhysWeight(tp) (*(Float *)((Uint8 *)(tp)->fwp + 0x84))

// setting it on a CCL_INFO takes that collider out of the test
#define CI_ATTR_OFF (0x10)

// the set flag that says this skull has been picked up
#define SKULL_HELD (0x8000)

// how long the skull sits untouched before it starts fading away
#define SKULL_LIFE (0xB40)

static void ObjectPSkullInit(task *tp) {
  taskwk *twp = tp->twp;
  motionwk *mwp = tp->mwp;

  tp->disp = ObjectSkullDisp;
  tp->exec = ObjectPSkullExec;
  tp->dest = ObjectSkullDest;
  twp->smode = 0;
  twp->btimer = twp->ang.z;
  CCL_Init(tp, _rename_pskull_colli_info, ARYLEN(_rename_pskull_colli_info),
           CID_OBJECT);
  // only the third volume is live until the skull is thrown
  twp->cwp->info[0].attr |= CI_ATTR_OFF;
  twp->cwp->info[1].attr |= CI_ATTR_OFF;
  twp->cwp->info[2].attr &= ~CI_ATTR_OFF;
  _rename_MovableObjInit(tp);

  if (tp->fwp != NULL) {
    PhysWeight(tp) = 0.1f;
    PhysFlag(tp) |= 0x10;
  }

  twp->smode = 2;
  GetAlpha(tp) = 1.0f;
  // the helper reads this slot as an integer, not as 'force'
  *(Sint32 *)&mwp->force = 0x1300;
}

// dead in most stages, kept by a few
void ObjectSkullObj(task *tp) {
  if (CheckRangeOut(tp)) {
    return;
  }

  ObjectPSkullInit(tp);
}

void ObjectPSkull(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  ObjectPSkullInit(tp);
  twp->smode = 0;

  if (twp->cwp == NULL) {
    return;
  }

  // the placed skull is the other way round: solid, and not yet a projectile
  twp->cwp->info[0].attr &= ~CI_ATTR_OFF;
  twp->cwp->info[1].attr &= ~CI_ATTR_OFF;
  twp->cwp->info[2].attr |= CI_ATTR_OFF;
}

// throws a fresh skull from 'pos' at a random facing; dead in most stages
task *CreateSkullObj(NJS_POINT3 *pos, Float spdy) {
  task *tp;
  taskwk *twp;
  motionwk *mwp;

  tp = CreateFundamentalTask(IM_MWK | IM_TWK, LEV_2, ObjectSkullObj);

  if (tp != NULL) {
    twp = tp->twp;
    mwp = tp->mwp;
    ObjectPSkullInit(tp);
    twp->pos = *pos;
    twp->ang.y =
        (Angle)(182.04445f * (360.0f * (0.000030517578f * (Float)rand())));
    mwp->spd.y = spdy;
    mwp->spd.x = 0.0f;
    mwp->spd.z = 0.0f;
    twp->smode = 1;
  }

  return tp;
}

static void ObjectSkullDest(task *tp) {
  _rename_MovableObjFree(tp);
  tp->awp = NULL;
}

// which player is carrying this skull, or -1
static Sint32 SkullGetHoldPlayerNo(task *tp) {
  Sint32 i;

  for (i = 0; i < 2; ++i) {
    if (playerpwp[i] != NULL && playerpwp[i]->htp == tp) {
      return i;
    }
  }

  return -1;
}

static void ObjectPSkullExec(task *tp) {
  motionwk *mwp = tp->mwp;
  taskwk *twp = tp->twp;
  Float unused[2]; // unused
  Float rate;

  if (CheckRangeOut(tp)) {
    return;
  }

  switch (twp->smode) {
  case 1:
    // thrown: fly until the timer runs out, or until falling again
    if ((twp->wtimer == 0 && mwp->spd.y < 0.0f) ||
        (twp->wtimer != 0 && --twp->wtimer == 0)) {
      twp->wtimer = 0;
      twp->smode = 0;
      twp->cwp->info[0].attr &= ~CI_ATTR_OFF;
      twp->cwp->info[1].attr &= ~CI_ATTR_OFF;
      twp->cwp->info[2].attr |= CI_ATTR_OFF;
      return;
    }

    mwp->spd.y *= 0.99f;
    mwp->spd.z *= 0.99f;
    mwp->spd.x *= 0.99f;
    mwp->spd.y -= 0.1f;
    twp->pos.y += mwp->spd.y;
    twp->pos.x += mwp->spd.x;
    twp->pos.z += mwp->spd.z;
    CCL_Entry(tp);
    return;

  case 2:
    // carried: a hit on the carry volume launches it
    if (twp->cwp->flag & 1) {
      mwp->spd.y = 2.0f;
      twp->smode = 1;
    }
    twp->cwp->info[2].attr &= ~CI_ATTR_OFF;
    CCL_Entry(tp);
    return;

  default:
    if (twp->flag & SKULL_HELD) {
      Sint32 pno;

      GetAlpha(tp) = 1.0f;
      pno = SkullGetHoldPlayerNo(tp);

      if (pno >= 0) {
        twp->ang.y = 0x4000 - playertwp[pno]->ang.y;

        if (tp->ocp != NULL && fn_80065388(tp) == 0) {
          _rename_SetConditionFlag(tp, 1);

          // the top nibble of the set data is how much debris it sheds
          if ((twp->btimer & 0xF0) != 0) {
            rate = (Float)((Uint8)(twp->btimer & 0xF0) >> 4);
            rate *= 0.1f;

            if (0.000030517578f * (Float)rand() < 0.05f + rate) {
              fn_800260FC(twp->pos.x, 3.0f + twp->pos.y, twp->pos.z,
                          (twp->btimer & 0xF) % 3);
            }
          }
        }
      }

      twp->wtimer = 0;
    } else if (twp->wtimer < SKULL_LIFE) {
      twp->wtimer++;
      GetAlpha(tp) = 1.0f;
    } else if (!_rename_EitherPlayerWithinSphere(&twp->pos, 50.0f)) {
      // nobody is near it any more: fade out, then take it off the stage
      if (GetAlpha(tp) <= 0.01f) {
        if (tp->ocp != NULL) {
          DeadOut(tp);
          return;
        }
        FreeTask(tp);
        return;
      }

      GetAlpha(tp) -= 0.016666668f;
      if (GetAlpha(tp) < 0.0f) {
        GetAlpha(tp) = 0.0f;
      }
    } else {
      GetAlpha(tp) = 1.0f;
    }

    _rename_MovableObjExec(tp);
    return;
  }
}

static void ObjectSkullDisp(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_skull_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);

  if (GetAlpha(tp) >= 1.0f) {
    fn_8011E158(&_rename_skull_model);
  } else {
    fn_8002B304();
    fn_8002B35C();
    OffControl3D(0x220);
    OnControl3D(0x810);
    fn_801218C8(0xFF, 0x800);
    fn_800156FC(GetAlpha(tp), 1.0f, 1.0f, 1.0f);
    fn_8011E158(&_rename_skull_model);
    fn_8002B348();
    fn_8002B2F8();
  }

  njPopMatrixEx();
}

static void ObjectSkullExec(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  if (twp->scl.x < 1.0f) {
    CCL_Entry(tp);
  }
}

void ObjectSkull(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->disp = ObjectSkullDisp;
  tp->exec = ObjectSkullExec;
  tp->dest = ObjectSkullDest;
  GetAlpha(tp) = 1.0f;

  if (twp->scl.x < 1.0f) {
    CCL_Init(tp, _rename_skull_colli_info, ARYLEN(_rename_skull_colli_info),
             CID_OBJECT);
  }
}
