#include "OBJECT/o_kdwarphole.h"

#include "samt/ninja/njmatrix.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/camera.h"
#include "samt/sonic/player.h"
#include "set.h"

extern Sint8 fn_80065388(task *tp);
extern void fn_800399BC(Sint32 pno, Float x, Float y, Float z);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);

extern Sint32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, Float);
extern void _rename_SetConditionFlag(task *tp, Uint8 smode);

// the two holes sit at twp->pos and twp->scl; entering one exits the other
// the two halves of the executor are not quite symmetric, as in the original
extern Sint32 _rename_GetKnuDaiMode(Sint32 no);
extern Sint32 _rename_GetKnuDaiPos(Sint32 no, NJS_POINT3 *pos);
extern task *CreateWpHole(NJS_POINT3 *pos, Float scale, Sint32 num,
                                  Uint8 flag, Float alpha, task **ptp);

extern Float _rename_wphole_size;  // 10.0f
extern Float _rename_wphole_scale; // 4.0f
extern Float _rename_wphole_alpha; // 0.4f

extern Sint32 lbl_803ADAD0;       // screen being drawn
extern camposwk *lbl_80175378[4]; // camera position work of each screen

// ^ extern
// v in this file

static void ObjectKDWarpHoleDest(task *tp);
static void ObjectKDWarpHoleExec(task *tp);

enum {
  MD_WAIT,   // closed, waiting for its ring group
  MD_OPEN,   // opening
  MD_OPENED, // fully open
};

typedef struct kdwarpholewk // sizeof=0x14
{
  /* 0x00 */ Float ratio;
  /* 0x04 */ Float size;
  /* 0x08 */ task *hole0;
  /* 0x0C */ task *hole1;
  /* 0x10 */ Sint16 pno0;
  /* 0x12 */ Sint16 pno1;
} kdwarpholewk;

#define GetWork(task) ((kdwarpholewk *)task->mwp)

// the hole task keeps its scale and alpha in pointer slots it does not use
#define GetHoleScale(task) (*(Float *)&task->fwp)
#define GetHoleAlpha(task) (*(Float *)&task->awp)

// the second hole's position in twp->scl, read as an array to match
#define HoleVec(twp) ((Float *)&(twp)->scl)

#define DegAng(n) ((Angle)(182.04445f * (n)))

// grows both holes, shared by MD_OPEN and MD_OPENED
#define KDWarpHoleGrowHoles(tp, twp)                                           \
  if (GetWork(tp)->hole0 != NULL) {                                            \
    GetHoleScale(GetWork(tp)->hole0) =                                         \
        GetWork(tp)->size * _rename_wphole_scale;                              \
    GetHoleAlpha(GetWork(tp)->hole0) =                                         \
        _rename_wphole_alpha * (GetWork(tp)->size / _rename_wphole_size);      \
    GetWork(tp)->hole0->work.f = GetWork(tp)->ratio;                           \
  } else {                                                                     \
    CreateWpHole(&twp->pos, GetWork(tp)->size * _rename_wphole_scale,  \
                         0xFE, 0, _rename_wphole_alpha,                        \
                         &GetWork(tp)->hole0);                                 \
  }                                                                            \
  if (GetWork(tp)->hole1 != NULL) {                                            \
    GetHoleScale(GetWork(tp)->hole1) =                                         \
        GetWork(tp)->size * _rename_wphole_scale;                              \
    GetHoleAlpha(GetWork(tp)->hole1) =                                         \
        _rename_wphole_alpha * (GetWork(tp)->size / _rename_wphole_size);      \
    GetWork(tp)->hole1->work.f = GetWork(tp)->ratio;                           \
  } else {                                                                     \
    task *htp =                                                                \
        CreateWpHole(&twp->scl, GetWork(tp)->size * _rename_wphole_scale, \
                             0xFE, 1, _rename_wphole_alpha,                    \
                             &GetWork(tp)->hole1);                             \
    if (htp != NULL) {                                                         \
      htp->twp->smode = 1;                                                     \
    }                                                                          \
  }

void ObjectKDWarpHole(task *tp) {
  taskwk *twp = tp->twp;

  if (tp->ocp != NULL && fn_80065388(tp) != 0) {
    twp->mode = MD_OPENED;
  }
  if (twp->mode != MD_OPENED && CheckRangeOut(tp)) {
    return;
  }
  tp->mwp = syCalloc(1, sizeof(kdwarpholewk));
  if (GetWork(tp) == NULL) {
    return;
  }
  if (twp->mode == MD_OPENED) {
    GetWork(tp)->size = _rename_wphole_size;
    GetWork(tp)->ratio = 1.0f;
  }
  tp->disp = NULL;
  tp->exec = ObjectKDWarpHoleExec;
  tp->dest = ObjectKDWarpHoleDest;
  GetWork(tp)->hole0 = NULL;
  GetWork(tp)->hole1 = NULL;
  twp->btimer = 0;
  GetWork(tp)->pno0 = -1;
  GetWork(tp)->pno1 = -1;
}

static void ObjectKDWarpHoleDest(task *tp) {
  if (GetWork(tp)->pno0 >= 0 && playertwp[GetWork(tp)->pno0] != NULL) {
    playertwp[GetWork(tp)->pno0]->scl.x = 1.0f;
    playertwp[GetWork(tp)->pno0]->scl.y = 1.0f;
    playertwp[GetWork(tp)->pno0]->scl.z = 1.0f;
  }
  if (GetWork(tp)->pno1 >= 0 && playertwp[GetWork(tp)->pno1] != NULL) {
    playertwp[GetWork(tp)->pno1]->scl.x = 1.0f;
    playertwp[GetWork(tp)->pno1]->scl.y = 1.0f;
    playertwp[GetWork(tp)->pno1]->scl.z = 1.0f;
  }
  if (GetWork(tp)->hole0 != NULL) {
    DestroyTask(GetWork(tp)->hole0);
  }
  if (GetWork(tp)->hole1 != NULL) {
    DestroyTask(GetWork(tp)->hole1);
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectKDWarpHoleExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 no;
  Float scl;
  Float len;
  NJS_POINT3 pos;
  Sint32 unused0[4]; // unused
  NJS_VECTOR v0;
  Sint32 unused1[4];
  NJS_VECTOR v1;

  if (twp->mode == MD_WAIT && CheckRangeOut(tp)) {
    return;
  }
  switch (twp->mode) {
  case MD_WAIT:
    if (_rename_GetKnuDaiMode((twp->ang.x & 0xF) % 8) != 1) {
      return;
    }
    if (!_rename_GetKnuDaiPos((twp->ang.x & 0xF) % 8, &pos)) {
      return;
    }
    GetWork(tp)->ratio = 0.0f;
    GetWork(tp)->size = 0.0f;
    twp->mode = MD_OPEN;
    if (tp->ocp != NULL) {
      _rename_SetConditionFlag(tp, 1);
    }
    break;

  case MD_OPEN:
    GetWork(tp)->ratio += 0.0125f;
    GetWork(tp)->size += _rename_wphole_size / 80.0f;
    if (GetWork(tp)->ratio > 1.0f) {
      GetWork(tp)->ratio = 1.0f;
      GetWork(tp)->size = _rename_wphole_size;
      twp->mode = MD_OPENED;
    }
    fn_8006AFFC(0x101F, twp, 1, 0, 30, &twp->pos);
    KDWarpHoleGrowHoles(tp, twp);
    break;

  case MD_OPENED:
    KDWarpHoleGrowHoles(tp, twp);

    no = _rename_EitherPlayerWithinSphere(&twp->pos, 1.5f * _rename_wphole_size);
    if (no != 0) {
      if (twp->btimer == 0 && GetWork(tp)->pno0 < 0) {
        GetWork(tp)->pno0 = no - 1;
        twp->wtimer = 0;
        SetInputP(GetWork(tp)->pno0, 9, 15);
        twp->smode = 0;
      }
      twp->btimer = 60;
    }
    no = _rename_EitherPlayerWithinSphere(&twp->scl, 1.5f * _rename_wphole_size);
    if (no != 0) {
      if (twp->btimer == 0 && GetWork(tp)->pno1 < 0) {
        GetWork(tp)->pno1 = no - 1;
        twp->wtimer = 0;
        twp->smode = 0;
        SetInputP(GetWork(tp)->pno1, 9, 15);
      }
      twp->btimer = 60;
    }

    // the player that went into the hole at twp->pos comes out at twp->scl
    if (GetWork(tp)->pno0 >= 0) {
      if (lbl_801CC168._42 == 1 ||
          playertwp[GetWork(tp)->pno0]->mode == 15 ||
          (playerpwp[GetWork(tp)->pno0]->item & 0x4000)) {
        playertwp[GetWork(tp)->pno0]->scl.x = 1.0f;
        playertwp[GetWork(tp)->pno0]->scl.y = 1.0f;
        playertwp[GetWork(tp)->pno0]->scl.z = 1.0f;
        SetInputP(GetWork(tp)->pno0, 15, 0);
        GetWork(tp)->pno0 = -1;
      } else if (twp->smode != 0) {
        Float ofs = 0.0f;
        scl = playertwp[GetWork(tp)->pno0]->scl.x;
        if (scl < 1.0f) {
          twp->wtimer += DegAng(3.0f * (1.0f - scl));
          scl += 0.008333334f;
          if (scl > 1.0f) {
            scl = 1.0f;
          }
          playertwp[GetWork(tp)->pno0]->ang.y =
              lbl_80175378[lbl_803ADAD0]->ang.y;
          playertwp[GetWork(tp)->pno0]->ang.x = twp->wtimer;
          playertwp[GetWork(tp)->pno0]->ang.z = twp->wtimer;
          fn_800399BC(GetWork(tp)->pno0, twp->scl.x + ofs, twp->scl.y + ofs,
                      twp->scl.z + ofs);
          playertwp[GetWork(tp)->pno0]->scl.x = scl;
          playertwp[GetWork(tp)->pno0]->scl.y = scl;
          playertwp[GetWork(tp)->pno0]->scl.z = scl;
          fn_8006AFFC(0x1018, twp, 1, 0, 30, &twp->scl);
        } else {
          fn_800399BC(GetWork(tp)->pno0, twp->scl.x + ofs, twp->scl.y + ofs,
                      twp->scl.z + ofs);
          playertwp[GetWork(tp)->pno0]->scl.x = scl;
          playertwp[GetWork(tp)->pno0]->scl.y = scl;
          playertwp[GetWork(tp)->pno0]->scl.z = scl;
          SetInputP(GetWork(tp)->pno0, 15, 0);
          GetWork(tp)->pno0 = -1;
        }
      } else {
        twp->wtimer += 0x222;
        v0.x = -twp->pos.x + playertwp[GetWork(tp)->pno0]->pos.x;
        v0.y = -twp->pos.y + playertwp[GetWork(tp)->pno0]->pos.y;
        v0.z = -twp->pos.z + playertwp[GetWork(tp)->pno0]->pos.z;
        v0.x *= 0.96f;
        v0.y *= 0.96f;
        v0.z *= 0.96f;
        len = njScalor(&v0);
        playertwp[GetWork(tp)->pno0]->ang.y = lbl_80175378[lbl_803ADAD0]->ang.y;
        playertwp[GetWork(tp)->pno0]->ang.x = twp->wtimer;
        playertwp[GetWork(tp)->pno0]->ang.z = twp->wtimer;
        if (len < 0.5f) {
          twp->smode = 1;
          twp->wtimer = 0;
        } else {
          if (len < _rename_wphole_size) {
            Float r = len / _rename_wphole_size;
            playertwp[GetWork(tp)->pno0]->scl.x = r;
            playertwp[GetWork(tp)->pno0]->scl.y = r;
            playertwp[GetWork(tp)->pno0]->scl.z = r;
          }
          fn_800399BC(GetWork(tp)->pno0, v0.x + twp->pos.x, twp->pos.y + v0.y,
                      twp->pos.z + v0.z);
          fn_8006AFFC(0x1017, twp, 1, 0, 30, &twp->pos);
        }
      }
    }

    // and the one that went into the hole at twp->scl comes out at twp->pos
    if (GetWork(tp)->pno1 >= 0) {
      if (lbl_801CC168._42 == 1 ||
          playertwp[GetWork(tp)->pno1]->mode == 15 ||
          (playerpwp[GetWork(tp)->pno1]->item & 0x4000)) {
        playertwp[GetWork(tp)->pno1]->scl.x = 1.0f;
        playertwp[GetWork(tp)->pno1]->scl.y = 1.0f;
        playertwp[GetWork(tp)->pno1]->scl.z = 1.0f;
        SetInputP(GetWork(tp)->pno1, 15, 0);
        GetWork(tp)->pno1 = -1;
      } else if (twp->smode != 0) {
        Float ofs = 0.0f;
        scl = playertwp[GetWork(tp)->pno1]->scl.x;
        if (scl < 1.0f) {
          twp->wtimer += DegAng(3.0f * (1.0f - scl));
          scl += 0.008333334f;
          if (scl > 1.0f) {
            scl = 1.0f;
          }
          playertwp[GetWork(tp)->pno1]->ang.y =
              lbl_80175378[lbl_803ADAD0]->ang.y;
          playertwp[GetWork(tp)->pno1]->ang.x = twp->wtimer;
          playertwp[GetWork(tp)->pno1]->ang.z = twp->wtimer;
          fn_800399BC(GetWork(tp)->pno1, twp->pos.x + ofs, twp->pos.y + ofs,
                      twp->pos.z + ofs);
          playertwp[GetWork(tp)->pno1]->scl.x = scl;
          playertwp[GetWork(tp)->pno1]->scl.y = scl;
          playertwp[GetWork(tp)->pno1]->scl.z = scl;
          fn_8006AFFC(0x1018, twp, 1, 0, 30, &twp->pos);
        } else {
          fn_800399BC(GetWork(tp)->pno1, twp->pos.x + ofs, twp->pos.y + ofs,
                      twp->pos.z + ofs);
          playertwp[GetWork(tp)->pno1]->scl.x = scl;
          playertwp[GetWork(tp)->pno1]->scl.y = scl;
          playertwp[GetWork(tp)->pno1]->scl.z = scl;
          SetInputP(GetWork(tp)->pno1, 15, 0);
          GetWork(tp)->pno1 = -1;
        }
      } else {
        twp->wtimer += 0x222;
        v1.x = -HoleVec(twp)[0] + playertwp[GetWork(tp)->pno1]->pos.x;
        v1.y = -HoleVec(twp)[1] + playertwp[GetWork(tp)->pno1]->pos.y;
        v1.z = -HoleVec(twp)[2] + playertwp[GetWork(tp)->pno1]->pos.z;
        v1.x *= 0.96f;
        v1.y *= 0.96f;
        v1.z *= 0.96f;
        len = njScalor(&v1);
        playertwp[GetWork(tp)->pno1]->ang.y = lbl_80175378[lbl_803ADAD0]->ang.y;
        playertwp[GetWork(tp)->pno1]->ang.x = twp->wtimer;
        playertwp[GetWork(tp)->pno1]->ang.z = twp->wtimer;
        if (len < 0.5f) {
          twp->smode = 1;
        } else {
          if (len < _rename_wphole_size) {
            Float r = len / _rename_wphole_size;
            playertwp[GetWork(tp)->pno1]->scl.x = r;
            playertwp[GetWork(tp)->pno1]->scl.y = r;
            playertwp[GetWork(tp)->pno1]->scl.z = r;
          }
          fn_800399BC(GetWork(tp)->pno1, v1.x + HoleVec(twp)[0], HoleVec(twp)[1] + v1.y,
                      HoleVec(twp)[2] + v1.z);
          fn_8006AFFC(0x1017, twp, 1, 0, 30, &twp->scl);
        }
      }
    }

    if (GetWork(tp)->pno0 < 0 && GetWork(tp)->pno1 < 0 && twp->btimer != 0) {
      twp->btimer--;
    }
    break;
  }
}
