#include "OBJECT/o_spring.h"

#include "CCL.h"
// player.h widens the player number to Sint32; this file passes it as Uint8
#define SetInputP SetInputP_Sint32
#define SetSpringVelocityP SetSpringVelocityP_Sint32
#include "samt/sonic/player.h"
#undef SetInputP
#undef SetSpringVelocityP
#include "set.h"
#include "qFabsf.h"
#include "fabsf.h"

extern void fn_8002FB2C(Sint8, int, int, int); 
extern void _rename_SetRelLocal(Sint8, int);
extern void SE_Call(int, int, int, int);
extern void SetInputP(Uint8 pno, Sint8 mode, Sint32 unk);
extern void SetSpringVelocityP(Uint8 pno, Float x, Float y, Float z);
extern void ds_DrawModelClip(NJS_MODEL *);

inline float sqrtf(float f);

extern NJS_TEXLIST _rename_spring_TexList;
extern NJS_OBJECT  _rename_SpringObj1;
extern NJS_OBJECT  _rename_SpringObj2;
extern NJS_OBJECT  _rename_SpringObj3;
extern NJS_OBJECT  _rename_SpringObj4;
 
extern GJS_OBJECT  _rename_SpringGjObj1;
extern GJS_OBJECT  _rename_SpringGjObj2;
extern GJS_OBJECT  _rename_SpringGjObj3;
extern GJS_MODEL   _rename_SpringGjModel1;
 
extern GJS_OBJECT  _rename_SpringGjObj4;
extern GJS_MODEL   _rename_SpringGjModel2;

extern CCL_INFO    spr_colli_info[1];
extern CCL_INFO    spr_colli_info_b[1];
extern NJS_MODEL*  _rename_SpringObjArr[][4];

// ^ extern
// v in this file

static void DrawSpring(task *task);
static void DrawSpringGC(task *task);
static void DrawSpringB(task *task);
static void DrawSpringBGC(task *task);
static void SpringDie(task *task);
static void SpringBDie(task *task);
static void VacumePlayer(task*, Uint8 flag);

static BOOL spring_cnkdraw = FALSE;

enum {
  SPR_INIT  = 0x0,
  SPR_NOR   = 0x1,
  SPR_TOUCH = 0x2,
  SPR_DONE  = 0x3,
};

enum {
  TYPE_A = 0x0,
  TYPE_B = 0x1,
};

enum {
  SPRING_H_MODE_NORMAL = 0x0,
  SPRING_H_MODE_SPRING = 0x1,
  SPRING_H_MODE_END    = 0x2,
};

struct spring_h_taskwk // sizeof=0x40
{
  /* 0x00*/ Sint8 mode;
  /* 0x01*/ Sint8 smode;
  /* 0x02*/ Uint8 id;
  /* 0x03*/ Uint8 btimer;
  /* 0x04*/ Sint16 flag;
  /* 0x06*/ Sint16 spring_timer;
  /* 0x08*/ Float k;
  /* 0x0C*/ Sint32 wave_ang;
  /* 0x10*/ Sint32 se_count;
  /* 0x14*/ Angle3 ang;
  /* 0x20*/ NJS_POINT3 pos;
  /* 0x2C*/ Float x_scl;
  /* 0x30*/ Float y_spd;
  /* 0x34*/ Float spring;
  /* 0x38*/ colliwk *cwp;
  /* 0x3C*/ struct eventwk *ewp;
};

#define GetV1(task) (*(Float *)&task->awp)
#define GetV2(task) (*(Float *)&task->mwp)
#define GetV3(task) (*(Float *)&task->fwp)

void ObjectSpring(task *t) {
  taskwk *twp = t->twp;
  if (CheckRangeOut(t)) {
    return;
  }

  switch (twp->mode) {

  case SPR_INIT: {
    t->dest = SpringDie;
    if (spring_cnkdraw) {
      t->disp = DrawSpring;
    } else {
      t->disp = DrawSpringGC;
    }
    CCL_Init(t, spr_colli_info, ARYLEN(spr_colli_info), 4);
    twp->cwp->flag |= 0x40;
    twp->scl.x = 180.f;
    twp->scl.z = 0.0f;
    GetV1(t) = 0.5f;
    twp->mode = SPR_NOR;
  } break;

  case SPR_NOR: {
    if (twp->wtimer != 0) {
      twp->wtimer--;
    }
    GetV2(t) = -qFabsf(GetV1(t) * njSin(twp->scl.x * (0x8000 / 180.f)));

    twp->scl.x += twp->scl.z;

    if (twp->scl.x >= 290.f && twp->scl.x < twp->scl.z + 290.f) {
      twp->scl.z = 18.f;
    }

    GetV1(t) = njCos((twp->scl.x - 180.0f) * 0.1f * (0x8000 / 180.0f)) * 0.6f;

    if (GetV1(t) < 0.0f) {
      twp->scl.x = 180.0f;
      twp->scl.z = 0.0f;
    }
    twp->btimer++;
    if (twp->btimer > 2) {
      task *hitPlayer = CCL_IsHitPlayer(t);
      if (hitPlayer != NULL) {
        Uint8 i;
        for (i = 0; i < 2; i++) {
          if (hitPlayer == playertp[i]) {
            GetV2(t) = 0.1f;
            twp->smode = i;
            twp->scl.z = 20.f;
            twp->scl.x = 270.f;
            twp->mode = SPR_TOUCH;
            twp->btimer = 0;
            break;
          }
        }
      }
    } else if (CCL_IsHitPlayer(t)) {
      twp->btimer = 0;
    }
    if (twp->mode != SPR_TOUCH) {
      CCL_Entry(t);
    }
    if (twp->btimer > 50) {
      twp->btimer = 50;
    }
  } break;

  case SPR_TOUCH: {
    SE_Call(0x1000, 0, 0, 0);
    twp->btimer = 0;
    VacumePlayer(t, TYPE_A);
  } break;
  
  case SPR_DONE: {
    DeadOut(t);
  } break;
  } // end switch
}

static void VacumePlayer(task *t, Uint8 flag) {
  Float stack_pad[2];
  Sint8 smode_;
  taskwk *twp = t->twp;
  Float runningSpeed;
  NJS_POINT3 inPoint = {0.0f, 5.0f, 0.0f};
  NJS_POINT3 offset;
  NJS_POINT3 outPoint;
  taskwk *player = playertwp[twp->smode];
  
  inPoint.y = 5.0f + twp->scl.y;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  njCalcPoint(NULL, &inPoint, &outPoint);
  njPopMatrixEx();
  offset.x = twp->pos.x + outPoint.x;
  offset.y = twp->pos.y + outPoint.y;
  offset.z = twp->pos.z + outPoint.z;
  if (player != NULL) {
    Sint8 smode = twp->smode;
    float diffX = offset.x - player->pos.x;
    float diffY = offset.y - player->pos.y;
    float diffZ = offset.z - player->pos.z;
    float distance;
    diffY = fabsf(diffY); // unused?
    distance = sqrtf(diffX * diffX + diffZ * diffZ);
    smode_ = smode;
    GetPlayerRunningSpeed(smode_, &runningSpeed);
    if (1.0f > runningSpeed || GetV2(t) != 0.1f) {
      if (14.0f > distance && distance >= 2.0f) {
        if ((playerpwp[twp->smode]->item & 0x4000) == 0) {
          SetInputP(smode_, 9, 0);
          player->pos.x += 2.0f * (diffX / distance);
          player->pos.z += 2.0f * (diffZ / distance);
          SetInputP(smode_, 15, 0);
        }
        if (flag == TYPE_A) {
          SetSpringVelocityP(smode_, outPoint.x, outPoint.y, outPoint.z);
        } else {
          player->pos.x = offset.x;
          player->pos.y = twp->pos.y + outPoint.y * 0.8f;
          player->pos.z = offset.z;
          if (twp->scl.x <= 0.0f) {
            twp->scl.x = 0.0f;
          }
          SetVelocityYAndRotationAndNoconTimeP(smode_, twp->scl.y + 5.0f,
                                               &twp->ang.x, (Sint32)twp->scl.x);
        }
        twp->mode = SPR_NOR;
      } else {
        if (flag == TYPE_A) {
          SetSpringVelocityP(smode_, outPoint.x, outPoint.y, outPoint.z);
        } else {
          player->pos.x = offset.x;
          player->pos.y = twp->pos.y + outPoint.y * 0.8f;
          player->pos.z = offset.z;
          if (twp->scl.x <= 0.f) {
            twp->scl.x = 0;
          }
          SetVelocityYAndRotationAndNoconTimeP(smode_, twp->scl.y + 5.0f,
                                               &twp->ang.x, (Sint32)twp->scl.x);
        }
        twp->mode = SPR_NOR;
      }
    } else {
      if ((playerpwp[twp->smode]->item & 0x4000) == 0) {
        SetInputP(smode_, 9, 0);
        player->pos = offset;
        SetInputP(smode_, 15, 0);
      }

      if (flag == TYPE_A) {
        SetSpringVelocityP(smode_, outPoint.x, outPoint.y, outPoint.z);
      } else {
        if (twp->scl.x <= 0.f) {
          twp->scl.x = 0.f;
        }
        SetVelocityYAndRotationAndNoconTimeP(smode_, twp->scl.y + 5.0f,
                                             &twp->ang.x, (Sint32)twp->scl.x);
      }
      twp->mode = SPR_NOR;
    }
    GetV2(t) = 0.1f;
    fn_8002FB2C(smode_, 4, 15, 0);
    _rename_SetRelLocal(smode_, 1);
  }
}

static void SpringDie(task *t) {
  t->mwp = NULL;
  t->fwp = NULL;
  t->awp = NULL;
}

static void DrawSpringGC(task *t) {
  taskwk *twp = t->twp;
  GJS_MODEL* model;
  njSetTexture(&_rename_spring_TexList);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  if (twp->wtimer != 0) {
    Float f = 1.f / (Float)twp->wtimer;
    njScale(NULL, f, f, f);
  }
  model = &_rename_SpringGjModel1;
  gjDrawModel(model);
  
  njTranslateEx(&_rename_SpringGjObj3.pos);
  njScale(NULL, 0.98f, 1.0f + GetV2(t), 0.98f);
  gjDrawModel(_rename_SpringGjObj3.model);

  njTranslateEx(&_rename_SpringGjObj2.pos);
  njTranslate(NULL, 0.0f, 4.f * GetV2(t), 0.0f);
  gjDrawModel(_rename_SpringGjObj2.model);
  njPopMatrixEx();
}

static void DrawSpring(task *t) {
  taskwk *twp = t->twp;
  njSetTexture(&_rename_spring_TexList);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  if (twp->wtimer != 0) {
    Float f = 1.f / (Float)twp->wtimer;
    njScale(NULL, f, f, f);
  }
  ds_DrawModelClip(_rename_SpringObjArr[0][3]); // model_bane_banebottom_banebottom
  njTranslateEx(&_rename_SpringObj2.pos);
  njScale(NULL, 0.98f, 1.0f + GetV2(t), 0.98f);
  ds_DrawModelClip(_rename_SpringObjArr[1][3]); // model_bane_banebottom_spring
  njTranslateEx(&_rename_SpringObj1.pos);
  njTranslate(NULL, 0.0f, 4.f * GetV2(t), 0.0f);
  ds_DrawModelClip(_rename_SpringObjArr[2][3]); // model_bane_banebottom_banehead
  njPopMatrixEx();
}

void ObjectSpringB(task *t) {
  taskwk *twp = t->twp;
  if (CheckRangeOut(t)) {
    return;
  }

  switch (twp->mode) {

  case SPR_INIT: {
    t->dest = SpringBDie;
    if (spring_cnkdraw) {
      t->disp = DrawSpringB;
    } else {
      t->disp = DrawSpringBGC;
    }
    CCL_Init(t, spr_colli_info_b, ARYLEN(spr_colli_info_b), 4);
    twp->cwp->flag |= 0x40;
    GetV3(t) = 180.f;
    twp->scl.z = 0.0f;
    GetV1(t) = 0.5f;
    twp->btimer = 0;
    twp->mode = SPR_NOR;
  } break;

  case SPR_NOR: {
    if (twp->wtimer != 0) {
      twp->wtimer--;
    }
    GetV2(t) = -qFabsf(GetV1(t) * njSin(GetV3(t) * (0x8000 / 180.f)));

    GetV3(t) += twp->scl.z;

    if (GetV3(t) >= 290.f && GetV3(t) < twp->scl.z + 290.f) {
      twp->scl.z = 15.f;
    }

    GetV1(t) = njCos((GetV3(t) - 180.0f) * 0.1f * (0x8000 / 180.0f)) * 0.6f;

    if (GetV1(t) < 0.0f) {
      GetV3(t) = 180.0f;
      twp->scl.z = 0.0f;
    }
    twp->btimer++;
    if ((Uint8)twp->btimer > 2) {
      task *hitPlayer = CCL_IsHitPlayer(t);
      if (hitPlayer != NULL) {
        Uint8 i;
        for (i = 0; i < 2; i++) {
          if (hitPlayer == playertp[i]) {
            GetV2(t) = 0.1f;
            twp->smode = i;
            twp->scl.z = 20.f;
            GetV3(t) = 270.f;
            twp->mode = 2;
            twp->btimer = 0;
            break;
          }
        }
      }
    } else if (CCL_IsHitPlayer(t)) {
      twp->btimer = 0;
    }
    if (twp->mode != 2) {
      CCL_Entry(t);
    }
    if ((Uint8)twp->btimer > 50) {
      twp->btimer = 50;
    }
  } break;

  case SPR_TOUCH: {
    twp->btimer = 0;
    VacumePlayer(t, 1);
    SE_Call(0x1000, 0, 0, 0);
  } break;
  
  case SPR_DONE: {
    DeadOut(t);
  } break;
  } // end switch
}


void DrawSpringBGC(task *t) {
  NJS_POINT3 STACK_PAD;
  taskwk *twp = t->twp;
  GJS_MODEL* model;
  njSetTexture(&_rename_spring_TexList);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  if (twp->wtimer != 0) {
    Float f = 1.f / (Float)twp->wtimer;
    njScale(NULL, f, f, f);
  }
  gjDrawModel(&_rename_SpringGjModel2);
  njPushMatrixEx();
  njTranslateEx(&_rename_SpringGjObj4.pos);
  njPushMatrixEx();
  njTranslateEx(&_rename_SpringGjObj1.pos);
  njTranslate(NULL, 0.0f, 4.f * GetV2(t), 0.0f);
  gjDrawModel(_rename_SpringGjObj1.model);
  njPopMatrixEx();
  njScale(NULL, 0.98f, 1.0f + GetV2(t), 0.98f);
  gjDrawModel(_rename_SpringGjObj4.model);
  njPopMatrix(2);
}

static void DrawSpringB(task *t) {
  float STACK_PAD[4];
  taskwk *twp = t->twp;
  njSetTexture(&_rename_spring_TexList);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  if (twp->wtimer != 0) {
    Float f = 1.f / (Float)twp->wtimer;
    njScale(NULL, f, f, f);
  }
  ds_DrawModelClip(_rename_SpringObjArr[3][3]);  // model_bane_type_b_type_b_type_b
  njPushMatrixEx();
  njTranslateEx(&_rename_SpringObj3.pos);
  njPushMatrixEx();
  njTranslateEx(&_rename_SpringObj4.pos);
  njTranslate(NULL, 0.0f, 4.f * GetV2(t), 0.0f); // model_bane_type_b_type_b_banehead
  ds_DrawModelClip(_rename_SpringObjArr[5][3]);
  njPopMatrixEx();
  njScale(NULL, 0.98f, 1.0f + GetV2(t), 0.98f);
  ds_DrawModelClip(_rename_SpringObjArr[4][3]); // model_bane_type_b_type_b_spring
  njPopMatrix(2);
}

static void SpringBDie(task *t) {
  t->mwp = NULL;
  t->fwp = NULL;
  t->awp = NULL;
}
