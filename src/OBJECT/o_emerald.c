#include "OBJECT/o_emerald.h"

#include "CCL.h"
#include "EFFECT/ef_kiran.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njcollision.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"

extern Sint32 _rename_GetStageNum(void);
extern void _rename_SetConditionFlag(task *tp, Sint32 flag);
extern void _rename_CnkDrawModelColor(NJS_CNK_MODEL *model, Uint32 color);
extern void fn_800068E4(colliwk *cwp);
extern void fn_80014650(NJS_POINT3 *pos, Angle3 *ang, NJS_VECTOR *scl, Sint32);
extern void fn_800155E0(NJS_POINT3 *p1, NJS_POINT3 *p2);
extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_80024CB8(Sint32);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void fn_80031EC8(task *tp, NJS_POINT3 *pos, void (*disp)(task *tp),
                        Float range);
extern BOOL fn_80032428(void);
extern Uint32 fn_800334B0(Uint32 color1, Uint32 color2, Float ratio);
extern Sint32 fn_80065388(task *tp);
extern Sint32 fn_8008C8F8(Sint32 kind, Sint32 id, NJS_POINT3 *pos);
extern void fn_8008C988(Sint32 kind, Sint32 id, Sint32 pno, NJS_POINT3 *pos);
extern void fn_8008CEB0(Sint32 kind, Sint32 id, NJS_POINT3 *pos);
extern void fn_80114058(Sint32 loc, const char *s); // njPrintC
extern void fn_80114168(Sint32 loc, Sint32 val, Sint32 digit); // njPrintD
extern void fn_8011C754(Sint32);
extern void fn_801218C8(Sint32, Sint32);
extern void *syCalloc(size_t num, size_t size);
extern Float atan2f(Float y, Float x);
extern int rand(void);
extern int sprintf(char *s, const char *format, ...);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);

extern NJS_TEXLIST _rename_ring_texlist;
extern NJS_TEXLIST _rename_kiran_texlist;
extern Sint8 lbl_803AD926;

// ^ extern
// v in this file

static void NormalInit(task *tp);
static void NormalEditDisp(task *tp);
static void NormalExec(task *tp);
static void NormalDisp(task *tp);
static void NormalDest(task *tp);
static void UndergroundInit(task *tp);
static void UndergroundEditDisp(task *tp);
static void UndergroundExec(task *tp);
static void UndergroundDisp(task *tp);
static void UndergroundDest(task *tp);
static void PathInit(task *tp);
static void PathEditDisp(task *tp);
static void PathEditDest(task *tp);
static void PathExec(task *tp);
static void PathDisp(task *tp);
static void PathDest(task *tp);
static void InEnemyInit(task *tp);
static void InEnemyExec(task *tp);
static void InEnemyDisp(task *tp);
static void InEnemyDest(task *tp);
static void FinalDisp(task *tp);
static void DrawEmeraldFlash(Sint32 no);
static void EmeraldGetSignKiran(task *tp);
static void EmeraldGetSign(task *tp);
static void EmeraldGetSignDisp(task *tp);

// emerald kinds, the second byte of the set file's ang.x
enum {
  KIND_B_NORMAL,
  KIND_C_NORMAL,
  KIND_B_HIDDEN,
  KIND_C_HIDDEN,
  KIND_A_UNDERGND,
  KIND_B_UNDERGND,
  KIND_A_2P_UNDGND,
  KIND_A_PATHMOVE,
  KIND_A_1P_TECH,
  KIND_FINAL,
  KIND_INENEMY,
  KIND_MAX,
};

enum {
  SMD_INIT,
  SMD_CHECK,
  SMD_ALIVE,
  SMD_TAKEN,
};

typedef struct {
  /* 0x00 */ Sint32 kind;
  /* 0x04 */ char *name;
  /* 0x08 */ void (*init)(task *tp);
  /* 0x0C */ void (*edit_disp)(task *tp);
  /* 0x10 */ void (*edit_dest)(task *tp);
  /* 0x14 */ void (*exec)(task *tp);
  /* 0x18 */ void (*disp)(task *tp);
  /* 0x1C */ void (*dest)(task *tp);
  /* 0x20 */ Sint32 model;
} EMERALD_INFO;

typedef struct {
  /* 0x00 */ Sint32 flag;
  /* 0x04 */ NJS_CNK_MODEL *model[3];
  /* 0x10 */ NJS_CNK_MODEL *flash_model[3];
  /* 0x1C */ NJS_TEXLIST *texlist[3];
  /* 0x28 */ Angle rot_spd;
  /* 0x2C */ Float float_h;
  /* 0x30 */ Angle float_spd;
} EMERALD_PARAM;

typedef struct {
  /* 0x00 */ Sint16 flag;
  /* 0x02 */ Sint16 num;
  /* 0x04 */ Float unk_04;
  /* 0x08 */ NJS_POINT3 *points;
} EMERALD_PATH;

// allocated into tp->awp by the path mover
typedef struct {
  /* 0x00 */ NJS_POINT3 pos;
  /* 0x0C */ NJS_POINT3 center;
} pathwk;

#define GetKind(twp) (((twp)->ang.x >> 8 & 0xFF) % KIND_MAX)
#define GetId(twp) ((twp)->ang.x & 0xFF)
#define GetPath(twp) (emerald_path_tbl[(Uint32)(twp)->scl.x % emerald_path_num])
#define GetPathWork(tp) ((pathwk *)(tp)->awp)
// the path mover keeps its point index and ratio in the unused work pointers
#define PathIndex(tp) (*(Sint32 *)&(tp)->fwp)
#define PathRatio(tp) (*(Float *)&(tp)->mwp)

#define RadAng(n) ((Angle)(10430.38043493439 * (n)))

static NJS_TEXNAME emerald_texname_0[] = {
    {"sikake_09_32"},
};

static NJS_TEXLIST emerald_texlist_0 = {
    emerald_texname_0,
    ARYLEN(emerald_texname_0),
};

static NJS_TEXNAME emerald_texname_1[] = {
    {"sikake_09_32"},
};

static NJS_TEXLIST emerald_texlist_1 = {
    emerald_texname_1,
    ARYLEN(emerald_texname_1),
};

static NJS_TEXNAME emerald_texname_2[] = {
    {"sikake_09_32"},
};

static NJS_TEXLIST emerald_texlist_2 = {
    emerald_texname_2,
    ARYLEN(emerald_texname_2),
};

static Sint16 emerald_plist[] = {
#include "assets/emerald_plist.inc"
};

static Sint32 emerald_vlist[] = {
#include "assets/emerald_vlist.inc"
};

static NJS_CNK_MODEL emerald_model = {
    emerald_vlist,
    emerald_plist,
    {0.0f, 0.023815f, -0.0f},
    3.880924f,
};

static Sint16 emerald_shine_plist_0[] = {
#include "assets/emerald_shine_plist_0.inc"
};

static Sint32 emerald_shine_vlist_0[] = {
#include "assets/emerald_shine_vlist_0.inc"
};

static NJS_CNK_MODEL emerald_shine_model_0 = {
    emerald_shine_vlist_0,
    emerald_shine_plist_0,
    {0.0f, 0.024048f, -0.0f},
    3.918957f,
};

static Sint16 emerald_shine_plist_1[] = {
#include "assets/emerald_shine_plist_1.inc"
};

static Sint32 emerald_shine_vlist_1[] = {
#include "assets/emerald_shine_vlist_1.inc"
};

static NJS_CNK_MODEL emerald_shine_model_1 = {
    emerald_shine_vlist_1,
    emerald_shine_plist_1,
    {0.0f, 0.024048f, 0.301183f},
    3.930513f,
};

static Sint16 emerald_shine_plist_2[] = {
#include "assets/emerald_shine_plist_2.inc"
};

static Sint32 emerald_shine_vlist_2[] = {
#include "assets/emerald_shine_vlist_2.inc"
};

static NJS_CNK_MODEL emerald_shine_model_2 = {
    emerald_shine_vlist_2,
    emerald_shine_plist_2,
    {-0.301183f, 0.024048f, -0.0f},
    3.930513f,
};

// the stage's texture load list points here
NJS_TEXLIST *emerald_texlist_tbl[] = {
    &emerald_texlist_0,     &emerald_texlist_1,
    &emerald_texlist_2,     &_rename_ring_texlist,
    &_rename_kiran_texlist, NULL,
};

// stages with moving emeralds fill these in
Sint32 emerald_path_num = -1;

static EMERALD_PARAM emerald_param = {
    1,
    {&emerald_model, &emerald_model, &emerald_model},
    {NULL, NULL, NULL},
    {&emerald_texlist_0, &emerald_texlist_1, &emerald_texlist_2},
    0x300,
    0.0f,
    0,
};

static EMERALD_INFO emerald_info[KIND_MAX] = {
    {KIND_B_NORMAL, "B NORMAL", NormalInit, NormalEditDisp, NULL, NormalExec,
     NormalDisp, NormalDest, 0},
    {KIND_C_NORMAL, "C NORMAL", NormalInit, NormalEditDisp, NULL, NormalExec,
     NormalDisp, NormalDest, 2},
    {KIND_B_HIDDEN, "B HIDDEN", NormalInit, NormalEditDisp, NULL, NormalExec,
     NormalDisp, NormalDest, 0},
    {KIND_C_HIDDEN, "C HIDDEN", NormalInit, NormalEditDisp, NULL, NormalExec,
     NormalDisp, NormalDest, 2},
    {KIND_A_UNDERGND, "A UNDERGND", NormalInit, NormalEditDisp, NULL,
     NormalExec, NormalDisp, NormalDest, 1},
    {KIND_B_UNDERGND, "B UNDERGND", NormalInit, NormalEditDisp, NULL,
     NormalExec, NormalDisp, NormalDest, 0},
    {KIND_A_2P_UNDGND, "A 2P UNDGND", UndergroundInit, UndergroundEditDisp,
     NULL, UndergroundExec, UndergroundDisp, UndergroundDest, 1},
    {KIND_A_PATHMOVE, "A PATHMOVE", PathInit, PathEditDisp, PathEditDest,
     PathExec, PathDisp, PathDest, 1},
    {KIND_A_1P_TECH, "A 1P TeCH", NormalInit, NormalEditDisp, NULL, NormalExec,
     NormalDisp, NormalDest, 1},
    {KIND_FINAL, "? FINAL", NormalInit, NormalEditDisp, NULL, NormalExec,
     FinalDisp, NormalDest, 1},
    {KIND_INENEMY, "? INENEMY", InEnemyInit, NormalEditDisp, NULL, InEnemyExec,
     InEnemyDisp, InEnemyDest, 1},
};

static Sint32 emerald_flash_period = 30;
static Sint32 emerald_flash_len = 5;

static NJS_CNK_MODEL *emerald_shine_models[] = {
    &emerald_shine_model_0,
    &emerald_shine_model_1,
    &emerald_shine_model_2,
};

EMERALD_PATH **emerald_path_tbl;

// the emerald with one of three highlight shells, picked by the camera angle
static void DrawEmeraldModel(void) {
  NJS_VECTOR v;
  Float f;
  Sint32 no;
  Uint32 color;

  v.x = 0.0f;
  v.y = 0.0f;
  v.z = 1.0f;
  njCalcVector(NULL, &v, &v);
  if (0.0f == v.x && 0.0f == v.z) {
    v.z = 1.0f;
  }
  v.y = 0.0f;
  njUnitVector(&v);

  f = 3.0f * (0.5f * (1.0f + v.x));
  no = (Sint32)f;
  if (no > 2) {
    no = 2;
  }
  if (no < 0) {
    no = 0;
  }
  f -= (Float)no;
  if (f < 0.0f) {
    f = 0.0f;
  }
  if (f > 1.0f) {
    f = 1.0f;
  }
  if (f > 0.3f) {
    f = (1.0f - f) / 0.7f;
  } else {
    f = f / 0.3f;
  }

  color = fn_800334B0(0x00FFFFFF, 0xE0FFFFFF, f);
  njCnkCacheDrawModel(&emerald_model);
  _rename_CnkDrawModelColor(emerald_shine_models[no], color);
}

// stages with moving emeralds hand their path list over before the set file is
// read
void EmeraldSetPathTable(EMERALD_PATH **tbl, Sint32 num) {
  emerald_path_tbl = tbl;
  emerald_path_num = num;
}

void EmeraldClearPathTable(void) {
  emerald_path_num = -1;
  emerald_path_tbl = NULL;
}

// underground pieces can only be dug up, except in the stages listed
static BOOL CheckPlayerCanGet(Sint32 kind, Sint32 pno) {
  switch (kind) {
  case KIND_A_UNDERGND:
  case KIND_B_UNDERGND:
  case KIND_A_2P_UNDGND:
    if (playertwp[pno] != NULL) {
      switch (playertwp[pno]->mode) {
      case 0x3E:
      case 0x42:
        return TRUE;
      }
      switch (_rename_GetStageNum()) {
      case 8:
      case 16:
      case 18:
      case 41:
      case 46:
        return TRUE;
      default:
        return FALSE;
      }
    }
  }
  return TRUE;
}

// an emerald placed by code instead of by the set file
task *CreateEmerald(NJS_POINT3 *pos, Sint32 kind, Sint32 id) {
  task *tp = CreateFundamentalTask(IM_TWK, LEV_1, ObjectEmerald);

  STACK_PAD_VAR(1);
  if (tp != NULL) {
    tp->twp->pos = *pos;
    tp->twp->ang.x = (Uint8)(Sint8)kind << 8 | (Uint8)(Sint8)id;
    tp->twp->mode = 3;
  }
  return tp;
}

// mode 5 hides the emerald and drops its collision, mode 4 shows it again
void EmeraldSetVisible(task *tp, Sint32 on) {
  if (on != 0) {
    tp->twp->mode = 4;
    if (tp->twp->smode == SMD_TAKEN && tp->twp->smode != SMD_ALIVE) {
      tp->twp->smode = SMD_CHECK;
    }
  } else {
    tp->twp->mode = 5;
  }
}

// an emerald carried by another object, drawn with the parent's angle
task *CreateEmeraldChild(task *ptp, NJS_POINT3 *pos, Sint32 kind, Sint32 id) {
  task *tp = CreateChildTask(IM_TWK, NormalExec, ptp);

  if (tp != NULL) {
    tp->twp->pos = *pos;
    tp->twp->ang.x = (Uint8)(Sint8)kind << 8 | (Uint8)(Sint8)id;
    tp->twp->mode = 4;
    if (fn_80032428()) {
      tp->disp = NormalDisp;
    } else {
      tp->disp_dely = NormalDisp;
    }
    tp->dest = NormalDest;
    NormalInit(tp);
  }
  return tp;
}

static void SetEmeraldLight(void) {
  fn_80024CB8(4);
}

static void SetEmeraldLightNoSpec(void) {
  fn_80024CB8(4);
}

static void ResetEmeraldLight(void) {
  fn_80024CB8(lbl_803AD926);
}

void ObjectEmerald(task *tp) {
  taskwk *twp = tp->twp;

  if (emerald_info[GetKind(twp)].disp == PathDisp && fn_80032428()) {
    tp->disp = emerald_info[GetKind(twp)].disp;
  } else {
    tp->disp_sort = emerald_info[GetKind(twp)].disp;
  }
  tp->dest = emerald_info[GetKind(twp)].dest;
  tp->exec = emerald_info[GetKind(twp)].exec;
  emerald_info[GetKind(twp)].init(tp);
}

static void DrawEmerald(NJS_POINT3 *pos, Angle3 *ang, Sint32 no) {
  Angle float_spd;

  njDisableFog();
  gjSetFog();
  if (emerald_param.flag & 1) {
    SetEmeraldLight();
  } else {
    SetEmeraldLightNoSpec();
  }

  njPushMatrixEx();
  float_spd = emerald_param.float_spd;
  if (float_spd != 0) {
    njTranslate(NULL, pos->x,
                pos->y + emerald_param.float_h *
                             njSin(lbl_801CC168._7C * float_spd),
                pos->z);
  } else {
    njTranslateEx(pos);
  }
  njRotateZ(NULL, ang->z);
  njRotateX(NULL, ang->x);
  njRotateY(NULL, ang->y);
  njSetTexture(emerald_param.texlist[no]);
  if (emerald_param.model[no] == &emerald_model) {
    DrawEmeraldModel();
  } else {
    njCnkCacheDrawModel(emerald_param.model[no]);
  }
  DrawEmeraldFlash(no);
  njPopMatrixEx();

  ResetEmeraldLight();
  njEnableFog();
  gjSetFog();
}

static void DrawEmeraldAngY(NJS_POINT3 *pos, Angle angy, Sint32 no) {
  Angle3 ang;

  ang.x = 0;
  ang.y = angy;
  ang.z = 0;
  DrawEmerald(pos, &ang, no);
}

// the set editor's emerald, drawn pulsing
static void DrawEmeraldEdit(NJS_POINT3 *pos, Angle3 *ang, Sint32 no) {
  Angle float_spd;

  njDisableFog();
  gjSetFog();

  njPushMatrixEx();
  float_spd = emerald_param.float_spd;
  if (float_spd != 0) {
    njTranslate(NULL, pos->x,
                pos->y + emerald_param.float_h *
                             njSin(lbl_801CC168._7C * float_spd),
                pos->z);
  } else {
    njTranslateEx(pos);
  }
  njRotateZ(NULL, ang->z);
  njRotateX(NULL, ang->x);
  njRotateY(NULL, ang->y);
  njScale(NULL, 1.5f + 0.5f * njCos(lbl_801CC168._7C * 0x300),
          4.0f + 2.0f * njSin(lbl_801CC168._7C * 0x300),
          1.5f + 0.5f * njCos(lbl_801CC168._7C * 0x300));
  if (emerald_param.flag & 1) {
    SetEmeraldLight();
  } else {
    SetEmeraldLightNoSpec();
  }
  njSetTexture(emerald_param.texlist[no]);
  if (emerald_param.model[no] == &emerald_model) {
    DrawEmeraldModel();
  } else {
    njCnkCacheDrawModel(emerald_param.model[no]);
  }
  DrawEmeraldFlash(no);
  njPopMatrixEx();

  ResetEmeraldLight();
  njEnableFog();
  gjSetFog();
}

static void DrawEmeraldEditAngY(NJS_POINT3 *pos, Angle angy, Sint32 no) {
  Angle3 ang;

  ang.x = 0;
  ang.y = angy;
  ang.z = 0;
  DrawEmeraldEdit(pos, &ang, no);
}

// the emerald that rises out of the player when one is collected
void CreateEmeraldGetSign(Sint32 pno, Sint32 no) {
  task *tp;

  STACK_PAD_VAR(2);
  // a player number outside the range hangs the game, as in the original
  while (pno < 0 || pno >= 2) {
  }

  tp = CreateFundamentalTask(IM_TWK, LEV_3, EmeraldGetSign);
  if (tp != NULL) {
    tp->twp->btimer = (Uint8)pno;
    tp->work.l = no;
  }
}

static Float emerald_kiran_spd = 2.5f;
static Float emerald_kiran_scl = 1.2f;

static void EmeraldGetSignKiran(task *tp) {
  taskwk *twp = tp->twp;
  NJS_VECTOR v;

  v.x = emerald_kiran_spd * (njRandom() - 0.5f);
  v.z = emerald_kiran_spd * (njRandom() - 0.5f);
  v.y = emerald_kiran_spd * (njRandom() - 0.5f);
  if (v.y < 0.0f) {
    v.y *= 0.3f;
  }
  CreateKiran(&twp->pos, &v, emerald_kiran_scl);
}

static void EmeraldGetSign(task *tp) {
  taskwk *twp = tp->twp;
  taskwk *ptwp;
  Sint32 i;

  if ((ptwp = playertwp[twp->btimer]) == NULL) {
    FreeTask(tp);
    return;
  }

  switch (twp->mode) {
  case 0:
    twp->mode = 1;
    twp->scl.x = 0.0f;
    twp->wtimer = 0;
    tp->disp_sort = EmeraldGetSignDisp;
    break;
  case 1:
    twp->pos = ptwp->cwp->info->center;
    twp->pos.y += 2.0f + (ptwp->cwp->info->a + 3.8f * twp->scl.x);
    twp->scl.x += 0.1f;
    twp->wtimer++;
    if (twp->wtimer > 120) {
      twp->ang.y += 0x13E9;
      if (twp->scl.x > 4.0f) {
        for (i = 0; i < 8; i++) {
          EmeraldGetSignKiran(tp);
        }
        FreeTask(tp);
      } else {
        EmeraldGetSignKiran(tp);
      }
    } else {
      twp->ang.y += 0x5B;
      if (twp->scl.x > 1.2f) {
        twp->scl.x = 1.2f;
      }
    }
    break;
  }
}

static void EmeraldGetSignDisp(task *tp) {
  taskwk *twp = tp->twp;
  Angle float_spd;

  njDisableFog();
  gjSetFog();
  if (emerald_param.flag & 1) {
    SetEmeraldLight();
  } else {
    SetEmeraldLightNoSpec();
  }

  njPushMatrixEx();
  float_spd = emerald_param.float_spd;
  if (float_spd != 0) {
    njTranslate(NULL, twp->pos.x,
                twp->pos.y + emerald_param.float_h *
                                 njSin(lbl_801CC168._7C * float_spd),
                twp->pos.z);
  } else {
    njTranslateEx(&twp->pos);
  }
  njRotateY(NULL, twp->ang.y);
  njSetTexture(emerald_param.texlist[tp->work.l]);
  njScale(NULL, twp->scl.x, twp->scl.x, twp->scl.x);
  if (emerald_param.model[tp->work.l] == &emerald_model) {
    DrawEmeraldModel();
  } else {
    njCnkCacheDrawModel(emerald_param.model[tp->work.l]);
  }
  DrawEmeraldFlash(tp->work.l);
  njPopMatrixEx();

  ResetEmeraldLight();
  njEnableFog();
  gjSetFog();
}

static CCL_INFO emerald_colli_info[1] = {
    {0, CI_FORM_SPHERE, (Sint8)0xF0, 0, 0x00788000, {0.0f, 0.0f, 0.0f}, 3.0f,
     0.0f, 0.0f, 0.0f, 0, 0, 0},
};

static NJS_POINT3 emerald_edit_origin = {0.0f, 0.0f, 0.0f};

static void DrawColliSphere(NJS_POINT3 *pos, Float r) {
  NJS_VECTOR scl;

  scl.x = r;
  scl.y = r;
  scl.z = r;
  fn_80014650(pos, NULL, &scl, 4);
}

static void NormalEditDisp(task *tp) {
  taskwk *twp = tp->twp;

  DrawEmeraldEditAngY(&twp->pos, tp->work.l, emerald_info[GetKind(twp)].model);
  DrawColliSphere(&twp->pos, 1.0f + twp->scl.y);
}

static void NormalInit(task *tp) {
  taskwk *twp = tp->twp;

  fn_8008CEB0(emerald_info[GetKind(twp)].kind, GetId(twp), &twp->pos);
  if (twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }

  CCL_Init(tp, emerald_colli_info, 1, 6);
  twp->cwp->info->a = 1.0f + twp->scl.y;
}

static void NormalDest(task *tp) {
  tp->mwp = NULL;
  tp->fwp = NULL;
  tp->awp = NULL;
}

// FALSE while something other than a player's body is touching the emerald
static BOOL CheckHitPlayer(taskwk *twp, Sint8 kind, Sint8 id) {
  task *tp;
  CCL_HIT_INFO *hit;

  CCL_ClearSearch();
  tp = twp->cwp->mytask;
  while ((hit = CCL_IsHitPlayerEx(tp)) != NULL) {
    if (hit->hit_num == 0) {
      return TRUE;
    }
  }

  if (_rename_GetStageNum() != 8) {
    return TRUE;
  }
  switch (kind) {
  case KIND_A_UNDERGND:
  case KIND_B_UNDERGND:
    return FALSE;
  case KIND_FINAL:
    if (id == 2) {
      return FALSE;
    }
    break;
  }
  return TRUE;
}

static void NormalExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 pno;

  if (twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }

  if (twp->smode <= SMD_CHECK) {
    if (twp->smode == SMD_CHECK) {
      switch (fn_8008C8F8(emerald_info[GetKind(twp)].kind, GetId(twp), NULL)) {
      case 0:
        twp->smode = SMD_TAKEN;
        break;
      case 1:
      case 2:
      case 3:
        twp->smode = SMD_ALIVE;
        break;
      }
    } else {
      twp->smode = SMD_CHECK;
    }
  }

  if (twp->smode == SMD_ALIVE && twp->mode != 5 && (twp->cwp->flag & 1) &&
      !(twp->cwp->hit_num != 0 && _rename_GetStageNum() == 26 &&
        lbl_801CC168.TWO_PLAYER == 0 &&
        emerald_info[GetKind(twp)].kind == KIND_FINAL) &&
      CheckHitPlayer(twp, emerald_info[GetKind(twp)].kind, GetId(twp)) &&
      (twp->cwp->hit_cwp->id == 0 || twp->cwp->hit_cwp->id == 1) &&
      CheckPlayerCanGet(emerald_info[GetKind(twp)].kind,
                        IsThisTaskPlayer(twp->cwp->hit_cwp->mytask))) {
    pno = IsThisTaskPlayer(twp->cwp->hit_cwp->mytask);
    fn_8008C988(emerald_info[GetKind(twp)].kind, GetId(twp), pno, &twp->pos);
    if (tp->ocp != NULL) {
      DeadOut(tp);
      return;
    }
    FreeTask(tp);
    return;
  }

  if ((twp->mode == 4 || twp->mode == 5) && twp->smode >= SMD_ALIVE) {
    fn_8008C8F8(emerald_info[GetKind(twp)].kind, GetId(twp), &twp->pos);
  }
  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    CCL_Entry(tp);
    tp->work.l += emerald_param.rot_spd;
  }
}

// unreferenced; reconstructed from the .rodata order, stripped by the linker
static Float GetRandomRatio(void) {
  return 0.000030517578f * (Float)rand();
}

static void NormalDispLate(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 pos; // unused
  Angle3 ang;
  Angle3 *pang;

  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    if (twp->mode == 4 && tp->ptp != NULL && twp->btimer == 0xFF) {
      pang = &tp->ptp->twp->ang;
      ang.x = pang->x;
      ang.y = tp->work.l;
      ang.z = pang->z;
      DrawEmerald(&twp->pos, &ang, emerald_info[GetKind(twp)].model);
    } else {
      DrawEmeraldAngY(&twp->pos, tp->work.l, emerald_info[GetKind(twp)].model);
    }
  }
}

static void NormalDisp(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 pos; // unused
  Angle3 ang;
  Angle3 *pang;

  if (tp->disp != NULL && fn_80032428()) {
    fn_80031EC8(tp, &twp->pos, NormalDispLate, 20.0f);
    return;
  }

  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    if (twp->mode == 4 && tp->ptp != NULL && twp->btimer == 0xFF) {
      pang = &tp->ptp->twp->ang;
      ang.x = pang->x;
      ang.y = tp->work.l;
      ang.z = pang->z;
      DrawEmerald(&twp->pos, &ang, emerald_info[GetKind(twp)].model);
    } else {
      DrawEmeraldAngY(&twp->pos, tp->work.l, emerald_info[GetKind(twp)].model);
    }
  }
}

static Uint32 GetRandomSeed(void) {
  Uint32 seed = (Uint32)(256.0f * (0.000030517578f * (Float)rand()));

  if (seed > 0xFF) {
    seed = 0xFF;
  }
  return seed;
}

// a point inside the set file's box, fixed by the seed kept in the condition
static void CalcRandomPos(NJS_VECTOR *scl, NJS_POINT3 *pos, Angle3 *ang,
                          NJS_POINT3 *out, Uint32 seed) {
  Float h;
  NJS_VECTOR v;
  Angle a = (Angle)(182.04445f * (Float)(Sint32)((seed & 0xF) << 12));
  Float r = (Float)((seed & 0xF0) >> 4) / 15.0f;

  v.x = scl->x * (r * njCos(a));
  h = (Float)((seed & 0x2C) >> 2) / 15.0f;
  v.y = 2.0f * (h - 0.5f) * scl->y;
  v.z = scl->z * (r * njSin(a));

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(pos);
  njRotateY(NULL, ang->y);
  njRotateZ(NULL, ang->z);
  njCalcPoint(NULL, &v, out);
  njPopMatrixEx();
}

static void UndergroundEditDisp(task *tp) {
  taskwk *twp = tp->twp;
  NJS_VECTOR scl;
  Angle3 ang;
  NJS_POINT3 pos;

  fn_80114058(NJM_LOCATION(17, 17), "<- ANGLE Y");
  fn_80114058(NJM_LOCATION(17, 18), "<- ANGLE Z");
  fn_80114058(NJM_LOCATION(17, 19), "<- SCLX");
  fn_80114058(NJM_LOCATION(17, 21), "<- SCLZ");

  scl.x = twp->scl.x;
  scl.y = 1.0f;
  scl.z = twp->scl.z;
  ang.x = 0;
  ang.y = twp->ang.y;
  ang.z = twp->ang.z;

  njPushMatrixEx();
  njTranslateEx(&emerald_edit_origin);
  njRotateY(NULL, ang.y);
  njRotateZ(NULL, ang.z);
  fn_80014650(&emerald_edit_origin, NULL, &scl, 2);
  njPopMatrixEx();

  CalcRandomPos(&scl, &twp->pos, &ang, &pos, GetRandomSeed());
  DrawEmeraldEditAngY(&pos, tp->work.l, emerald_info[GetKind(twp)].model);
  DrawColliSphere(&pos, 1.0f + twp->scl.y);
}

static void UndergroundInit(task *tp) {
  taskwk *twp = tp->twp;
  Uint32 seed = 0;
  NJS_VECTOR scl;
  NJS_POINT3 pos;
  Angle3 ang;

  if (tp->ocp != NULL) {
    seed = (Uint8)(Sint8)fn_80065388(tp);
  }
  if (seed == 0) {
    seed = GetRandomSeed();
  }
  if (tp->ocp != NULL) {
    _rename_SetConditionFlag(tp, (Sint8)seed);
  }

  scl.x = twp->scl.x;
  scl.y = 0.0f;
  scl.z = twp->scl.z;
  ang.x = 0;
  ang.y = twp->ang.y;
  ang.z = twp->ang.z;
  CalcRandomPos(&scl, &twp->pos, &ang, &pos, seed);
  twp->pos = pos;

  fn_8008CEB0(emerald_info[GetKind(twp)].kind, GetId(twp), &twp->pos);
  if (twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }

  CCL_Init(tp, emerald_colli_info, 1, 4);
  twp->cwp->info->a = 1.0f + twp->scl.y;
}

static void UndergroundDest(task *tp) {
  tp->mwp = NULL;
  tp->fwp = NULL;
  tp->awp = NULL;
}

static void UndergroundExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 pno;

  if (twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }

  if (twp->smode <= SMD_CHECK) {
    if (twp->smode == SMD_CHECK) {
      switch (fn_8008C8F8(emerald_info[GetKind(twp)].kind, GetId(twp), NULL)) {
      case 0:
        twp->smode = SMD_TAKEN;
        break;
      case 1:
      case 2:
      case 3:
        twp->smode = SMD_ALIVE;
        break;
      }
    } else {
      twp->smode = SMD_CHECK;
    }
  }

  if (twp->smode == SMD_ALIVE && twp->mode != 5 && (twp->cwp->flag & 1) &&
      !(twp->cwp->hit_num != 0 && _rename_GetStageNum() == 26 &&
        lbl_801CC168.TWO_PLAYER == 0 &&
        emerald_info[GetKind(twp)].kind == KIND_FINAL) &&
      (twp->cwp->hit_cwp->id == 0 || twp->cwp->hit_cwp->id == 1) &&
      CheckPlayerCanGet(emerald_info[GetKind(twp)].kind,
                        IsThisTaskPlayer(twp->cwp->hit_cwp->mytask))) {
    pno = IsThisTaskPlayer(twp->cwp->hit_cwp->mytask);
    fn_8008C988(emerald_info[GetKind(twp)].kind, GetId(twp), pno, &twp->pos);
    if (tp->ocp != NULL) {
      DeadOut(tp);
      return;
    }
    FreeTask(tp);
    return;
  }

  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    CCL_Entry(tp);
    tp->work.l += emerald_param.rot_spd;
  }
}

static void UndergroundDisp(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    DrawEmeraldAngY(&twp->pos, tp->work.l, emerald_info[GetKind(twp)].model);
  }
}

static CCL_INFO emerald_path_colli_info[1] = {
    {0, CI_FORM_SPHERE, (Sint8)0xF0, 0, 0x00788020, {0.0f, 0.0f, 0.0f}, 3.0f,
     0.0f, 0.0f, 0.0f, 0, 0, 0},
};

static Angle CalcAngY(NJS_POINT3 *from, NJS_POINT3 *to) {
  Float dx = from->x - to->x;
  Float dz = from->z - to->z;
  Float rad = atan2f(-dx, -dz);
  Angle ang = RadAng(rad);

  return ang;
}

// moves spd along the path from point *idx + *ratio; TRUE when it wrapped
static BOOL MovePath(taskwk *twp, Sint32 *idx, Float *ratio, Float spd,
                     NJS_POINT3 *pos, Angle *ang) {
  BOOL looped = FALSE;
  EMERALD_PATH *path = GetPath(twp);
  NJS_POINT3 *points;

  if (path->flag == 0) {
    points = path->points;
    if (spd > 0.0f) {
      NJS_POINT3 *pa;
      NJS_POINT3 *pb;
      Sint32 i;
      Sint32 i1;
      Sint32 i2;
      Sint16 a0;
      Sint16 a1;
      Float r;
      Float t;
      Float dist;
      Float done;
      Float rest;

      i = *idx;
      r = *ratio;
      while (TRUE) {

        i %= GetPath(twp)->num;
        i1 = (i + 1) % GetPath(twp)->num;
        i2 = (i + 2) % GetPath(twp)->num;
        dist = njDistanceP2P(&points[i], &points[i1]);
        done = dist * r;
        rest = dist - dist * r;
        if (rest < 0.0f) {
          rest = 0.0f;
        }
        if (spd > rest) {
          spd -= rest;
        } else {
          rest -= spd;
          t = rest / dist;
          r = 1.0f - t;
          if (r < 0.0f) {
            r = 0.0f;
            t = 1.0f;
          }
          *idx = i;
          *ratio = r;
          pa = &points[i];
          pb = &points[i1];
          pos->x = pa->x * t + pb->x * r;
          pos->y = pa->y * t + pb->y * r;
          pos->z = pa->z * t + pb->z * r;
          a0 = CalcAngY(pa, pb);
          a1 = CalcAngY(pb, &points[i2]);
          a1 -= a0;
          a1 = (Sint16)(a1 * r);
          *ang = a1 + a0;
          return looped;
        }
        r = 0.0f;
        i++;
        if (i >= GetPath(twp)->num) {
          looped = TRUE;
        }
      }
    } else {
      NJS_POINT3 *pa;
      NJS_POINT3 *pb;
      Sint32 i;
      Sint32 i1;
      Sint32 i2;
      Sint16 a0;
      Sint16 a1;
      Float r;
      Float t;
      Float dist;
      Float done;
      Float rest;

      i = *idx;
      r = *ratio;
      while (TRUE) {

        i %= GetPath(twp)->num;
        i1 = (i + 1) % GetPath(twp)->num;
        i2 = (i + 2) % GetPath(twp)->num;
        dist = njDistanceP2P(&points[i], &points[i1]);
        done = dist * r;
        rest = dist - dist * r;
        if (rest < 0.0f) {
          done = dist;
        }
        if (-spd > done) {
          spd += done;
        } else {
          done += spd;
          r = done / dist;
          t = 1.0f - r;
          if (t < 0.0f) {
            r = 1.0f;
            t = 0.0f;
          }
          *idx = i;
          *ratio = r;
          pa = &points[i];
          pb = &points[i1];
          pos->x = pa->x * t + pb->x * r;
          pos->y = pa->y * t + pb->y * r;
          pos->z = pa->z * t + pb->z * r;
          a0 = CalcAngY(pa, pb);
          a1 = CalcAngY(pb, &points[i2]);
          a1 -= a0;
          a1 = (Sint16)(a1 * r);
          *ang = a1 + a0;
          return looped;
        }
        r = 1.0f;
        i--;
        if (i < 0) {
          i = GetPath(twp)->num - 1;
          looped = TRUE;
        }
      }
    }
  }
  return FALSE;
}

// middle of the path's bounding box
static void CalcPathCenter(task *tp, NJS_POINT3 *center) {
  taskwk *twp = tp->twp;
  NJS_POINT3 *points = GetPath(twp)->points;
  Sint32 i;
  Float maxx, maxy, maxz;
  Float minx, miny, minz;
  Float x, y, z;

  for (i = 0; i < GetPath(twp)->num; i++) {
    if (i == 0) {
      maxx = minx = points[i].x;
      maxy = miny = points[i].y;
      maxz = minz = points[i].z;
    } else {
      x = points[i].x;
      y = points[i].y;
      z = points[i].z;
      if (maxx < x) {
        maxx = x;
      }
      if (maxy < y) {
        maxy = y;
      }
      if (maxz < z) {
        maxz = z;
      }
      if (minx > x) {
        minx = x;
      }
      if (miny > y) {
        miny = y;
      }
      if (minz > z) {
        minz = z;
      }
    }
  }

  center->x = 0.5f * (maxx + minx);
  center->y = 0.5f * (maxy + miny);
  center->z = 0.5f * (maxz + minz);
}

static void PathEditDisp(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 i;
  NJS_POINT3 *points;
  Angle ang;
  NJS_POINT3 pos;
  NJS_POINT3 center;
  char buf[0x100];

  if (emerald_path_num < 0) {
    return;
  }
  points = GetPath(twp)->points;

  fn_80114058(NJM_LOCATION(17, 17), "<- ANGLE Y");
  fn_80114058(NJM_LOCATION(17, 18), "<- ANGLE Z");
  fn_80114058(NJM_LOCATION(17, 19), "<- PATHID[   ]");
  fn_80114058(NJM_LOCATION(17, 21), "<- MOVE SPD(+1)");
  fn_80114168(NJM_LOCATION(28, 19), (Uint32)twp->scl.x % emerald_path_num, 2);

  MovePath(twp, &PathIndex(tp), &PathRatio(tp), 3.0f + twp->scl.z, &pos, &ang);
  CalcPathCenter(tp, &center);
  sprintf(buf, "CEN:(%5.3f,%5.3f,%5.3f)", center.x, center.y, center.z);
  fn_80114058(NJM_LOCATION(1, 24), buf);

  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njTranslate(NULL, -center.x, -center.y, -center.z);
  for (i = 0; i < GetPath(twp)->num - 1; i++) {
    fn_800155E0(&points[i], &points[i + 1]);
  }
  DrawEmeraldEditAngY(&pos, tp->work.l, emerald_info[GetKind(twp)].model);
  DrawColliSphere(&pos, 1.0f + twp->scl.y);
  njPopMatrixEx();
}

static void PathEditDest(task *tp) {
  tp->mwp = NULL;
  tp->fwp = NULL;
  tp->awp = NULL;
}

static void PathInit(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 pos;
  NJS_POINT3 center; // unused
  Angle ang;

  if (emerald_path_num < 0) {
    return;
  }
  tp->awp = syCalloc(1, sizeof(pathwk));
  if (tp->awp == NULL) {
    return;
  }

  if (tp->ocp != NULL) {
    PathIndex(tp) = (Uint8)fn_80065388(tp);
  }
  MovePath(twp, &PathIndex(tp), &PathRatio(tp), 3.0f + twp->scl.z, &pos, &ang);
  CalcPathCenter(tp, &GetPathWork(tp)->center);

  njPushMatrixEx();
  njUnitMatrix(NULL);
  njTranslateEx(&twp->pos);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  njTranslate(NULL, -GetPathWork(tp)->center.x, -GetPathWork(tp)->center.y,
              -GetPathWork(tp)->center.z);
  njCalcPoint(NULL, &pos, &GetPathWork(tp)->pos);
  njPopMatrixEx();

  fn_8008CEB0(emerald_info[GetKind(twp)].kind, GetId(twp),
              &GetPathWork(tp)->pos);
  if (twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }

  CCL_Init(tp, emerald_path_colli_info, 1, 6);
  twp->cwp->info->a = 1.0f + twp->scl.y;
  twp->cwp->info->center = GetPathWork(tp)->pos;
  fn_800068E4(twp->cwp);
}

static void PathDest(task *tp) {
  Uint8 flag;

  if (tp->ocp != NULL) {
    flag = PathIndex(tp);
    _rename_SetConditionFlag(tp, flag);
  }
  if (tp->awp != NULL) {
    syFree(tp->awp);
  }
  tp->awp = NULL;

  tp->mwp = NULL;
  tp->fwp = NULL;
  tp->awp = NULL;
}

static void PathExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 pno;
  NJS_POINT3 pos;
  Angle ang;

  if (twp->mode == 0 && CheckRangeOut(tp)) {
    return;
  }
  if (tp->awp == NULL) {
    return;
  }

  if (twp->smode != SMD_TAKEN) {
    MovePath(twp, &PathIndex(tp), &PathRatio(tp), 3.0f + twp->scl.z, &pos,
             &ang);

    njPushMatrixEx();
    njUnitMatrix(NULL);
    njTranslateEx(&twp->pos);
    njRotateY(NULL, twp->ang.y);
    njRotateZ(NULL, twp->ang.z);
    njTranslate(NULL, -GetPathWork(tp)->center.x, -GetPathWork(tp)->center.y,
                -GetPathWork(tp)->center.z);
    njCalcPoint(NULL, &pos, &GetPathWork(tp)->pos);
    njPopMatrixEx();
  }

  if (twp->smode <= SMD_CHECK) {
    if (twp->smode == SMD_CHECK) {
      switch (fn_8008C8F8(emerald_info[GetKind(twp)].kind, GetId(twp), NULL)) {
      case 0:
        twp->smode = SMD_TAKEN;
        break;
      case 1:
      case 2:
      case 3:
        twp->smode = SMD_ALIVE;
        break;
      }
    } else {
      twp->smode = SMD_CHECK;
    }
  }

  if (twp->smode == SMD_ALIVE && twp->mode != 5 && (twp->cwp->flag & 1) &&
      !(twp->cwp->hit_num != 0 && _rename_GetStageNum() == 26 &&
        lbl_801CC168.TWO_PLAYER == 0 &&
        emerald_info[GetKind(twp)].kind == KIND_FINAL) &&
      (twp->cwp->hit_cwp->id == 0 || twp->cwp->hit_cwp->id == 1) &&
      CheckPlayerCanGet(emerald_info[GetKind(twp)].kind,
                        IsThisTaskPlayer(twp->cwp->hit_cwp->mytask))) {
    pno = IsThisTaskPlayer(twp->cwp->hit_cwp->mytask);
    fn_8008C988(emerald_info[GetKind(twp)].kind, GetId(twp), pno,
                &GetPathWork(tp)->pos);
    if (tp->ocp != NULL) {
      DeadOut(tp);
      return;
    }
    FreeTask(tp);
    return;
  }

  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    twp->cwp->info->a = 1.0f + twp->scl.y;
    twp->cwp->info->center = GetPathWork(tp)->pos;
    fn_800068E4(twp->cwp);
    fn_8008C8F8(emerald_info[GetKind(twp)].kind, GetId(twp),
                &GetPathWork(tp)->pos);
    CCL_Entry(tp);
    tp->work.l += emerald_param.rot_spd;
  }
}

static void PathDispLate(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    DrawEmeraldAngY(&GetPathWork(tp)->pos, tp->work.l,
                    emerald_info[GetKind(twp)].model);
  }
}

static void PathDisp(task *tp) {
  taskwk *twp = tp->twp;

  if (tp->awp == NULL) {
    return;
  }
  if (tp->disp != NULL && fn_80032428()) {
    fn_80031EC8(tp, &GetPathWork(tp)->pos, PathDispLate, 20.0f);
    return;
  }

  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    DrawEmeraldAngY(&GetPathWork(tp)->pos, tp->work.l,
                    emerald_info[GetKind(twp)].model);
  }
}

// additive pulse over the emerald, emerald_flash_len frames up and the rest of
// emerald_flash_period down
static void DrawEmeraldFlash(Sint32 no) {
  Sint32 t;
  Float a;

  if (emerald_param.flash_model[no] == NULL) {
    return;
  }

  t = lbl_801CC168._7C % emerald_flash_period;
  if (t < emerald_flash_len) {
    a = (Float)t / (Float)(emerald_flash_len - 1);
  } else {
    a = (Float)(emerald_flash_period - t) /
        (Float)(emerald_flash_period - emerald_flash_len - 1);
  }

  fn_8002B304();
  fn_8002B35C();
  OffControl3D(0x220);
  OnControl3D(0x8810);
  fn_801218C8(0xFF, 0xB00);
  fn_8011C754(0x900);
  fn_800156FC(a, a, a, a);
  if (emerald_param.model[no] == &emerald_model) {
    DrawEmeraldModel();
  } else {
    njCnkCacheDrawModel(emerald_param.model[no]);
  }
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  fn_8002B348();
  fn_8002B2F8();
}

static void InEnemyInit(task *tp) {
  taskwk *twp = tp->twp;

  CCL_Init(tp, emerald_colli_info, 1, 6);
  twp->cwp->info->a = 7.0f;
  twp->cwp->colli_range = 7.0f;

  switch (fn_8008C8F8(KIND_INENEMY, GetId(twp), NULL)) {
  case 1:
    twp->smode = SMD_ALIVE;
    break;
  case 2:
    twp->smode = SMD_INIT;
    break;
  default:
    FreeTask(tp);
    break;
  }
}

static void InEnemyExec(task *tp) {
  taskwk *twp = tp->twp;
  task *hit_tp = CCL_IsHitPlayer(tp);

  if (hit_tp != NULL) {
    fn_8008C988(KIND_INENEMY, GetId(twp), IsThisTaskPlayer(hit_tp), &twp->pos);
    FreeTask(tp);
    return;
  }

  CCL_Entry(tp);
  tp->work.l += emerald_param.rot_spd;
}

static void InEnemyDisp(task *tp) {
  taskwk *twp = tp->twp;

  DrawEmeraldAngY(&twp->pos, tp->work.l, twp->smode);
}

static void InEnemyDest(task *tp) {
}

static void FinalDisp(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 no;
  NJS_POINT3 pos; // unused
  Angle3 ang;
  Angle3 *pang;

  switch (GetId(twp)) {
  case 0:
    no = 2;
    break;
  case 1:
    no = 0;
    break;
  case 2:
  default:
    no = 1;
    break;
  }

  if (twp->smode == SMD_ALIVE && twp->mode != 5) {
    if (twp->mode == 4 && tp->ptp != NULL && twp->btimer == 0xFF) {
      pang = &tp->ptp->twp->ang;
      ang.x = pang->x;
      ang.y = tp->work.l;
      ang.z = pang->z;
      DrawEmerald(&twp->pos, &ang, no);
    } else {
      DrawEmeraldAngY(&twp->pos, tp->work.l, no);
    }
  }
}
