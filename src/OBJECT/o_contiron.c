#include "OBJECT/o_contiron.h"

#include "CCL.h"
#include "fabsf.h"
#include "samt/ninja/gjdraw.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/shadow.h"
#include "set.h"
#include "stdlib.h"

extern void FreeTaskC(task *tp);
extern void AddScore(Sint32 score);
// the stage's pushable-object registry: join on init, leave on destroy
extern void _rename_BreakObjEntry(task *tp);
extern void _rename_BreakObjFree(task *tp);
// creates a child task carrying its own collision volume
extern task *_rename_CreateBreakObjColli(task *tp, Sint32 a, Sint32 b,
                                      CCL_INFO *ci, Sint32 num, Uint32 flag);
extern void _rename_CreateBoxDust(NJS_POINT3 *pos, Float x, Float z,
                                  Angle3 *ang, Float scl, Float interval,
                                  Float spd);
// everything stacked on top of the broken container starts falling
extern void _rename_BreakObjDropAbove(NJS_POINT3 *pos);
// one flying piece of a broken object, with and without a ds model
extern void _rename_CreateBrokenPiece(NJS_POINT3 *pos, Sint32 num, NJS_VECTOR *scl,
                                 Angle ang, Float spd, Float ground,
                                 NJS_CNK_MODEL *model, NJS_TEXLIST *texlist);
extern void _rename_CreateBrokenPieceDS(NJS_POINT3 *pos, Sint32 num,
                                   NJS_VECTOR *scl, Angle ang, Float spd,
                                   Float ground, NJS_CNK_MODEL *model,
                                   NJS_MODEL *dsmodel, NJS_TEXLIST *texlist);
// a puff of dust thrown out in a random direction
extern void _rename_CreateBreakSmoke(NJS_POINT3 *pos, Float spd, Float scl,
                                      Float y);
extern BOOL _rename_CheckFlag0x20(task *tp);
extern void _rename_SetFlag0x20(task *tp);
extern void _rename_gjSetTexMtx(NJS_POINT3 *pos, Angle3 *ang);
extern void ds_DrawModelClip(NJS_MODEL *model);

extern Sint32 fn_800067C0(task *tp);
extern CCL_HIT_INFO *fn_80005904(task *tp, Sint32 num);
extern BOOL fn_800368A0(task *tp, task *hit_tp);
// warp effect; the kind is the last argument
extern void fn_800260FC(Float x, Float y, Float z, Uint32 kind);
extern void fn_80066820(Sint32 alpha, NJS_POINT3 *pos, Float r);
extern void fn_8006648C(void);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);

// ^ extern
// v in this file

typedef struct dsmodel // sizeof=0x10
{
  /* 0x00 */ Sint32 flag;
  /* 0x04 */ NJS_TEXLIST *texlist;
  /* 0x08 */ void *unk_8;
  /* 0x0C */ NJS_MODEL *model;
} dsmodel;

// one kind of debris the container breaks into
typedef struct contpiece // sizeof=0xC
{
  /* 0x00 */ NJS_TEXLIST *texlist;
  /* 0x04 */ NJS_CNK_MODEL *model;
  /* 0x08 */ Sint32 num;
} contpiece;

extern NJS_TEXLIST _rename_contiron_texlist;
extern GJS_MODEL   _rename_contiron_gjmodel;
extern CCL_INFO    _rename_contiron_colli_info[1];
extern contpiece   _rename_contiron_pieces[3];
// [0] is the container itself, [1..3] match the three kinds of debris
extern dsmodel _rename_contiron_ObjArr[4];

static void ObjectContIronDest(task *tp);
static void ObjectContIronExec(task *tp);
static void ObjectContIronDisp(task *tp);
static void ObjectContIronDispDs(task *tp);

static BOOL contiron_dsdraw = FALSE;

// the ground height under the container, cached in the unused 'awp' slot;
// NO_SHADOW means "not looked up yet"
#define GetShadowY(task) (*(Float *)&task->awp)
#define NO_SHADOW (-1000000.0f)

#define GetPieceGround(task, twp)                                              \
  (((twp)->ang.x & 1)                                                          \
       ? NO_SHADOW                                                             \
       : (NO_SHADOW != GetShadowY(task) ? GetShadowY(task) : (twp)->pos.y))

#define rnd() (0.000030517578f * (Float)rand())

enum {
  SMD_WAIT,  // standing, waiting to be hit
  SMD_BREAK, // coming apart
  SMD_DEAD,  // gone, drop whatever was inside
};

void ObjectContIron(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  if (contiron_dsdraw) {
    tp->disp = ObjectContIronDispDs;
  } else {
    tp->disp = ObjectContIronDisp;
  }
  tp->exec = ObjectContIronExec;
  tp->dest = ObjectContIronDest;
  twp->smode = SMD_WAIT;
  GetShadowY(tp) = NO_SHADOW;
  // the set's z angle carries the item to drop: kind in the low nybble,
  // odds in the high one
  twp->btimer = (Uint8)twp->ang.z;
  CCL_InitShare(tp, _rename_contiron_colli_info,
                ARYLEN(_rename_contiron_colli_info), CID_OBJECT);
  twp->scl.y = 0.0f;
  twp->scl.z = 0.0f;
  _rename_BreakObjEntry(tp);
}

static void ObjectContIronDest(task *tp) {
  _rename_BreakObjFree(tp);
  tp->awp = NULL;
}

static void ObjectContIronExec(task *tp) {
  Float unused[3]; // unused
  NJS_POINT3 hitpos;
  Angle3 ang;
  NJS_POINT3 droppos;
  NJS_POINT3 piecepos;
  NJS_VECTOR piecescl;
  taskwk *twp = tp->twp;
  taskwk *ctwp;
  playerwk *ppwp;
  task *hit_tp;
  Sint32 num;
  Sint32 pno;
  BOOL found;
  Sint32 dust;
  Sint32 i;
  Sint32 j;
  Sint32 kind;
  Sint8 basechar;
  Float spd;
  Float ground; // unused
  Float odds;

  if (CheckRangeOut(tp)) {
    return;
  }

  // 'scl.y' is how far the container still has to fall, 'scl.z' its speed
  if (twp->scl.y > 0.0f) {
    spd = twp->scl.z;
    if (twp->scl.y < -twp->scl.z) {
      spd = -twp->scl.y;
    }
    twp->pos.y += spd;
    twp->scl.y += spd;
    twp->scl.z = 0.99f * twp->scl.z - 0.08f;

    if (tp->ctp == NULL) {
      _rename_CreateBreakObjColli(tp, 0, 0, _rename_contiron_colli_info,
                               ARYLEN(_rename_contiron_colli_info), 0);
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

  switch (twp->smode) {
  case SMD_WAIT:
    num = fn_800067C0(tp);
    found = FALSE;
    while (num-- != 0) {
      hit_tp = fn_80005904(tp, num)->hit_tp;
      if (fn_800368A0(tp, hit_tp)) {
        found = TRUE;
        break;
      }
    }

    if (found) {
      hitpos = twp->pos;
      hitpos.y += 10.0f;
      fn_8006AFFC(0x1011, twp, 1, 0x7F, 0x50, &twp->pos);
      for (dust = 0; dust < 14; dust++) {
        _rename_CreateBreakSmoke(&hitpos, 7.0f + 3.0f * rnd(), 0.5f + rnd(),
                                  2.5f + 4.0f * rnd());
      }
      twp->wtimer = 0;
      twp->smode = SMD_BREAK;
      AddScore(20);

      // stops the player that smashed it (some characters only in one mode)
      if (hit_tp->twp->id != 1) {
        break;
      }
      pno = IsThisTaskPlayer(hit_tp);
      if (pno == -1) {
        break;
      }
      if (playertwp[pno] == NULL) {
        break;
      }
      ppwp = playerpwp[pno];
      if (ppwp == NULL) {
        break;
      }
      basechar = ppwp->basechar;
      if (basechar == 0 && playertwp[pno]->mode != 0x45) {
        break;
      }
      if (basechar == 4 && playertwp[pno]->mode != 0x4E) {
        break;
      }
      if (basechar == 5 && playertwp[pno]->mode != 0x4E) {
        break;
      }
      SetVelocityP(pno, 0.0f, 0.0f, 0.0f);
      break;
    }

    twp->wtimer++;
    if (NO_SHADOW == GetShadowY(tp) && (twp->wtimer & 0x1F) == 0) {
      ang.x = 0;
      ang.y = twp->ang.y;
      ang.z = 0;
      GetShadowY(tp) =
          GetShadowPos(twp->pos.x, 5.0f + twp->pos.y, twp->pos.z, &ang);
    }
    // an upgrade that breaks the container changes the way it collides
    if ((playerpwp[0]->equipment & 0x08001000) ||
        ((playerpwp[0]->equipment & 0x10) &&
         (playerpwp[0]->equipment & 0x00040008))) {
      twp->cwp->info->attr |= 0x4000;
    } else {
      twp->cwp->info->attr &= ~0x4000;
    }
    CCL_Entry(tp);
    break;

  case SMD_BREAK:
    if (twp->wtimer == 1) {
      droppos = twp->pos;
      _rename_BreakObjDropAbove(&droppos);
    }
    if (twp->wtimer++ > 2) {
      piecepos = twp->pos;
      piecepos.y += 10.0f;
      piecescl.x = 10.0f;
      piecescl.y = 10.0f;
      piecescl.z = 10.0f;

      // pieces land on the cached ground, or fall forever if ang.x bit 0 is set
      if (contiron_dsdraw) {
        for (i = 0; i < 3; i++) {
          for (j = 0; j < _rename_contiron_pieces[i].num; j++) {
            _rename_CreateBrokenPieceDS(&piecepos, 1, &piecescl, twp->ang.y,
                                   0.7f + 1.2f * rnd(), GetPieceGround(tp, twp),
                                   _rename_contiron_pieces[i].model,
                                   _rename_contiron_ObjArr[i + 1].model,
                                   _rename_contiron_pieces[i].texlist);
          }
        }
      } else {
        for (i = 0; i < 3; i++) {
          for (j = 0; j < _rename_contiron_pieces[i].num; j++) {
            _rename_CreateBrokenPiece(&piecepos, 1, &piecescl, twp->ang.y,
                                 0.7f + 1.2f * rnd(), GetPieceGround(tp, twp),
                                 _rename_contiron_pieces[i].model,
                                 _rename_contiron_pieces[i].texlist);
          }
        }
      }
      twp->smode = SMD_DEAD;
    }
    if ((playerpwp[0]->equipment & 0x08001000) ||
        ((playerpwp[0]->equipment & 0x10) &&
         (playerpwp[0]->equipment & 0x00040008))) {
      twp->cwp->info->attr |= 0x4000;
      break;
    }
    twp->cwp->info->attr &= ~0x4000;
    CCL_Entry(tp);
    break;

  case SMD_DEAD:
    kind = (twp->btimer & 0xF0) >> 4;
    kind &= 0xF;
    if (kind != 0 && tp->ocp != NULL && !_rename_CheckFlag0x20(tp)) {
      // one chance in 100 per step of 'kind'; 10 and up always drop
      spd = 0.1f * (Float)kind;
      spd *= 0.1f;
      if (rnd() < spd || kind >= 10) {
        fn_800260FC(twp->pos.x, 10.0f + twp->pos.y, twp->pos.z,
                    (twp->btimer & 0xF) % 3);
      }
      _rename_SetFlag0x20(tp);
    }
    if (tp->ocp != NULL) {
      DeadOut(tp);
    } else {
      FreeTask(tp);
    }
    return;
  }
}

static void ObjectContIronDisp(task *tp) {
  Angle3 ang;
  taskwk *twp = tp->twp;

  ang.x = 0;
  ang.y = twp->ang.y;
  ang.z = 0;
  njSetTexture(&_rename_contiron_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  fn_80066820(0xFF, &twp->pos, _rename_contiron_gjmodel.r);
  _rename_gjSetTexMtx(&twp->pos, &ang);
  gjDrawModel(&_rename_contiron_gjmodel);
  fn_8006648C();
  OffControl3D(0x2400);
  njPopMatrixEx();
}

static void ObjectContIronDispDs(task *tp) {
  taskwk *twp = tp->twp;

  njSetTexture(_rename_contiron_ObjArr[0].texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  ds_DrawModelClip(_rename_contiron_ObjArr[0].model);
  OffControl3D(0x2400);
  njPopMatrixEx();
}
