#include "OBJECT/o_searchbox.h"

#include "CCL.h"
#include "qFabsf.h"
#include "samt/ninja/gjdraw.h"
#include "samt/ninja/gjmodel.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/shinobi/sg_pad.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/shadow.h"
#include "samt/sonic/sound.h"
#include "set.h"

// not stl/math.h, to match
extern f32 atan2f(f32 y, f32 x);
extern void ds_DrawModelClip(NJS_MODEL *);
extern void _rename_RingDrawShadowModel(void);
extern BOOL _rename_CheckDrawRange(NJS_POINT3 *pos, Float range, Float *dist);
extern void _rename_SetVertexListColor(Sint32 *vlist, Uint32 color);
extern void fn_8011E17C(NJS_CNK_OBJECT *object);
extern task *fn_800E018C(void);
extern void fn_800E0118(Sint32, const char *msg, Sint32 time, Sint32 lang,
                        task **tpp);
extern const char *fn_8008C6C0(void);
extern Sint32 fn_8001CD40(void);
extern void fn_800399BC(Sint32 pno, Float x, Float y, Float z);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void fn_801218C8(Sint32, Sint32);
extern void fn_800156FC(Float, Float, Float, Float);

// the hint book: 'num' counts the pages the player has unlocked
typedef struct hintbook {
  /* 0x00 */ Uint8 unk_00[3];
  /* 0x03 */ Sint8 num;
} hintbook;

extern hintbook *lbl_803ADA88;
extern Sint32 lbl_803ADAD0;
extern PDS_PERIPHERAL *lbl_801E55D8[8];

extern NJS_TEXLIST    _rename_searchbox_texlist;
extern NJS_CNK_OBJECT _rename_searchbox_cnk_object;
extern NJS_TEXLIST    _rename_searchbox_screen_texlist;
extern Sint32         _rename_searchbox_screen_vlist[];
extern NJS_CNK_OBJECT _rename_searchbox_screen_object;
extern GJS_OBJECT     _rename_searchbox_gj_object;
extern NJS_MODEL     *_rename_searchbox_models[3][4];
extern NJS_MODEL     *_rename_searchbox_models_lod[3][4];
extern CCL_INFO       _rename_searchbox_colli_info[1];

// ^ extern

typedef struct searchboxwk {
  /* 0x00 */ Float      alpha;
  /* 0x04 */ Float      light;
  /* 0x08 */ Angle      ang;
  /* 0x0C */ NJS_POINT3 shadow_pos;
  /* 0x18 */ Angle      spin_ang;
  /* 0x1C */ Angle      wave_ang;
  /* 0x20 */ Float      ofs_y;
  /* 0x24 */ task      *msg_tp;
} searchboxwk; // sizeof=0x28

#define GetWork(task) ((searchboxwk *)task->mwp)

// the pause hint keeps its message task in 'mwp' instead of a work struct
#define GetMsgTask(t) (*(task **)&t->mwp)

// the constant is a double built from the float NJD_PI
#define RadAng(n) ((Angle)((65536.0 / (2.0 * NJD_PI)) * (n)))

// at most 0x200 a frame; sign tested first to match
#define LimitTurn(d)                                                           \
  if ((d) > 0) {                                                               \
    if ((d) > 0x200) {                                                         \
      (d) = 0x200;                                                             \
    }                                                                          \
  } else if ((d) < -0x200) {                                                   \
    (d) = -0x200;                                                              \
  }

#define ARGB(a, r, g, b) (((a) << 24) + ((r) << 16) + ((g) << 8) + (b))

enum {
  MD_INIT,
  MD_WAIT,
  MD_OPEN,
  MD_TALK,
};

#define MSG_NUM 3

static void PauseHintDisp(task *tp);
static void PauseHintExec(task *tp);
static void PauseHintDest(task *tp);
static void SearchBoxManDest(task *tp);
static void SearchBoxManExec(task *tp);
static void ObjectSearchBoxDest(task *tp);
static void ObjectSearchBoxExec(task *tp);
static void ObjectSearchBoxDisp(task *tp);
static void ObjectSearchBoxDispDelay(task *tp);
static void ObjectSearchBoxDispSimple(task *tp);

static const char *searchbox_msg[MSG_NUM] = { NULL, NULL, NULL };

static Sint32 searchbox_simple_disp;
static task *searchbox_man_tp;
static Sint32 searchbox_hint_ready;
static task *pause_hint_tp;

static void SearchBoxManDest(task *tp) {
  if (searchbox_man_tp == tp) {
    searchbox_man_tp = NULL;
  }
}

static void SearchBoxManExec(task *tp) {
  taskwk *twp = tp->twp;

  twp->wtimer++;
  if (twp->wtimer > 301) {
    DestroyTask(tp);
  }
}

static void CreateSearchBoxMan(void) {
  task *tp;

  if (searchbox_man_tp == NULL) {
    tp = CreateFundamentalTask(2, 3, SearchBoxManExec);
    if (tp != NULL) {
      tp->exec = SearchBoxManExec;
      tp->dest = SearchBoxManDest;
      searchbox_man_tp = tp;
    }
  }
  if (searchbox_man_tp != NULL) {
    searchbox_man_tp->twp->wtimer = 1;
  }
}

static Sint32 SearchBoxHintActive(void) {
  if (searchbox_man_tp != NULL && searchbox_man_tp->twp->wtimer != 0) {
    return searchbox_man_tp->twp->wtimer;
  }
  if (searchbox_hint_ready != 0 && lbl_803ADA88 != NULL &&
      lbl_803ADA88->num >= 2) {
    return 1;
  }
  return 0;
}

static void CreatePauseHint(void) {
  task *tp;

  if (pause_hint_tp != NULL) {
    return;
  }
  tp = CreateFundamentalTask(2, 2, PauseHintExec);
  if (tp == NULL) {
    return;
  }
  pause_hint_tp = tp;
  tp->disp = PauseHintDisp;
  tp->exec = PauseHintExec;
  tp->dest = PauseHintDest;
}

static void PauseHintDisp(task *tp) {
  taskwk *twp = tp->twp;
  const char **msg;
  Sint32 num;
  Sint32 i;

  if (fn_8001CD40() != 0x11) {
    return;
  }
  if ((lbl_801E55D8[0]->on & 0x600) == 0x600) {
    return;
  }
  if (lbl_803ADAD0 != 0) {
    return;
  }
  if (lbl_803ADA88 == NULL) {
    return;
  }
  if (lbl_803ADA88->num <= 0) {
    return;
  }

  if (fn_800E018C() == NULL && GetMsgTask(tp) == NULL) {
    twp->wtimer = 0;
  }
  if (twp->wtimer % 180 == 0 &&
      (fn_800E018C() == NULL || fn_800E018C() == GetMsgTask(tp))) {
    num = 0;
    msg = searchbox_msg;
    for (i = 0; i < MSG_NUM; i++) {
      if (*msg == NULL) {
        break;
      }
      num = i + 1;
      msg++;
    }
    if (num > 0) {
      if (num == 1) {
        if (fn_800E018C() == NULL && GetMsgTask(tp) == NULL) {
          fn_800E0118(0, searchbox_msg[0], 0, lbl_801CC168._11,
                      &GetMsgTask(tp));
        }
      } else {
        if (GetMsgTask(tp) != NULL) {
          DestroyTask(GetMsgTask(tp));
        }
        i = (twp->wtimer / 180) % num;
        fn_800E0118(0, searchbox_msg[i], 0, lbl_801CC168._11,
                    &GetMsgTask(tp));
      }
    }
  }

  if (lbl_801CC168._17 == 1 && (lbl_801E55D8[0]->on & 0x30000) == 0x30000) {
    return;
  }
  if (lbl_801CC168._17 == 2 && (lbl_801E55D8[1]->on & 0x30000) == 0x30000) {
    return;
  }
  if ((lbl_801CC168._17 == 1 &&
       (lbl_801E55D8[0]->press & 0x30000) == 0x20000) ||
      (lbl_801CC168._17 == 2 &&
       (lbl_801E55D8[1]->press & 0x30000) == 0x20000)) {
    twp->wtimer = (twp->wtimer / 180 - 1) * 180;
    return;
  }
  if ((lbl_801CC168._17 == 1 &&
       (lbl_801E55D8[0]->press & 0x30000) == 0x10000) ||
      (lbl_801CC168._17 == 2 &&
       (lbl_801E55D8[1]->press & 0x30000) == 0x10000)) {
    twp->wtimer = (twp->wtimer / 180 + 1) * 180;
    return;
  }
  // written as negations to match
  if (!(lbl_801CC168._17 == 1 && (lbl_801E55D8[0]->on & 0x30000)) &&
      !(lbl_801CC168._17 == 2 && (lbl_801E55D8[1]->on & 0x30000))) {
    twp->wtimer++;
  }
}

static void PauseHintExec(task *tp) {
  if (GetMsgTask(tp) != NULL) {
    DestroyTask(GetMsgTask(tp));
  }
}

static void PauseHintDest(task *tp) {
  if (GetMsgTask(tp) != NULL) {
    DestroyTask(GetMsgTask(tp));
  }
  if (pause_hint_tp == tp) {
    pause_hint_tp = NULL;
  }
  GetMsgTask(tp) = NULL;
}

// unreferenced; reconstructed from the .rodata order, stripped by the linker
static void SearchBoxSetLight(task *tp, Float light) {
  searchboxwk *wk = GetWork(tp);

  wk->alpha = 0.4f;
  wk->light = 1.0f - qFabsf(light);
  wk->ofs_y = 0.0f;
}

static void SearchBoxDrawShadow(task *tp) {
  njPushMatrixEx();
  njTranslate(NULL, GetWork(tp)->shadow_pos.x,
              0.2f + GetWork(tp)->shadow_pos.y, GetWork(tp)->shadow_pos.z);
  njScale(NULL, 3.0f, 1.0f, 3.0f);
  _rename_RingDrawShadowModel();
  njPopMatrixEx();
}

void ObjectSearchBox(task *tp) {
  taskwk *twp = tp->twp;

  if (lbl_801CC168._23 == 1 || lbl_801CC168._23 == 2) {
    if (tp->ocp != NULL) {
      DeadOut(tp);
    } else {
      FreeTask(tp);
    }
    return;
  }
  if (CheckRangeOut(tp)) {
    return;
  }
  tp->mwp = syCalloc(1, sizeof(searchboxwk));
  if (tp->mwp == NULL) {
    return;
  }

  if (searchbox_simple_disp != 0) {
    tp->disp = ObjectSearchBoxDispSimple;
  } else {
    tp->disp = ObjectSearchBoxDisp;
    tp->disp_dely = ObjectSearchBoxDispDelay;
  }
  tp->exec = ObjectSearchBoxExec;
  tp->dest = ObjectSearchBoxDest;
  twp->mode = MD_INIT;
  twp->smode = 0;
  twp->wtimer = 0;
  CreatePauseHint();
  CCL_InitShare(tp, _rename_searchbox_colli_info,
                ARYLEN(_rename_searchbox_colli_info), CID_OBJECT);
  if (twp->cwp != NULL) {
    twp->cwp->colli_range = 8.0f;
  }

  GetWork(tp)->alpha = 1.0f;
  GetWork(tp)->light = 0.0f;
  GetWork(tp)->ang = twp->ang.y;
  GetWork(tp)->shadow_pos = twp->pos;
  GetWork(tp)->ofs_y = 0.0f;
  GetWork(tp)->wave_ang = 0;
  GetWork(tp)->msg_tp = NULL;
}

static void ObjectSearchBoxDest(task *tp) {
  if (GetWork(tp)->msg_tp != NULL) {
    DestroyTask(GetWork(tp)->msg_tp);
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectSearchBoxExec(task *tp) {
  taskwk *twp = tp->twp;
  // unused1..3 reserve stack, keep them
  Angle3 ang;
  Float unused1[1];
  NJS_VECTOR p;
  Float unused2[6];
  NJS_VECTOR v;
  Float unused3[2];
  Float posy;
  const char *msg;
  Sint32 no;
  Sint32 i;
  Sint32 j;
  Sint32 pno;
  Sint32 page;
  Angle a;
  Sint16 diff;
  Uint32 count;

  if (twp->wtimer == 0 && CheckRangeOut(tp)) {
    return;
  }

  count = lbl_801CC168._7C;
  if (GetWork(tp)->shadow_pos.y == twp->pos.y && count % 4 == 0) {
    posy = GetShadowPos(twp->pos.x, twp->pos.y, twp->pos.z, &ang);
    if (-1000000.0f != posy) {
      GetWork(tp)->shadow_pos.x = twp->pos.x;
      GetWork(tp)->shadow_pos.y = posy;
      GetWork(tp)->shadow_pos.z = twp->pos.z;
    }
  }
  GetWork(tp)->spin_ang = count << 9;
  GetWork(tp)->ofs_y = 0.7f * njSin(GetWork(tp)->wave_ang);

  switch (twp->mode) {
  case MD_INIT:
    if (GetWork(tp)->light > 0.0f) {
      GetWork(tp)->light -= 0.05f;
      if (GetWork(tp)->light < 0.0f) {
        GetWork(tp)->light = 0.0f;
      }
      break;
    }
    diff = twp->ang.y - GetWork(tp)->ang;
    LimitTurn(diff);
    GetWork(tp)->ang += diff;
    if (diff == 0) {
      twp->mode = MD_WAIT;
    }
    break;
  case MD_WAIT:
    for (i = 0; i < 2 && playerpwp[i] != NULL; i++) {
      if (njDistanceP2P(&twp->pos, &playertwp[i]->pos) < 50.0f) {
        no = i + 1;
      } else {
        no = 0;
      }
      if (no != 0 && twp->smode == 6 &&
          !(lbl_801CC168._38 & (1 << (no - 1)))) {
        no = 0;
      }
      if (no != 0 && playerpwp[no - 1]->action_last != 50 &&
          fn_800E018C() == NULL) {
        njPushMatrixEx();
        njUnitMatrix(NULL);
        njTranslateEx(&playertwp[no - 1]->pos);
        njRotateZ(NULL, playertwp[no - 1]->ang.z);
        njRotateX(NULL, playertwp[no - 1]->ang.x);
        njRotateY(NULL, 0x8000 - playertwp[no - 1]->ang.y);
        njInvertMatrix(NULL);
        njCalcPoint(NULL, &twp->pos, &p);
        njPopMatrixEx();
        if (p.x < 0.0f) {
          playerpwp[no - 1]->action_sel = 50;
        }
        no = 0;
      }
      if (no != 0) {
        break;
      }
    }
    if (SearchBoxHintActive()) {
      break;
    }
    if (no <= 0) {
      break;
    }
    if (fn_800E018C() != NULL) {
      break;
    }
    if (GetWork(tp)->msg_tp != NULL) {
      break;
    }
    no--;
    if (no < 0) {
      break;
    }
    playerpwp[no]->action_last = 0;
    twp->btimer = no;
    twp->mode = MD_OPEN;
    break;
  case MD_OPEN:
    GetWork(tp)->light += 0.05f;
    if (GetWork(tp)->light >= 1.0f) {
      page = MSG_NUM;
      if (lbl_803ADA88 != NULL) {
        page = lbl_803ADA88->num;
      }
      if (page < 0) {
        page = 0;
      }
      if (page >= MSG_NUM) {
        page = MSG_NUM - 1;
      }
      if (lbl_803ADA88 != NULL && lbl_803ADA88->num >= 2) {
        searchbox_hint_ready = 1;
      } else {
        searchbox_hint_ready = 0;
      }
      msg = fn_8008C6C0();
      GetWork(tp)->light = 1.0f;
      twp->mode = MD_TALK;
      if (msg == NULL) {
        msg = "NoText";
      }
      searchbox_msg[page] = msg;
      for (j = page + 1; j < MSG_NUM; j++) {
        searchbox_msg[j] = NULL;
      }
      fn_800E0118(1, msg, 120, lbl_801CC168._11, &GetWork(tp)->msg_tp);
      SE_Call(0x8013, NULL, 0, 0);
    }
    a = RadAng(atan2f(playertwp[twp->btimer]->pos.x - twp->pos.x,
                      playertwp[twp->btimer]->pos.z - twp->pos.z));
    diff = a - GetWork(tp)->ang;
    LimitTurn(diff);
    GetWork(tp)->wave_ang += 0x300;
    GetWork(tp)->ang += diff;
    break;
  case MD_TALK:
    if (GetWork(tp)->msg_tp == NULL) {
      twp->mode = MD_INIT;
      CreateSearchBoxMan();
      twp->wtimer = 1;
      break;
    }
    a = RadAng(atan2f(playertwp[twp->btimer]->pos.x - twp->pos.x,
                      playertwp[twp->btimer]->pos.z - twp->pos.z));
    diff = a - GetWork(tp)->ang;
    LimitTurn(diff);
    GetWork(tp)->ang += diff;
    GetWork(tp)->wave_ang += 0x300;
    break;
  }

  if (GetWork(tp)->msg_tp == NULL && SearchBoxHintActive()) {
    if (GetWork(tp)->alpha > 0.4f) {
      GetWork(tp)->alpha -= 0.09f;
      if (GetWork(tp)->alpha < 0.4f) {
        GetWork(tp)->alpha = 0.4f;
      }
    }
  } else if (GetWork(tp)->alpha < 1.0f) {
    GetWork(tp)->alpha += 0.09f;
    if (GetWork(tp)->alpha > 1.0f) {
      GetWork(tp)->alpha = 1.0f;
    }
  }

  if (twp->wtimer != 0) {
    twp->wtimer++;
    if (twp->wtimer > 60) {
      if (tp->ocp != NULL) {
        DeadOut(tp);
      } else {
        FreeTask(tp);
      }
      return;
    }
    twp->pos.y += 1.0f;
    GetWork(tp)->ang += 0x111;
  }

  if (twp->smode != 6) {
    CCL_Entry(tp);
    return;
  }
  for (pno = 0; pno < 2 && playertwp[pno] != NULL; pno++) {
    if (!(lbl_801CC168._38 & (1 << pno))) {
      continue;
    }
    v.x = playertwp[pno]->pos.x - twp->pos.x;
    v.y = playertwp[pno]->pos.y - twp->pos.y;
    v.z = playertwp[pno]->pos.z - twp->pos.z;
    posy = njScalor(&v);
    if (posy > 0.01f && posy < 12.0f) {
      posy = 12.0f / posy;
      v.x *= posy;
      v.y *= posy;
      v.z *= posy;
      fn_800399BC(pno, v.x + twp->pos.x, v.y + twp->pos.y,
                  v.z + twp->pos.z);
    }
  }
}

static void ObjectSearchBoxDisp(task *tp) {
  taskwk *twp = tp->twp;
  Float unused0[2];
  Float dist;
  Float f;
  Float alpha;
  GJS_OBJECT *object;
  Angle spin;
  Uint32 col;

  if (twp->smode == 6 && !(lbl_801CC168._38 & (1 << lbl_803ADAD0))) {
    return;
  }
  if (!_rename_CheckDrawRange(&twp->pos, 20.0f, &dist)) {
    return;
  }
  alpha = GetWork(tp)->alpha;
  spin = GetWork(tp)->spin_ang;

  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x, twp->pos.y + GetWork(tp)->ofs_y, twp->pos.z);
  njRotateY(NULL, GetWork(tp)->ang);

  object = &_rename_searchbox_gj_object;
  f = 255.0f * GetWork(tp)->light;
  col = (Uint32)f;
  if ((Uint32)f > 255) {
    col = 255;
  }

  if (1.0f == alpha) {
    njSetTexture(&_rename_searchbox_screen_texlist);
    _rename_SetVertexListColor(_rename_searchbox_screen_vlist,
                               ARGB(0xFF, col, col, col));
    fn_8011E17C(&_rename_searchbox_screen_object);
    njSetTexture(&_rename_searchbox_texlist);
    gjDrawModel(object->model);
    object = object->child;
    njSetTexture(&_rename_searchbox_texlist);
    njTranslateEx(&object->pos);
    njRotateY(NULL, spin);
    gjDrawModel(object->model);
  }
  njPopMatrixEx();
}

static void ObjectSearchBoxDispDelay(task *tp) {
  taskwk *twp = tp->twp;
  Float unused0[1];
  Float dist;
  Float f;
  Float alpha;
  NJS_CNK_OBJECT *object;
  Uint32 col;
  Angle spin;
  Uint32 a;

  if (twp->smode == 6 && !(lbl_801CC168._38 & (1 << lbl_803ADAD0))) {
    return;
  }
  if (!_rename_CheckDrawRange(&twp->pos, 20.0f, &dist)) {
    return;
  }
  alpha = GetWork(tp)->alpha;
  spin = GetWork(tp)->spin_ang;

  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x, twp->pos.y + GetWork(tp)->ofs_y, twp->pos.z);
  njRotateY(NULL, GetWork(tp)->ang);

  object = &_rename_searchbox_cnk_object;
  f = 255.0f * GetWork(tp)->light;
  col = (Uint32)f;
  if ((Uint32)f > 255) {
    col = 255;
  }

  if (1.0f != alpha) {
    f = 255.0f * alpha;
    a = (Uint32)f;
    if ((Uint32)f > 255) {
      a = 255;
    }
    fn_8002B304();
    fn_8002B35C();
    OffControl3D(0x220);
    OnControl3D(0x810);
    fn_800156FC(alpha, 1.0f, 1.0f, 1.0f);
    fn_801218C8(0xFF, 0x800);
    njSetTexture(&_rename_searchbox_screen_texlist);
    _rename_SetVertexListColor(_rename_searchbox_screen_vlist,
                               ARGB(a, col, col, col));
    fn_8011E17C(&_rename_searchbox_screen_object);
    njSetTexture(&_rename_searchbox_texlist);
    njCnkCacheDrawModel(object->model);
    object = object->child;
    njSetTexture(&_rename_searchbox_texlist);
    njTranslateEx(&object->pos);
    njRotateY(NULL, spin);
    njCnkCacheDrawModel(object->model);
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
    fn_8002B348();
    fn_8002B2F8();
  }
  njPopMatrixEx();

  if (GetWork(tp)->shadow_pos.y != twp->pos.y) {
    SearchBoxDrawShadow(tp);
  }
}

static void ObjectSearchBoxDispSimple(task *tp) {
  taskwk *twp = tp->twp;
  Float unused0[1];
  Float dist;
  Float f;
  Float alpha;
  NJS_CNK_OBJECT *object;
  NJS_CNK_OBJECT *child;
  Angle spin;
  Uint32 col;
  Uint32 a;

  if (twp->smode == 6 && !(lbl_801CC168._38 & (1 << lbl_803ADAD0))) {
    return;
  }
  if (!_rename_CheckDrawRange(&twp->pos, 20.0f, &dist)) {
    return;
  }
  alpha = GetWork(tp)->alpha;
  spin = GetWork(tp)->spin_ang;

  njPushMatrixEx();
  njTranslate(NULL, twp->pos.x, twp->pos.y + GetWork(tp)->ofs_y, twp->pos.z);
  njRotateY(NULL, GetWork(tp)->ang);

  object = &_rename_searchbox_cnk_object;
  f = 255.0f * GetWork(tp)->light;
  col = (Uint32)f;
  if ((Uint32)f > 255) {
    col = 255;
  }

  if (1.0f == alpha) {
    njSetTexture(&_rename_searchbox_texlist);
    ds_DrawModelClip(_rename_searchbox_models[0][3]);
    if (dist < 200.0f) {
      njSetTexture(&_rename_searchbox_screen_texlist);
      _rename_SetVertexListColor(_rename_searchbox_screen_vlist,
                                 ARGB(0xFF, col, col, col));
      fn_8011E17C(&_rename_searchbox_screen_object);
    }
    child = object->child;
    njSetTexture(&_rename_searchbox_texlist);
    njTranslateEx(&child->pos);
    njRotateY(NULL, spin);
    ds_DrawModelClip(_rename_searchbox_models[1][3]);
  } else if (0.4f == alpha) {
    f = 255.0f * alpha;
    a = (Uint32)f;
    if ((Uint32)f > 255) {
      a = 255;
    }
    njSetTexture(&_rename_searchbox_texlist);
    ds_DrawModelClip(_rename_searchbox_models_lod[0][3]);
    if (dist < 100.0f) {
      fn_8002B304();
      fn_8002B35C();
      OffControl3D(0x220);
      OnControl3D(0x810);
      fn_800156FC(alpha, 1.0f, 1.0f, 1.0f);
      fn_801218C8(0xFF, 0x800);
      njSetTexture(&_rename_searchbox_screen_texlist);
      _rename_SetVertexListColor(_rename_searchbox_screen_vlist,
                                 ARGB(a, col, col, col));
      fn_8011E17C(&_rename_searchbox_screen_object);
      fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
      fn_8002B348();
      fn_8002B2F8();
    }
    if (dist < 300.0f) {
      child = object->child;
      njSetTexture(&_rename_searchbox_texlist);
      njTranslateEx(&child->pos);
      njRotateY(NULL, spin);
      ds_DrawModelClip(_rename_searchbox_models_lod[1][3]);
    }
  } else {
    f = 255.0f * alpha;
    a = (Uint32)f;
    if ((Uint32)f > 255) {
      a = 255;
    }
    fn_8002B304();
    fn_8002B35C();
    OffControl3D(0x220);
    OnControl3D(0x810);
    fn_800156FC(alpha, 1.0f, 1.0f, 1.0f);
    fn_801218C8(0xFF, 0x800);
    njSetTexture(&_rename_searchbox_texlist);
    njCnkCacheDrawModel(object->model);
    if (dist < 100.0f) {
      njSetTexture(&_rename_searchbox_screen_texlist);
      _rename_SetVertexListColor(_rename_searchbox_screen_vlist,
                                 ARGB(a, col, col, col));
      fn_8011E17C(&_rename_searchbox_screen_object);
    }
    if (dist < 300.0f) {
      child = object->child;
      njSetTexture(&_rename_searchbox_texlist);
      njTranslateEx(&child->pos);
      njRotateY(NULL, spin);
      njCnkCacheDrawModel(child->model);
    }
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
    fn_8002B348();
    fn_8002B2F8();
  }
  njPopMatrixEx();

  if (GetWork(tp)->shadow_pos.y != twp->pos.y) {
    SearchBoxDrawShadow(tp);
  }
}
