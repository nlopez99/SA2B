#include "OBJECT/o_kasoku.h"

#include "CCL.h"
#include "samt/core.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"

extern Float ceilf(Float x);

extern void SE_Call(int tone, int id, int pri, int volofs);
extern void fn_8002FB2C(Sint8, int, int, int);
// blow the player away from 'pos' along 'ang' for 'frames'
extern void fn_80039EB0(Sint32 pno, NJS_POINT3 *pos, Angle3 *ang, Sint32 frames);
// rewrites the UVs of a model every few frames
extern void fn_80073028(NJS_CNK_MODEL *model, void *info, Uint32 frame);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern void ds_DrawModelClip(NJS_MODEL *model);

// TRUE while the point is no further than 'z' down the view axis
extern Sint32 _rename_CheckDrawZ(NJS_POINT3 *pos, Float z);

// never assigned, so DrawKasoku is the drawer that runs
extern BOOL _rename_kasoku_cnkdraw;

extern NJS_TEXLIST _rename_kasoku_arrow_texlist;
extern NJS_TEXLIST _rename_kasoku_base_texlist;
extern NJS_CNK_MODEL _rename_kasoku_arrow_model;
extern NJS_CNK_MODEL _rename_kasoku_base_model;
extern Uint8 _rename_kasoku_arrow_uvanim[0x24];
extern NJS_MODEL *_rename_kasoku_base_models[4];
extern CCL_INFO _rename_kasoku_colli_info[1];

// ^ extern
// v in this file

static void DrawKasoku(task *tp);
static void DrawKasokuCnk(task *tp);
static void KasokuDie(task *tp);

// the task owns no work, so mwp and awp hold the arrow phase and the
// no-retrigger timer
#define GetAng(task) (*(Uint16 *)&task->mwp)
#define GetTimer(task) (*(Uint32 *)&task->awp)

enum {
  KSK_INIT = 0x0,
  KSK_NOR = 0x1,
  KSK_DONE = 0x2,
};

// no player has been boosted yet
#define PNO_NONE 13

static void DrawKasoku(task *tp) {
  taskwk *twp = tp->twp;

  if (!_rename_CheckDrawZ(&twp->pos, 30.0f)) {
    return;
  }

  njPushMatrixEx();
  OnControl3D(0x2400);
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y + 0x8000);
  njSetTexture(&_rename_kasoku_arrow_texlist);
  fn_80073028(&_rename_kasoku_arrow_model, _rename_kasoku_arrow_uvanim,
              lbl_801CC168._7C);
  fn_8011E158(&_rename_kasoku_arrow_model);
  njSetTexture(&_rename_kasoku_base_texlist);
  fn_8011E158(&_rename_kasoku_base_model);
  OffControl3D(0x2400);
  njPopMatrixEx();
  GetAng(tp) += 0xE38;
}

static void DrawKasokuCnk(task *tp) {
  taskwk *twp = tp->twp;

  if (!_rename_CheckDrawZ(&twp->pos, 30.0f)) {
    return;
  }

  njPushMatrixEx();
  OnControl3D(0x2400);
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y + 0x8000);
  njSetTexture(&_rename_kasoku_arrow_texlist);
  fn_80073028(&_rename_kasoku_arrow_model, _rename_kasoku_arrow_uvanim,
              lbl_801CC168._7C);
  fn_8011E158(&_rename_kasoku_arrow_model);
  njSetTexture(&_rename_kasoku_base_texlist);
  ds_DrawModelClip(_rename_kasoku_base_models[3]);
  OffControl3D(0x2400);
  njPopMatrixEx();
  GetAng(tp) += 0xE38;
}

static void KasokuDie(task *tp) {
  tp->mwp = NULL;
  tp->fwp = NULL;
  tp->awp = NULL;
  FreeTask(tp);
}

void ObjectKasoku(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {

  case KSK_INIT: {
    GetAng(tp) = 0;
    twp->mode = KSK_NOR;
    twp->smode = PNO_NONE;
    tp->dest = KasokuDie;
    if (_rename_kasoku_cnkdraw) {
      tp->disp = DrawKasokuCnk;
    } else {
      tp->disp = DrawKasoku;
    }
    CCL_InitShare(tp, _rename_kasoku_colli_info,
                  ARYLEN(_rename_kasoku_colli_info), 4);
    if (twp->scl.x <= 0.0f) {
      twp->scl.x = 14.0f;
    }
    if (twp->scl.y <= 0.0f) {
      twp->scl.y = 60.0f;
    }
  } break;

  case KSK_NOR: {
    task *ptp;
    Sint8 pno;

    if (GetTimer(tp) != 0) {
      GetTimer(tp)--;
    }

    // nested to match
    ptp = CCL_IsHitPlayer(tp);
    if (ptp != NULL) {
      pno = (ptp != playertp[1]) ? 0 : 1;
      if ((playertwp[pno]->flag & 1) &&
          (GetTimer(tp) == 0 || twp->smode != pno)) {
        Angle3 ang;
        NJS_POINT3 pos = {0.0f, 0.0f, 0.0f};

        pos.x = twp->scl.x;
        ang.x = twp->ang.x;
        ang.y = twp->ang.y;
        ang.z = twp->ang.z;
        GetTimer(tp) = (Sint32)ceilf(twp->scl.y);
        fn_80039EB0(pno, &pos, &ang, GetTimer(tp));
        SE_Call(0x1003, 0, 0, 0);
        fn_8002FB2C(pno, 4, 15, 0);
        twp->smode = pno;
      }
    }
    CCL_Entry(tp);
  } break;

  case KSK_DONE:
  default:
    break;
  }
}
