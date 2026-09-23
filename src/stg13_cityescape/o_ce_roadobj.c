#include "stg13_cityescape/o_ce_roadobj.h"

#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/c_colli.h"
#include "samt/sonic/task.h"
#include "set.h"

extern u8 fn_80065388(task *tp);
extern void _rename_SetConditionFlag(task *tp, u8 smode);

extern void fn_80031980(GJS_OBJECT *object);
extern void late_DrawModel(GJS_MODEL *model);
extern void ds_DrawModelClip(NJS_MODEL *model);

extern void OnControl3D(Sint32 flag);
extern void OffControl3D(Sint32 flag);

// ^ extern
// v in this file

#define ROADOBJ_GROUP_NUM 3

#define CTRL3D_CLIP 0x2400

#define MWP_00(mwp) (*(ObjectCeRoadObj_t **)(&(mwp)->spd.x))
#define MWP_04(mwp) (*(Sint32 *)(&(mwp)->spd.y))
#define TWP_08(twp) (*(Sint32 *)(&(twp)->ang.x))

struct ObjectCeRoadObj_t;

typedef void (*_1c_exec_type)(task *, taskwk *, struct ObjectCeRoadObj_t *);

typedef struct ObjectCeRoadObj_t {
  /* 0x00 */ Sint32 _00;
  /* 0x04 */ NJS_TEXLIST *texlist;
  /* 0x08 */ GJS_OBJECT *object;
  /* 0x0C */ CCL_INFO *coll;
  /* 0x10 */ Uint32 collCount;
  /* 0x14 */ task_exec disp;
  artificial_padding(0x14, 0x1c, task_exec);
  /* 0x1C */ _1c_exec_type exec;
  artificial_padding(0x1c, 0x28, _1c_exec_type);
  /* 0x28 */ NJS_MODEL *clip_model;
} ObjectCeRoadObj_t; // size: 0x2C

typedef struct ObjectCeRoadObjGroup_t {
  /* 0x00 */ ObjectCeRoadObj_t *entry;
  /* 0x04 */ Sint32 num;
} ObjectCeRoadObjGroup_t;

extern ObjectCeRoadObjGroup_t _rename_roadobj_group[ROADOBJ_GROUP_NUM];

static void RoadObjInit(task *tp);
static void RoadObjDest(task *tp);
static void RoadObjExec(task *tp);

Sint32 GetRoadObjNum(void) {
  ObjectCeRoadObjGroup_t *gp;
  Sint32 num;
  Sint32 i;

  num = 0;
  for (i = 0, gp = _rename_roadobj_group; i < ROADOBJ_GROUP_NUM; i++, gp++) {
    num += gp->num;
  }
  return num;
}

// only the last group is searchable by index, though GetRoadObjNum counts all
// three
Sint32 GetRoadObjInfo(Sint32 idx, NJS_TEXLIST **tex, void **obj) {
  ObjectCeRoadObj_t *ep;
  Sint32 i;

  for (i = ROADOBJ_GROUP_NUM - 1; i < ROADOBJ_GROUP_NUM; i++) {
    if (idx < _rename_roadobj_group[i].num) {
      ep = &_rename_roadobj_group[i].entry[idx];
      *tex = ep->texlist;
      *obj = ep->object;
      return 1;
    }
  }
  return 0;
}

void ObjectRoadObj(task *tp) {
  if (CheckRangeOut(tp)) {
    return;
  }
  tp->mwp = (motionwk *)&_rename_roadobj_group[0];
  RoadObjInit(tp);
}

void ObjectSigns(task *tp) {
  if (CheckRangeOut(tp)) {
    return;
  }
  tp->mwp = (motionwk *)&_rename_roadobj_group[1];
  RoadObjInit(tp);
}

static void RoadObjInit(task *tp) {
  taskwk *twp = tp->twp;
  ObjectCeRoadObj_t *ep;

  tp->exec = RoadObjExec;
  tp->dest = RoadObjDest;
  TWP_08(twp) &= 0xFF;
  TWP_08(twp) %= MWP_04(tp->mwp);

  ep = &MWP_00(tp->mwp)[TWP_08(twp)];
  if (ep->coll != NULL) {
    CCL_Init(tp, ep->coll, ep->collCount, CID_OBJECT);
  }

  tp->disp = ep->disp;
  tp->work.ptr = NULL;
  if (ep->_00 & 2) {
    tp->work.ptr = syMalloc(sizeof(NJS_MATRIX));
    if (tp->work.ptr) {
      njPushMatrixEx();
      njUnitMatrix(NULL);
      njTranslateEx(&twp->pos);
      njRotateY(NULL, twp->ang.y);
      njGetMatrix(tp->work.ptr);
      njPopMatrixEx();
    }
  }

  if (tp->ocp) {
    twp->smode = fn_80065388(tp);
  }
}

static void RoadObjDest(task *tp) {
  taskwk *twp = tp->twp;

  if (tp->work.ptr) {
    syFree(tp->work.ptr);
    tp->work.ptr = NULL;
  }
  tp->mwp = NULL;
  tp->awp = NULL;
  tp->fwp = NULL;
  if (tp->ocp) {
    _rename_SetConditionFlag(tp, twp->smode);
  }
}

static void RoadObjExec(task *tp) {
  taskwk *twp = tp->twp;
  ObjectCeRoadObj_t *ep;

  if (CheckRangeOut(tp)) {
    return;
  }
  ep = &MWP_00(tp->mwp)[TWP_08(twp)];
  if (ep->exec != NULL) {
    ep->exec(tp, twp, ep);
    return;
  }
  if (twp->cwp != NULL) {
    CCL_Entry(tp);
  }
}

void RoadObjDisp(task *tp) {
  taskwk *twp = tp->twp;
  ObjectCeRoadObj_t *ep = &MWP_00(tp->mwp)[TWP_08(twp)];
  Sint32 flag = ep->_00;
  Sint32 clip;

  njSetTexture(ep->texlist);
  njPushMatrixEx();
  if (tp->work.ptr != NULL) {
    njMultiMatrix(NULL, tp->work.ptr);
  } else {
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
  }

  clip = flag & 0x20;
  if (clip) {
    OnControl3D(CTRL3D_CLIP);
  }
  if (flag & 0x8000) {
    ds_DrawModelClip(ep->clip_model);
  } else {
    fn_80031980(ep->object);
  }
  if (clip) {
    OffControl3D(CTRL3D_CLIP);
  }
  njPopMatrixEx();
}

// unreferenced; reconstructed from the .rodata order, stripped by the linker
static Float RoadObjZero(void) { return 0.0f; }

void RoadObjDispTrans(task *tp) {
  taskwk *twp = tp->twp;
  ObjectCeRoadObj_t *ep = &MWP_00(tp->mwp)[TWP_08(twp)];
  Sint32 flag = ep->_00;
  Sint32 clip;

  njSetTexture(ep->texlist);
  njPushMatrixEx();
  if (tp->work.ptr != NULL) {
    njMultiMatrix(NULL, tp->work.ptr);
  } else {
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
  }

  // sign posts 8 and 9 get their own offset
  if (TWP_08(twp) == 8) {
    njTranslate(NULL, 1.5f, 0.8f, 1.5f);
  } else {
    njTranslate(NULL, (TWP_08(twp) == 9) ? -1.5f : 1.5f, 0.0f, 0.0f);
  }

  clip = flag & 0x20;
  if (clip) {
    OnControl3D(CTRL3D_CLIP);
  }
  if (flag & 0x8000) {
    ds_DrawModelClip(ep->clip_model);
  } else {
    fn_80031980(ep->object);
  }
  if (clip) {
    OffControl3D(CTRL3D_CLIP);
  }
  njPopMatrixEx();
}

void RoadObjDispScale(task *tp) {
  taskwk *twp = tp->twp;
  ObjectCeRoadObj_t *ep = &MWP_00(tp->mwp)[TWP_08(twp)];
  Sint32 flag = ep->_00;
  Sint32 clip;
  Float scl;

  njSetTexture(ep->texlist);
  njPushMatrixEx();
  if (tp->work.ptr != NULL) {
    njMultiMatrix(NULL, tp->work.ptr);
  } else {
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
  }

  scl = 1.0f + twp->scl.y;
  njScale(NULL, 1.0f, scl, scl);

  clip = flag & 0x20;
  if (clip) {
    OnControl3D(CTRL3D_CLIP);
  }
  if (flag & 0x8000) {
    ds_DrawModelClip(ep->clip_model);
  } else {
    late_DrawModel(ep->object->model);
  }
  if (clip) {
    OffControl3D(CTRL3D_CLIP);
  }
  njPopMatrixEx();
}
