#include "OBJECT/o_wsmmls.h"

#include "samt/ninja/njmatrix.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "fabsf.h"

extern BOOL _rename_CheckFlag0x20(task *tp);
extern void _rename_SetFlag0x20(task *tp);
// warp effect; the kind is the last argument
extern void fn_800260FC(Float x, Float y, Float z, Uint32 kind);
// nearest player to a point
extern Sint32 fn_80037C84(NJS_POINT3 *pos);

// ^ extern
// v in this file

static void ObjectWSMMLSDest(task *tp);
static void ObjectWSMMLSExec(task *tp);
static void ObjectWSMMLSDisp(task *tp);

#define ACTION_WSMMLS (0x2A)

void ObjectWSMMLS(task *tp) {
  if (tp->ocp != NULL && _rename_CheckFlag0x20(tp)) {
    DeadOut(tp);
  } else if (!CheckRangeOut(tp)) {
    tp->disp = ObjectWSMMLSDisp;
    tp->exec = ObjectWSMMLSExec;
    tp->dest = ObjectWSMMLSDest;
  }
}

static void ObjectWSMMLSDest(task *tp) {}

static void ObjectWSMMLSExec(task *tp) {
  taskwk *twp = tp->twp;
  Float unused[2]; // unused
  NJS_POINT3 point;
  Sint32 pno;
  Sint32 hit;
  playerwk *pwp;

  if (CheckRangeOut(tp)) {
    return;
  }

  pno = fn_80037C84(&twp->pos);
  if (playertwp[pno] == NULL) {
    return;
  }

  hit = 0;

  // player position in the trigger box's own space
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njScale(NULL, 1.0f / (10.0f + twp->scl.x), 1.0f / (10.0f + twp->scl.y),
          1.0f / (10.0f + twp->scl.z));
  njRotateY(NULL, -twp->ang.y);
  njRotateX(NULL, -twp->ang.x);
  njTranslate(NULL, -twp->pos.x, -twp->pos.y, -twp->pos.z);
  njCalcPoint(NULL, &playertwp[pno]->pos, &point);
  njPopMatrix(1);

  switch (twp->ang.z & 1) {
  case 0:
  default:
    if (njScalor(&point) < 1.0f) {
      hit = 1;
    }
    break;
  case 1:
    if (fabsf(point.x) < 1.0f && fabsf(point.y) < 1.0f &&
        fabsf(point.z) < 1.0f) {
      hit = 1;
    }
    break;
  }

  if (hit == 0) {
    return;
  }

  pwp = playerpwp[pno];
  if (pwp->action_last == ACTION_WSMMLS) {
    fn_800260FC(twp->pos.x, twp->pos.y, twp->pos.z,
                ((twp->ang.z & 0xF0) >> 4) % 3);

    if (tp->ocp != NULL) {
      _rename_SetFlag0x20(tp);
      DeadOut(tp);
    } else {
      FreeTask(tp);
    }
  } else {
    pwp->action_sel = ACTION_WSMMLS;
  }
}

static void ObjectWSMMLSDisp(task *tp) {}
