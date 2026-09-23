#include "OBJECT/o_colli.h"

#include "CCL.h"
#include "set.h"

extern void fn_800068E4(colliwk *cwp);

extern CCL_INFO _rename_sphere_colli_info[1];
extern CCL_INFO _rename_ccyl_colli_info[1];
extern CCL_INFO _rename_ccube_colli_info[1];
extern CCL_INFO _rename_cwall_colli_info[1];
extern CCL_INFO _rename_ccircle_colli_info[1];

// ^ extern
// v in this file

// the set scale is added to a minimum collision size of 10
static void ColliSetSize(taskwk *twp) {
  CCL_INFO *info = twp->cwp->info;

  info->a = 10.0f + twp->scl.x;
  info->b = 10.0f + twp->scl.y;
  info->c = 10.0f + twp->scl.z;
  fn_800068E4(twp->cwp);
}

void ObjectSphere(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOut(tp)) {
    return;
  }
  switch (twp->mode) {
  case 0:
    CCL_Init(tp, _rename_sphere_colli_info, ARYLEN(_rename_sphere_colli_info),
             CID_OBJECT);
    ColliSetSize(twp);
    twp->mode = 1;
    break;
  case 1:
    CCL_Entry(tp);
    break;
  default:
    twp->mode = 0;
    break;
  }
}

void ObjectCCyl(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOut(tp)) {
    return;
  }
  switch (twp->mode) {
  case 0:
    CCL_Init(tp, _rename_ccyl_colli_info, ARYLEN(_rename_ccyl_colli_info),
             CID_OBJECT);
    ColliSetSize(twp);
    twp->mode = 1;
    break;
  case 1:
    CCL_Entry(tp);
    break;
  default:
    twp->mode = 0;
    break;
  }
}

void ObjectCCube(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOut(tp)) {
    return;
  }
  switch (twp->mode) {
  case 0:
    CCL_Init(tp, _rename_ccube_colli_info, ARYLEN(_rename_ccube_colli_info),
             CID_OBJECT);
    ColliSetSize(twp);
    twp->mode = 1;
    break;
  case 1:
    CCL_Entry(tp);
    break;
  default:
    twp->mode = 0;
    break;
  }
}

void ObjectCWall(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOut(tp)) {
    return;
  }
  switch (twp->mode) {
  case 0:
    CCL_Init(tp, _rename_cwall_colli_info, ARYLEN(_rename_cwall_colli_info),
             CID_WALL);
    ColliSetSize(twp);
    twp->mode = 1;
    break;
  case 1:
    CCL_Entry(tp);
    break;
  default:
    twp->mode = 0;
    break;
  }
}

void ObjectCCircle(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOutWithR(tp, 9000000.0f)) {
    return;
  }
  switch (twp->mode) {
  case 0:
    CCL_Init(tp, _rename_ccircle_colli_info, ARYLEN(_rename_ccircle_colli_info),
             CID_WALL);
    ColliSetSize(twp);
    twp->mode = 1;
    break;
  case 1:
    CCL_Entry(tp);
    break;
  default:
    twp->mode = 0;
    break;
  }
}
