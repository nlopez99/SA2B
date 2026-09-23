#include "OBJECT/o_savepoint.h"

#include "CCL.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "samt/sonic/sound.h"
#include "set.h"
#include "fabsf.h"

// argument of fn_800618B4, which draws a number (or the ':') on screen
typedef struct numinfo // sizeof=0x20
{
  /* 0x00 */ Uint8 type;
  /* 0x01 */ Uint8 unk_1;
  /* 0x02 */ Uint16 unk_2;
  /* 0x04 */ Sint32 max;
  /* 0x08 */ Sint32 num;
  /* 0x0C */ NJS_POINT3 pos;
  /* 0x18 */ Float scl;
  /* 0x1C */ Uint32 color;
} numinfo;

// argument of fn_80072E50, which animates the lamp texture of a chunk model
typedef struct texanim // sizeof=0x24
{
  /* 0x00 */ Sint32 unk_0;
  /* 0x04 */ Sint32 nbFrame;
  /* 0x08 */ Sint32 unk_8;
  /* 0x0C */ Sint32 unk_C;
  /* 0x10 */ void *data;
  /* 0x14 */ Sint32 unk_14[4];
} texanim;

typedef struct lbl_803AD860_t {
  Uint8 unk_0[0x10];
  /* 0x10 */ Sint16 _10;
} lbl_803AD860_t;

extern Angle SubAngle(Angle a, Angle b);
extern Sint16 GetRingNumber(Sint32 pno);
extern BOOL _rename_CheckFlag0x40(task *tp);
extern void _rename_SetFlag0x40(task *tp);
extern void _rename_SetRelLocal(Sint32 pno, Sint32);
extern void _rename_GiveItemP(Sint32 pno, Sint32 kind);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern void fn_8001475C(void);
extern void fn_800148FC(void);
extern void fn_80014CD8(void);
extern void fn_8001CA78(Sint32 pno, NJS_POINT3 *pos, Angle3 *ang, Uint8 id);
extern Uint32 fn_8001DDA8(Sint32);
extern void fn_800618B4(numinfo *info);
extern void fn_80062AEC(Sint8 *min, Sint8 *sec, Sint8 *frame);
extern void fn_80072E50(NJS_CNK_MODEL *model, texanim *anim, Float frame);
extern void fn_8011E158(NJS_CNK_MODEL *model);

extern BOOL DisableObjectFog;
extern Sint32 lbl_803ADAD0;
extern Sint32 lbl_803ADAD4;
extern lbl_803AD860_t *lbl_803AD860;

extern NJS_TEXLIST    _rename_savepoint_texlist;
extern NJS_CNK_MODEL  _rename_savepoint_model_L;
extern NJS_CNK_OBJECT _rename_savepoint_object_L;
extern NJS_CNK_MODEL  _rename_savepoint_model_R;
extern NJS_CNK_OBJECT _rename_savepoint_object_R;
extern NJS_CNK_MODEL  _rename_savepoint_model_base;
extern texanim        _rename_savepoint_texanim_R;
extern texanim        _rename_savepoint_texanim_L;

extern CCL_INFO       _rename_savepoint_colli_info[1];
extern Float          _rename_savepoint_time_scl;
extern Float          _rename_savepoint_time_ofs_colon;
extern Float          _rename_savepoint_time_ofs_sec;

// ^ extern
// v in this file

typedef struct savepointwk // sizeof=0x3C
{
  /* 0x00 */ Sint32 unk_0;
  /* 0x04 */ Angle3 ang;
  /* 0x10 */ Angle3 spd;
  /* 0x1C */ task *ctp[2];
  /* 0x24 */ Float frame;
  /* 0x28 */ Sint32 timer[2];
  /* 0x30 */ Sint16 count[2];
  /* 0x34 */ Sint16 min[2];
  /* 0x38 */ Sint16 sec[2];
} savepointwk;

#define GetWork(task) ((savepointwk *)task->awp->work.ptr[0])
#define GetTimer(wk) ((Sint32 *)(wk)->timer)

static void savepointExit(task *tp);
static void savepointDisplay_dely(task *tp);
static void savepointDisplay(task *tp);
static savepointwk *allocSavePoint(void);
static void dummyFunc(task *tp);
static void initCollidata(taskwk *twp);
static void savepointCheckHit(task *tp, taskwk *twp);
static void savepointSwing(task *tp, taskwk *twp);
static void savepointSetFrame(task *tp, taskwk *twp);
static void savepointCountTimer(task *tp, taskwk *twp);
static Float savepointGetHitSpeed(task *tp, taskwk *twp, Sint32 pno);

// work of the save point being processed, set by the executor and displayer
static savepointwk *savepoint_data;

Uint32 savepoint_time_color = 0xFFFFFFFF;

void ObjectSavePoint(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  savepoint_data = GetWork(tp);
  switch (twp->mode) {
  case 0:
    if (lbl_801CC168._23 == 3) {
      DeadOut(tp);
      return;
    }
    twp->mode = 1;
    tp->awp->work.ptr[0] = allocSavePoint();
    savepoint_data = GetWork(tp);
    tp->dest = savepointExit;
    tp->disp = savepointDisplay;
    tp->disp_dely = savepointDisplay_dely;
    savepoint_data->ctp[0] = CreateChildTask(IM_TWK, dummyFunc, tp);
    savepoint_data->ctp[1] = CreateChildTask(IM_TWK, dummyFunc, tp);
    CCL_Init(tp, _rename_savepoint_colli_info,
             ARYLEN(_rename_savepoint_colli_info), CID_OBJECT);
    initCollidata(twp);
    savepoint_data->ang.x = 0x4000;
    savepoint_data->ang.y = -0x4000;
    if (fn_8001DDA8(0) <= 1 && lbl_801CC168.TWO_PLAYER) {
      twp->btimer = twp->ang.y;
    } else {
      twp->btimer = 0;
    }
    if (tp->ocp == NULL) {
      break;
    }
    SetNoRevive(tp);
    if (_rename_CheckFlag0x40(tp)) {
      savepoint_data->ang.x = 0;
      twp->mode = 3;
    }
    break;
  case 1:
    savepointCheckHit(tp, twp);
    break;
  case 2:
    savepointCheckHit(tp, twp);
    savepointSwing(tp, twp);
    savepointSetFrame(tp, twp);
    savepointCountTimer(tp, twp);
    break;
  case 3:
    savepoint_data->frame = 1.0f;
    savepointCheckHit(tp, twp);
    savepointCountTimer(tp, twp);
    break;
  case 4:
    break;
  case 5:
  default:
    DeadOut(tp);
    break;
  }
}

static void savepointDispTime(Sint32 min, Sint32 sec, NJS_POINT3 *pos) {
  numinfo info;

  info.type = 0x49;
  info.unk_1 = 0;
  info.unk_2 = 0;
  info.max = 99;
  info.num = min;
  info.pos.x = pos->x;
  info.pos.y = pos->y;
  info.pos.z = 0.0f;
  info.scl = _rename_savepoint_time_scl;
  info.color = savepoint_time_color;

  if (lbl_801CC168.TWO_PLAYER) {
    fn_800148FC();
    fn_80014CD8();
  }
  njDisableFog();
  gjSetFog();

  fn_800618B4(&info);
  info.pos.x += _rename_savepoint_time_ofs_colon;
  info.max = 0;
  info.num = 0;
  info.type = 0x21;
  fn_800618B4(&info);
  info.pos.x += _rename_savepoint_time_ofs_sec;
  info.type = 0x49;
  info.max = 60;
  info.num = sec;
  fn_800618B4(&info);

  njEnableFog();
  gjSetFog();
  if (lbl_801CC168.TWO_PLAYER) {
    fn_8001475C();
  }
}

static void savepointExit(task *tp) {
  if (GetWork(tp) != NULL) {
    syFree(GetWork(tp));
    tp->awp->work.ptr[0] = NULL;
  }
}

static void savepointDisplay_dely(task *tp) {
  taskwk *twp = tp->twp;
  savepointwk *wk;
  Sint32 *timer;
  Sint32 pno;
  NJS_POINT3 pos;

  if (lbl_801CC168.TWO_PLAYER) {
    return;
  }
  if (playertwp[1] != NULL) {
    return;
  }
  if ((wk = savepoint_data) == NULL) {
    return;
  }
  if (twp->mode != 2 && twp->mode != 3) {
    return;
  }

  if (*(timer = &GetTimer(wk)[pno = lbl_803ADAD0]) <= 0) {
    return;
  }
  if (lbl_803ADAD4 <= 0) {
    return;
  }

  pos.x = 640.0f / (Float)lbl_803ADAD4 * (1.0f + (Float)pno) - 135.0f;
  pos.y = 430.0f;
  pos.z = 0.0f;
  // blink: shown on even 10 frame steps, hidden on odd ones
  switch ((300 - *timer) / 10) {
  case 0:
  case 2:
  case 4:
  case 6:
  case 8:
  case 10:
  default:
    savepointDispTime(wk->min[pno], wk->sec[pno], &pos);
    break;
  case 1:
  case 3:
  case 5:
  case 7:
  case 9:
  case 11:
    break;
  }
}

static void savepointDisplay(task *tp) {
  taskwk *twp = tp->twp;

  savepoint_data = GetWork(tp);
  njPushMatrix(NULL);
  njTranslateV(NULL, &twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y & 0xFF00);
  njSetTexture(&_rename_savepoint_texlist);
  fn_8011E158(&_rename_savepoint_model_base);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }

  njPushMatrix(NULL);
  njTranslateV(NULL, &_rename_savepoint_object_R.pos);
  njRotateY(NULL, savepoint_data->ang.y);
  njRotateX(NULL, savepoint_data->ang.x);
  fn_80072E50(&_rename_savepoint_model_R, &_rename_savepoint_texanim_R,
              savepoint_data->frame);
  fn_8011E158(&_rename_savepoint_model_R);
  njPopMatrix(1);

  njTranslateV(NULL, &_rename_savepoint_object_L.pos);
  njRotateY(NULL, -savepoint_data->ang.y);
  njRotateX(NULL, savepoint_data->ang.x);
  fn_80072E50(&_rename_savepoint_model_L, &_rename_savepoint_texanim_L,
              savepoint_data->frame);
  fn_8011E158(&_rename_savepoint_model_L);
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
  njPopMatrix(1);
}

static savepointwk *allocSavePoint(void) {
  savepointwk *wk = syMalloc(sizeof(savepointwk));
  Uint8 *p = (Uint8 *)wk;
  Uint32 i;

  for (i = 0; i < sizeof(savepointwk); i++) {
    p[i] = 0;
  }
  return wk;
}

static void dummyFunc(task *tp) {}

static void initCollidata(taskwk *twp) {
  static NJS_VECTOR vec[2] = {{-15.0f, 9.8f, 0.0f}, {15.0f, 9.8f, 0.0f}};
  NJS_VECTOR v[2];

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y & 0xFF00);
  njCalcVector(NULL, &vec[0], &v[0]);
  njCalcVector(NULL, &vec[1], &v[1]);
  njPopMatrixEx();

  savepoint_data->ctp[0]->twp->pos.x += v[0].x;
  savepoint_data->ctp[0]->twp->pos.y += v[0].y;
  savepoint_data->ctp[0]->twp->pos.z += v[0].z;
  savepoint_data->ctp[1]->twp->pos.x += v[1].x;
  savepoint_data->ctp[1]->twp->pos.y += v[1].y;
  savepoint_data->ctp[1]->twp->pos.z += v[1].z;
}

static void savepointCheckHit(task *tp, taskwk *twp) {
  task *hit_tp;
  Sint32 pno;
  Sint16 ring;
  Sint8 min;
  Sint8 sec;
  Sint8 frame;

  savepoint_data->ctp[0]->twp->ang.x = savepoint_data->ang.x + twp->ang.x;
  savepoint_data->ctp[0]->twp->ang.y =
      (twp->ang.y & 0xFF00) - savepoint_data->ang.y;
  savepoint_data->ctp[0]->twp->ang.z = savepoint_data->ang.z + twp->ang.z;
  savepoint_data->ctp[1]->twp->ang.x = savepoint_data->ang.x + twp->ang.x;
  savepoint_data->ctp[1]->twp->ang.y =
      savepoint_data->ang.y + (twp->ang.y & 0xFF00);
  savepoint_data->ctp[1]->twp->ang.z = savepoint_data->ang.z + twp->ang.z;

  if (twp->mode == 1 && (hit_tp = CCL_IsHitPlayer(tp)) != NULL &&
      IsThisTaskPlayer(hit_tp) != -1 && hit_tp->ptp == NULL) {
    pno = IsThisTaskPlayer(hit_tp);
    twp->mode = 2;
    savepoint_data->spd.y =
        NJM_DEG_ANG(10.0f * savepointGetHitSpeed(tp, twp, pno));
    savepoint_data->timer[pno] = 300;
    fn_8001CA78(pno, &playertwp[pno]->pos, &playertwp[pno]->ang, twp->btimer);
    _rename_SetFlag0x40(tp);
    SE_Call(0x1001, NULL, 0, 0);
    fn_80062AEC(&min, &sec, &frame);
    savepoint_data->min[pno] = min;
    savepoint_data->sec[pno] = sec;
    _rename_SetRelLocal(pno, 3);

    // bonus item, the more rings the better
    ring = GetRingNumber(pno);
    if (ring >= 90) {
      if ((playerpwp[pno]->item & 1) || (playerpwp[pno]->item & 2)) {
        _rename_GiveItemP(pno, 6);
      } else {
        _rename_GiveItemP(pno, 3);
      }
    } else if (ring >= 80) {
      _rename_GiveItemP(pno, 8);
    } else if (ring >= 60) {
      _rename_GiveItemP(pno, 1);
      lbl_801CC168._6E += 20;
      lbl_803AD860->_10 += 20;
    } else if (ring >= 40) {
      _rename_GiveItemP(pno, 0);
      lbl_801CC168._6E += 10;
      lbl_803AD860->_10 += 10;
    } else if (ring >= 20) {
      _rename_GiveItemP(pno, 2);
      lbl_801CC168._6E += 5;
      lbl_803AD860->_10 += 5;
    }
  }

  CCL_Entry(tp);
}

// the bars spin around y from the hit and fall back down around x once slow
static void savepointSwing(task *tp, taskwk *twp) {
  Angle ang_x = savepoint_data->ang.x & 0xFFFF;
  Angle ang_y = savepoint_data->ang.y & 0xFFFF;
  Sint16 spd_x = savepoint_data->spd.x & 0xFFFF;
  Angle spd_y = savepoint_data->spd.y & 0xFFFF;
  Angle new_spd;
  Angle abs_spd;
  Angle ang;
  Sint32 i;
  STACK_PAD_VAR(14);

  ang_x += spd_x;
  ang_y += spd_y;
  new_spd = 0.995f * (Sint16)spd_y;
  abs_spd = fabsf((Sint16)new_spd);
  if ((Sint16)abs_spd < 0xE38) {
    if ((Sint16)ang_x > 0x2000) {
      ang = 0x47D2 - (Sint16)ang_x;
    } else {
      ang = ang_x;
    }
    spd_x = -(Sint16)(0.1f * (Sint16)ang * (0xE38 - (Sint16)abs_spd) / 8192.0f);
  }

  savepoint_data->ang.x = (Sint16)ang_x;
  savepoint_data->ang.y = (Sint16)ang_y;
  savepoint_data->spd.x = spd_x;
  savepoint_data->spd.y = (Sint16)new_spd;
  if (spd_x == 0 && (Sint16)new_spd == 0) {
    twp->mode = 3;
    return;
  }

  for (i = 0; i < 2; i++) {
    Sint16 *count = &savepoint_data->count[i];
    if (*count < 3) {
      (*count)++;
    }
  }
}

static void savepointSetFrame(task *tp, taskwk *twp) {
  savepoint_data->frame = 1.0f;
}

static void savepointCountTimer(task *tp, taskwk *twp) {
  Sint32 i;

  for (i = 0; i < 2; i++) {
    Sint32 *timer = &savepoint_data->timer[i];
    if (*timer > 0) {
      (*timer)--;
    }
  }
}

static Float savepointGetHitSpeed(task *tp, taskwk *twp, Sint32 pno) {
  Float spd = njScalor(&playerpwp[pno]->spd);
  Angle ang = SubAngle(0x4000 - playertwp[pno]->ang.y, twp->ang.y & 0xFF00);

  if ((Sint32)fabsf(ang) > 0x4000) {
    return -spd;
  }
  return spd;
}
