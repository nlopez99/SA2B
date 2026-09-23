#include "OBJECT/o_levupdai.h"

#include "CCL.h"
#include "EFFECT/ef_particle.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/camera.h"
#include "samt/sonic/figure/ewalker.h"
#include "samt/sonic/figure/knuckles.h"
#include "samt/sonic/figure/sonic.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"

extern Sint32 _rename_GetPlayerCharacter(Sint32);
extern Uint8 fn_8001AE58(Sint32 id);
extern void fn_8001AE84(Sint32 id);
extern void fn_80011E4C(const char *name);
extern void fn_800120D4(void);
extern void fn_8001DD50(void);
extern void fn_8001DD64(void);
extern void fn_80022E24(Sint32);
extern void fn_80022E64(Sint32);
extern void fn_8005B990(void);
extern void fn_80062C4C(void);
extern void fn_80062C60(void);
extern void fn_800399BC(Sint8, Float, Float, Float);
extern void fn_80039CE0(Sint8, Angle, Angle, Angle);
extern void fn_800C3780(Sint32, Sint32);
extern void fn_800C382C(Sint32, Sint32, Sint32);
extern void fn_800C3A58(Sint32, Sint32);
extern Sint32 fn_800C3BDC(Sint32);
extern void fn_800E0118(Sint32, Sint32, Sint32, Sint32, task **);
extern void fn_8006AFFC(Sint32, void *, Sint32, Sint32, Sint32, NJS_POINT3 *);
extern void fn_80073028(NJS_CNK_MODEL *model, void *info, Uint32 frame);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void fn_801218C8(Sint32, Sint32);
extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_8012297C(Sint32);
extern void fn_8011E158(NJS_CNK_MODEL *model);
extern void fn_8011E17C(NJS_CNK_OBJECT *object);
// blends two colors
extern Uint32 fn_800334B0(Uint32 col0, Uint32 col1, Float ratio);
extern void fn_80033620(NJS_POINT3 *, Sint32, Sint32, Angle, Float, Float,
                        Sint32, Sint32, Float, Float);
extern void __njColorBlendingMode(Int, Int);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);

extern camcontwk lbl_8020C5E0; // cameraControlWork

extern void *_rename_CreateSmoke(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
extern void _rename_CnkDrawModelColor2(NJS_CNK_MODEL *model, Uint32 col);
extern Sint32 *_rename_GetLevUpItemMessage(void);

typedef struct levupiteminfo // sizeof=0x24
{
  /* 0x00 */ Sint32 character;
  /* 0x04 */ char *name;
  /* 0x08 */ Sint32 equipment;
  /* 0x0C */ Sint32 id;
  /* 0x10 */ Sint32 unk10;
  /* 0x14 */ task_exec disp;
  /* 0x18 */ task_exec exec;
  /* 0x1C */ NJS_CNK_OBJECT *object;
  /* 0x20 */ Sint32 unk20;
} levupiteminfo;

extern NJS_TEXLIST   _rename_levupdai_texlist;
extern NJS_CNK_MODEL _rename_levupdai_model;
extern NJS_TEXLIST   _rename_levupdai_top_texlist;
extern NJS_CNK_MODEL _rename_levupdai_top_model;
extern NJS_TEXLIST   _rename_levupdai_light_texlist;
extern NJS_CNK_MODEL _rename_levupdai_light_model;
extern Uint8         _rename_levupdai_light_uvinfo[0x24];
extern CCL_INFO      _rename_levupdai_colli_info[1];

extern NJS_TEXLIST   _rename_levupitem_melody_texlist;
extern NJS_TEXLIST   _rename_levupitem_sparkle_texlist;
extern particle_info _rename_melody_effect0_info;
extern particle_info _rename_melody_effect1_info;
extern particle_info _rename_melody_note_info[2];
extern levupiteminfo _rename_levupitem_info[28];

extern Float  _rename_levupitem_melody_scl;
extern Uint32 _rename_levupitem_melody_col;
extern Float  _rename_levupitem_melody_posy;
extern Float  _rename_levupitem_melody_ptcl_spd;
extern Float  _rename_levupitem_melody_effect0_scl;
extern Float  _rename_levupitem_melody_effect1_scl;
extern Float  _rename_levupitem_melody_note_scl;
extern Float  _rename_levupitem_melody_effect0_spd_y;
extern Uint32 _rename_levupitem_melody_effect0_cycle;
extern Uint32 _rename_levupitem_melody_effect1_cycle;
extern Uint32 _rename_levupitem_melody_note_cycle;
extern Float  _rename_levupitem_sparkle_scl;
extern Float  _rename_levupitem_sparkle_ptcl_scl;
extern Uint32 _rename_levupitem_sparkle_col;
extern Float  _rename_levupitem_sparkle_r;
extern Sint32 _rename_levupitem_sparkle_num;
extern Sint32 _rename_levupitem_sparkle_spd_x;
extern Sint32 _rename_levupitem_sparkle_spd_y;
extern Sint32 _rename_levupitem_sparkle_ofs_x;
extern Sint32 _rename_levupitem_sparkle_ofs_y;

// ^ extern
// v in this file

static void ObjectLevUpDaiDest(task *tp);
static void ObjectLevUpDaiExec(task *tp);
static void ObjectLevUpDaiDisp(task *tp);
static void ObjectLevUpDaiDispSort(task *tp);

enum {
  MD_WAIT,
  MD_GET,
  MD_END,
};

typedef struct levupdaiwk // sizeof=0x14
{
  /* 0x00 */ Sint32 unk0;
  /* 0x04 */ Sint32 unk4;
  /* 0x08 */ levupiteminfo *info;
  /* 0x0C */ NJS_TEXLIST *texlist; // of the player the item belongs to
  /* 0x10 */ task *msg_tp;
} levupdaiwk;

#define GetWork(task) ((levupdaiwk *)task->mwp)

#define MAX_SPARKLE 20

static particle *CreateMelodyEffect0(NJS_POINT3 *pos, NJS_VECTOR *spd,
                                     Float scl) {
  particle *p = fn_80032B78(&_rename_melody_effect0_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(360.0f * ParticleRandom());
    p->ang2 = ParticleDegAng(20.0f * ParticleRandom()) + 0x4000;
    p->ang_spd = ParticleDegAng(3.0f * ParticleRandom() - 1.5f);
  }
  return p;
}

static particle *CreateMelodyEffect1(NJS_POINT3 *pos, NJS_VECTOR *spd,
                                     Float scl) {
  particle *p = fn_80032B78(&_rename_melody_effect1_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(360.0f * ParticleRandom());
    p->ang2 = ParticleDegAng(20.0f * ParticleRandom()) + 0x4000;
    p->ang_spd = ParticleDegAng(3.0f * ParticleRandom() - 1.5f);
  }
  return p;
}

static particle *CreateMelodyNote(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                                  Sint32 kind) {
  particle *p = fn_80032B78(&_rename_melody_note_info[kind]);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(360.0f * ParticleRandom());
    p->ang2 = ParticleDegAng(20.0f * ParticleRandom()) + 0x4000;
    p->ang_spd = 0;
  }
  return p;
}

// no return for a live particle, as in the original
Bool MelodyEffectExec(particle_info *info, particle *p) {
  Float frame;
  Uint32 alpha;

  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  frame = p->frame + info->frame_spd;
  if ((Sint16)frame >= info->frame_num) {
    alpha = p->argb >> 24;
    if (alpha < 10 || p->scl < 0.0f) {
      return FALSE;
    }
    p->argb = (p->argb & 0x00FFFFFF) | ((alpha - 2) << 24);
  } else {
    p->frame = frame;
  }
}

void LevUpItemMelodyDisp(task *tp) {
  Float STACK_PAD[1];
  NJS_POINT3 pos;

  if (_rename_levupitem_melody_texlist.textures[0].texaddr == 0) {
    return;
  }

  pos.x = 0.0f;
  pos.y = _rename_levupitem_melody_posy + 1.5f * njSin(lbl_801CC168._7C << 8);
  pos.z = 0.0f;
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 10);
  njDisableFog();
  gjSetFog();
  njSetTexture(&_rename_levupitem_melody_texlist);
  fn_80033620(&pos, (lbl_801CC168._7C / 3) & 3, 0x4000, lbl_801CC168._7C << 7,
              _rename_levupitem_melody_scl, _rename_levupitem_melody_scl,
              _rename_levupitem_melody_col, 1, 0.0f, 0.0f);
  njEnableFog();
  gjSetFog();
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
}

static Uint32 MelodyNoteRandomColor(void) {
  Float r = ParticleRandom();
  Float g = ParticleRandom();
  Float b = ParticleRandom();
  Float a = 0.7f + 0.3f * ParticleRandom();
  Sint32 col;

  col = (Sint32)(255.0f * a);
  col = (col << 8) | (Sint32)(255.0f * r);
  col = (col << 8) | (Sint32)(255.0f * g);
  col = (col << 8) | (Sint32)(255.0f * b);
  return col;
}

void LevUpItemMelodyExec(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 pos;
  NJS_VECTOR spd;
  particle *p;
  Sint32 kind;
  STACK_PAD_VAR(1);

  if (_rename_levupitem_melody_texlist.textures[0].texaddr == 0) {
    return;
  }

  if (lbl_801CC168._7C % _rename_levupitem_melody_effect0_cycle == 0) {
    Float *pp;
    Float *ps;

    pos.x = twp->pos.x;
    pos.y = twp->pos.y + _rename_levupitem_melody_posy +
            1.5f * njSin(lbl_801CC168._7C << 8);
    pos.z = twp->pos.z;
    spd.x = _rename_levupitem_melody_ptcl_spd * (ParticleRandom() - 0.5f);
    spd.y = _rename_levupitem_melody_effect0_spd_y +
            0.5f * (_rename_levupitem_melody_ptcl_spd *
                    (ParticleRandom() - 0.2f));
    spd.z = _rename_levupitem_melody_ptcl_spd * (ParticleRandom() - 0.5f);
    // pos += spd, walked by pointer to match
    pp = &pos.x;
    ps = &spd.x;
    *pp++ += *ps++;
    *pp++ += *ps++;
    *pp += *ps;
    CreateMelodyEffect0(&pos, &spd, _rename_levupitem_melody_effect0_scl);
  }

  if (lbl_801CC168._7C % _rename_levupitem_melody_effect1_cycle == 0) {
    pos.x = twp->pos.x;
    pos.y = twp->pos.y + _rename_levupitem_melody_posy +
            3.0f * njSin(lbl_801CC168._7C << 7);
    pos.z = twp->pos.z;
    spd.x = _rename_levupitem_melody_ptcl_spd * (ParticleRandom() - 0.5f);
    spd.y = 0.01f + 0.1f * (_rename_levupitem_melody_ptcl_spd *
                            (ParticleRandom() - 0.5f));
    spd.z = _rename_levupitem_melody_ptcl_spd * (ParticleRandom() - 0.5f);
    CreateMelodyEffect1(&pos, &spd, _rename_levupitem_melody_effect1_scl);
  }

  if (lbl_801CC168._7C % _rename_levupitem_melody_note_cycle == 0) {
    kind = (Sint32)(100.0f * ParticleRandom()) % 2;
    pos.x = twp->pos.x;
    pos.y = twp->pos.y + _rename_levupitem_melody_posy +
            3.0f * njSin(lbl_801CC168._7C << 7);
    pos.z = twp->pos.z;
    spd.x = 0.35f * (ParticleRandom() - 0.5f);
    spd.y = 0.015f + 0.5f * (_rename_levupitem_melody_ptcl_spd *
                             (ParticleRandom() - 0.5f));
    spd.z = 0.35f * (ParticleRandom() - 0.5f);
    p = CreateMelodyNote(&pos, &spd, _rename_levupitem_melody_note_scl, kind);
    if (p != NULL) {
      p->argb = MelodyNoteRandomColor();
    }
  }
}

static Sint32 levupitem_sparkle_acc_x;
static Sint32 levupitem_sparkle_acc_y;

void LevUpItemSparkleDisp(task *tp) {
  NJS_POINT3 center;
  NJS_POINT3 v;
  NJS_POINT3 pos[MAX_SPARKLE];
  Uint32 col[MAX_SPARKLE];
  Sint32 i;
  Angle angx;
  Angle angy;
  Angle ang;

  if (_rename_levupitem_sparkle_texlist.textures[0].texaddr == 0) {
    return;
  }

  center.x = 0.0f;
  center.y = 20.0f + njSin(lbl_801CC168._7C * 0xE0);
  center.z = 0.0f;
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 10);
  njDisableFog();
  gjSetFog();
  njSetTexture(&_rename_levupitem_sparkle_texlist);
  fn_80033620(&center, (lbl_801CC168._7C / 3) & 7, 0x4000,
              lbl_801CC168._7C << 7, _rename_levupitem_sparkle_scl,
              _rename_levupitem_sparkle_scl, _rename_levupitem_sparkle_col, 1,
              0.0f, 0.0f);

  v.x = 0.0f;
  v.y = 0.0f;
  v.z = _rename_levupitem_sparkle_r;
  njPushMatrixEx();
  for (i = 0; i < _rename_levupitem_sparkle_num; i++) {
    ang = i * 0xCCC + lbl_801CC168._7C * 0x650;
    col[i] = fn_800334B0(
        0x50FFFFFF, 0xFFFFFFFF,
        0.5f * (1.0f + njSin(ang)));
    angx = _rename_levupitem_sparkle_ofs_x * i +
           lbl_801CC168._7C * (_rename_levupitem_sparkle_spd_x +
                               levupitem_sparkle_acc_x * i);
    angy = _rename_levupitem_sparkle_ofs_y * i +
           lbl_801CC168._7C * (_rename_levupitem_sparkle_spd_y +
                               levupitem_sparkle_acc_y * i);
    if (i & 1) {
      angx = -angx;
    }
    if (i & 2) {
      angy = -angy;
    }
    njUnitMatrix(NULL);
    njRotateX(NULL, angx);
    njRotateY(NULL, angy);
    njCalcPoint(NULL, &v, &pos[i]);
    pos[i].y += 20.0f;
  }
  njPopMatrixEx();

  for (i = 0; i < _rename_levupitem_sparkle_num; i++) {
    fn_80033620(&pos[i], ((lbl_801CC168._7C / 3 + i) & 7) + 8, 0x4000,
                lbl_801CC168._7C << 7, _rename_levupitem_sparkle_ptcl_scl,
                _rename_levupitem_sparkle_ptcl_scl, col[i], 1, 0.0f, 0.0f);
  }
  njEnableFog();
  gjSetFog();
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
}

void LevUpItemSparkleExec(task *tp) {
  if (_rename_levupitem_sparkle_texlist.textures[0].texaddr == 0) {
    return;
  }
}

void LevUpItemRubberUnitDisp(task *tp) {
  NJS_TEXLIST *texlist;

  texlist = GetWork(tp)->texlist;
  if (texlist == NULL) {
    return;
  }

  njTranslate(NULL, 0.0f, 0.5f * (1.0f + njSin(lbl_801CC168._7C * 0x1D0)),
              0.0f);
  njRotateY(NULL, lbl_801CC168._7C * 0x180);
  njSetTexture(texlist);
  fn_8011E17C(GetWork(tp)->info->object);
}

void LevUpItemRubberUnitExec(task *tp) {}

void LevUpItemShoesDisp(task *tp) {
  NJS_TEXLIST *texlist;

  texlist = GetWork(tp)->texlist;
  if (texlist == NULL) {
    return;
  }

  njTranslate(NULL, 0.0f, 1.0f + njSin(lbl_801CC168._7C * 0x120), 0.0f);
  njRotateY(NULL, lbl_801CC168._7C * 0x180);
  njSetTexture(texlist);
  fn_8011E17C(GetWork(tp)->info->object);
}

void LevUpItemShoesExec(task *tp) {}

void LevUpItemDisp(task *tp) {
  NJS_TEXLIST *texlist;
  NJS_CNK_OBJECT *object;
  NJS_CNK_OBJECT *child;

  texlist = GetWork(tp)->texlist;
  if (texlist == NULL) {
    return;
  }

  object = GetWork(tp)->info->object;
  njTranslate(NULL, 0.0f, 0.5f * (1.0f + njSin(lbl_801CC168._7C * 0x1D0)),
              0.0f);
  njRotateY(NULL, lbl_801CC168._7C * 0x180);
  njSetTexture(texlist);
  njTranslateEx(&object->pos);
  njRotateEx(&object->ang, 0);
  fn_8011E158(object->model);
  child = object->child;
  if (child == NULL) {
    return;
  }

  // the glowing part
  njPushMatrixEx();
  njTranslateEx(&child->pos);
  njRotateEx(&child->ang, 0);
  _rename_CnkDrawModelColor2(
      child->model,
      fn_800334B0(0x30FFFFFF, 0xFFFFFFFF,
                  0.5f * (1.0f + njSin(lbl_801CC168._7C * 0x500))));
  njPopMatrixEx();
}

void LevUpItemExec(task *tp) {}

static NJS_TEXLIST *LevUpItemGetTexlist(Sint32 pno, Sint32 character) {
  if (character == _rename_GetPlayerCharacter(pno)) {
    switch (character) {
    case PLNO_SONIC:
    case PLNO_SHADOW:
      return ((sonicwk *)playertp[pno]->awp)->tlist;
    case PLNO_TAILS_WALKER:
    case PLNO_EGG_WALKER:
      return ((walkerwk *)playertp[pno]->awp)->tlist;
    case PLNO_KNUCKLES:
    case PLNO_ROUGE:
      return ((knuckleswk *)playertp[pno]->awp)->tlist;
    }
  }
  return NULL;
}

void ObjectLevUpDai(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }

  tp->mwp = syCalloc(1, sizeof(levupdaiwk));
  if (tp->mwp == NULL) {
    return;
  }

  tp->disp = ObjectLevUpDaiDisp;
  tp->disp_sort = ObjectLevUpDaiDispSort;
  tp->exec = ObjectLevUpDaiExec;
  tp->dest = ObjectLevUpDaiDest;
  twp->mode = MD_WAIT;
  twp->smode = (Uint8)twp->scl.x % (Sint32)ARYLEN(_rename_levupitem_info);
  GetWork(tp)->info = &_rename_levupitem_info[twp->smode];
  GetWork(tp)->texlist = LevUpItemGetTexlist(0, GetWork(tp)->info->character);
  if (GetWork(tp)->texlist == NULL) {
    GetWork(tp)->texlist =
        LevUpItemGetTexlist(1, GetWork(tp)->info->character);
  }
  if (fn_8001AE58(GetWork(tp)->info->id)) {
    twp->mode = MD_END;
    twp->wtimer = 1000;
  }
  CCL_Init(tp, _rename_levupdai_colli_info,
           ARYLEN(_rename_levupdai_colli_info), CID_OBJECT);
}

static void ObjectLevUpDaiDest(task *tp) {
  if (GetWork(tp)->msg_tp != NULL) {
    DestroyTask(GetWork(tp)->msg_tp);
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static Sint32 levupdai_se_volume = -10;

static void ObjectLevUpDaiExec(task *tp) {
  taskwk *twp = tp->twp;
  task *hit_tp;
  Sint32 pno;
  Sint32 *msg;
  NJS_POINT3 pos;
  NJS_VECTOR spd;

  if (CheckRangeOut(tp)) {
    return;
  }

  if (GetWork(tp)->info->exec != NULL && twp->mode == MD_WAIT) {
    GetWork(tp)->info->exec(tp);
  }

  switch (twp->mode) {
  case MD_WAIT:
    hit_tp = CCL_IsHitPlayer(tp);
    if (hit_tp == NULL) {
      break;
    }
    if (hit_tp != playertp[0]) {
      pno = 1;
    } else {
      pno = 0;
    }
    fn_8001AE84(GetWork(tp)->info->id);
    playerpwp[pno]->equipment |= GetWork(tp)->info->equipment;
    twp->mode = MD_GET;
    twp->wtimer = 0;
    fn_8006AFFC(0x1015, twp, 1, (Sint16)levupdai_se_volume, 30, &twp->pos);
    msg = _rename_GetLevUpItemMessage();
    if (GetWork(tp)->msg_tp != NULL) {
      DestroyTask(GetWork(tp)->msg_tp);
    }
    if (msg != NULL) {
      msg += twp->smode;
      fn_800E0118(1, *msg, -1 - pno, *((Sint8 *)&lbl_801CC168 + 0x11),
                  &GetWork(tp)->msg_tp);
    }
    pos = twp->pos;
    pos.y += 10.0f;
    spd.x = 0.0f;
    spd.y = 0.5f;
    spd.z = 0.0f;
    _rename_CreateSmoke(&pos, &spd, 10.0f);

    // hold the player in place and look at it from the front
    if (!(playerpwp[0]->item & 0x8000)) {
      playerpwp[0]->item |= 0x8000;
      twp->btimer = TRUE;
    }
    SetInputP(0, 9, PLMOT_GOTLEVELUPITEM);
    fn_800399BC(0, twp->pos.x, twp->pos.y, twp->pos.z);
    fn_80039CE0(0, twp->ang.x, twp->ang.y, twp->ang.z);
    fn_80011E4C("ITEM_GET.ADX");
    fn_800C3A58(0, 20);
    fn_800C382C(0, fn_800C3BDC(0), 0);
    lbl_8020C5E0.tgtmode = 3;
    lbl_8020C5E0.colflag = FALSE;
    lbl_8020C5E0.timer = 0;
    lbl_8020C5E0.dist = 20.0f + twp->scl.y;
    lbl_8020C5E0.tgt = playertwp[0]->pos;
    lbl_8020C5E0.tgt.y += playerpwp[0]->p.eyes_height;
    lbl_8020C5E0.ang.y = 0x4000 - playertwp[0]->ang.y;
    lbl_8020C5E0.ang.x = 0xF800;
    fn_8001DD50();
    fn_80022E24(0);
    fn_80062C4C();
    fn_8005B990();
    break;
  case MD_GET:
    if (GetWork(tp)->msg_tp == NULL) {
      twp->mode = MD_END;
      if (twp->btimer) {
        playerpwp[0]->item &= ~0x8000;
        twp->btimer = FALSE;
      }
      SetInputP(0, 15, 0);
      fn_800C3780(0, fn_800C3BDC(0));
      fn_800120D4();
      fn_8001DD64();
      fn_80022E64(0);
      fn_80062C60();
    } else {
      lbl_8020C5E0.ang.y += 0x80;
    }
    break;
  case MD_END:
    if (twp->wtimer < 1000) {
      twp->wtimer++;
    }
    break;
  }

  CCL_Entry(tp);

  // the player may not exist yet when the object is created
  if (GetWork(tp)->texlist == NULL) {
    GetWork(tp)->texlist = LevUpItemGetTexlist(0, GetWork(tp)->info->character);
    if (GetWork(tp)->texlist == NULL) {
      GetWork(tp)->texlist =
          LevUpItemGetTexlist(1, GetWork(tp)->info->character);
    }
  }
}

static void ObjectLevUpDaiDisp(task *tp) {
  taskwk *twp = tp->twp;

  fn_8012297C(1);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  njPushMatrixEx();
  njSetTexture(&_rename_levupdai_texlist);
  fn_8011E158(&_rename_levupdai_model);
  if (twp->mode == MD_WAIT || twp->mode == MD_GET) {
    njSetTexture(&_rename_levupdai_top_texlist);
    fn_8011E158(&_rename_levupdai_top_model);
  }
  njPopMatrixEx();
  if (GetWork(tp)->info->disp != NULL && twp->mode == MD_WAIT) {
    GetWork(tp)->info->disp(tp);
  }
  njPopMatrixEx();
  fn_8012297C(3);
}

static Float  levupdai_light_scl = 0.61f;
static Uint32 levupdai_light_col0 = 0x60FFFFC0;
static Uint32 levupdai_light_col1 = 0xB0A0E0FF;

static void ObjectLevUpDaiDispSort(task *tp) {
  taskwk *twp = tp->twp;
  Uint32 col;

  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  njPushMatrixEx();
  njSetTexture(&_rename_levupdai_texlist);
  fn_8011E158(&_rename_levupdai_model);
  if (twp->mode == MD_WAIT || twp->mode == MD_GET) {
    njSetTexture(&_rename_levupdai_top_texlist);
    fn_8011E158(&_rename_levupdai_top_model);
  }

  njDisableFog();
  gjSetFog();
  fn_80073028(&_rename_levupdai_light_model, _rename_levupdai_light_uvinfo,
              lbl_801CC168._7C);
  njSetTexture(&_rename_levupdai_light_texlist);
  if (twp->mode == MD_GET) {
    col = fn_800334B0(levupdai_light_col0, levupdai_light_col1,
                      0.5f * (1.0f + njSin(lbl_801CC168._7C * 0x300)));
  } else {
    col = fn_800334B0(0xE0FFFFFF, 0x80FFFFFF,
                      0.5f * (1.0f + njSin(lbl_801CC168._7C * 0x200)));
  }
  fn_8002B304();
  fn_8002B35C();
  OffControl3D(0x220);
  OnControl3D(0x810);
  fn_801218C8(0xFF, 0x800);
  fn_800156FC(((Uint8 *)&col)[0] / 255.0f, ((Uint8 *)&col)[1] / 255.0f,
              ((Uint8 *)&col)[2] / 255.0f, ((Uint8 *)&col)[3] / 255.0f);

  if (twp->mode == MD_WAIT) {
    fn_8011E158(&_rename_levupdai_light_model);
  } else if (twp->wtimer < 35) {
    // the light shrinks once the item is taken
    njScale(NULL, -0.7f,
            (Float)(30 - twp->wtimer) / 30.0f *
                (1.7f + 0.2f * njSin(lbl_801CC168._7C * 0x180)),
            0.72f);
    if (twp->mode == MD_GET) {
      njCnkCacheDrawModel(&_rename_levupdai_light_model);
    } else {
      fn_8011E158(&_rename_levupdai_light_model);
    }
  }

  njRotateY(NULL, lbl_801CC168._7C << 7);
  if (twp->mode == MD_WAIT) {
    njScale(NULL, -0.7f,
            levupdai_light_scl *
                (1.7f + 0.2f * njSin(lbl_801CC168._7C * 0x180)),
            0.72f);
    fn_8011E158(&_rename_levupdai_light_model);
  } else if (twp->wtimer < 35) {
    njScale(NULL, -0.7f,
            (Float)(30 - twp->wtimer) / 30.0f *
                (levupdai_light_scl *
                 (1.7f + 0.2f * njSin(lbl_801CC168._7C * 0x180))),
            0.72f);
    if (twp->mode == MD_GET) {
      njCnkCacheDrawModel(&_rename_levupdai_light_model);
    } else {
      fn_8011E158(&_rename_levupdai_light_model);
    }
  }

  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  fn_8002B348();
  fn_8002B2F8();
  njEnableFog();
  gjSetFog();
  njPopMatrixEx();
  if (GetWork(tp)->info->disp != NULL && twp->mode == MD_WAIT) {
    GetWork(tp)->info->disp(tp);
  }
  njPopMatrixEx();
}
