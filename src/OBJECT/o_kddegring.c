#include "OBJECT/o_kddegring.h"

#include "OBJECT/o_ring.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "set.h"

extern NJS_TEXLIST _rename_ring_texlist;
extern Sint32 lbl_803ADAD0;         // active camera
extern NJS_POINT3 *lbl_80175378[];  // camera positions

extern Sint8 fn_80065388(task *tp);
extern void fn_80033620(NJS_POINT3 *pos, Sint32 kind, Angle ang, Sint32 unk,
                        Float w, Float h, Sint32 col, Sint32 unk2, Float u,
                        Float v);
extern void fn_8006AFFC(Sint32 id, taskwk *twp, Sint32 unk, Sint32 vol,
                        Sint32 unk2, NJS_POINT3 *pos);
extern void _rename_SetConditionFlag(task *tp, Sint8 flag);
extern void _rename_RingSetGroup(task *tp, void *entry);
extern void _rename_RingClearGroup(task *tp);
extern Sint32 _rename_GetStageNum(void);
extern Sint32 _rename_GetRingGroupState(Sint32 no);
extern Sint32 _rename_GetRingGroupPos(Sint32 no, NJS_POINT3 *pos);

// ^ extern
// v in this file

static void KdDegRingInitWork(task *tp);
static void ObjectKdDegRingDest(task *tp);
static void ObjectKdDegRingDisp(task *tp);
static void ObjectKdDegRingLinearExec(task *tp);
static void ObjectKdDegRingCircleExec(task *tp);

#define KDDEGRING_MAX 8

// which ring group this set object belongs to
#define GetGroupNo(twp) (((twp)->ang.z & 0xF) % KDDEGRING_MAX)

typedef struct kddegringent // sizeof=0x8
{
  /* 0x00 */ Sint32 flag;
  /* 0x04 */ task *tp;
} kddegringent;

typedef struct kddegringwk // sizeof=0xAC
{
  /* 0x00 */ kddegringent ring[KDDEGRING_MAX];
  /* 0x40 */ NJS_POINT3 pos[KDDEGRING_MAX];
  /* 0xA0 */ NJS_POINT3 goal; // where the group is coming from
} kddegringwk;

#define GetWork(task) ((kddegringwk *)task->mwp)

static Sint32 KdDegRingGetNum(taskwk *twp) {
  Sint32 num = twp->scl.z;

  if (num < 1) {
    num = 1;
  }
  if (num > KDDEGRING_MAX) {
    num = KDDEGRING_MAX;
  }
  return num;
}

static void KdDegRingCalcPosLinear(NJS_POINT3 *pos, Angle angx, Angle angy,
                                   Angle angz, Angle range, Float height,
                                   NJS_POINT3 *out, Sint32 num, Float dist) {
  NJS_POINT3 v;
  Sint32 i;
  Angle ang;

  njPushMatrix(NULL);
  njUnitMatrix(NULL);
  njTranslateV(NULL, pos);
  njRotateZ(NULL, angz);
  njRotateY(NULL, angy);
  njRotateX(NULL, angx);
  v.x = 0.0f;
  v.y = 0.0f;
  v.z = 0.0f;
  range /= num;
  ang = 0;
  for (i = 0; i < num; i++) {
    v.y = height * njSin(ang);
    njCalcPoint(NULL, &v, &out[i]);
    v.z -= dist;
    ang += range;
  }
  njPopMatrix(1);
}

static void KdDegRingCalcPosCircle(NJS_POINT3 *pos, Angle angx, Angle angy,
                                   Angle angz, Angle range, Float r,
                                   Sint32 num, NJS_POINT3 *out) {
  NJS_POINT3 v;
  Sint32 i;
  Angle ang;

  range /= num;

  njPushMatrix(NULL);
  njUnitMatrix(NULL);
  njTranslateV(NULL, pos);
  njRotateZ(NULL, angz);
  njRotateY(NULL, angy);
  njRotateX(NULL, angx);
  v.x = 0.0f;
  v.y = 0.0f;
  v.z = 0.0f;
  ang = 0;
  for (i = 0; i < num; i++) {
    v.y = r * njSin(ang);
    v.z = r * njCos(ang);
    njCalcPoint(NULL, &v, &out[i]);
    ang += range;
  }
  njPopMatrix(1);
}

void ObjectKdDegRingLinear(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 goal;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(kddegringwk));
  if (tp->mwp == NULL) {
    return;
  }

  if (tp->ocp == NULL) {
    if (fn_80065388(tp)) {
      twp->mode = 2;
    } else if (_rename_GetRingGroupState(GetGroupNo(twp)) == 1 &&
               _rename_GetRingGroupPos(GetGroupNo(twp), &goal)) {
      twp->mode = 1;
    }
  }

  tp->disp = ObjectKdDegRingDisp;
  tp->exec = ObjectKdDegRingLinearExec;
  tp->dest = ObjectKdDegRingDest;
  KdDegRingInitWork(tp);

  if (GetWork(tp) != NULL) {
    KdDegRingCalcPosLinear(&twp->pos, (Uint16)twp->ang.x, (Uint16)twp->ang.y, 0,
                           0x4000, twp->scl.y, GetWork(tp)->pos, twp->btimer,
                           10.0f + twp->scl.x);
  }

  if (twp->mode == 1) {
    GetWork(tp)->goal = goal;
    twp->wtimer = 20000;
  }
}

static void KdDegRingInitWork(task *tp) {
  taskwk *twp = tp->twp;
  Uint32 flag;
  Sint32 i;

  twp->btimer = KdDegRingGetNum(twp);
  if (tp->mwp == NULL) {
    return;
  }

  flag = 0;
  if (tp->ocp != NULL) {
    flag = fn_80065388(tp);
  }
  for (i = KDDEGRING_MAX - 1; i >= 0; i--) {
    if (flag & 1) {
      GetWork(tp)->ring[i].flag = 2;
    }
    flag >>= 1;
  }
}

static void ObjectKdDegRingDest(task *tp) {
  if (tp->mwp != NULL) {
    Sint32 i;
    Sint32 flag;
    task *ring;

    flag = 0;
    for (i = 0; i < KDDEGRING_MAX; i++) {
      if ((ring = GetWork(tp)->ring[i].tp) != NULL) {
        _rename_RingClearGroup(ring);
        FreeTask(ring);
      }
      flag = (flag << 1) | ((GetWork(tp)->ring[i].flag & 2) ? 1 : 0);
    }
    if (tp->ocp != NULL) {
      _rename_SetConditionFlag(tp, flag);
    }
  }

  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectKdDegRingLinearExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 found;
  kddegringent *ent;
  Sint32 i;
  task *ring;
  Sint32 left;
  NJS_POINT3 *pos;
  Float d;
  Sint32 unused[2]; // unused

  if (tp->mwp == NULL) {
    return;
  }

  found = 0;
  ent = GetWork(tp)->ring;
  for (i = 0; i < twp->btimer; i++) {
    if (ent->tp != NULL) {
      found = 1;
      break;
    }
    ent++;
  }
  if (!found && CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case 0:
    if (_rename_GetRingGroupState(GetGroupNo(twp)) == 1 &&
        _rename_GetRingGroupPos(GetGroupNo(twp), &GetWork(tp)->goal)) {
      twp->mode = 1;
    }
    break;
  case 1:
    if (twp->wtimer < 20000) {
      twp->wtimer++;
    }
    break;
  }

  ent = GetWork(tp)->ring;
  pos = GetWork(tp)->pos;
  found = 0; // reused here as the count of rings not ready to spawn yet
  left = 0;
  for (i = 0; i < twp->btimer; i++, ent++, pos++) {
    if (!(ent->flag & 2) && ent->tp == NULL) {
      switch (twp->mode) {
      case 1:
        // the rings stream in from the group's home, nearest first
        if (_rename_GetStageNum() != 0x22) {
          d = njDistanceP2P(pos, &GetWork(tp)->goal) * 0.8f;
          if (d > 160.0f) {
            d = 160.0f;
          }
          if (twp->wtimer < (Sint16)d) {
            found++;
            break;
          }
        }
        // fallthrough
      case 2:
        if ((ring = CreateElementalTask(IM_TWK, LEV_1, Ring, "Ring")) != NULL) {
          ent->flag = 0;
          ring->twp->pos = *pos;
          ring->twp->ang.x = 0;
          ring->twp->ang.y = 0;
          ring->twp->ang.z = 0;
          _rename_RingSetGroup(ring, ent);
        }
        break;
      }
    }
    if (!(ent->flag & 2)) {
      left++;
    }
  }

  if (twp->mode == 1 && found == 0) {
    twp->mode = 2;
  }
  if (left != 0) {
    return;
  }

  if (tp->ocp != NULL) {
    DeadOut(tp);
    return;
  }
  FreeTask(tp);
}

static void ObjectKdDegRingDisp(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 i;
  kddegringent *ent;
  NJS_POINT3 *pos;
  NJS_POINT3 p;
  Float d;
  Float t;

  njSetTexture(&_rename_ring_texlist);
  njDisableFog();
  gjSetFog();
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 10);

  if (tp->mwp != NULL) {
    ent = GetWork(tp)->ring;
    pos = GetWork(tp)->pos;
    for (i = 0; i < twp->btimer; i++, ent++, pos++) {
      if (ent->tp == NULL) {
        continue;
      }
      // pull the glow a little towards the camera
      p.x = pos->x - lbl_80175378[lbl_803ADAD0]->x;
      p.y = (2.0f + pos->y) - lbl_80175378[lbl_803ADAD0]->y;
      p.z = pos->z - lbl_80175378[lbl_803ADAD0]->z;
      d = njScalor(&p);
      if (d > 15.0f) {
        t = (d - 12.0f) / d;
      } else {
        t = 1.0f;
      }
      p.x = p.x * t + lbl_80175378[lbl_803ADAD0]->x;
      p.y = p.y * t + lbl_80175378[lbl_803ADAD0]->y;
      p.z = p.z * t + lbl_80175378[lbl_803ADAD0]->z;
      fn_80033620(&p, 4, 0x4000, 0, 3.0f, 3.0f, -1, 1, 0.0f, 0.0f);
    }
  }

  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  njEnableFog();
  gjSetFog();
}

void ObjectKdDegRingCircle(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 goal;
  NJS_POINT3 pos; // filled, but the unset 'goal' is copied, as in the original

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(kddegringwk));
  if (tp->mwp == NULL) {
    return;
  }

  if (tp->ocp == NULL) {
    if (fn_80065388(tp)) {
      twp->mode = 2;
    } else if (_rename_GetRingGroupState(GetGroupNo(twp)) == 1 &&
               _rename_GetRingGroupPos(GetGroupNo(twp), &pos)) {
      twp->mode = 1;
    }
  }

  tp->disp = ObjectKdDegRingDisp;
  tp->exec = ObjectKdDegRingCircleExec;
  tp->dest = ObjectKdDegRingDest;
  KdDegRingInitWork(tp);

  if (GetWork(tp) != NULL) {
    KdDegRingCalcPosCircle(&twp->pos, (Uint16)twp->ang.x, (Uint16)twp->ang.y, 0,
                           NJM_DEG_ANG(360.0f * (1.0f + twp->scl.y)),
                           10.0f + twp->scl.x, twp->btimer, GetWork(tp)->pos);
  }

  if (twp->mode == 1) {
    GetWork(tp)->goal = goal;
  }
}

static void ObjectKdDegRingCircleExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 found;
  kddegringent *ent;
  Sint32 i;
  task *ring;
  Sint32 left;
  NJS_POINT3 *pos;
  Float d;
  Sint32 unused[2]; // unused

  if (tp->mwp == NULL) {
    return;
  }

  found = 0;
  ent = GetWork(tp)->ring;
  for (i = 0; i < twp->btimer; i++) {
    if (ent->tp != NULL) {
      found = 1;
      break;
    }
    ent++;
  }
  if (!found && CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case 0:
    if (_rename_GetRingGroupState(GetGroupNo(twp)) == 1 &&
        _rename_GetRingGroupPos(GetGroupNo(twp), &GetWork(tp)->goal)) {
      twp->mode = 1;
    }
    break;
  case 1:
    if (twp->wtimer < 20000) {
      twp->wtimer++;
    }
    break;
  }

  ent = GetWork(tp)->ring;
  pos = GetWork(tp)->pos;
  found = 0; // reused here as the count of rings not ready to spawn yet
  left = 0;
  for (i = 0; i < twp->btimer; i++, ent++, pos++) {
    if (!(ent->flag & 2) && ent->tp == NULL) {
      switch (twp->mode) {
      case 1:
        d = njDistanceP2P(pos, &GetWork(tp)->goal) * 0.8f;
        if (d > 10000.0f) {
          d = 10000.0f;
        }
        if (twp->wtimer + 3 > (Sint16)d) {
          fn_8006AFFC(0x100F, twp, 1, 0x7F, 0x50, &twp->pos);
        }
        if (twp->wtimer < (Sint16)d) {
          found++;
          break;
        }
        // fallthrough
      case 2:
        if ((ring = CreateElementalTask(IM_TWK, LEV_1, Ring, "Ring")) != NULL) {
          ent->flag = 0;
          ring->twp->pos = *pos;
          ring->twp->ang.x = 0;
          ring->twp->ang.y = 0;
          ring->twp->ang.z = 0;
          _rename_RingSetGroup(ring, ent);
        }
        break;
      }
    }
    if (!(ent->flag & 2)) {
      left++;
    }
  }

  if (twp->mode == 1 && found == 0) {
    twp->mode = 2;
  }
  if (left != 0) {
    return;
  }

  if (tp->ocp != NULL) {
    DeadOut(tp);
    return;
  }
  FreeTask(tp);
}
