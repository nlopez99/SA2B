#include "OBJECT/o_stoplsd.h"

#include "CCL.h"
#include "set.h"

extern void fn_800068E4(colliwk *cwp);

// [0] is a box that can be turned around Z, [1] is a sphere
extern CCL_INFO _rename_stoplsd_colli_info[2];

// ^ extern
// v in this file

static void ObjectStopLSDDisp(task *tp);
static void ObjectStopLSDDest(task *tp);
static void ObjectStopLSDExec(task *tp);

void ObjectStopLSD(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOut(tp)) {
    return;
  }
  twp->smode = twp->ang.z & 1;
  twp->wtimer = twp->ang.x;
  twp->ang.x = 0;
  twp->ang.y &= ~0xFF;
  twp->ang.z &= ~0xFF;
  CCL_Init(tp, &_rename_stoplsd_colli_info[twp->smode], 1, CID_OBJECT);
  twp->cwp->info->a = twp->scl.x;
  twp->cwp->info->b = twp->scl.y;
  twp->cwp->info->c = twp->scl.z;
  if (twp->smode == 0) {
    twp->cwp->info->angz = twp->ang.z;
  }
  fn_800068E4(twp->cwp);
  twp->id = 21;
  tp->exec = ObjectStopLSDExec;
  tp->disp = ObjectStopLSDDisp;
  tp->dest = ObjectStopLSDDest;
}

static void ObjectStopLSDDisp(task *tp) {}

static void ObjectStopLSDDest(task *tp) {}

static void ObjectStopLSDExec(task *tp) {
  if (CheckRangeOut(tp)) {
    return;
  }
  CCL_Entry(tp);
}
