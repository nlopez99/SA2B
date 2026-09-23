#include "OBJECT/o_hintbox.h"

#include "CCL.h"
#include "fabsf.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/shadow.h"
#include "samt/sonic/sound.h"
#include "set.h"

// not stl/math.h, to match
extern f32 atan2f(f32 y, f32 x);
extern s32 _rename_EitherPlayerWithinSphere(NJS_VECTOR *, f32);
extern void fn_8011E244(NJS_CNK_OBJECT *object);
extern void fn_8011E17C(NJS_CNK_OBJECT *object);
extern BOOL fn_80022E0C(Sint32 pno);
extern void fn_80022E24(Uint8 pno);
extern void fn_80022E64(Uint8 pno);
extern void fn_8003A54C(Sint32 pno, task *tp, NJS_VECTOR *v, Sint32);
extern task *fn_800E018C(void);
extern void fn_800E0118(Sint32, const char *msg, Sint32 time, Sint32 lang,
                        task **tpp);
extern void fn_8002B2F8(void);
extern void fn_8002B304(void);
extern void fn_8002B348(void);
extern void fn_8002B35C(void);
extern void fn_801218C8(Sint32, Sint32);
extern void fn_800156FC(Float, Float, Float, Float);

extern NJS_CNK_OBJECT lbl_801B6408;
extern NJS_TEXLIST    _rename_hintbox_texlist;
extern NJS_CNK_OBJECT _rename_hintbox_object;
extern NJS_TEXLIST    _rename_hintbox_screen_texlist;
extern Sint32         _rename_hintbox_screen_vlist[];
extern NJS_CNK_OBJECT _rename_hintbox_screen_object;

// ^ extern

typedef struct hintboxwk {
  /* 0x00 */ Float      alpha;
  /* 0x04 */ Float      light;
  /* 0x08 */ Angle      ang;
  /* 0x0C */ NJS_POINT3 shadow_pos;
  /* 0x18 */ Sint32     unk18;
  /* 0x1C */ Angle      wave_ang;
  /* 0x20 */ Float      ofs_y;
  /* 0x24 */ task      *msg_tp;
  /* 0x28 */ Sint32     unk28;
} hintboxwk; // sizeof=0x2C

#define GetWork(task) ((hintboxwk *)task->mwp)

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

static void ObjectHintBoxDest(task *tp);
static void ObjectHintBoxExec(task *tp);
static void ObjectHintBoxDisp(task *tp);

// "Mufufu (commentary: <cyan>text<reset> is missing)" in Shift-JIS
#define MSG_DEFAULT                                                            \
  "\x82\xDE\x82\xD3\x82\xD3\x82\xA3\x81\x69\x89\xF0\x90\xE0\x81\x46"           \
  "\x08" "80ffff"                                                              \
  "\x83\x65\x83\x4C\x83\x58\x83\x67"                                           \
  "\x08" "r"                                                                   \
  "\x82\xAA\x82\xC8\x82\xA2\x82\xB6\x82\xA5\x81\x6A"

const char **hintbox_msg_tbl;

static CCL_INFO hintbox_colli_info[1] = {
    {0, CI_FORM_SPHERE, 0x77, 0, 0, {0.0f, 0.0f, 0.0f}, 8.0f, 0.0f, 0.0f, 0.0f,
     0, 0, 0},
};
Sint32 hintbox_msg_num = -1;

static void HintBoxDrawShadow(task *tp) {
  njPushMatrixEx();
  njTranslate(NULL, GetWork(tp)->shadow_pos.x,
              0.2f + GetWork(tp)->shadow_pos.y, GetWork(tp)->shadow_pos.z);
  njScale(NULL, 3.0f, 1.0f, 3.0f);
  fn_8011E244(&lbl_801B6408);
  njPopMatrix(1);
}

void ObjectHintBox(task *tp) {
  taskwk *twp = tp->twp;

  if (CheckRangeOut(tp)) {
    return;
  }
  tp->mwp = syCalloc(1, sizeof(hintboxwk));
  if (tp->mwp == NULL) {
    return;
  }

  tp->disp = ObjectHintBoxDisp;
  tp->exec = ObjectHintBoxExec;
  tp->dest = ObjectHintBoxDest;
  twp->mode = MD_INIT;
  twp->smode = 0;
  twp->wtimer = 0;
  CCL_Init(tp, hintbox_colli_info, ARYLEN(hintbox_colli_info), CID_OBJECT);

  GetWork(tp)->alpha = 1.0f;
  GetWork(tp)->light = 0.0f;
  GetWork(tp)->ang = twp->ang.y;
  GetWork(tp)->shadow_pos = twp->pos;
  GetWork(tp)->ofs_y = 0.0f;
  GetWork(tp)->wave_ang = 0;
  GetWork(tp)->msg_tp = NULL;
  GetWork(tp)->unk28 = -1;
}

static void ObjectHintBoxDest(task *tp) {
  if (GetWork(tp)->msg_tp != NULL) {
    DestroyTask(GetWork(tp)->msg_tp);
  }
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static void ObjectHintBoxExec(task *tp) {
  taskwk *twp = tp->twp;
  Float unused0[1];
  Angle3 ang;
  NJS_POINT3 unused1;
  NJS_VECTOR v;
  Float unused2[5];
  Float posy;
  Sint32 pno;
  Sint32 no;
  Angle a;
  Sint16 diff;
  Uint32 count;

  if (CheckRangeOut(tp)) {
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
  GetWork(tp)->ofs_y = 0.7f * njSin(GetWork(tp)->wave_ang);

  switch (twp->mode) {
  case MD_INIT:
    twp->mode = MD_WAIT;
    break;
  case MD_WAIT:
    pno = _rename_EitherPlayerWithinSphere(&twp->pos, 50.0f);
    if (pno != 0 && playerpwp[pno - 1]->action_last != 50) {
      playerpwp[pno - 1]->action_sel = 50;
      pno = 0;
    }
    if (twp->wtimer != 0 || pno <= 0) {
      break;
    }
    pno--;
    if (pno < 0 || !fn_80022E0C(pno)) {
      break;
    }
    playerpwp[pno]->action_last = 0;
    twp->btimer = pno;
    twp->mode = MD_OPEN;
    break;
  case MD_OPEN:
    GetWork(tp)->light += 0.05f;
    // nested to match
    if (fn_800E018C() == NULL && GetWork(tp)->light >= 1.0f) {
      GetWork(tp)->light = 1.0f;
      if (GetWork(tp)->msg_tp != NULL) {
        DestroyTask(GetWork(tp)->msg_tp);
      }
      no = (hintbox_msg_num >= 0 &&
            (Sint32)fabsf(twp->scl.x) < hintbox_msg_num)
               ? (Sint32)fabsf(twp->scl.x)
               : -1;
      twp->mode = MD_TALK;
      fn_80022E24(twp->btimer);
      fn_800E0118(1, no >= 0 ? hintbox_msg_tbl[no] : MSG_DEFAULT, 180,
                  lbl_801CC168._11, &GetWork(tp)->msg_tp);
      SE_Call(0x8013, NULL, 0, 0);
    }
    GetWork(tp)->wave_ang += 0x300;
    break;
  case MD_TALK:
    if (fn_800E018C() == NULL) {
      fn_80022E64(twp->btimer);
      twp->wtimer = 30;
      twp->mode = MD_INIT;
    } else {
      GetWork(tp)->wave_ang += 0x300;
    }
    break;
  }

  pno = _rename_EitherPlayerWithinSphere(&twp->pos, 50.0f);
  if (pno > 0) {
    GetWork(tp)->light += 0.05f;
    if (GetWork(tp)->light >= 1.0f) {
      v.x = 0.0f;
      v.y = 0.0f;
      v.z = 0.0f;
      GetWork(tp)->light = 1.0f;
      fn_8003A54C(pno - 1, tp, &v, 1);
      a = RadAng(atan2f(playertwp[twp->btimer]->pos.x - twp->pos.x,
                        playertwp[twp->btimer]->pos.z - twp->pos.z));
      diff = a - GetWork(tp)->ang;
      LimitTurn(diff);
      GetWork(tp)->ang += diff;
    }
  } else if (GetWork(tp)->light > 0.0f) {
    GetWork(tp)->light -= 0.05f;
    if (GetWork(tp)->light < 0.0f) {
      GetWork(tp)->light = 0.0f;
    }
  } else {
    diff = twp->ang.y - GetWork(tp)->ang;
    LimitTurn(diff);
    GetWork(tp)->ang += diff;
  }

  if (twp->wtimer != 0 && !(twp->cwp->flag & 1)) {
    twp->wtimer--;
  }
  CCL_Entry(tp);
}

static void HintBoxSetVertexColor(Sint32 *vlist, Sint32 nbVertex,
                                  Uint32 color) {
  Uint32 *p = (Uint32 *)&vlist[5];

  while (nbVertex--) {
    *p = color;
    p += 4;
  }
}

static void ObjectHintBoxDisp(task *tp) {
  taskwk *twp = tp->twp;
  Float alpha = GetWork(tp)->alpha;
  NJS_CNK_OBJECT *object;
  Float f;
  Uint32 col;
  Uint32 a;

  njPushMatrix(NULL);
  njTranslate(NULL, twp->pos.x, twp->pos.y + GetWork(tp)->ofs_y, twp->pos.z);
  njRotateY(NULL, GetWork(tp)->ang);

  object = &_rename_hintbox_object;
  f = 255.0f * GetWork(tp)->light;
  col = (Uint32)f;
  if ((Uint32)f > 255) {
    col = 255;
  }

  if (1.0f == alpha) {
    njSetTexture(&_rename_hintbox_texlist);
    njCnkCacheDrawModel(object->model);
    njSetTexture(&_rename_hintbox_screen_texlist);
    HintBoxSetVertexColor(_rename_hintbox_screen_vlist, 4,
                          ARGB(0xFF, col, col, col));
    fn_8011E17C(&_rename_hintbox_screen_object);
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
    njSetTexture(&_rename_hintbox_texlist);
    njCnkCacheDrawModel(object->model);
    njSetTexture(&_rename_hintbox_screen_texlist);
    HintBoxSetVertexColor(_rename_hintbox_screen_vlist, 4,
                          ARGB(a, col, col, col));
    fn_8011E17C(&_rename_hintbox_screen_object);
    fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
    fn_8002B348();
    fn_8002B2F8();
  }
  njPopMatrix(1);

  if (GetWork(tp)->shadow_pos.y != twp->pos.y) {
    HintBoxDrawShadow(tp);
  }
}
