#include "OBJECT/o_3spring.h"

#include "CCL.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "fabsf.h"

extern void fn_80038008(Sint8, int, NJS_POINT3 *, Angle3 *);
extern void fn_800399BC(Sint8, Float, Float, Float);
extern void fn_80039D20(Sint8, NJS_POINT3 *, Angle3 *, int);
extern void fn_8002FB2C(Sint8, int, int, int);
extern void SE_Call(int, int, int, int);

// ^ extern
// v in this file

static void Object3SpringDest(task *tp);
static void Object3SpringExec(task *tp);
static void Object3SpringDisp(task *tp);

typedef struct spring3wk // sizeof=0xC
{
  /* 0x00 */ Float spd;
  /* 0x04 */ Float pos;
  /* 0x08 */ Uint8 timer[2];
} spring3wk;

#define GetWork(task) ((spring3wk *)(task)->mwp)

static NJS_TEXNAME spring3_texname[] = {
    {"sikake_01_128"},
    {"sikake_02_128"},
    {"sikake_08_64"},
    {"sikake_15_64"},
};

static NJS_TEXLIST spring3_texlist = {
    spring3_texname,
    ARRAY_COUNT(spring3_texname),
};

static NJS_POINT3 spring3_model2_pos[] = {
#include "assets/spring3_model2_pos.inc"
};

static NJS_VECTOR spring3_model2_nrm[] = {
#include "assets/spring3_model2_nrm.inc"
};

static NJS_TEX spring3_model2_uv[] = {
#include "assets/spring3_model2_uv.inc"
};

static GJS_ARRAY spring3_model2_arrays[] = {
    {
        GJ_VA_POS,
        sizeof(*spring3_model2_pos),
        ARRAY_COUNT(spring3_model2_pos),
        GJ_ARR_TYPE(GJ_POS_XYZ, GJ_F32),
        spring3_model2_pos,
        sizeof(spring3_model2_pos),
    },
    {
        GJ_VA_NRM,
        sizeof(*spring3_model2_nrm),
        ARRAY_COUNT(spring3_model2_nrm),
        GJ_ARR_TYPE(GJ_NRM_XYZ, GJ_F32),
        spring3_model2_nrm,
        sizeof(spring3_model2_nrm),
    },
    {
        GJ_VA_TEX0,
        sizeof(*spring3_model2_uv),
        ARRAY_COUNT(spring3_model2_uv),
        GJ_ARR_TYPE(GJ_TEX_ST, GJ_S16),
        spring3_model2_uv,
        sizeof(spring3_model2_uv),
    },
    {GJ_VA_NULL},
};

static GJS_MATERIAL spring3_model2_mat_0[] = {
#include "assets/spring3_model2_mat_0.inc"
};

static GJS_MATERIAL spring3_model2_mat_1[] = {
#include "assets/spring3_model2_mat_1.inc"
};

static GJS_MATERIAL spring3_model2_mat_2[] = {
#include "assets/spring3_model2_mat_2.inc"
};

static Uint8 spring3_model2_dl_0[] ATTRIBUTE_ALIGN(32) = {
#include "assets/spring3_model2_dl_0.inc"
};

static Uint8 spring3_model2_dl_1[] ATTRIBUTE_ALIGN(32) = {
#include "assets/spring3_model2_dl_1.inc"
};

static Uint8 spring3_model2_dl_2[] ATTRIBUTE_ALIGN(32) = {
#include "assets/spring3_model2_dl_2.inc"
};

static Uint8 spring3_model2_dl_3[] ATTRIBUTE_ALIGN(32) = {
#include "assets/spring3_model2_dl_3.inc"
};

static GJS_MESHSET spring3_model2_meshset[] = {
    {spring3_model2_mat_0, ARRAY_COUNT(spring3_model2_mat_0),
     spring3_model2_dl_0, ARRAY_COUNT(spring3_model2_dl_0)},
    {spring3_model2_mat_1, ARRAY_COUNT(spring3_model2_mat_1),
     spring3_model2_dl_1, ARRAY_COUNT(spring3_model2_dl_1)},
    {NULL, 0, spring3_model2_dl_2, ARRAY_COUNT(spring3_model2_dl_2)},
    {spring3_model2_mat_2, ARRAY_COUNT(spring3_model2_mat_2),
     spring3_model2_dl_3, ARRAY_COUNT(spring3_model2_dl_3)},
};

static GJS_MODEL spring3_model2 = {
    spring3_model2_arrays,
    NULL,
    spring3_model2_meshset,
    NULL,
    ARRAY_COUNT(spring3_model2_meshset),
    0,
    {0.0f, 5.975694f, 6.280429f},
    25.453106f,
};

static NJS_POINT3 spring3_model1_pos[] = {
#include "assets/spring3_model1_pos.inc"
};

static NJS_VECTOR spring3_model1_nrm[] = {
#include "assets/spring3_model1_nrm.inc"
};

static NJS_TEX spring3_model1_uv[] = {
#include "assets/spring3_model1_uv.inc"
};

static GJS_ARRAY spring3_model1_arrays[] = {
    {
        GJ_VA_POS,
        sizeof(*spring3_model1_pos),
        ARRAY_COUNT(spring3_model1_pos),
        GJ_ARR_TYPE(GJ_POS_XYZ, GJ_F32),
        spring3_model1_pos,
        sizeof(spring3_model1_pos),
    },
    {
        GJ_VA_NRM,
        sizeof(*spring3_model1_nrm),
        ARRAY_COUNT(spring3_model1_nrm),
        GJ_ARR_TYPE(GJ_NRM_XYZ, GJ_F32),
        spring3_model1_nrm,
        sizeof(spring3_model1_nrm),
    },
    {
        GJ_VA_TEX0,
        sizeof(*spring3_model1_uv),
        ARRAY_COUNT(spring3_model1_uv),
        GJ_ARR_TYPE(GJ_TEX_ST, GJ_S16),
        spring3_model1_uv,
        sizeof(spring3_model1_uv),
    },
    {GJ_VA_NULL},
};

static GJS_MATERIAL spring3_model1_mat_0[] = {
#include "assets/spring3_model1_mat_0.inc"
};

static GJS_MATERIAL spring3_model1_mat_1[] = {
#include "assets/spring3_model1_mat_1.inc"
};

static GJS_MATERIAL spring3_model1_mat_2[] = {
#include "assets/spring3_model1_mat_2.inc"
};

static Uint8 spring3_model1_dl_0[] ATTRIBUTE_ALIGN(32) = {
#include "assets/spring3_model1_dl_0.inc"
};

static Uint8 spring3_model1_dl_1[] ATTRIBUTE_ALIGN(32) = {
#include "assets/spring3_model1_dl_1.inc"
};

static Uint8 spring3_model1_dl_2[] ATTRIBUTE_ALIGN(32) = {
#include "assets/spring3_model1_dl_2.inc"
};

static GJS_MESHSET spring3_model1_meshset[] = {
    {spring3_model1_mat_0, ARRAY_COUNT(spring3_model1_mat_0),
     spring3_model1_dl_0, ARRAY_COUNT(spring3_model1_dl_0)},
    {spring3_model1_mat_1, ARRAY_COUNT(spring3_model1_mat_1),
     spring3_model1_dl_1, ARRAY_COUNT(spring3_model1_dl_1)},
    {spring3_model1_mat_2, ARRAY_COUNT(spring3_model1_mat_2),
     spring3_model1_dl_2, ARRAY_COUNT(spring3_model1_dl_2)},
};

static GJS_MODEL spring3_model1 = {
    spring3_model1_arrays,
    NULL,
    spring3_model1_meshset,
    NULL,
    ARRAY_COUNT(spring3_model1_meshset),
    0,
    {0.0f, 4.38871f, 5.830001f},
    23.485126f,
};

NJS_TEXLIST *spring3_texlists[] = {&spring3_texlist, NULL};

static CCL_INFO spring3_colli_info[] = {
    {0,
     CI_FORM_CYLINDER2,
     0x77,
     0,
     0,
     {0.0f, 5.0f, 5.0f},
     6.0f,
     23.0f,
     0.0f,
     0.0f,
     0,
     0,
     0x4000},
};

static Float spring3_k = 0.25f;
static Float spring3_touch_spd = 1.0f;
static Float spring3_damp = 0.95f;

static void Object3SpringJump(task *tp, Sint32 player) {
  Sint8 pno;
  taskwk *twp = tp->twp;
  NJS_POINT3 spd = {0.0f, 5.0f, 0.0f};
  Angle3 ang;
  NJS_POINT3 pos;

  // a Sint8 parameter would not be re-extended; the original narrows here
  pno = player;

  fn_80038008(pno, 0, &pos, NULL);
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  njPushMatrixEx();
  njInvertMatrix(NULL);
  njCalcPoint(NULL, &pos, &pos);
  pos.z = 8.0f + 0.3f * (pos.z - 8.0f);
  pos.y += 5.0f;
  njPopMatrixEx();
  njCalcPoint(NULL, &pos, &pos);
  njPopMatrixEx();
  fn_800399BC(pno, pos.x, pos.y, pos.z);
  spd.y += twp->scl.x;
  ang.x = twp->ang.x;
  ang.y = twp->ang.y + 0x8000;
  ang.z = twp->ang.z;
  fn_80039D20(pno, &spd, &ang, 30);
  SE_Call(0x1000, 0, 0, 0);
  fn_8002FB2C(pno, 4, 15, 0);
}

void Object3Spring(task *tp) {
  taskwk *twp = tp->twp;
  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(spring3wk));
  if (tp->mwp == NULL) {
    return;
  }

  tp->disp = Object3SpringDisp;
  tp->exec = Object3SpringExec;
  tp->dest = Object3SpringDest;
  twp->smode = 0;
  CCL_Init(tp, spring3_colli_info, ARYLEN(spring3_colli_info),
           CID_OBJECT);
  twp->cwp->flag |= 0x40;
  GetWork(tp)->spd = 0.0f;
  GetWork(tp)->pos = 0.0f;
}

static void Object3SpringDest(task *tp) {
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void Object3SpringExec(task *tp) {
  task *hitPlayer;
  Sint32 pno;
  Sint32 i;

  if (CheckRangeOut(tp)) {
    return;
  }

  hitPlayer = CCL_IsHitPlayer(tp);
  if (hitPlayer != NULL && (pno = IsThisTaskPlayer(hitPlayer)) >= 0) {
    // indexing through a pointer, not the member array, gives add rD, base, idx
    if (((Uint8 *)GetWork(tp)->timer)[pno] == 0) {
      GetWork(tp)->spd = spring3_touch_spd;
      GetWork(tp)->pos = 0.0f;
      Object3SpringJump(tp, (Sint8)pno);
    }
    ((Uint8 *)GetWork(tp)->timer)[pno] = 10;
  }

  for (i = 0; i < 2; i++) {
    // without the pointer the unrolled second pass adds 8 and 1 separately
    Uint8 *timer = &GetWork(tp)->timer[i];
    if (*timer != 0) {
      (*timer)--;
    }
  }

  CCL_Entry(tp);
  GetWork(tp)->spd = spring3_damp *
                     (GetWork(tp)->spd - GetWork(tp)->pos * spring3_k);
  GetWork(tp)->pos += GetWork(tp)->spd;
}

static void Object3SpringDisp(task *tp) {
  taskwk *twp = tp->twp;
  Float f;
  njSetTexture(&spring3_texlist);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  gjDrawModel(&spring3_model1);
  f = fabsf(GetWork(tp)->pos);
  njTranslate(NULL, 0.0f, f, f);
  gjDrawModel(&spring3_model2);
  njPopMatrixEx();
}
