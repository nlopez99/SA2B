#include "OBJECT/o_degring_linear.h"

#include "OBJECT/o_ring.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/shinobi/sg_maloc.h"
#include "set.h"

extern Sint8 fn_80065388(task *tp);
extern void _rename_SetConditionFlag(task *tp, Sint8 flag);
extern void _rename_RingSetGroup(task *tp, void *entry);
extern void _rename_RingClearGroup(task *tp);

// ^ extern
// v in this file

static void DegRingInitWork(task *tp);
static void ObjectDegRingDest(task *tp);
static void ObjectDegRingLinearExec(task *tp);
static void ObjectDegRingCircleExec(task *tp);

#define DEGRING_MAX 8

typedef struct degringent // sizeof=0x8
{
  /* 0x00 */ Sint32 flag;
  /* 0x04 */ task *tp;
} degringent;

typedef struct degringwk // sizeof=0xA0
{
  /* 0x00 */ degringent ring[DEGRING_MAX];
  /* 0x40 */ NJS_POINT3 pos[DEGRING_MAX];
} degringwk;

#define GetWork(task) ((degringwk *)task->mwp)

static Sint32 DegRingGetNum(taskwk *twp) {
  Sint32 num = twp->scl.z;

  if (num < 1) {
    num = 1;
  }
  if (num > DEGRING_MAX) {
    num = DEGRING_MAX;
  }
  return num;
}

static void DegRingCalcPosLinear(NJS_POINT3 *pos, Angle angx, Angle angy,
                                 Angle angz, Angle range, Float height,
                                 NJS_POINT3 *out, Sint32 num, Float dist) {
  NJS_POINT3 v;
  Sint32 i;
  Angle ang;

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(pos);
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
  njPopMatrixEx();
}

static void DegRingCalcPosCircle(NJS_POINT3 *pos, Angle angx, Angle angy,
                                 Angle angz, Angle range, Float r, Sint32 num,
                                 NJS_POINT3 *out) {
  NJS_POINT3 v;
  Sint32 i;
  Angle ang;

  range /= num;

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(pos);
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
  njPopMatrixEx();
}

void ObjectDegRingLinear(task *tp) {
  taskwk *twp = tp->twp;
  degringwk *wk;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->exec = ObjectDegRingLinearExec;
  tp->dest = ObjectDegRingDest;
  DegRingInitWork(tp);
  wk = GetWork(tp);
  if (wk == NULL) {
    return;
  }

  DegRingCalcPosLinear(&twp->pos, (Uint16)twp->ang.x, (Uint16)twp->ang.y,
                       (Uint16)twp->ang.z, 0x4000, twp->scl.y, wk->pos,
                       twp->btimer, 10.0f + twp->scl.x);
}

static void DegRingInitWork(task *tp) {
  taskwk *twp = tp->twp;
  Uint32 flag;
  Sint32 i;

  tp->mwp = syCalloc(1, sizeof(degringwk));
  if (tp->mwp == NULL) {
    return;
  }

  twp->btimer = DegRingGetNum(twp);
  if (tp->mwp == NULL) {
    return;
  }

  flag = 0;
  if (tp->ocp != NULL) {
    flag = fn_80065388(tp);
  }
  for (i = DEGRING_MAX - 1; i >= 0; i--) {
    if (flag & 1) {
      GetWork(tp)->ring[i].flag = 2;
    }
    flag >>= 1;
  }
}

static void ObjectDegRingDest(task *tp) {
  if (tp->mwp != NULL) {
    Sint32 i;
    Sint32 flag;
    task *ring;

    flag = 0;
    for (i = 0; i < DEGRING_MAX; i++) {
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

  if (tp->mwp != NULL) {
    syFree(tp->mwp);
  }
  tp->mwp = NULL;
}

static void ObjectDegRingLinearExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 count;
  degringent *ent;
  NJS_POINT3 *pos;
  task *ring;
  Sint32 i;
  STACK_PAD_VAR(1);

  if (tp->mwp == NULL) {
    return;
  }

  count = 0;
  ent = GetWork(tp)->ring;
  for (i = 0; i < twp->btimer; i++) {
    if (ent->tp != NULL) {
      count = 1;
      break;
    }
    ent++;
  }
  if (!count && CheckRangeOut(tp)) {
    return;
  }

  ent = GetWork(tp)->ring;
  pos = GetWork(tp)->pos;
  count = 0;
  for (i = 0; i < twp->btimer; i++, ent++, pos++) {
    if (!(ent->flag & 2) && ent->tp == NULL) {
      ring = CreateFundamentalTask(IM_TWK, LEV_1, Ring);
      if (ring != NULL) {
        ent->flag = 0;
        ring->twp->pos = *pos;
        ring->twp->ang.x = 0;
        ring->twp->ang.y = 0;
        ring->twp->ang.z = 0;
        _rename_RingSetGroup(ring, ent);
      }
    }
    if (!(ent->flag & 2)) {
      count++;
    }
  }
  if (count != 0) {
    return;
  }

  if (tp->ocp != NULL) {
    DeadOut(tp);
    return;
  }
  FreeTask(tp);
}

void ObjectDegRingCircle(task *tp) {
  taskwk *twp = tp->twp;
  degringwk *wk;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->exec = ObjectDegRingCircleExec;
  tp->dest = ObjectDegRingDest;
  DegRingInitWork(tp);
  wk = GetWork(tp);
  if (wk == NULL) {
    return;
  }

  DegRingCalcPosCircle(&twp->pos, (Uint16)twp->ang.x, (Uint16)twp->ang.y,
                       (Uint16)twp->ang.z,
                       NJM_DEG_ANG(360.0f * (1.0f + twp->scl.y)),
                       10.0f + twp->scl.x, twp->btimer, wk->pos);
}

static void ObjectDegRingCircleExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 count;
  degringent *ent;
  NJS_POINT3 *pos;
  task *ring;
  Sint32 i;
  STACK_PAD_VAR(1);

  if (tp->mwp == NULL) {
    return;
  }

  count = 0;
  ent = GetWork(tp)->ring;
  for (i = 0; i < twp->btimer; i++) {
    if (ent->tp != NULL) {
      count = 1;
      break;
    }
    ent++;
  }
  if (!count && CheckRangeOut(tp)) {
    return;
  }

  ent = GetWork(tp)->ring;
  pos = GetWork(tp)->pos;
  count = 0;
  for (i = 0; i < twp->btimer; i++, ent++, pos++) {
    if (!(ent->flag & 2) && ent->tp == NULL) {
      ring = CreateFundamentalTask(IM_TWK, LEV_1, Ring);
      if (ring != NULL) {
        ent->flag = 0;
        ring->twp->pos = *pos;
        ring->twp->ang.x = 0;
        ring->twp->ang.y = 0;
        ring->twp->ang.z = 0;
        _rename_RingSetGroup(ring, ent);
      }
    }
    if (!(ent->flag & 2)) {
      count++;
    }
  }
  if (count != 0) {
    return;
  }

  if (tp->ocp != NULL) {
    DeadOut(tp);
    return;
  }
  FreeTask(tp);
}
