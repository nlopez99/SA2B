#include "OBJECT/o_ironball2.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "samt/sonic/shadow.h"
#include "set.h"

extern void *_rename_CreateObjDust0(NJS_POINT3 *pos, NJS_VECTOR *spd,
                                    Float scl);
extern void *_rename_CreateObjDust1(NJS_POINT3 *pos, NJS_VECTOR *spd,
                                    Float scl);
extern void _rename_RingDrawShadowModel(void);
extern void ds_DrawModelClip(NJS_MODEL *);
extern void fn_800068E4(colliwk *cwp);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);

extern NJS_TEXLIST   _rename_ironball2_ball_texlist;
extern NJS_CNK_MODEL _rename_ironball2_ball_model;
extern NJS_TEXLIST   _rename_ironball2_texlist;
extern NJS_CNK_MODEL _rename_ironball2_bar_model;
extern NJS_CNK_MODEL _rename_ironball2_center_model;
extern NJS_MODEL    *_rename_ironball2_ObjArr[][4];
extern CCL_INFO      _rename_ironball2_colli_info[4];

// ^ extern
// v in this file

static void ObjectIronBall2Dest(task *tp);
static void ObjectIronBall2Exec(task *tp);
static void ObjectIronBall2Disp(task *tp);
static void ObjectIronBall2DispSort(task *tp);
static void ObjectIronBall2DispDS(task *tp);

typedef struct ironball2wk // sizeof=0x8
{
  /* 0x00 */ Angle roll; // rotation of the balls around the bar
  /* 0x04 */ Float posy; // set position, centre of the up/down movement
} ironball2wk;

#define GetWork(task) ((ironball2wk *)task->mwp)
#define GetLength(twp) (20.0f * (1.0f + twp->scl.x)) // centre to ball
#define GetSpeed(twp) (1.0f + twp->scl.y)            // degrees per frame
// distance a ball travels per frame
#define GetBallSpeed(twp) (6.28f * GetLength(twp) * (GetSpeed(twp) / 360.0f))
#define DegAng(n) ((Angle)(182.04445f * (n)))

enum {
  ARG_NODUST = 1, // twp->ang.z: no dust and no shadow
  ARG_UPDOWN = 2, // twp->ang.z: moves up and down by twp->scl.z
};

enum {
  MD_UP = 1,
  MD_DOWN = 2,
};

static BOOL ironball2_dsdraw = FALSE;

Float ironball2_shadow_scl = 5.0f;
Float ironball2_ball_shadow_scl = 10.0f;
Float ironball2_dust_scl = 3.0f;
NJS_POINT3 ironball2_shadow_pos = {0.0f, 0.0f, 0.0f};
NJS_POINT3 ironball2_shadow_pos_ds = {0.0f, -10.0f, 0.0f};

static void IronBall2DrawShadow(task *tp, NJS_POINT3 *pos, Float scl) {
  njPushMatrixEx();
  njTranslate(NULL, pos->x, 0.1f + pos->y, pos->z);
  njScale(NULL, scl, 1.0f, scl);
  _rename_RingDrawShadowModel();
  njPopMatrix(1);
}

void ObjectIronBall2(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }
  tp->mwp = syCalloc(1, sizeof(ironball2wk));
  if (tp->mwp == NULL) {
    return;
  }

  if (ironball2_dsdraw) {
    tp->disp = ObjectIronBall2DispDS;
  } else {
    tp->disp = ObjectIronBall2Disp;
    if (!(twp->ang.z & ARG_NODUST)) {
      tp->disp_sort = ObjectIronBall2DispSort;
    }
  }
  tp->exec = ObjectIronBall2Exec;
  tp->dest = ObjectIronBall2Dest;
  twp->mode = 0;
  twp->ang.x = 0;
  twp->ang.z &= ARG_NODUST | ARG_UPDOWN;
  CCL_Init(tp, _rename_ironball2_colli_info,
           ARYLEN(_rename_ironball2_colli_info), CID_OBJECT);
  twp->cwp->info[1].center.x = GetLength(twp);
  twp->cwp->info[2].center.x = -GetLength(twp);
  twp->cwp->info[3].b = GetLength(twp);
  fn_800068E4(twp->cwp);
  GetWork(tp)->posy = twp->pos.y;
  twp->wtimer = 1;
  twp->ang.y = 0;
  GetWork(tp)->roll = 0;
  twp->btimer = 0;
  tp->work.f = -1000000.0f;
}

static void ObjectIronBall2Dest(task *tp) {
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectIronBall2Exec(task *tp) {
  taskwk *twp = tp->twp;
  Float posy;
  Float unused0[4]; // unused
  Angle3 ang;
  NJS_VECTOR vs;
  NJS_VECTOR spd;
  NJS_POINT3 vp;
  NJS_POINT3 pos;
  Float unused1[2]; // unused

  if (CheckRangeOut(tp)) {
    return;
  }
  CCL_Entry(tp);

  // nested to match
  if (twp->wtimer != 0) {
    if (twp->wtimer == 1) {
      // look for the ground, again in 40 frames if there is none yet
      posy = GetShadowPos(twp->pos.x, 10.0f + twp->pos.y, twp->pos.z, &ang);
      tp->work.f = posy;
      if (-1000000.0f != posy) {
        if (!(twp->ang.z & ARG_UPDOWN)) {
          twp->pos.y = posy;
        }
        twp->wtimer = 0;
      } else {
        twp->wtimer = 40;
      }
    } else {
      twp->wtimer--;
    }
  } else if (!lbl_801CC168._37 && !(twp->ang.z & ARG_NODUST) &&
             twp->btimer++ > 6) {
    twp->btimer = 0;
    vs.x = 0.0f;
    vs.y = 0.0f;
    vs.z = -(GetBallSpeed(twp));
    vp.x = GetLength(twp);
    vp.y = 0.0f;
    vp.z = 0.0f;
    njPushMatrixEx();
    njUnitMatrix(NULL);
    njTranslateV(NULL, &twp->pos);
    njRotateY(NULL, twp->ang.y);
    njCalcPoint(NULL, &vp, &pos);
    njCalcVector(NULL, &vs, &spd);
    _rename_CreateObjDust1(&pos, &spd, ironball2_dust_scl);
    njRotateY(NULL, 0x8000);
    njCalcPoint(NULL, &vp, &pos);
    njCalcVector(NULL, &vs, &spd);
    _rename_CreateObjDust0(&pos, &spd, ironball2_dust_scl);
    njPopMatrix(1);
  }

  if (twp->ang.z & ARG_UPDOWN) {
    switch (twp->mode) {
    default:
      twp->pos.y += 0.1f;
      if (twp->pos.y > twp->scl.z + GetWork(tp)->posy) {
        twp->pos.y = twp->scl.z + GetWork(tp)->posy;
        twp->mode = MD_DOWN;
      }
      break;
    case MD_DOWN:
      twp->pos.y -= 0.1f;
      if (twp->pos.y < -twp->scl.z + GetWork(tp)->posy) {
        twp->pos.y = -twp->scl.z + GetWork(tp)->posy;
        twp->mode = MD_UP;
      }
      break;
    }
  }

  if (lbl_801CC168._37) {
    return;
  }
  twp->ang.y += DegAng(GetSpeed(twp));
  // the balls (radius 11) roll along the ground
  GetWork(tp)->roll -= DegAng(360.0f * (GetBallSpeed(twp) / 69.08f));
  fn_8006AFFC(0x1019, twp, 1, 0, 30, &twp->pos);
}

static void ObjectIronBall2Disp(task *tp) {
  taskwk *twp = tp->twp;

  njPushMatrix(NULL);
  njTranslate(NULL, twp->pos.x, 10.0f + twp->pos.y, twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  njSetTexture(&_rename_ironball2_texlist);
  njCnkCacheDrawModel(&_rename_ironball2_center_model);

  njPushMatrix(NULL);
  njScale(NULL, 1.0f + twp->scl.x, 1.0f, 1.0f);
  njCnkCacheDrawModel(&_rename_ironball2_bar_model);
  njPopMatrix(1);

  njPushMatrix(NULL);
  njTranslate(NULL, GetLength(twp), 0.0f, 0.0f);
  njRotateX(NULL, GetWork(tp)->roll);
  njSetTexture(&_rename_ironball2_ball_texlist);
  njCnkCacheDrawModel(&_rename_ironball2_ball_model);
  njPopMatrix(1);

  njPushMatrix(NULL);
  njScale(NULL, -(1.0f + twp->scl.x), 1.0f, 1.0f);
  njCnkCacheDrawModel(&_rename_ironball2_bar_model);
  njPopMatrix(1);

  njTranslate(NULL, -GetLength(twp), 0.0f, 0.0f);
  njRotateX(NULL, -GetWork(tp)->roll);
  njRotateY(NULL, 0x8000);
  njSetTexture(&_rename_ironball2_ball_texlist);
  njCnkCacheDrawModel(&_rename_ironball2_ball_model);
  njPopMatrix(1);
}

static void ObjectIronBall2DispSort(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->ang.z & ARG_NODUST) {
    return;
  }

  njPushMatrix(NULL);
  njTranslate(NULL, twp->pos.x, 0.1f + tp->work.f, twp->pos.z);
  njRotateY(NULL, twp->ang.y);
  IronBall2DrawShadow(tp, &ironball2_shadow_pos, ironball2_shadow_scl);

  njPushMatrix(NULL);
  njTranslate(NULL, GetLength(twp), 0.0f, 0.0f);
  IronBall2DrawShadow(tp, &ironball2_shadow_pos, ironball2_ball_shadow_scl);
  njPopMatrix(1);

  njTranslate(NULL, -GetLength(twp), 0.0f, 0.0f);
  IronBall2DrawShadow(tp, &ironball2_shadow_pos, ironball2_ball_shadow_scl);
  njPopMatrix(1);
}

static void ObjectIronBall2DispDS(task *tp) {
  taskwk *twp = tp->twp;

  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x, 10.0f + twp->pos.y, twp->pos.z);
  njRotateY(NULL, twp->ang.y);

  njPushMatrixEx();
  njSetTexture(&_rename_ironball2_texlist);
  ds_DrawModelClip(_rename_ironball2_ObjArr[1][3]);
  IronBall2DrawShadow(tp, &ironball2_shadow_pos_ds, ironball2_shadow_scl);
  njScale(NULL, 1.0f + twp->scl.x, 1.0f, 1.0f);
  ds_DrawModelClip(_rename_ironball2_ObjArr[2][3]);
  njPopMatrixEx();

  njPushMatrixEx();
  njTranslate(NULL, GetLength(twp), 0.0f, 0.0f);
  IronBall2DrawShadow(tp, &ironball2_shadow_pos_ds,
                      ironball2_ball_shadow_scl);
  njRotateX(NULL, GetWork(tp)->roll);
  njSetTexture(&_rename_ironball2_ball_texlist);
  ds_DrawModelClip(_rename_ironball2_ObjArr[0][3]);
  njPopMatrixEx();

  njPushMatrixEx();
  njScale(NULL, -(1.0f + twp->scl.x), 1.0f, 1.0f);
  ds_DrawModelClip(_rename_ironball2_ObjArr[2][3]);
  njPopMatrixEx();

  njPushMatrixEx();
  njTranslate(NULL, -GetLength(twp), 0.0f, 0.0f);
  IronBall2DrawShadow(tp, &ironball2_shadow_pos_ds,
                      ironball2_ball_shadow_scl);
  njRotateX(NULL, -GetWork(tp)->roll);
  njRotateY(NULL, 0x8000);
  njSetTexture(&_rename_ironball2_ball_texlist);
  ds_DrawModelClip(_rename_ironball2_ObjArr[0][3]);
  njPopMatrix(2);
}
