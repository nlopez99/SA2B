#include "OBJECT/o_goalring.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njmotion.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "samt/sonic/shadow.h"
#include "set.h"
#include "fabsf.h"

extern Sint32 _rename_GetPlayerCharacter(Sint32);
extern Sint32 _rename_GetStageNum(void);
extern void _rename_SetRelLocal(Sint32, Sint32);
extern void fn_8001BB28(Sint32);
extern void fn_8001C810(Sint32);
extern void fn_80033620(NJS_POINT3 *, Sint32, Sint32, Angle, Float, Float,
                        Sint32, Sint32, Float, Float);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);
extern void fn_8011610C(NJS_VECTOR *);
extern void fn_8011C3A0(void (*)(NJS_CNK_OBJECT *));
extern void fn_8011E1EC(NJS_CNK_OBJECT *, NJS_MOTION *, Float);
extern void fn_8012297C(Sint32);
extern void __njColorBlendingMode(Int, Int);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);

extern void _rename_ShadowTexInit(void *, Sint32);
extern void _rename_ShadowTexFree(void *);
extern void _rename_ShadowTexBegin(void *, Float, Float, void *, void *,
                                   void *);
extern void _rename_ShadowTexEnd(void *);
extern void _rename_ShadowTexDraw(Sint32, NJS_VECTOR *, Float, void *);
extern void _rename_RingDrawShadowModel(void);
extern void _rename_MakeParticle2(NJS_POINT3 *, NJS_VECTOR *, Float);
extern void _rename_MakeParticle3(NJS_POINT3 *, NJS_VECTOR *, Float);
extern void _rename_GoalRingChildExec(task *tp);

extern BOOL DisableObjectFog;

extern NJS_TEXLIST    _rename_goalring_texlist;
extern NJS_CNK_MODEL  _rename_goalring_model;
extern NJS_TEXLIST    _rename_goalring_text1_texlist;
extern NJS_CNK_MODEL  _rename_goalring_text1_model;
extern NJS_TEXLIST    _rename_goalring_text2_texlist;
extern NJS_CNK_MODEL  _rename_goalring_text2_model;
extern NJS_TEXLIST    _rename_goalring_anim_texlist;
extern NJS_CNK_OBJECT _rename_goalring_anim_center_object;
extern NJS_CNK_OBJECT _rename_goalring_anim_object;
extern NJS_MOTION     _rename_goalring_anim_motion;
extern NJS_TEXLIST    _rename_goalring_anim_hit_texlist;
extern NJS_CNK_OBJECT _rename_goalring_anim_hit_center_object;
extern NJS_CNK_OBJECT _rename_goalring_anim_hit_object;
extern NJS_MOTION     _rename_goalring_anim_hit_motion;

extern CCL_INFO   _rename_goalring_colli_info[2];
extern Angle      _rename_goalring_rot_spd;
extern Angle      _rename_goalring_rot_spd_hit;
extern Float      _rename_goalring_ptcl_scl;
extern Float      _rename_goalring_ptcl_spd;
extern Float      _rename_goalring_ptcl_spd_y;
extern Uint32     _rename_goalring_blink_on;
extern Uint32     _rename_goalring_blink_cycle;
extern Float      _rename_goalring_rise_spd;
extern Float      _rename_goalring_anim_spd;
extern Float      _rename_goalring_anim_spd_hit;
extern NJS_POINT3 _rename_goalring_ptcl_pos0;
extern NJS_POINT3 _rename_goalring_ptcl_pos1;
extern NJS_VECTOR _rename_goalring_ptcl_vec0;
extern NJS_VECTOR _rename_goalring_ptcl_vec1;
extern NJS_POINT3 _rename_goalring_glow_pos;
extern Angle      _rename_goalring_glow_ang;
extern NJS_VECTOR _rename_goalring_shadow_scl;
extern NJS_VECTOR _rename_goalring_anim_shadow_scl;

extern NJS_MATRIX *_rename_goalring_matrix_p;

// ^ extern
// v in this file

static void ObjectGoalRingDest(task *tp);
static void ObjectGoalRingExec(task *tp);
static void ObjectGoalRingDisp(task *tp);
static void ObjectGoalRingDispSort(task *tp);
static void ObjectGoalRingDispShad(task *tp);

typedef struct goalringwk // sizeof=0x94
{
  /* 0x00 */ NJS_MATRIX mat;
  /* 0x30 */ Sint32 mat_ok;
  /* 0x34 */ Float posy;
  /* 0x38 */ Sint32 ptcl_num;
  /* 0x3C */ Uint8 shadow[0x58];
} goalringwk;

#define GetWork(task) ((goalringwk *)task->mwp)
#define GetType(twp) (twp->ang.x % 3)

// NJS_MATRIX on the stack at an 8-byte aligned address; volatile to match
#define ALIGNED_MATRIX(name)                                                   \
  Uint8 name##_buf[sizeof(NJS_MATRIX) + 8];                                    \
  NJS_MATRIX *volatile name =                                                  \
      (NJS_MATRIX *)(((Uint32)name##_buf + 4) & ~7)

static void ObjectGoalRingDispSortDummy(task *tp) {}

void ObjectGoalRing(task *tp) {
  taskwk *twp = tp->twp;

  twp->scl.x = 0.0f;
  twp->scl.y = 0.0f;
  twp->scl.z = 0.0f;

  switch (_rename_GetPlayerCharacter(0)) {
  case PLNO_KNUCKLES:
  case PLNO_ROUGE:
    if (lbl_801CC168._23 != 1) {
      if (GetType(twp) == 1) {
        break;
      }
      return;
    }
    twp->ang.x = 2;
    twp->mode = 1;
    if (playertwp[0] == NULL || playertwp[0]->wtimer < 120) {
      return;
    }
    break;
  }

  if (lbl_801CC168._23 != 2 && GetType(twp) == 1) {
    task *ctp;

    if (tp->ctp == NULL &&
        (ctp = CreateChildTask(IM_TWK, _rename_GoalRingChildExec, tp)) !=
            NULL) {
      tp->disp_sort = ObjectGoalRingDispSortDummy;
      ctp->twp->scl.x = 2.0f;
      ctp->twp->smode = 1;
    }
    if (tp->ctp != NULL) {
      tp->ctp->twp->pos = twp->pos;
    }
    return;
  }

  if (twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(goalringwk));
  if (tp->mwp == NULL) {
    return;
  }

  _rename_ShadowTexInit(GetWork(tp)->shadow, 0x40);
  tp->disp = ObjectGoalRingDisp;
  tp->disp_sort = ObjectGoalRingDispSort;
  tp->dest = ObjectGoalRingDest;
  tp->exec = ObjectGoalRingExec;
  if (GetType(twp) != 1 &&
      (_rename_GetStageNum() == 57 || _rename_GetStageNum() == 6)) {
    tp->disp_shad = ObjectGoalRingDispShad;
  }
  twp->btimer = 0;
  if (GetType(twp) == 1) {
    CCL_Init(tp, &_rename_goalring_colli_info[1], 1, CID_OBJECT);
  } else {
    CCL_Init(tp, _rename_goalring_colli_info, 1, CID_OBJECT);
  }
  twp->smode = 0;
  twp->scl.z = -1000000.0f;
}

static void ObjectGoalRingDest(task *tp) {
  _rename_ShadowTexFree(GetWork(tp)->shadow);
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectGoalRingExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 pno;
  task *hit;
  Float dy;

  if (twp->smode == 0 && twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }

  if (lbl_801CC168._7C % _rename_goalring_blink_cycle <
      _rename_goalring_blink_on) {
    switch (lbl_801CC168._23) {
    case 1:
    case 2:
      twp->btimer = 2;
      break;
    default:
      twp->btimer = 1;
      break;
    }
  } else {
    twp->btimer = 0;
  }

  if (-1000000.0f == twp->scl.z && (lbl_801CC168._7C & 0x1F) == 0) {
    Angle3 ang;

    ang.x = 0;
    ang.y = twp->ang.y;
    ang.z = 0;
    twp->scl.z =
        GetShadowPos(twp->pos.x, 3.0f + twp->pos.y, twp->pos.z, &ang);
  }

  // type tests in this order (other, 1, 0) to match
  if (lbl_801CC168._37 != 0) {
    // frozen: no animation, no sound
  } else if (GetType(twp) != 0) {
    if (GetType(twp) == 1) {
      if (twp->smode != 0) {
        twp->scl.y += _rename_goalring_anim_spd_hit;
        if (twp->scl.y >
            (Float)(_rename_goalring_anim_hit_motion.nbFrame - 1)) {
          twp->scl.y = 0.0f;
        }
        fn_8006AFFC(0x100E, tp, 1, 30, 30, &twp->pos);
      } else {
        twp->scl.y += _rename_goalring_anim_spd;
        if (twp->scl.y > (Float)(_rename_goalring_anim_motion.nbFrame - 1)) {
          twp->scl.y = 0.0f;
        }
        fn_8006AFFC(0x100D, tp, 1, 30 - (twp->wtimer << 1), 30, &twp->pos);
      }
    } else {
      fn_8006AFFC(0x1012, twp, 1, (Sint16) - (twp->wtimer << 1), 30,
                  &twp->pos);
    }
  } else {
    if (twp->smode != 0) {
      twp->ang.y += _rename_goalring_rot_spd_hit;
      twp->wtimer++;
      twp->pos.y += _rename_goalring_rise_spd;
    } else {
      twp->ang.y += _rename_goalring_rot_spd;
    }
    if (njRandom() < 0.9f) {
      NJS_POINT3 pos;
      NJS_VECTOR vec;

      pos.x = 0.0f;
      pos.y = 25.0f;
      pos.z = 0.0f;
      vec.x = 0.0f;
      vec.y = 1.0f;
      vec.z = 0.0f;
      njPushMatrixEx();
      njUnitMatrix(NULL);
      njTranslateEx(&twp->pos);
      njRotateY(NULL, twp->ang.y);
      njRotateZ(NULL, NJM_DEG_ANG(180.0f * (njRandom() - 0.5f)));
      njCalcVector(NULL, &vec, &vec);
      njCalcPoint(NULL, &pos, &pos);
      njPopMatrixEx();
      if (fabsf(vec.y) < 0.9f) {
        Float len;

        vec.y = 0.0f;
        len = njScalor(&vec);
        vec.x *= _rename_goalring_ptcl_spd / len;
        vec.y = _rename_goalring_ptcl_spd_y;
        vec.z *= _rename_goalring_ptcl_spd / len;
        _rename_MakeParticle2(&pos, &vec, 1.5f);
      }
    }
    fn_8006AFFC(0x1012, twp, 1, (Sint16) - (twp->wtimer << 1), 30,
                &twp->pos);
  }

  if (twp->smode == 0 && (hit = CCL_IsHitPlayer(tp)) != NULL &&
      (pno = IsThisTaskPlayer(hit)) != -1) {
    if (lbl_801CC168._23 != 1 &&
        (lbl_801CC168._23 != 2 || GetType(twp) != 0)) {
      fn_8001C810(pno);
      twp->smode = 1;
      fn_8006AFFC(0x1013, tp, 1, 30, 180, &twp->pos);
      _rename_SetRelLocal(pno, 2);
    } else {
      fn_8006AFFC(0x1014, tp, 1, 30, 180, &twp->pos);
      fn_8001BB28(pno);
      twp->smode = 1;
    }
    twp->scl.y = 0.0f;
  } else {
    CCL_Entry(tp);
  }

  if (GetType(twp) != 0 && GetWork(tp)->mat_ok != 0 && twp->smode == 0 &&
      lbl_801CC168._37 == 0) {
    NJS_VECTOR vec;
    NJS_POINT3 pos;
    Float *posy;
    Float old;
    Sint32 *num;

    njPushMatrixEx();
    njSetMatrix(NULL, &GetWork(tp)->mat);
    njCalcPoint(NULL, &_rename_goalring_ptcl_pos0, &pos);
    njCalcVector(NULL, &_rename_goalring_ptcl_vec0, &vec);
    posy = &GetWork(tp)->posy;
    old = *posy;
    *posy = pos.y;
    dy = pos.y - old;
    if (old > pos.y) {
      GetWork(tp)->ptcl_num = 6;
    }
    num = &GetWork(tp)->ptcl_num;
    if (*num != 0 && lbl_801CC168._7C % 6 < 3) {
      (*num)--;
      vec.y += 0.8f * dy;
      _rename_MakeParticle3(&pos, &vec, _rename_goalring_ptcl_scl);
      njCalcPoint(NULL, &_rename_goalring_ptcl_pos1, &pos);
      njCalcVector(NULL, &_rename_goalring_ptcl_vec1, &vec);
      vec.y += 0.5f * dy;
      _rename_MakeParticle3(&pos, &vec, _rename_goalring_ptcl_scl);
    }
    njPopMatrixEx();
  }

  if (twp->smode == 1 && twp->wtimer >= 60) {
    if (tp->ocp != NULL) {
      DeadOut(tp);
    } else {
      FreeTask(tp);
    }
  }
}

static void GoalRingGetMatrix(void) {
  if (_rename_goalring_matrix_p != NULL) {
    njGetMatrix(_rename_goalring_matrix_p);
  }
}

static void GoalRingObjectCallback(NJS_CNK_OBJECT *object) {
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  if (object == &_rename_goalring_anim_hit_center_object) {
    fn_80033620(&_rename_goalring_glow_pos, 3, 0x4000, 0, 1.5f, 1.5f, -1, 0,
                0.0f, 0.0f);
    GoalRingGetMatrix();
  }
  if (object == &_rename_goalring_anim_center_object) {
    fn_80033620(&_rename_goalring_glow_pos, 3, 0x4000,
                _rename_goalring_glow_ang, 1.5f, 1.5f, -1, 0, 0.0f, 0.0f);
    GoalRingGetMatrix();
  }
}

static void ObjectGoalRingDisp(task *tp) {
  taskwk *twp = tp->twp;
  ALIGNED_MATRIX(m);

  njGetMatrix(m);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  if (GetType(twp) == 1) {
    fn_8012297C(1);
    if (twp->smode != 0) {
      njSetTexture(&_rename_goalring_anim_hit_texlist);
      fn_8011E1EC(&_rename_goalring_anim_hit_object,
                  &_rename_goalring_anim_hit_motion, twp->scl.y);
    } else {
      njSetTexture(&_rename_goalring_anim_texlist);
      fn_8011E1EC(&_rename_goalring_anim_object, &_rename_goalring_anim_motion,
                  twp->scl.y);
    }
    fn_8012297C(3);
  } else {
    njSetTexture(&_rename_goalring_texlist);
    if (twp->smode != 0) {
      Float scl = 1.0f - twp->wtimer / 60.0f;

      if (scl < 0.0f) {
        scl = 0.0f;
      }
      njScale(NULL, scl, 1.0f, scl);
    }
    njCnkCacheDrawModel(&_rename_goalring_model);
    switch (twp->btimer) {
    case 1:
      njSetTexture(&_rename_goalring_text1_texlist);
      njCnkCacheDrawModel(&_rename_goalring_text1_model);
      break;
    case 2:
      njSetTexture(&_rename_goalring_text2_texlist);
      njCnkCacheDrawModel(&_rename_goalring_text2_model);
      break;
    }
  }
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
  njPopMatrixEx();
}

static void ObjectGoalRingDispSort(task *tp) {
  taskwk *twp = tp->twp;

  if (GetType(twp) == 1) {
    ALIGNED_MATRIX(m);
    NJS_ARGB argb; // unused

    njGetMatrix(m);
    njPushMatrixEx();
    njTranslate(NULL, twp->pos.x, 0.3f + twp->scl.z, twp->pos.z);
    njRotateY(NULL, twp->ang.y);
    njTranslate(NULL, 0.0f, 0.0f, 0.8f);
    fn_8011610C(&_rename_goalring_anim_shadow_scl);
    _rename_RingDrawShadowModel();
    njPopMatrixEx();
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    if (DisableObjectFog) {
      njDisableFog();
      gjSetFog();
    }
    _rename_goalring_matrix_p = &GetWork(tp)->mat;
    fn_8011C3A0(GoalRingObjectCallback);
    if (twp->smode != 0) {
      njSetTexture(&_rename_goalring_anim_hit_texlist);
      fn_8011E1EC(&_rename_goalring_anim_hit_object,
                  &_rename_goalring_anim_hit_motion, twp->scl.y);
    } else {
      njSetTexture(&_rename_goalring_anim_texlist);
      fn_8011E1EC(&_rename_goalring_anim_object, &_rename_goalring_anim_motion,
                  twp->scl.y);
    }
    fn_8011C3A0(NULL);
    njPushMatrixEx();
    njSetMatrix(NULL, m);
    njInvertMatrix(NULL);
    njMultiMatrix(NULL, &GetWork(tp)->mat);
    njGetMatrix(&GetWork(tp)->mat);
    njPopMatrixEx();
    GetWork(tp)->mat_ok = 1;
    _rename_goalring_matrix_p = NULL;
    if (DisableObjectFog) {
      njEnableFog();
      gjSetFog();
    }
    njPopMatrixEx();
  } else if (tp->disp_shad == NULL) {
    njPushMatrixEx();
    njTranslate(NULL, twp->pos.x, 0.8f + twp->scl.z, twp->pos.z);
    njRotateY(NULL, twp->ang.y);
    fn_8011610C(&_rename_goalring_shadow_scl);
    _rename_RingDrawShadowModel();
    njPopMatrixEx();
  }
}

static void ObjectGoalRingDispShad(task *tp) {
  taskwk *twp = tp->twp;

  _rename_ShadowTexBegin(GetWork(tp)->shadow, 2.5f, 0.0f, &twp->pos, NULL,
                         NULL);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  if (DisableObjectFog) {
    njDisableFog();
    gjSetFog();
  }
  if (GetType(twp) == 1) {
    fn_8012297C(1);
    if (twp->smode != 0) {
      njSetTexture(&_rename_goalring_anim_hit_texlist);
      fn_8011E1EC(&_rename_goalring_anim_hit_object,
                  &_rename_goalring_anim_hit_motion, twp->scl.y);
    } else {
      njSetTexture(&_rename_goalring_anim_texlist);
      fn_8011E1EC(&_rename_goalring_anim_object, &_rename_goalring_anim_motion,
                  twp->scl.y);
    }
    fn_8012297C(3);
  } else {
    njSetTexture(&_rename_goalring_texlist);
    if (twp->smode != 0) {
      Float scl = 1.0f - twp->wtimer / 60.0f;

      if (scl < 0.0f) {
        scl = 0.0f;
      }
      njScale(NULL, scl, 1.0f, scl);
    }
    njCnkCacheDrawModel(&_rename_goalring_model);
    switch (twp->btimer) {
    case 1:
      njSetTexture(&_rename_goalring_text1_texlist);
      njCnkCacheDrawModel(&_rename_goalring_text1_model);
      break;
    case 2:
      njSetTexture(&_rename_goalring_text2_texlist);
      njCnkCacheDrawModel(&_rename_goalring_text2_model);
      break;
    }
  }
  if (DisableObjectFog) {
    njEnableFog();
    gjSetFog();
  }
  njPopMatrixEx();
  _rename_ShadowTexEnd(GetWork(tp)->shadow);
  _rename_ShadowTexDraw(2, &twp->pos, 90.0f, GetWork(tp)->shadow);
}
