#include "OBJECT/o_ori.h"

#include "CCL.h"
#include "OBJECT/o_rocketmissile.h"
#include "samt/ninja/gjdraw.h"
#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njcollision.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/shadow.h"
#include "set.h"
#include "fabsf.h"
#include "PowerPC_EABI_Support/MSL_C/MSL_Common/rand.h"

// the task that hit this one with the given collision kind
extern task *fn_800060EC(task *tp, Sint32 kind);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);
extern void AddScore(int);
extern void FreeTaskC(task *tp);
extern void ds_DrawModelClip(NJS_MODEL *model);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern void _rename_CreateBoxDust(NJS_POINT3 *pos, Float x, Float z,
                                  Angle3 *ang, Float scl, Float interval,
                                  Float spd);
// breakable-object stack: register / withdraw / make the ones above fall
extern void _rename_BreakObjEntry(task *tp);
extern void _rename_BreakObjFree(task *tp);
extern void _rename_BreakObjDropAbove(NJS_POINT3 *pos);
extern task *_rename_CreateBreakObjColli(task *tp, Sint32, Sint32,
                                         CCL_INFO *info, Sint32 nbInfo,
                                         Sint32);
extern void _rename_CreateBrokenPiece(NJS_POINT3 *pos, Sint32, NJS_VECTOR *scl,
                                      Angle angy, Float spd, Float shadow_y,
                                      NJS_MODEL *model, NJS_TEXLIST *texlist);
extern void _rename_CreateBrokenPieceDS(NJS_POINT3 *pos, Sint32,
                                        NJS_VECTOR *scl, Angle angy, Float spd,
                                        Float shadow_y, NJS_MODEL *model,
                                        NJS_MODEL *dsmodel,
                                        NJS_TEXLIST *texlist);
extern void _rename_CreateBreakSmoke(NJS_POINT3 *pos, Float, Float, Float);

extern BOOL DisableObjectFog;

typedef struct oripiece // sizeof=0xC
{
  /* 0x00 */ NJS_TEXLIST *texlist;
  /* 0x04 */ NJS_MODEL *model;
  /* 0x08 */ Sint32 nbPiece;
} oripiece;

typedef struct dsmodel // sizeof=0x10
{
  /* 0x00 */ Sint32 flag;
  /* 0x04 */ NJS_TEXLIST *texlist;
  /* 0x08 */ void *unk_8;
  /* 0x0C */ NJS_MODEL *model;
} dsmodel;

extern NJS_TEXLIST _rename_ori_texlist;
extern GJS_MODEL _rename_ori_model;
extern oripiece _rename_ori_piece_tbl[3];
extern dsmodel _rename_ori_ObjArr[4];
extern CCL_INFO _rename_ori_colli_info[];

// ^ extern
// v in this file

static void ObjectOriDest(task *tp);
static void ObjectOriExec(task *tp);
static void ObjectOriDisp(task *tp);
static void ObjectOriDispDS(task *tp);

// the ground height under the cage, once it has been looked up
#define GetGroundY(task) (*(Float *)&(task)->awp)
// the position of whatever rocket missile has locked on to the cage
#define GetTarget(task) ((NJS_POINT3 **)(task)->mwp)

#define NO_GROUND (-1000000.0f)

enum {
  MD_STAND, // waiting to be destroyed
  MD_BREAK, // breaking up
  MD_DEAD,  // gone
};

static BOOL ori_dsdraw = FALSE;
static Uint32 ori_se_count = 0;

void ObjectOri(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(NJS_POINT3 *));
  if (tp->mwp == NULL) {
    return;
  }

  if (ori_dsdraw) {
    tp->disp = ObjectOriDispDS;
  } else {
    tp->disp = ObjectOriDisp;
  }
  tp->exec = ObjectOriExec;
  tp->dest = ObjectOriDest;

  twp->smode = MD_STAND;
  GetGroundY(tp) = NO_GROUND;
  *GetTarget(tp) = NULL;

  // twp->ang.x: which set of fragments to break into
  twp->ang.x &= 0xF;
  // twp->ang.z: rocket missile target id
  twp->ang.z &= 0x1F;

  CCL_InitShare(tp, _rename_ori_colli_info, 1, CID_OBJECT);
  RocketMissileSetTarget(twp->ang.z & 0x1F, GetTarget(tp));
  _rename_BreakObjEntry(tp);

  // twp->scl.y: how far the cage still has to fall, twp->scl.z: its speed
  twp->scl.y = 0.0f;
  twp->scl.z = 0.0f;
}

static void ObjectOriDest(task *tp) {
  RocketMissileFreeTarget(GetTarget(tp));
  _rename_BreakObjFree(tp);
  tp->awp = NULL;
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectOriExec(task *tp) {
  taskwk *twp = tp->twp;
  taskwk *ctwp;
  Float d;
  Sint32 i;
  Sint32 j;

  if (twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }

  if (twp->scl.y > 0.0f) {
    d = twp->scl.z;
    if (twp->scl.y < -twp->scl.z) {
      d = -twp->scl.y;
    }
    twp->pos.y += d;
    twp->scl.y += d;
    twp->scl.z = 0.99f * twp->scl.z - 0.08f;

    // the collision box stays where the cage is going to land
    if (tp->ctp == NULL) {
      _rename_CreateBreakObjColli(tp, 0, 0, _rename_ori_colli_info, 1, 0);
    }
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
    if (fabsf(twp->pos.y - GetGroundY(tp)) < 5.0f) {
      _rename_CreateBoxDust(&twp->pos, 10.0f, 10.0f, &twp->ang, 8.0f, 1.2f,
                            0.5f);
    }
  }

  switch (twp->smode) {
  case MD_STAND: {
    Angle3 ang;

    if (*GetTarget(tp) != NULL ||
        fn_800060EC(tp, CI_KIND_BOMB_EXPLOSION) != NULL) {
      // missile locked on or bomb hit: frames to wait before breaking
      if (*GetTarget(tp) != NULL) {
        d = njDistanceP2P(&twp->pos, *GetTarget(tp)) - 18.0f;
        if (d < 0.0f) {
          d = 0.0f;
        }
        tp->work.l = (Sint32)d;
      } else {
        tp->work.l = 1;
      }
      RocketMissileFreeTarget(GetTarget(tp));
      twp->wtimer = 0;
      twp->smode = MD_BREAK;
      fn_8006AFFC(0x1011, (Uint8 *)ObjectOriExec + ori_se_count++ % 5, 1, 0x70,
                  60, &twp->pos);
      AddScore(20);
      CCL_Entry(tp);
      break;
    }

    twp->wtimer++;
    if (GetGroundY(tp) == NO_GROUND && (twp->wtimer & 0x1F) == 0) {
      ang.x = 0;
      ang.y = twp->ang.y;
      ang.z = 0;
      GetGroundY(tp) =
          GetShadowPos(twp->pos.x, 5.0f + twp->pos.y, twp->pos.z, &ang);
    }
    CCL_Entry(tp);
    break;
  }
  case MD_BREAK: {
    NJS_POINT3 v;
    NJS_POINT3 pos;
    NJS_VECTOR scl;

    if (twp->wtimer == 0 && tp->work.l != 0) {
      tp->work.l--;
      CCL_Entry(tp);
      break;
    }
    if (twp->wtimer == 1) {
      // everything stacked on top of the cage starts falling
      v = twp->pos;
      _rename_BreakObjDropAbove(&v);
    }
    if (twp->wtimer++ > 2) {
      pos = twp->pos;
      pos.y += 10.0f;
      scl.x = 10.0f;
      scl.y = 10.0f;
      scl.z = 10.0f;

      if (ori_dsdraw) {
        Sint32 kind = (twp->ang.x & 0xF) % 3;

        if (kind != 2) {
          for (i = 0; i < 3; ++i) {
            for (j = 0; j < _rename_ori_piece_tbl[i].nbPiece; ++j) {
              _rename_CreateBrokenPieceDS(
                  &pos, 1, &scl, twp->ang.y,
                  0.7f + 1.2f * (0.000030517578f * (Float)rand()),
                  kind == 1 ? NO_GROUND
                            : (NO_GROUND != GetGroundY(tp) ? GetGroundY(tp)
                                                           : twp->pos.y),
                  _rename_ori_piece_tbl[i].model,
                  _rename_ori_ObjArr[i + 1].model,
                  _rename_ori_piece_tbl[i].texlist);
            }
          }
        }
      } else {
        Sint32 kind = (twp->ang.x & 0xF) % 3;

        if (kind != 2) {
          for (i = 0; i < 3; ++i) {
            for (j = 0; j < _rename_ori_piece_tbl[i].nbPiece; ++j) {
              _rename_CreateBrokenPiece(
                  &pos, 1, &scl, twp->ang.y,
                  0.7f + 1.2f * (0.000030517578f * (Float)rand()),
                  kind == 1 ? NO_GROUND
                            : (NO_GROUND != GetGroundY(tp) ? GetGroundY(tp)
                                                           : twp->pos.y),
                  _rename_ori_piece_tbl[i].model,
                  _rename_ori_piece_tbl[i].texlist);
            }
          }
        }
      }

      for (i = 0; i < 14; ++i) {
        _rename_CreateBreakSmoke(
            &pos, 7.0f + 3.0f * (0.000030517578f * (Float)rand()),
            0.5f + 0.000030517578f * (Float)rand(),
            2.5f + 4.0f * (0.000030517578f * (Float)rand()));
      }
      twp->smode = MD_DEAD;
    }
    CCL_Entry(tp);
    break;
  }
  case MD_DEAD:
    if (tp->ocp != NULL) {
      DeadOut(tp);
    } else {
      FreeTask(tp);
    }
    break;
  }
}

static void ObjectOriDisp(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_ori_texlist);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  njPushMatrix(NULL);
  njTranslateV(NULL, &twp->pos);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  gjDrawModel(&_rename_ori_model);
  OffControl3D(0x2400);
  njPopMatrix(1);
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
}

static void ObjectOriDispDS(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(&_rename_ori_texlist);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  njPushMatrix(NULL);
  njTranslateV(NULL, &twp->pos);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  ds_DrawModelClip(_rename_ori_ObjArr[0].model);
  OffControl3D(0x2400);
  njPopMatrix(1);
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
}
