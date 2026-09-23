#include "OBJECT/o_contwood.h"

#include "CCL.h"
#include "fabsf.h"
#include "samt/ninja/gjdraw.h"
#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/shadow.h"
#include "set.h"

extern Sint32 _rename_GetStageNum(void);
extern BOOL _rename_CheckFlag0x20(task *tp);
extern void _rename_SetFlag0x20(task *tp);
extern void _rename_njSubVector(NJS_VECTOR *a, NJS_VECTOR *b, NJS_VECTOR *out);
extern void _rename_gjSetTexMtx(NJS_POINT3 *pos, Angle3 *ang);
extern void _rename_CreateBoxDust(NJS_POINT3 *pos, Float x, Float z,
                                  Angle3 *ang, Float scl, Float interval,
                                  Float spd);

extern void ds_DrawModelClip(NJS_MODEL *model);
extern void AddScore(Sint32 score);
extern void FreeTaskC(task *tp);
extern CCL_HIT_INFO *fn_80005904(task *tp, Sint32 num);
extern Sint32 fn_800067C0(task *tp);
extern void fn_800156FC(Float, Float, Float, Float);
extern void fn_800260FC(Float x, Float y, Float z, Uint32 kind);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern BOOL fn_8003699C(task *tp, task *hittp);
extern void fn_8006648C(void);
extern void fn_80066820(Sint32 alpha, NJS_POINT3 *pos, Float r);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern void fn_801218C8(Sint32, Sint32);

// the camera the container is shown in; only the two points are needed here
typedef struct contcam {
  /* 0x00 */ NJS_POINT3 pos;
  /* 0x0C */ Uint8      unk0C[0x18];
  /* 0x24 */ NJS_POINT3 tgt;
} contcam;

extern contcam *lbl_80175378[4];
extern Sint32 lbl_803ADAD0;

// every container registers itself here so the others can find it
extern task *_rename_container_list[256];
extern Sint32 _rename_BreakObjEntry(task *tp);
extern void _rename_BreakObjFree(task *tp);
extern void _rename_BreakObjDropAbove(NJS_POINT3 *pos);
extern task *_rename_CreateBreakObjColli(task *tp, void *, void *,
                                          CCL_INFO *info, Sint32 nbInfo,
                                          Sint32 flag);
extern void _rename_CreateBreakSmoke(NJS_POINT3 *pos, Float a, Float b,
                                    Float c);

typedef struct contpiece // sizeof=0xC
{
  /* 0x00 */ NJS_TEXLIST *texlist;
  /* 0x04 */ void        *model;
  /* 0x08 */ Sint32       num;
} contpiece;

typedef struct contmodel // sizeof=0x10
{
  /* 0x00 */ Sint32       flag;
  /* 0x04 */ NJS_TEXLIST *texlist;
  /* 0x08 */ void        *unk08;
  /* 0x0C */ NJS_MODEL   *model;
} contmodel;

extern void _rename_CreateBrokenPiece(NJS_POINT3 *pos, Sint32 num, NJS_VECTOR *spd,
                                  Angle ang, Float scl, Float y, void *model,
                                  NJS_TEXLIST *texlist);
extern void _rename_CreateBrokenPieceDS(NJS_POINT3 *pos, Sint32 num,
                                    NJS_VECTOR *spd, Angle ang, Float scl,
                                    Float y, void *model, NJS_MODEL *broken,
                                    NJS_TEXLIST *texlist);

extern Sint32       _rename_contwood_model_loaded;
extern NJS_TEXLIST  _rename_contwood_texlist;
extern NJS_CNK_MODEL _rename_contwood_cnk_model;
extern GJS_MODEL    _rename_contwood_gjmodel;
extern contpiece    _rename_contwood_piece_tbl[3];
extern contmodel    _rename_contwood_model_tbl[4];
extern CCL_INFO     _rename_contwood_colli_info[1];

// ^ extern
// v in this file

static void ObjectContWoodDest(task *tp);
static void ObjectContWoodExec(task *tp);
static void ObjectContWoodDisp(task *tp);
static void ObjectContWoodDispSort(task *tp);

// the task's unused 'awp' slot caches the ground height under the container
#define ShadowY(tp) (*(Float *)&(tp)->awp)

enum {
  MD_WAIT,
  MD_BREAK,
  MD_GONE,
};

void ObjectContWood(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->disp = ObjectContWoodDisp;
  tp->exec = ObjectContWoodExec;
  tp->dest = ObjectContWoodDest;
  switch (_rename_GetStageNum()) {
  case 60:
  case 61:
  case 62:
    tp->disp_sort = ObjectContWoodDispSort;
    break;
  }

  twp->smode = MD_WAIT;
  ShadowY(tp) = -1000000.0f;
  twp->scl.y = 0.0f;
  twp->scl.z = 0.0f;
  CCL_InitShare(tp, _rename_contwood_colli_info,
                ARYLEN(_rename_contwood_colli_info), CID_OBJECT);
  twp->btimer = (Uint8)twp->ang.z;
  _rename_BreakObjEntry(tp);
}

static void ObjectContWoodDest(task *tp) {
  _rename_BreakObjFree(tp);
  tp->awp = NULL;
}

static void ObjectContWoodExec(task *tp) {
  taskwk *twp = tp->twp;
  taskwk *ctwp;
  Sint32     unused[4];
  NJS_POINT3 dustpos;
  Angle3     ang;
  NJS_POINT3 fallpos;
  NJS_POINT3 pos;
  NJS_VECTOR spd;
  Sint32     unused2[2];
  Float      step;
  Float      rate;
  Float      y;
  Sint32     n;
  BOOL       hit;
  task      *hittp;
  Sint32     pno;
  Sint8      mode;
  Sint8      base;
  Sint32     dust;
  Sint32     i;
  Sint32     j;
  Sint32     num;

  if (CheckRangeOut(tp)) {
    return;
  }

  if (twp->scl.y > 0.0f) {
    // scl.y is how far the container still has to drop, scl.z its speed
    step = twp->scl.z;
    if (twp->scl.y < -twp->scl.z) {
      step = -twp->scl.y;
    }
    twp->pos.y += step;
    twp->scl.y += step;
    twp->scl.z = 0.99f * twp->scl.z - 0.08f;
    if (tp->ctp == NULL) {
      _rename_CreateBreakObjColli(tp, NULL, NULL, _rename_contwood_colli_info,
                                   ARYLEN(_rename_contwood_colli_info), 0);
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
    if (fabsf(twp->pos.y - ShadowY(tp)) < 5.0f) {
      _rename_CreateBoxDust(&twp->pos, 10.0f, 10.0f, &twp->ang, 8.0f, 1.2f,
                            0.5f);
    }
  }

  switch (twp->smode) {
  case MD_WAIT:
    n = fn_800067C0(tp);
    hit = FALSE;
    while (n--) {
      hittp = fn_80005904(tp, n)->hit_tp;
      if (fn_8003699C(tp, hittp)) {
        hit = TRUE;
        break;
      }
    }
    if (hit) {
      dustpos = twp->pos;
      dustpos.y += 10.0f;
      for (dust = 0; dust < 14; dust++) {
        _rename_CreateBreakSmoke(&dustpos, 7.0f + 3.0f * njRandom(),
                                0.5f + njRandom(), 2.5f + 4.0f * njRandom());
      }
      twp->wtimer = 0;
      twp->smode = MD_BREAK;
      fn_8006AFFC(0x1010, twp, 1, 0x7F, 0x50, &twp->pos);
      if (hittp->twp->id == 1) {
        pno = IsThisTaskPlayer(hittp);
        if (pno != -1 && (base = playerpwp[pno]->basechar) != 0) {
          mode = playertwp[pno]->mode;
          // base 4 and 5 as one unsigned byte compare
          if (mode != 0x45 && (Uint8)(base - 4) > 1 && mode != 0x4E) {
            SetVelocityP(pno, 0.0f, 0.0f, 0.0f);
          }
        }
      }
      AddScore(20);
      break;
    }
    twp->wtimer++;
    if (-1000000.0f == ShadowY(tp) && (twp->wtimer & 31) == 0) {
      ang.x = 0;
      ang.y = twp->ang.y;
      ang.z = 0;
      ShadowY(tp) = GetShadowPos(twp->pos.x, 5.0f + twp->pos.y, twp->pos.z,
                                 &ang);
    }
    if (playerpwp[0]->equipment & 0x2000410) {
      twp->cwp->info->attr |= 0x4000;
    } else {
      twp->cwp->info->attr &= ~0x4000;
    }
    CCL_Entry(tp);
    break;
  case MD_BREAK:
    if (twp->wtimer == 1) {
      fallpos = twp->pos;
      _rename_BreakObjDropAbove(&fallpos);
    }
    if (twp->wtimer++ > 2) {
      pos = twp->pos;
      pos.y += 10.0f;
      spd.x = 10.0f;
      spd.y = 10.0f;
      spd.z = 10.0f;
      if (_rename_contwood_model_loaded != 0) {
        for (i = 0; i < 3; i++) {
          for (j = 0; j < _rename_contwood_piece_tbl[i].num; j++) {
            if (twp->ang.x & 1) {
              y = -1000000.0f;
            } else if (-1000000.0f != (y = ShadowY(tp))) {
              y = ShadowY(tp);
            } else {
              y = twp->pos.y;
            }
            _rename_CreateBrokenPieceDS(&pos, 1, &spd, twp->ang.y,
                                    0.7f + 1.2f * njRandom(), y,
                                    _rename_contwood_piece_tbl[i].model,
                                    _rename_contwood_model_tbl[i + 1].model,
                                    _rename_contwood_piece_tbl[i].texlist);
          }
        }
      } else {
        for (i = 0; i < 3; i++) {
          for (j = 0; j < _rename_contwood_piece_tbl[i].num; j++) {
            if (twp->ang.x & 1) {
              y = -1000000.0f;
            } else if (-1000000.0f != (y = ShadowY(tp))) {
              y = ShadowY(tp);
            } else {
              y = twp->pos.y;
            }
            _rename_CreateBrokenPiece(&pos, 1, &spd, twp->ang.y,
                                  0.7f + 1.2f * njRandom(), y,
                                  _rename_contwood_piece_tbl[i].model,
                                  _rename_contwood_piece_tbl[i].texlist);
          }
        }
      }
      twp->smode = MD_GONE;
    }
    if (playerpwp[0]->equipment & 0x2000410) {
      twp->cwp->info->attr |= 0x4000;
      break;
    }
    twp->cwp->info->attr &= ~0x4000;
    CCL_Entry(tp);
    break;
  case MD_GONE:
    num = (twp->btimer & 0xF0) >> 4;
    num &= 0xF;
    if (num != 0 && tp->ocp != NULL && !_rename_CheckFlag0x20(tp)) {
      rate = 0.1f * (Float)num;
      rate *= 0.1f;
      if (njRandom() < rate || num >= 10) {
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

static void ObjectContWoodDisp(task *tp) {
  taskwk    *twp = tp->twp;
  NJS_VECTOR v;
  Angle3     ang;

  switch (_rename_GetStageNum()) {
  case 60:
  case 61:
  case 62:
    _rename_njSubVector(&twp->pos, &lbl_80175378[lbl_803ADAD0]->pos, &v);
    if (njScalor2(&v) < 900.0f) {
      return;
    }
    break;
  }

  njSetTexture(&_rename_contwood_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  ang.x = 0;
  ang.y = twp->ang.y;
  ang.z = 0;
  fn_80066820(0xFF, &twp->pos, _rename_contwood_gjmodel.r);
  _rename_gjSetTexMtx(&twp->pos, &ang);
  gjDrawModel(&_rename_contwood_gjmodel);
  fn_8006648C();
  OffControl3D(0x2400);
  njPopMatrixEx();
}

// in stages 60 to 62, of the containers near the camera only the one nearest
// its target is drawn
static void ObjectContWoodDispSort(task *tp) {
  NJS_VECTOR v;
  Angle3     ang;
  Float      near;
  Float      len;
  BOOL       alone;
  taskwk    *twp;
  Sint32     i;
  task      *neartp;

  alone = FALSE;
  twp = tp->twp;
  switch (_rename_GetStageNum()) {
  case 60:
  case 61:
  case 62:
    near = 10000.0f;
    neartp = NULL;
    _rename_njSubVector(&twp->pos, &lbl_80175378[lbl_803ADAD0]->pos, &v);
    if (!(njScalor2(&v) < 900.0f)) {
      break;
    }
    alone = TRUE;
    for (i = 0; i < 256; i++) {
      if (_rename_container_list[i] == NULL) {
        continue;
      }
      if (_rename_container_list[i]->exec != ObjectContWoodExec) {
        continue;
      }
      _rename_njSubVector(&_rename_container_list[i]->twp->pos,
                          &lbl_80175378[lbl_803ADAD0]->pos, &v);
      if (njScalor2(&v) > 900.0f) {
        continue;
      }
      _rename_njSubVector(&lbl_80175378[lbl_803ADAD0]->tgt,
                          &_rename_container_list[i]->twp->pos, &v);
      len = njScalor(&v);
      if (len < near) {
        near = len;
        neartp = _rename_container_list[i];
      }
    }
    if (neartp != tp) {
      return;
    }
    fn_8002B304();
    fn_8002B35C();
    OffControl3D(0x220);
    OnControl3D(0x810);
    fn_801218C8(0xFF, 0x800);
    fn_800156FC(0.5f, 1.0f, 1.0f, 1.0f);
    break;
  }

  njSetTexture(&_rename_contwood_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  OnControl3D(0x2400);
  if (_rename_contwood_model_loaded != 0 && !alone) {
    ds_DrawModelClip(_rename_contwood_model_tbl[0].model);
  } else if (alone) {
    fn_8011E158(&_rename_contwood_cnk_model);
  } else {
    ang.x = 0;
    ang.y = twp->ang.y;
    ang.z = 0;
    fn_80066820(0xFF, &twp->pos, _rename_contwood_gjmodel.r);
    _rename_gjSetTexMtx(&twp->pos, &ang);
    gjDrawModel(&_rename_contwood_gjmodel);
    fn_8006648C();
  }
  OffControl3D(0x2400);
  if (alone) {
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
    fn_8002B348();
    fn_8002B2F8();
  }
  njPopMatrixEx();
}
