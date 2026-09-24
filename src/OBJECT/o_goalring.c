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
#include "types.h"

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

extern NJS_TEXLIST _rename_ring_texlist;

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

enum {
  MD_GOALRING_0,
  MD_GOALRING_1,
  MD_GOALRING_2,
};

#define GetWork(task) ((goalringwk *)(task)->mwp)
#define GetType(twp) ((twp)->ang.x % 3)

// NJS_MATRIX on the stack at an 8-byte aligned address; volatile to match.
// ATTRIBUTE_ALIGN is ignored on locals, so the buffer is aligned by hand.
#define ALIGNED_MATRIX(name)                                                   \
  Uint8 name##_buf[sizeof(NJS_MATRIX) + 8];                                    \
  NJS_MATRIX *volatile name =                                                  \
      (NJS_MATRIX *)ALIGN_PREV((Uint32)name##_buf + 4, 8)

static NJS_TEXNAME goalring_texname[] = {
    {"sikake_14_32"},
    {"sikake_05_64"},
};

static NJS_TEXLIST goalring_texlist = {goalring_texname,
                                       ARRAY_COUNT(goalring_texname)};

static Sint16 goalring_plist[] = {
#include "assets/goalring_plist.inc"
};

static Sint32 goalring_vlist[] = {
#include "assets/goalring_vlist.inc"
};

static NJS_CNK_MODEL goalring_model = {
    goalring_vlist,
    goalring_plist,
    {3e-6f, 3e-6f, 0.0f},
    26.292267f,
};

static NJS_TEXNAME goalring_text1_texname[] = {
    {"sikake_10_128"},
};

static NJS_TEXLIST goalring_text1_texlist = {
    goalring_text1_texname, ARRAY_COUNT(goalring_text1_texname)};

static Sint16 goalring_text1_plist[] = {
#include "assets/goalring_text1_plist.inc"
};

static Sint32 goalring_text1_vlist[] = {
#include "assets/goalring_text1_vlist.inc"
};

static NJS_CNK_MODEL goalring_text1_model = {
    goalring_text1_vlist,
    goalring_text1_plist,
    {0.0f, 0.0f, -1e-6f},
    15.230057f,
};

static NJS_TEXNAME goalring_text2_texname[] = {
    {"sikake_10_128"},
};

static NJS_TEXLIST goalring_text2_texlist = {
    goalring_text2_texname, ARRAY_COUNT(goalring_text2_texname)};

static Sint16 goalring_text2_plist[] = {
#include "assets/goalring_text2_plist.inc"
};

static Sint32 goalring_text2_vlist[] = {
#include "assets/goalring_text2_vlist.inc"
};

static NJS_CNK_MODEL goalring_text2_model = {
    goalring_text2_vlist,
    goalring_text2_plist,
    {0.0f, 0.0f, -1e-6f},
    15.230057f,
};

static NJS_TEXNAME goalring_anim_texname[] = {
    {"sikake_43_32"},
    {"sikake_44_32"},
    {"sikake_45_64"},
    {"sikake_49_64"},
};

static NJS_TEXLIST goalring_anim_texlist = {goalring_anim_texname,
                                            ARRAY_COUNT(goalring_anim_texname)};

static Sint16 goalring_anim_plist_8[] = {
#include "assets/goalring_anim_plist_8.inc"
};

static Sint32 goalring_anim_vlist_8[] = {
#include "assets/goalring_anim_vlist_8.inc"
};

static NJS_CNK_MODEL goalring_anim_model_8 = {
    goalring_anim_vlist_8,
    goalring_anim_plist_8,
    {0.300701f, -0.852327f, -0.111707f},
    1.090747f,
};

static NJS_CNK_OBJECT goalring_anim_object_8 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_model_8,
    {-0.661664f, 0.811234f, 0.16f},
    {0x38E3, -0x7FFF, -0xE38},
    {1.0f, 1.0f, 1.0f},
    NULL,
    NULL,
    0.0f,
};

static Sint16 goalring_anim_plist_7[] = {
#include "assets/goalring_anim_plist_7.inc"
};

static Sint32 goalring_anim_vlist_7[] = {
#include "assets/goalring_anim_vlist_7.inc"
};

static NJS_CNK_MODEL goalring_anim_model_7 = {
    goalring_anim_vlist_7,
    goalring_anim_plist_7,
    {0.268491f, -0.862628f, 0.42467f},
    1.329391f,
};

static NJS_CNK_OBJECT goalring_anim_object_7 = {
    NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_model_7,
    {0.441818f, 0.031819f, 0.0f},
    {0, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_object_8,
    0.0f,
};

static Sint16 goalring_anim_plist_6[] = {
#include "assets/goalring_anim_plist_6.inc"
};

static Sint32 goalring_anim_vlist_6[] = {
#include "assets/goalring_anim_vlist_6.inc"
};

static NJS_CNK_MODEL goalring_anim_model_6 = {
    goalring_anim_vlist_6,
    goalring_anim_plist_6,
    {0.268491f, -0.862628f, -0.42467f},
    1.329391f,
};

static NJS_CNK_OBJECT goalring_anim_object_6 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_model_6,
    {-0.441818f, 0.031819f, 0.0f},
    {0, -0x8000, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_object_7,
    0.0f,
};

static Sint16 goalring_anim_plist_5[] = {
#include "assets/goalring_anim_plist_5.inc"
};

static Sint32 goalring_anim_vlist_5[] = {
#include "assets/goalring_anim_vlist_5.inc"
};

static NJS_CNK_MODEL goalring_anim_model_5 = {
    goalring_anim_vlist_5,
    goalring_anim_plist_5,
    {0.0f, -0.411845f, -0.185203f},
    0.567583f,
};

static NJS_CNK_OBJECT goalring_anim_object_5 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_model_5,
    {0.0f, -0.015406f, -1.009871f},
    {0x18E3, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_object_6,
    0.0f,
};

static Sint16 goalring_anim_plist_4[] = {
#include "assets/goalring_anim_plist_4.inc"
};

static Sint32 goalring_anim_vlist_4[] = {
#include "assets/goalring_anim_vlist_4.inc"
};

static NJS_CNK_MODEL goalring_anim_model_4 = {
    goalring_anim_vlist_4,
    goalring_anim_plist_4,
    {0.300701f, -0.852327f, 0.111707f},
    1.090747f,
};

static NJS_CNK_OBJECT goalring_anim_object_4 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_model_4,
    {0.661664f, 0.811234f, 0.16f},
    {-0x38E3, 0, 0xE38},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_object_5,
    0.0f,
};

static Sint16 goalring_anim_plist_3[] = {
#include "assets/goalring_anim_plist_3.inc"
};

static Sint32 goalring_anim_vlist_3[] = {
#include "assets/goalring_anim_vlist_3.inc"
};

static NJS_CNK_MODEL goalring_anim_model_3 = {
    goalring_anim_vlist_3,
    goalring_anim_plist_3,
    {-0.62439f, 0.036828f, -0.579342f},
    1.078913f,
};

static NJS_CNK_OBJECT goalring_anim_object_3 = {
    NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_model_3,
    {-0.378701f, 0.648003f, -0.56423f},
    {0, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_object_4,
    0.0f,
};

static Sint16 goalring_anim_plist_2[] = {
#include "assets/goalring_anim_plist_2.inc"
};

static Sint32 goalring_anim_vlist_2[] = {
#include "assets/goalring_anim_vlist_2.inc"
};

static NJS_CNK_MODEL goalring_anim_model_2 = {
    goalring_anim_vlist_2,
    goalring_anim_plist_2,
    {0.62439f, 0.036828f, -0.579342f},
    1.078913f,
};

static NJS_CNK_OBJECT goalring_anim_object_2 = {
    NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_model_2,
    {0.378701f, 0.648003f, -0.56423f},
    {0, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_object_3,
    0.0f,
};

static Sint16 goalring_anim_plist_1[] = {
#include "assets/goalring_anim_plist_1.inc"
};

static Sint32 goalring_anim_vlist_1[] = {
#include "assets/goalring_anim_vlist_1.inc"
};

static NJS_CNK_MODEL goalring_anim_model_1 = {
    goalring_anim_vlist_1,
    goalring_anim_plist_1,
    {-4e-6f, 1.729028f, -0.018082f},
    2.618065f,
};

static NJS_CNK_OBJECT goalring_anim_center_object = {
    NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_model_1,
    {0.0f, 0.724436f, 1e-6f},
    {0, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_object_2,
    0.0f,
};

static Sint16 goalring_anim_plist_0[] = {
#include "assets/goalring_anim_plist_0.inc"
};

static Sint32 goalring_anim_vlist_0[] = {
#include "assets/goalring_anim_vlist_0.inc"
};

static NJS_CNK_MODEL goalring_anim_model_0 = {
    goalring_anim_vlist_0,
    goalring_anim_plist_0,
    {2.7e-5f, 0.178874f, 0.065301f},
    1.465567f,
};

static NJS_CNK_OBJECT goalring_anim_object = {
    NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL,
    &goalring_anim_model_0,
    {0.0f, 1.25f, 0.0f},
    {0, 0, 0},
    {1.0f, 1.0f, 1.0f},
    &goalring_anim_center_object,
    NULL,
    0.0f,
};

static NJS_MKEY_F goalring_anim_pos_0[] = {
    {0, {0.0f, 1.25f, 0.0f}},
    {9, {0.0f, 1.25f, 0.0f}},
};

static NJS_MKEY_A goalring_anim_ang_0[] = {
    {0, {0, 0, 0}},
    {9, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_ang_1[] = {
    {0, {0, 0, 0}},      {1, {0x1DD, 0, 0}},  {2, {0x38E, 0, 0}},
    {3, {0x268, 0, 0}},  {4, {0x50, 0, 0}},   {5, {-0x1C7, 0, 0}},
    {6, {-0x38E, 0, 0}}, {7, {-0x26F, 0, 0}}, {8, {-0xD2, 0, 0}},
    {9, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_ang_2[] = {
    {0, {0, 0, 0}},     {1, {0, 0x2C7, 0}},  {2, {0, 0x8E3, 0}},
    {3, {0, 0xEFF, 0}}, {4, {0, 0x11C7, 0}}, {5, {0, 0xFED, 0}},
    {6, {0, 0xB85, 0}}, {7, {0, 0x641, 0}},  {8, {0, 0x1D9, 0}},
    {9, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_ang_3[] = {
    {0, {0, 0, 0}},      {1, {0, -0x2C7, 0}},  {2, {0, -0x8E3, 0}},
    {3, {0, -0xEFF, 0}}, {4, {0, -0x11C7, 0}}, {5, {0, -0xFED, 0}},
    {6, {0, -0xB85, 0}}, {7, {0, -0x641, 0}},  {8, {0, -0x1D9, 0}},
    {9, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_ang_4[] = {
    {0, {-0x38E3, 0, 0xE38}}, {1, {-0x36D0, 0, 0xE38}},
    {2, {-0x3555, 0, 0xE38}}, {3, {-0x3792, 0, 0xE38}},
    {4, {-0x3B01, 0, 0xE38}}, {5, {-0x3E44, 0, 0xE38}},
    {6, {-0x3FFF, 0, 0xE38}}, {7, {-0x3E61, 0, 0xE38}},
    {8, {-0x3AD8, 0, 0xE38}}, {9, {-0x38E3, 0, 0xE38}},
};

static NJS_MKEY_A goalring_anim_ang_5[] = {
    {0, {0x18E3, 0, 0}},       {1, {0x18E3, -0xFFF, 0}},
    {2, {0x18E3, -0x1FFF, 0}}, {3, {0x18E3, -0x1333, 0}},
    {4, {0x18E3, 0, 0}},       {5, {0x18E3, 0xDFC, 0}},
    {6, {0x18E3, 0x1A8C, 0}},  {7, {0x18E3, 0x1FFF, 0}},
    {8, {0x18E3, 0xFFF, 0}},   {9, {0x18E3, 0, 0}},
};

static NJS_MKEY_A goalring_anim_ang_6[] = {
    {0, {0, -0x7FFF, 0}},
    {9, {0, -0x7FFF, 0}},
};

static NJS_MKEY_A goalring_anim_ang_7[] = {
    {0, {0, 0, 0}},
    {9, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_ang_8[] = {
    {0, {0x38E3, -0x7FFF, -0xE38}}, {1, {0x36D0, -0x7FFF, -0xE38}},
    {2, {0x3555, -0x7FFF, -0xE38}}, {3, {0x3792, -0x7FFF, -0xE38}},
    {4, {0x3B01, -0x7FFF, -0xE38}}, {5, {0x3E44, -0x7FFF, -0xE38}},
    {6, {0x3FFF, -0x7FFF, -0xE38}}, {7, {0x3E61, -0x7FFF, -0xE38}},
    {8, {0x3AD8, -0x7FFF, -0xE38}}, {9, {0x38E3, -0x7FFF, -0xE38}},
};

static NJS_MDATA2 goalring_anim_mdata[] = {
    {{goalring_anim_pos_0, goalring_anim_ang_0},
     {ARRAY_COUNT(goalring_anim_pos_0), ARRAY_COUNT(goalring_anim_ang_0)}},
    {{NULL, goalring_anim_ang_1}, {0, ARRAY_COUNT(goalring_anim_ang_1)}},
    {{NULL, goalring_anim_ang_2}, {0, ARRAY_COUNT(goalring_anim_ang_2)}},
    {{NULL, goalring_anim_ang_3}, {0, ARRAY_COUNT(goalring_anim_ang_3)}},
    {{NULL, goalring_anim_ang_4}, {0, ARRAY_COUNT(goalring_anim_ang_4)}},
    {{NULL, goalring_anim_ang_5}, {0, ARRAY_COUNT(goalring_anim_ang_5)}},
    {{NULL, goalring_anim_ang_6}, {0, ARRAY_COUNT(goalring_anim_ang_6)}},
    {{NULL, goalring_anim_ang_7}, {0, ARRAY_COUNT(goalring_anim_ang_7)}},
    {{NULL, goalring_anim_ang_8}, {0, ARRAY_COUNT(goalring_anim_ang_8)}},
};

static NJS_MOTION goalring_anim_motion = {goalring_anim_mdata, 10, 3, 2};

static NJS_TEXNAME goalring_anim_hit_texname[] = {
    {"sikake_43_32"},
    {"sikake_44_32"},
    {"sikake_45_64"},
    {"sikake_48_64"},
};

static NJS_TEXLIST goalring_anim_hit_texlist = {
    goalring_anim_hit_texname, ARRAY_COUNT(goalring_anim_hit_texname)};

static Sint16 goalring_anim_hit_plist_8[] = {
#include "assets/goalring_anim_hit_plist_8.inc"
};

static Sint32 goalring_anim_hit_vlist_8[] = {
#include "assets/goalring_anim_hit_vlist_8.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_8 = {
    goalring_anim_hit_vlist_8,
    goalring_anim_hit_plist_8,
    {0.300701f, -0.852327f, -0.111707f},
    0.991277f,
};

static NJS_CNK_OBJECT goalring_anim_hit_object_8 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_hit_model_8,
    {-0.661664f, 0.811234f, 0.16f},
    {0x38E, -0x7FFF, -0xE38},
    {1.0f, 1.0f, 1.0f},
    NULL,
    NULL,
    0.0f,
};

static Sint16 goalring_anim_hit_plist_7[] = {
#include "assets/goalring_anim_hit_plist_7.inc"
};

static Sint32 goalring_anim_hit_vlist_7[] = {
#include "assets/goalring_anim_hit_vlist_7.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_7 = {
    goalring_anim_hit_vlist_7,
    goalring_anim_hit_plist_7,
    {0.268491f, -0.862628f, 0.42467f},
    1.250255f,
};

static NJS_CNK_OBJECT goalring_anim_hit_object_7 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_hit_model_7,
    {0.441818f, 0.031819f, 0.0f},
    {0x1C71, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_hit_object_8,
    0.0f,
};

static Sint16 goalring_anim_hit_plist_6[] = {
#include "assets/goalring_anim_hit_plist_6.inc"
};

static Sint32 goalring_anim_hit_vlist_6[] = {
#include "assets/goalring_anim_hit_vlist_6.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_6 = {
    goalring_anim_hit_vlist_6,
    goalring_anim_hit_plist_6,
    {0.268491f, -0.862628f, -0.42467f},
    1.250255f,
};

static NJS_CNK_OBJECT goalring_anim_hit_object_6 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_hit_model_6,
    {-0.441818f, 0.031819f, 0.0f},
    {-0x1C71, -0x8000, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_hit_object_7,
    0.0f,
};

static Sint16 goalring_anim_hit_plist_5[] = {
#include "assets/goalring_anim_hit_plist_5.inc"
};

static Sint32 goalring_anim_hit_vlist_5[] = {
#include "assets/goalring_anim_hit_vlist_5.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_5 = {
    goalring_anim_hit_vlist_5,
    goalring_anim_hit_plist_5,
    {0.0f, -0.411845f, -0.185203f},
    0.587591f,
};

static NJS_CNK_OBJECT goalring_anim_hit_object_5 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_hit_model_5,
    {0.0f, -0.015406f, -1.009871f},
    {0x71C, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_hit_object_6,
    0.0f,
};

static Sint16 goalring_anim_hit_plist_4[] = {
#include "assets/goalring_anim_hit_plist_4.inc"
};

static Sint32 goalring_anim_hit_vlist_4[] = {
#include "assets/goalring_anim_hit_vlist_4.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_4 = {
    goalring_anim_hit_vlist_4,
    goalring_anim_hit_plist_4,
    {0.300701f, -0.852327f, 0.111707f},
    1.014864f,
};

static NJS_CNK_OBJECT goalring_anim_hit_object_4 = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_hit_model_4,
    {0.661664f, 0.811234f, 0.16f},
    {0x38E, 0, 0xE38},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_hit_object_5,
    0.0f,
};

static Sint16 goalring_anim_hit_plist_3[] = {
#include "assets/goalring_anim_hit_plist_3.inc"
};

static Sint32 goalring_anim_hit_vlist_3[] = {
#include "assets/goalring_anim_hit_vlist_3.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_3 = {
    goalring_anim_hit_vlist_3,
    goalring_anim_hit_plist_3,
    {-0.62439f, 0.036828f, -0.579342f},
    1.078913f,
};

static NJS_CNK_OBJECT goalring_anim_hit_object_3 = {
    NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_hit_model_3,
    {-0.378701f, 0.648003f, -0.56423f},
    {0, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_hit_object_4,
    0.0f,
};

static Sint16 goalring_anim_hit_plist_2[] = {
#include "assets/goalring_anim_hit_plist_2.inc"
};

static Sint32 goalring_anim_hit_vlist_2[] = {
#include "assets/goalring_anim_hit_vlist_2.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_2 = {
    goalring_anim_hit_vlist_2,
    goalring_anim_hit_plist_2,
    {0.62439f, 0.036828f, -0.579342f},
    1.078913f,
};

static NJS_CNK_OBJECT goalring_anim_hit_object_2 = {
    NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_hit_model_2,
    {0.378701f, 0.648003f, -0.56423f},
    {0, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_hit_object_3,
    0.0f,
};

static Sint16 goalring_anim_hit_plist_1[] = {
#include "assets/goalring_anim_hit_plist_1.inc"
};

static Sint32 goalring_anim_hit_vlist_1[] = {
#include "assets/goalring_anim_hit_vlist_1.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_1 = {
    goalring_anim_hit_vlist_1,
    goalring_anim_hit_plist_1,
    {-4e-6f, 1.729028f, -0.018082f},
    2.545652f,
};

static NJS_CNK_OBJECT goalring_anim_hit_center_object = {
    NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK,
    &goalring_anim_hit_model_1,
    {-0.0f, 0.724436f, 1e-6f},
    {0x71C, 0, 0},
    {1.0f, 1.0f, 1.0f},
    NULL,
    &goalring_anim_hit_object_2,
    0.0f,
};

static Sint16 goalring_anim_hit_plist_0[] = {
#include "assets/goalring_anim_hit_plist_0.inc"
};

static Sint32 goalring_anim_hit_vlist_0[] = {
#include "assets/goalring_anim_hit_vlist_0.inc"
};

static NJS_CNK_MODEL goalring_anim_hit_model_0 = {
    goalring_anim_hit_vlist_0,
    goalring_anim_hit_plist_0,
    {2.7e-5f, 0.178874f, 0.065301f},
    1.465567f,
};

static NJS_CNK_OBJECT goalring_anim_hit_object = {
    NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL,
    &goalring_anim_hit_model_0,
    {0.0f, 2.442472f, 0.0f},
    {0, 0, 0},
    {1.0f, 1.0f, 1.0f},
    &goalring_anim_hit_center_object,
    NULL,
    0.0f,
};

static NJS_MKEY_F goalring_anim_hit_pos_0[] = {
    {0, {0.0f, 2.442472f, 0.0f}},  {1, {0.0f, 3.463042f, 0.0f}},
    {2, {0.0f, 4.322634f, 0.0f}},  {3, {0.0f, 5.032173f, 0.0f}},
    {4, {0.0f, 5.602581f, 0.0f}},  {5, {0.0f, 6.044782f, 0.0f}},
    {6, {0.0f, 6.369701f, 0.0f}},  {7, {0.0f, 6.588261f, 0.0f}},
    {8, {0.0f, 6.711387f, 0.0f}},  {9, {0.0f, 6.75f, 0.0f}},
    {10, {0.0f, 6.721509f, 0.0f}}, {11, {0.0f, 6.624254f, 0.0f}},
    {12, {0.0f, 6.440562f, 0.0f}}, {13, {0.0f, 6.152761f, 0.0f}},
    {14, {0.0f, 5.743177f, 0.0f}}, {15, {0.0f, 5.19414f, 0.0f}},
    {16, {0.0f, 4.487977f, 0.0f}}, {17, {0.0f, 3.607013f, 0.0f}},
    {18, {0.0f, 2.53358f, 0.0f}},  {19, {0.0f, 1.25f, 0.0f}},
};

static NJS_MKEY_A goalring_anim_hit_ang_0[] = {
    {0, {0, 0, 0}},
    {19, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_hit_ang_1[] = {
    {0, {0x71C, 0, 0}},   {1, {0x1C, 0, 0}},    {2, {-0x71C, 0, 0}},
    {3, {-0x773, 0, 0}},  {4, {-0x798, 0, 0}},  {5, {-0x78F, 0, 0}},
    {6, {-0x75C, 0, 0}},  {7, {-0x703, 0, 0}},  {8, {-0x688, 0, 0}},
    {9, {-0x5F1, 0, 0}},  {10, {-0x541, 0, 0}}, {11, {-0x47C, 0, 0}},
    {12, {-0x3A6, 0, 0}}, {13, {-0x2C4, 0, 0}}, {14, {-0x1DA, 0, 0}},
    {15, {-0xED, 0, 0}},  {16, {0, 0, 0}},      {17, {0x29F, 0, 0}},
    {18, {0x686, 0, 0}},  {19, {0x888, 0, 0}},
};

static NJS_MKEY_A goalring_anim_hit_ang_2[] = {
    {0, {0, 0, 0}},       {1, {0, 0x471, 0}},   {2, {0, 0xE38, 0}},
    {3, {0, 0x17FF, 0}},  {4, {0, 0x1C71, 0}},  {5, {0, 0x196D, 0}},
    {6, {0, 0x1242, 0}},  {7, {0, 0x9C1, 0}},   {8, {0, 0x2BB, 0}},
    {9, {0, 0, 0}},       {10, {0, 0x38E, 0}},  {11, {0, 0xB85, 0}},
    {12, {0, 0x14E8, 0}}, {13, {0, 0x1CBA, 0}}, {14, {0, 0x1FFF, 0}},
    {15, {0, 0x1CAB, 0}}, {16, {0, 0x14BC, 0}}, {17, {0, 0xB43, 0}},
    {18, {0, 0x353, 0}},  {19, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_hit_ang_3[] = {
    {0, {0, 0, 0}},        {1, {0, -0x4FF, 0}},   {2, {0, -0xFFF, 0}},
    {3, {0, -0x1AFF, 0}},  {4, {0, -0x1FFF, 0}},  {5, {0, -0x1CAB, 0}},
    {6, {0, -0x14BC, 0}},  {7, {0, -0xB43, 0}},   {8, {0, -0x353, 0}},
    {9, {0, 0, 0}},        {10, {0, -0x353, 0}},  {11, {0, -0xB43, 0}},
    {12, {0, -0x14BC, 0}}, {13, {0, -0x1CAB, 0}}, {14, {0, -0x1FFF, 0}},
    {15, {0, -0x1CAB, 0}}, {16, {0, -0x14BC, 0}}, {17, {0, -0xB43, 0}},
    {18, {0, -0x353, 0}},  {19, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_hit_ang_4[] = {
    {0, {0x38E, 0, 0xE38}},     {1, {0x294, 0, 0xF70}},
    {2, {-0x8, 0, 0x12B5}},     {3, {-0x3D1, 0, 0x1770}},
    {4, {-0x84E, 0, 0x1D0C}},   {5, {-0xD06, 0, 0x22F3}},
    {6, {-0x1183, 0, 0x288F}},  {7, {-0x154C, 0, 0x2D4A}},
    {8, {-0x17E9, 0, 0x308E}},  {9, {-0x18E3, 0, 0x31C6}},
    {10, {-0x1817, 0, 0x30C8}}, {11, {-0x15EE, 0, 0x2E14}},
    {12, {-0x12BE, 0, 0x2A18}}, {13, {-0xEE0, 0, 0x2542}},
    {14, {-0xAAA, 0, 0x1FFF}},  {15, {-0x674, 0, 0x1ABC}},
    {16, {-0x296, 0, 0x15E6}},  {17, {0x98, 0, 0x11EB}},
    {18, {0x2C2, 0, 0xF37}},    {19, {0x38E, 0, 0xE38}},
};

static NJS_MKEY_A goalring_anim_hit_ang_5[] = {
    {0, {0x71C, 0, 0}},   {1, {0x111C, 0, 0}},  {2, {0x271C, 0, 0}},
    {3, {0x3D1C, 0, 0}},  {4, {0x471C, 0, 0}},  {5, {0x4074, 0, 0}},
    {6, {0x3095, 0, 0}},  {7, {0x1DA3, 0, 0}},  {8, {0xDC4, 0, 0}},
    {9, {0x71C, 0, 0}},   {10, {0xDC4, 0, 0}},  {11, {0x1DA3, 0, 0}},
    {12, {0x3095, 0, 0}}, {13, {0x4074, 0, 0}}, {14, {0x471C, 0, 0}},
    {15, {0x4074, 0, 0}}, {16, {0x3095, 0, 0}}, {17, {0x1DA3, 0, 0}},
    {18, {0xDC4, 0, 0}},  {19, {0x71C, 0, 0}},
};

static NJS_MKEY_A goalring_anim_hit_ang_6[] = {
    {0, {-0x1C71, -0x7FFF, 0}}, {1, {-0x1C37, -0x7FFF, 0}},
    {2, {-0x1B90, -0x7FFF, 0}}, {3, {-0x1A8A, -0x7FFF, 0}},
    {4, {-0x1931, -0x7FFF, 0}}, {5, {-0x1792, -0x7FFF, 0}},
    {6, {-0x15B9, -0x7FFF, 0}}, {7, {-0x13B4, -0x7FFF, 0}},
    {8, {-0x118F, -0x7FFF, 0}}, {9, {-0xF58, -0x7FFF, 0}},
    {10, {-0xD19, -0x7FFF, 0}}, {11, {-0xAE1, -0x7FFF, 0}},
    {12, {-0x8BC, -0x7FFF, 0}}, {13, {-0x6B7, -0x7FFF, 0}},
    {14, {-0x4DF, -0x7FFF, 0}}, {15, {-0x340, -0x7FFF, 0}},
    {16, {-0x1E7, -0x7FFF, 0}}, {17, {-0xE1, -0x7FFF, 0}},
    {18, {-0x3A, -0x7FFF, 0}},  {19, {0, -0x7FFF, 0}},
};

static NJS_MKEY_A goalring_anim_hit_ang_7[] = {
    {0, {0x1C71, 0, 0}}, {1, {0x1C37, 0, 0}}, {2, {0x1B90, 0, 0}},
    {3, {0x1A8A, 0, 0}}, {4, {0x1931, 0, 0}}, {5, {0x1792, 0, 0}},
    {6, {0x15B9, 0, 0}}, {7, {0x13B4, 0, 0}}, {8, {0x118F, 0, 0}},
    {9, {0xF58, 0, 0}},  {10, {0xD19, 0, 0}}, {11, {0xAE1, 0, 0}},
    {12, {0x8BC, 0, 0}}, {13, {0x6B7, 0, 0}}, {14, {0x4DF, 0, 0}},
    {15, {0x340, 0, 0}}, {16, {0x1E7, 0, 0}}, {17, {0xE1, 0, 0}},
    {18, {0x3A, 0, 0}},  {19, {0, 0, 0}},
};

static NJS_MKEY_A goalring_anim_hit_ang_8[] = {
    {0, {0x38E, -0x7FFF, -0xE38}},    {1, {0x449, -0x7FFF, -0xF70}},
    {2, {0x63F, -0x7FFF, -0x12B5}},   {3, {0x916, -0x7FFF, -0x1770}},
    {4, {0xC73, -0x7FFF, -0x1D0C}},   {5, {0xFFE, -0x7FFF, -0x22F3}},
    {6, {0x135B, -0x7FFF, -0x288F}},  {7, {0x1632, -0x7FFF, -0x2D4A}},
    {8, {0x1828, -0x7FFF, -0x308E}},  {9, {0x18E3, -0x7FFF, -0x31C6}},
    {10, {0x184A, -0x7FFF, -0x30C8}}, {11, {0x16AB, -0x7FFF, -0x2E14}},
    {12, {0x1447, -0x7FFF, -0x2A18}}, {13, {0x1161, -0x7FFF, -0x2542}},
    {14, {0xE38, -0x7FFF, -0x1FFF}},  {15, {0xB10, -0x7FFF, -0x1ABC}},
    {16, {0x829, -0x7FFF, -0x15E6}},  {17, {0x5C6, -0x7FFF, -0x11EB}},
    {18, {0x427, -0x7FFF, -0xF37}},   {19, {0x38E, -0x7FFF, -0xE38}},
};

static NJS_MDATA2 goalring_anim_hit_mdata[] = {
    {{goalring_anim_hit_pos_0, goalring_anim_hit_ang_0},
     {ARRAY_COUNT(goalring_anim_hit_pos_0),
      ARRAY_COUNT(goalring_anim_hit_ang_0)}},
    {{NULL, goalring_anim_hit_ang_1},
     {0, ARRAY_COUNT(goalring_anim_hit_ang_1)}},
    {{NULL, goalring_anim_hit_ang_2},
     {0, ARRAY_COUNT(goalring_anim_hit_ang_2)}},
    {{NULL, goalring_anim_hit_ang_3},
     {0, ARRAY_COUNT(goalring_anim_hit_ang_3)}},
    {{NULL, goalring_anim_hit_ang_4},
     {0, ARRAY_COUNT(goalring_anim_hit_ang_4)}},
    {{NULL, goalring_anim_hit_ang_5},
     {0, ARRAY_COUNT(goalring_anim_hit_ang_5)}},
    {{NULL, goalring_anim_hit_ang_6},
     {0, ARRAY_COUNT(goalring_anim_hit_ang_6)}},
    {{NULL, goalring_anim_hit_ang_7},
     {0, ARRAY_COUNT(goalring_anim_hit_ang_7)}},
    {{NULL, goalring_anim_hit_ang_8},
     {0, ARRAY_COUNT(goalring_anim_hit_ang_8)}},
};

static NJS_MOTION goalring_anim_hit_motion = {goalring_anim_hit_mdata, 20, 3,
                                              2};

// referenced by the stage's texture load list
NJS_TEXLIST *goalring_texlist_tbl[] = {
    &goalring_texlist,
    &goalring_text1_texlist,
    &goalring_text2_texlist,
    &goalring_anim_texlist,
    &goalring_anim_hit_texlist,
    &_rename_ring_texlist,
    NULL,
};

static CCL_INFO goalring_colli_info[] = {
    {0,
     0,
     0x70,
     0,
     0x8000,
     {0.0f, 0.0f, 0.0f},
     25.0f,
     0.0f,
     0.0f,
     0.0f,
     0,
     0,
     0},
    {0,
     0,
     0x70,
     0,
     0x8000,
     {0.0f, 0.0f, 0.0f},
     8.0f,
     0.0f,
     0.0f,
     0.0f,
     0,
     0,
     0},
};

static Angle goalring_rot_spd = 0x180;
static Angle goalring_rot_spd_hit = 0x350;
static Float goalring_ptcl_scl = 0.1f;
static Float goalring_ptcl_spd = 0.1f;
static Float goalring_ptcl_spd_y = 0.03f;
static Uint32 goalring_blink_on = 25;
static Uint32 goalring_blink_cycle = 40;
static Float goalring_rise_spd = 0.2f;
static Float goalring_anim_spd = 0.12f;
static Float goalring_anim_spd_hit = 0.33f;
static NJS_POINT3 goalring_ptcl_pos0 = {0.7f, 0.55f, 1.5f};
static NJS_POINT3 goalring_ptcl_pos1 = {-0.7f, 0.55f, 1.5f};
static NJS_VECTOR goalring_ptcl_vec0 = {0.2f, 0.07f, 0.02f};
static NJS_VECTOR goalring_ptcl_vec1 = {-0.2f, 0.07f, 0.02f};
static NJS_POINT3 goalring_glow_pos = {0.0f, 5.0f, 0.0f};
static Angle goalring_glow_ang = 0xE000;
static NJS_VECTOR goalring_shadow_scl = {30.0f, 5.0f, 10.0f};
static NJS_VECTOR goalring_anim_shadow_scl = {2.3f, 1.0f, 2.5f};

static NJS_MATRIX *goalring_matrix_p;

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
      if (GetType(twp) == MD_GOALRING_1) {
        break;
      }
      return;
    }
    twp->ang.x = MD_GOALRING_2;
    twp->mode = 1;
    if (playertwp[0] == NULL || playertwp[0]->wtimer < 120) {
      return;
    }
    break;
  }

  if (lbl_801CC168._23 != 2 && GetType(twp) == MD_GOALRING_1) {
    task *ctp;

    if (tp->ctp == NULL) {
      ctp = CreateChildTask(IM_TWK, _rename_GoalRingChildExec, tp);
      if (ctp != NULL) {
        tp->disp_sort = ObjectGoalRingDispSortDummy;
        ctp->twp->scl.x = 2.0f;
        ctp->twp->smode = 1;
      }
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
  if (GetType(twp) != MD_GOALRING_1 &&
      (_rename_GetStageNum() == 57 || _rename_GetStageNum() == 6)) {
    tp->disp_shad = ObjectGoalRingDispShad;
  }
  twp->btimer = 0;
  if (GetType(twp) == MD_GOALRING_1) {
    CCL_Init(tp, &goalring_colli_info[1], 1, CID_OBJECT);
  } else {
    CCL_Init(tp, &goalring_colli_info[0], 1, CID_OBJECT);
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

  if (lbl_801CC168._7C % goalring_blink_cycle <
      goalring_blink_on) {
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
  } else if (GetType(twp) != MD_GOALRING_0) {
    if (GetType(twp) == MD_GOALRING_1) {
      if (twp->smode != 0) {
        twp->scl.y += goalring_anim_spd_hit;
        if (twp->scl.y >
            (Float)(goalring_anim_hit_motion.nbFrame - 1)) {
          twp->scl.y = 0.0f;
        }
        fn_8006AFFC(0x100E, tp, 1, 30, 30, &twp->pos);
      } else {
        twp->scl.y += goalring_anim_spd;
        if (twp->scl.y > (Float)(goalring_anim_motion.nbFrame - 1)) {
          twp->scl.y = 0.0f;
        }
        fn_8006AFFC(0x100D, tp, 1, 30 - twp->wtimer * 2, 30, &twp->pos);
      }
    } else {
      fn_8006AFFC(0x1012, twp, 1, (Sint16)-(twp->wtimer * 2), 30, &twp->pos);
    }
  } else {
    if (twp->smode != 0) {
      twp->ang.y += goalring_rot_spd_hit;
      twp->wtimer++;
      twp->pos.y += goalring_rise_spd;
    } else {
      twp->ang.y += goalring_rot_spd;
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
        vec.x *= goalring_ptcl_spd / len;
        vec.y = goalring_ptcl_spd_y;
        vec.z *= goalring_ptcl_spd / len;
        _rename_MakeParticle2(&pos, &vec, 1.5f);
      }
    }
    fn_8006AFFC(0x1012, twp, 1, (Sint16)-(twp->wtimer * 2), 30, &twp->pos);
  }

  if (twp->smode == 0 && (hit = CCL_IsHitPlayer(tp)) != NULL &&
      (pno = IsThisTaskPlayer(hit)) != -1) {
    if (lbl_801CC168._23 != 1 &&
        (lbl_801CC168._23 != 2 || GetType(twp) != MD_GOALRING_0)) {
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

  if (GetType(twp) != MD_GOALRING_0 && GetWork(tp)->mat_ok != 0 &&
      twp->smode == 0 && lbl_801CC168._37 == 0) {
    NJS_VECTOR vec;
    NJS_POINT3 pos;
    Float old;

    njPushMatrixEx();
    njSetMatrix(NULL, &GetWork(tp)->mat);
    njCalcPoint(NULL, &goalring_ptcl_pos0, &pos);
    njCalcVector(NULL, &goalring_ptcl_vec0, &vec);
    old = GetWork(tp)->posy;
    GetWork(tp)->posy = pos.y;
    dy = pos.y - old;
    if (old > pos.y) {
      GetWork(tp)->ptcl_num = 6;
    }
    if (GetWork(tp)->ptcl_num != 0 && lbl_801CC168._7C % 6 < 3) {
      GetWork(tp)->ptcl_num--;
      vec.y += 0.8f * dy;
      _rename_MakeParticle3(&pos, &vec, goalring_ptcl_scl);
      njCalcPoint(NULL, &goalring_ptcl_pos1, &pos);
      njCalcVector(NULL, &goalring_ptcl_vec1, &vec);
      vec.y += 0.5f * dy;
      _rename_MakeParticle3(&pos, &vec, goalring_ptcl_scl);
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
  if (goalring_matrix_p != NULL) {
    njGetMatrix(goalring_matrix_p);
  }
}

static void GoalRingObjectCallback(NJS_CNK_OBJECT *object) {
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  if (object == &goalring_anim_hit_center_object) {
    fn_80033620(&goalring_glow_pos, 3, 0x4000, 0, 1.5f, 1.5f, -1, 0,
                0.0f, 0.0f);
    GoalRingGetMatrix();
  }
  if (object == &goalring_anim_center_object) {
    fn_80033620(&goalring_glow_pos, 3, 0x4000,
                goalring_glow_ang, 1.5f, 1.5f, -1, 0, 0.0f, 0.0f);
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
  if (GetType(twp) == MD_GOALRING_1) {
    fn_8012297C(1);
    if (twp->smode != 0) {
      njSetTexture(&goalring_anim_hit_texlist);
      fn_8011E1EC(&goalring_anim_hit_object,
                  &goalring_anim_hit_motion, twp->scl.y);
    } else {
      njSetTexture(&goalring_anim_texlist);
      fn_8011E1EC(&goalring_anim_object, &goalring_anim_motion,
                  twp->scl.y);
    }
    fn_8012297C(3);
  } else {
    njSetTexture(&goalring_texlist);
    if (twp->smode != 0) {
      Float scl = 1.0f - twp->wtimer / 60.0f;

      if (scl < 0.0f) {
        scl = 0.0f;
      }
      njScale(NULL, scl, 1.0f, scl);
    }
    njCnkCacheDrawModel(&goalring_model);
    switch (twp->btimer) {
    case 1:
      njSetTexture(&goalring_text1_texlist);
      njCnkCacheDrawModel(&goalring_text1_model);
      break;
    case 2:
      njSetTexture(&goalring_text2_texlist);
      njCnkCacheDrawModel(&goalring_text2_model);
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

  if (GetType(twp) == MD_GOALRING_1) {
    ALIGNED_MATRIX(m);
    STACK_PAD_VAR(4);

    njGetMatrix(m);
    njPushMatrixEx();
    njTranslate(NULL, twp->pos.x, 0.3f + twp->scl.z, twp->pos.z);
    njRotateY(NULL, twp->ang.y);
    njTranslate(NULL, 0.0f, 0.0f, 0.8f);
    fn_8011610C(&goalring_anim_shadow_scl);
    _rename_RingDrawShadowModel();
    njPopMatrixEx();
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    if (DisableObjectFog) {
      njDisableFog();
      gjSetFog();
    }
    goalring_matrix_p = &GetWork(tp)->mat;
    fn_8011C3A0(GoalRingObjectCallback);
    if (twp->smode != 0) {
      njSetTexture(&goalring_anim_hit_texlist);
      fn_8011E1EC(&goalring_anim_hit_object,
                  &goalring_anim_hit_motion, twp->scl.y);
    } else {
      njSetTexture(&goalring_anim_texlist);
      fn_8011E1EC(&goalring_anim_object, &goalring_anim_motion,
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
    goalring_matrix_p = NULL;
    if (DisableObjectFog) {
      njEnableFog();
      gjSetFog();
    }
    njPopMatrixEx();
  } else if (tp->disp_shad == NULL) {
    njPushMatrixEx();
    njTranslate(NULL, twp->pos.x, 0.8f + twp->scl.z, twp->pos.z);
    njRotateY(NULL, twp->ang.y);
    fn_8011610C(&goalring_shadow_scl);
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
  if (GetType(twp) == MD_GOALRING_1) {
    fn_8012297C(1);
    if (twp->smode != 0) {
      njSetTexture(&goalring_anim_hit_texlist);
      fn_8011E1EC(&goalring_anim_hit_object,
                  &goalring_anim_hit_motion, twp->scl.y);
    } else {
      njSetTexture(&goalring_anim_texlist);
      fn_8011E1EC(&goalring_anim_object, &goalring_anim_motion,
                  twp->scl.y);
    }
    fn_8012297C(3);
  } else {
    njSetTexture(&goalring_texlist);
    if (twp->smode != 0) {
      Float scl = 1.0f - twp->wtimer / 60.0f;

      if (scl < 0.0f) {
        scl = 0.0f;
      }
      njScale(NULL, scl, 1.0f, scl);
    }
    njCnkCacheDrawModel(&goalring_model);
    switch (twp->btimer) {
    case 1:
      njSetTexture(&goalring_text1_texlist);
      njCnkCacheDrawModel(&goalring_text1_model);
      break;
    case 2:
      njSetTexture(&goalring_text2_texlist);
      njCnkCacheDrawModel(&goalring_text2_model);
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
