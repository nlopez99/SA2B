#include "OBJECT/o_treest.h"

#include "CCL.h"
#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "set.h"

extern void late_DrawModel(GJS_MODEL *model);

typedef struct camwk {
  /* 0x00 */ Uint8 unk_00[0xC];
  /* 0x0C */ Angle3 ang;
} camwk;

extern Sint32 lbl_803ADAD0; // screen being drawn
extern camwk *lbl_80175378[4];

extern CCL_INFO    _rename_treest_colli_info;
extern NJS_TEXLIST _rename_treest_texlist;
extern GJS_OBJECT  _rename_treest_object;

// ^ extern
// v in this file

static void ObjectTreeStDisp(task *tp);
static void ObjectTreeStDest(task *tp);
static void ObjectTreeStExec(task *tp);

void ObjectTreeSt(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOut(tp)) {
    return;
  }
  tp->disp = ObjectTreeStDisp;
  tp->exec = ObjectTreeStExec;
  tp->dest = ObjectTreeStDest;
  twp->mode = 0;
  CCL_Init(tp, &_rename_treest_colli_info, 1, CID_OBJECT);
}

static void ObjectTreeStDest(task *tp) {}

static void ObjectTreeStExec(task *tp) {
  if (CheckRangeOut(tp)) {
    return;
  }
  CCL_Entry(tp);
}

static void ObjectTreeStDisp(task *tp) {
  taskwk *twp = tp->twp;
  GJS_OBJECT *object = &_rename_treest_object;

  njSetTexture(&_rename_treest_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njTranslate(NULL, 0.0f, 1.1f, 0.0f);
  njRotateY(NULL, twp->ang.y);
  late_DrawModel(object->model);

  // the crown is a billboard: turn it back to face the screen
  object = object->child;
  njTranslateV(NULL, &object->pos);
  njRotateY(NULL, lbl_80175378[lbl_803ADAD0]->ang.y - twp->ang.y);
  late_DrawModel(object->model);
  njPopMatrix(1);
}
