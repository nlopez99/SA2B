#include "OBJECT/o_solidbox.h"

#include "CCL.h"
#include "fabsf.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/shadow.h"
#include "set.h"

extern void FreeTaskC(task *tp);
// the stage's pushable-object registry: join on init, leave on destroy
extern void _rename_EntryObjectList(task *tp);
extern void _rename_LeaveObjectList(task *tp);
// creates a child task carrying its own collision volume
extern task *_rename_CreateColliChild(task *tp, Sint32 a, Sint32 b,
                                      CCL_INFO *ci, Sint32 num, Uint32 flag);
extern void _rename_CreateBoxDust(NJS_POINT3 *pos, Float x, Float z,
                                  Angle3 *ang, Float scl, Float interval,
                                  Float spd);
extern void fn_8011E158(NJS_CNK_MODEL *model);

extern NJS_TEXLIST    _rename_solidbox_texlist;
extern NJS_CNK_MODEL  _rename_solidbox_model;
extern CCL_INFO       _rename_solidbox_colli_info[1];

// ^ extern
// v in this file

static void ObjectSolidBoxDest(task *tp);
static void ObjectSolidBoxExec(task *tp);
static void ObjectSolidBoxDisp(task *tp);

// the ground height under the box, cached in the unused 'awp' slot;
// NO_SHADOW means "not looked up yet"
#define GetShadowY(task) (*(Float *)&task->awp)
#define NO_SHADOW (-1000000.0f)

void ObjectSolidBox(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->disp = ObjectSolidBoxDisp;
  tp->exec = ObjectSolidBoxExec;
  tp->dest = ObjectSolidBoxDest;
  twp->smode = 0;
  GetShadowY(tp) = NO_SHADOW;
  twp->scl.y = 0.0f;
  twp->scl.z = 0.0f;
  CCL_Init(tp, _rename_solidbox_colli_info, ARYLEN(_rename_solidbox_colli_info),
           CID_OBJECT);
  _rename_EntryObjectList(tp);
  twp->ang.x = 0;
  twp->ang.z = 0;
}

static void ObjectSolidBoxDest(task *tp) {
  _rename_LeaveObjectList(tp);
  tp->awp = NULL;
}

static void ObjectSolidBoxExec(task *tp) {
  taskwk *twp = tp->twp;
  Float unused[2]; // unused
  Angle3 ang;
  Float spd;
  taskwk *ctwp;

  if (CheckRangeOut(tp)) {
    return;
  }

  // 'scl.y' is how far the box still has to fall, 'scl.z' its speed
  if (twp->scl.y > 0.0f) {
    spd = twp->scl.z;
    if (twp->scl.y < -twp->scl.z) {
      spd = -twp->scl.y;
    }
    twp->pos.y += spd;
    twp->scl.y += spd;
    twp->scl.z = 0.99f * twp->scl.z - 0.08f;

    if (tp->ctp == NULL) {
      _rename_CreateColliChild(tp, 0, 0, _rename_solidbox_colli_info,
                               ARYLEN(_rename_solidbox_colli_info), 0);
    }
    // the child collision sits where the fall ends
    if (tp->ctp != NULL) {
      ctwp = tp->ctp->twp;
      ctwp->pos.x = twp->pos.x;
      ctwp->pos.z = twp->pos.z;
      ctwp->pos.y = twp->pos.y - twp->scl.y;
    }
  } else if (tp->ctp != NULL) {
    FreeTaskC(tp);
  }

  if (twp->scl.y <= 0.0f && 0.0f != twp->scl.z) {
    twp->scl.y = 0.0f;
    twp->scl.z = 0.0f;
    if (fabsf(twp->pos.y - GetShadowY(tp)) < 5.0f) {
      _rename_CreateBoxDust(&twp->pos, 10.0f, 10.0f, &twp->ang, 8.0f, 1.2f,
                            0.5f);
    }
  }

  if (NO_SHADOW == GetShadowY(tp) && (twp->wtimer & 0x1F) == 0) {
    ang.x = 0;
    ang.y = twp->ang.y;
    ang.z = 0;
    GetShadowY(tp) =
        GetShadowPos(twp->pos.x, 5.0f + twp->pos.y, twp->pos.z, &ang);
  }
  CCL_Entry(tp);
  twp->wtimer++;
}

static void ObjectSolidBoxDisp(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_solidbox_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  fn_8011E158(&_rename_solidbox_model);
  OffControl3D(0x2400);
  njPopMatrixEx();
}
