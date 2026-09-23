#include "OBJECT/o_switch.h"

#include "CCL.h"
#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "samt/sonic/sound.h"
#include "set.h"
#include "types.h"
#include "fabsf.h"

inline float sqrtf(float f);
extern Float atan2f(Float y, Float x);

extern void fn_8011E158(NJS_CNK_MODEL *model);
extern void fn_8002B304(void);
extern void fn_8002B35C(void);
extern void fn_8002B348(void);
extern void fn_8002B2F8(void);
extern void fn_801218C8(Sint32, Sint32);
extern Uint32 fn_800334B0(Uint32 col0, Uint32 col1, Float ratio);
extern void fn_800156FC(Float, Float, Float, Float);
extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern Float _rename_GetBlinkRatio(Sint32 cycle, Sint32 on, Sint32 fade);
extern void ObjectMovableInitialize(taskwk *twp, motionwk *mwp, Sint32 id);
extern void fn_800DDF88(task *tp);
extern void fn_80014650(NJS_POINT3 *pos, Angle3 *ang, NJS_VECTOR *scl, Sint32);

// one byte per switch: bit 0 is the on/off state, bit 1 "changed this frame"
extern Uint8 lbl_802B99E0[0x20];

extern NJS_TEXLIST   _rename_switch_texlist;
extern NJS_CNK_MODEL _rename_switch_model;
extern NJS_CNK_MODEL _rename_switch_lamp_model;
extern NJS_CNK_MODEL _rename_switch_clip_model;
extern CCL_INFO      _rename_sss_colli_info[2];
// positions the floating switch keeps clear of (registered by the SSS poles)
extern NJS_POINT3   *_rename_switch_avoid_pos[16];
extern Uint32        _rename_switch_lamp_col;
extern Uint32        _rename_switch_body_col;
extern Uint32        _rename_switch_hilight_col;

// ^ extern
// v in this file

static void SwitchInit(task *tp);
static void SwitchInitWork(task *tp);
static void SwitchExec(task *tp);
static void SwitchDisp(task *tp);
static void SwitchDest(task *tp);
static void SwitchFreeWork(task *tp);
static void SwitchModeToggle(task *tp);
static void SwitchModeHold(task *tp);
static void SwitchModeTimer(task *tp);
static void SwitchModeOnce(task *tp);
static void SwitchFadeLamp(task *tp);
static void SwitchSetTimer(taskwk *twp);
static void SwitchSetBlink(task *tp);
static void SwitchSetLamp(task *tp, Sint32 on);
static void SwitchTurn(task *tp, Sint32 on);
static Sint32 SwitchIsOn(task *tp);
static Sint32 SwitchIsPushed(task *tp);
static void SSSExec(task *tp);
static void SSSDest(task *tp);
static void SSSDisp(task *tp);

typedef struct switchwk // sizeof=0x38
{
  /* 0x00 */ Uint8 pushed; // the switch was already held down last frame
  /* 0x01 */ Uint8 lamp;   // lamp brightness, 0..255
  /* 0x02 */ Uint16 blink;
  /* 0x04 */ Uint16 type;
  /* 0x06 */ Uint16 no;
  /* 0x08 */ Sint32 unk_8;
  /* 0x0C */ Angle3 ang; // tilt of the ball on its pole
  /* 0x18 */ Sint32 touch;
  /* 0x1C */ Sint32 push;
  /* 0x20 */ NJS_POINT3 pos; // offset of the ball from the pole top
  /* 0x2C */ NJS_VECTOR spd;
} switchwk;

#define GetWork(task) ((switchwk *)task->awp)

// degrees to Angle; literal on the left to match
#define DegAng(n) ((Angle)(182.04445f * (n)))

NJS_POINT3 **AddSwitchAvoidPos(NJS_POINT3 *pos) {
  NJS_POINT3 **p = _rename_switch_avoid_pos;
  Sint32 i;

  for (i = 16; i > 0; i--) {
    if (*p == NULL) {
      *p = pos;
      return p;
    }
    p++;
  }
  return NULL;
}

NJS_POINT3 **RemoveSwitchAvoidPos(NJS_POINT3 *pos) {
  NJS_POINT3 **p = _rename_switch_avoid_pos;
  Sint32 i;

  for (i = 16; i > 0; i--) {
    if (*p == pos) {
      *p = NULL;
      return p;
    }
    p++;
  }
  return NULL;
}

void ObjectSwitch(task *tp) {
  taskwk *twp = tp->twp;
  STACK_PAD_VAR(64);

  if (CheckRangeOut(tp)) {
    return;
  }

  switch (twp->mode) {
  case 0:
    SwitchInit(tp);
    break;
  case 1:
    SwitchExec(tp);
    break;
  }
}

static void SwitchInit(task *tp) {
  taskwk *twp = tp->twp;

  tp->dest = SwitchDest;
  tp->disp = SwitchDisp;
  SwitchInitWork(tp);
  twp->mode = 1;
}

static void SwitchInitWork(task *tp) {
  taskwk *twp = tp->twp;
  Uint32 type;
  Uint16 no;

  tp->awp = syCalloc(1, sizeof(switchwk));

  type = twp->scl.x;
  if ((Uint32)twp->scl.x > 4) {
    type = 4;
  }
  GetWork(tp)->type = type;

  no = (Uint32)twp->scl.y;
  if ((Uint16)(Uint32)twp->scl.y >= 32) {
    no = 31;
  }
  GetWork(tp)->no = no;

  twp->btimer = 0;
  twp->wtimer = 0;
  GetWork(tp)->pushed = 0;
  twp->smode = 0;
  GetWork(tp)->lamp = 0;
  GetWork(tp)->ang.x = 0;
  GetWork(tp)->ang.y = 0;
  GetWork(tp)->ang.z = 0;
  GetWork(tp)->touch = 0;
  GetWork(tp)->push = 0;
  GetWork(tp)->pos.x = 0.0f;
  GetWork(tp)->pos.y = 0.0f;
  GetWork(tp)->pos.z = 0.0f;
  GetWork(tp)->spd.x = 0.0f;
  GetWork(tp)->spd.y = 0.0f;
  GetWork(tp)->spd.z = 0.0f;
}

// push "out" out of the sphere of radius r around "pos", starting from "d"
static Sint32 SwitchAvoidPoint(NJS_POINT3 *pos, Float r, NJS_POINT3 *d,
                               NJS_POINT3 *out) {
  Float px;
  Float py;
  Float pz;
  Float dx;
  Float dy;
  Float dz;
  Float vx;
  Float vy;
  Float vz;
  Float r2;
  Float len2;
  Float len;

  r2 = r * r;
  px = pos->x;
  py = pos->y;
  pz = pos->z;
  dx = d->x;
  dy = d->y;
  dz = d->z;
  vx = dx - px;
  vy = dy - py;
  vz = dz - pz;
  if (vy > 0.0f) {
    len2 = vx * vx + vy * vy + vz * vz;
    len = sqrtf(len2);
    if (len2 >= r2 && 0.0f != len2) {
      out->x = px;
      out->y = py;
      out->z = pz;
    } else {
      len = r / len;
      out->x = dx + -vx * len;
      out->y = dy + -vy * len;
      out->z = dz + -vz * len;
    }
  } else {
    vy = fabsf(vy);
    if (vy < 20.0f) {
      vy = vx * vx + vz * vz;
    } else {
      vy = vy - 20.0f;
      vy = vx * vx + vy * vy + vz * vz;
    }
    len = sqrtf(vy);
    if (vy >= r2 && 0.0f != vy) {
      out->x = px;
      out->y = py;
      out->z = pz;
    } else {
      len = r / len;
      out->x = dx + -vx * len;
      out->y = py;
      out->z = dz + -vz * len;
    }
  }
  return 1;
}

static void SwitchSwing(task *tp) {
  taskwk *twp = tp->twp;
  Float unused0[1]; // unused
  NJS_POINT3 target;
  NJS_POINT3 sum;
  Float unused1[7];
  NJS_POINT3 d;
  NJS_VECTOR out;
  Float sx;
  Float sy;
  Float sz;
  Float x;
  Float y;
  Float z;
  Float len;
  Float spd2;
  Float hlen;
  Sint32 num;
  taskwk **ptwp; // unused
  Sint32 i;
  NJS_POINT3 *pos;

  sx = GetWork(tp)->spd.x;
  sy = GetWork(tp)->spd.y;
  sz = GetWork(tp)->spd.z;
  x = GetWork(tp)->pos.x;
  y = GetWork(tp)->pos.y;
  z = GetWork(tp)->pos.z;
  x += sx;
  y += sy;
  z += sz;

  len = sqrtf(x * x + y * y + z * z);
  if (len > 0.001f) {
    Float k = 0.15f * (len / 8.0f) / len;
    sx = sx + -x * k;
    sz = sz + -z * k;
    sy = sy + -y * k;
  }

  GetWork(tp)->touch = 0;
  if (len < 1.0f) {
    spd2 = sx * sx + sy * sy + sz * sz;
    if (spd2 < 0.25f) {
      GetWork(tp)->push = 0;
      if (twp->btimer != 0) {
        twp->btimer--;
      }
    }
  }
  if (len > 3.2f) {
    GetWork(tp)->push = 1;
    GetWork(tp)->touch = 1;
    twp->btimer = 10;
  }
  if (len > 8.0f) {
    Float k = 8.0f / len;
    x = x * k;
    y = y * k;
    z = z * k;
  }
  if (y < -5.0f) {
    y = -5.0f;
  }
  sx *= 0.95f;
  sy *= 0.95f;
  sz *= 0.95f;

  target.x = x;
  target.y = 5.0f + y;
  target.z = z;

  num = 0;
  sum.x = 0.0f;
  sum.y = 0.0f;
  sum.z = 0.0f;

  for (i = 0; i < 2 && playertwp[i] != NULL; i++) {
    pos = &playertwp[i]->pos;
    d.x = pos->x - twp->pos.x;
    d.y = pos->y - twp->pos.y;
    d.z = pos->z - twp->pos.z;
    if (SwitchAvoidPoint(&target, 8.0f, &d, &out)) {
      num++;
      sum.x = sum.x + out.x;
      sum.y = sum.y + out.y;
      sum.z = sum.z + out.z;
    }
  }

  {
    NJS_POINT3 **p;

    for (i = 0, p = _rename_switch_avoid_pos; i < 16; i++, p++) {
      if ((pos = *p) != NULL) {
        d.x = pos->x - twp->pos.x;
        d.y = pos->y - twp->pos.y;
        d.z = pos->z - twp->pos.z;
        if (SwitchAvoidPoint(&target, 12.0f, &d, &out)) {
          num++;
          sum.x = sum.x + out.x;
          sum.y = sum.y + out.y;
          sum.z = sum.z + out.z;
        }
      }
    }
  }

  if (num > 1) {
    Float k = 1.0f / (Float)num;
    sum.x = sum.x * k;
    sum.y = sum.y * k;
    sum.z = sum.z * k;
  } else if (num == 0) {
    sum = target;
  }

  x = sum.x;
  y = sum.y - 5.0f;
  z = sum.z;
  sx = sx + 0.2f * (sum.x - target.x);
  sy = sy + 0.2f * (sum.y - target.y);
  sz = sz + 0.2f * (sum.z - target.z);

  len = sqrtf(sx * sx + sy * sy + sz * sz);
  if (len > 25.0f) {
    Float k = 25.0f / len;
    sx = sx * k;
    sz = sz * k;
    sy = sy * k;
  }

  GetWork(tp)->pos.x = sum.x;
  GetWork(tp)->pos.y = y;
  GetWork(tp)->pos.z = z;
  GetWork(tp)->spd.x = sx;
  GetWork(tp)->spd.y = sy;
  GetWork(tp)->spd.z = sz;

  spd2 = sum.x * sum.x + z * z;
  hlen = sqrtf(spd2);
  if (hlen >= 8.0f) {
    Float k = 8.0f / hlen;
    x = x * k;
    z = z * k;
    hlen = 8.0f;
  }
  if (0.0f != spd2) {
    GetWork(tp)->ang.z =
        -(Angle)(10430.38043493439 * atan2f(x, sqrtf(65.61001f - x * x)));
  }
  GetWork(tp)->ang.x =
      (Angle)(10430.38043493439 * atan2f(z, sqrtf(65.61001f - hlen * hlen)));
}

static void SwitchExec(task *tp) {
  SwitchSwing(tp);
  switch (GetWork(tp)->type) {
  case 0:
    SwitchModeToggle(tp);
    break;
  case 1:
    SwitchModeHold(tp);
    break;
  case 2:
    SwitchModeTimer(tp);
    break;
  case 3:
    SwitchModeOnce(tp);
    break;
  default:
    SwitchModeToggle(tp);
    break;
  }
  SwitchFadeLamp(tp);
}

static void SwitchDisp(task *tp) {
  Uint32 *fr;
  taskwk *twp = tp->twp;
  Float sclz;
  Float alpha;
  Float scly;
  Uint32 col;

  njSetTexture(&_rename_switch_texlist);
  njPushMatrix(NULL);
  njTranslateV(NULL, &twp->pos);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  njRotateZ(NULL, twp->ang.z);
  fn_8011E158(&_rename_switch_clip_model);
  fn_8002B304();
  fn_8002B35C();
  OffControl3D(0x220);
  OnControl3D(0x10);
  OnControl3D(0x800);

  alpha = (Float)GetWork(tp)->lamp / 256.0f;
  if (GetWork(tp)->type == 2) {
    alpha = alpha * _rename_GetBlinkRatio(30, 15, 6);
  }
  if (alpha > 0.5f) {
    fn_801218C8(0xFF, 0x300);
    njDisableFog();
    gjSetFog();
  } else {
    fn_801218C8(0xFF, 0x200);
  }

  col = fn_800334B0(_rename_switch_body_col, _rename_switch_hilight_col,
                    (Float)GetWork(tp)->lamp / 256.0f);
  fn_800156FC((Float)(col >> 24) / 255.0f, (Float)((col >> 16) & 0xFF) / 255.0f,
              (Float)((col >> 8) & 0xFF) / 255.0f, (Float)(col & 0xFF) / 255.0f);
  njCnkCacheDrawModel(&_rename_switch_model);

  njTranslate(NULL, 0.0f, GetWork(tp)->pos.y, 0.0f);
  njRotateY(NULL, -twp->ang.y);
  njRotateX(NULL, GetWork(tp)->ang.x);
  njRotateZ(NULL, GetWork(tp)->ang.z);

  fr = &lbl_801CC168._7C;
  sclz = 1.0f + 0.1f * njSin(DegAng((Float)(*fr + 30)) * 5);
  scly = 1.0f + 0.04f * njCos(DegAng((Float)*fr) * 3);
  njScale(NULL, 1.0f + 0.1f * njSin(DegAng((Float)*fr) * 5), scly,
          sclz);
  if (SwitchIsOn(tp)) {
    njRotateY(NULL, DegAng((Float)*fr) * 2);
  }

  col = fn_800334B0(_rename_switch_lamp_col, _rename_switch_hilight_col, alpha);
  fn_800156FC((Float)(col >> 24) / 255.0f, (Float)((col >> 16) & 0xFF) / 255.0f,
              (Float)((col >> 8) & 0xFF) / 255.0f, (Float)(col & 0xFF) / 255.0f);
  njCnkCacheDrawModel(&_rename_switch_lamp_model);

  if (alpha > 0.5f) {
    njEnableFog();
    gjSetFog();
  }
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  fn_8002B348();
  fn_8002B2F8();
  njPopMatrix(1);
}

static void SwitchDest(task *tp) { SwitchFreeWork(tp); }

static void SwitchFreeWork(task *tp) {
  STACK_PAD_VAR(2);

  syFree(GetWork(tp));
  tp->awp = NULL;
}

static void SwitchModeToggle(task *tp) {
  if (SwitchIsPushed(tp)) {
    if (GetWork(tp)->pushed == 0) {
      SwitchTurn(tp, 1 - SwitchIsOn(tp));
    }
    GetWork(tp)->pushed = 1;
  } else {
    GetWork(tp)->pushed = 0;
  }
  SwitchSetLamp(tp, SwitchIsOn(tp));
}

static void SwitchModeHold(task *tp) {
  if (SwitchIsPushed(tp)) {
    SwitchTurn(tp, 1);
    SwitchSetLamp(tp, 1);
  } else {
    SwitchTurn(tp, 0);
    SwitchSetLamp(tp, 0);
  }
}

static void SwitchModeTimer(task *tp) {
  taskwk *twp = tp->twp;

  if (GetSwitchChanged(GetWork(tp)->no)) {
    lbl_802B99E0[GetWork(tp)->no] &= ~2;
    if (SwitchIsOn(tp) == 1) {
      SwitchSetLamp(tp, 1);
      SwitchSetTimer(twp);
      SwitchSetBlink(tp);
    } else {
      SwitchSetLamp(tp, 0);
      GetWork(tp)->blink = 0;
    }
  }

  if (SwitchIsPushed(tp)) {
    SwitchTurn(tp, 1);
    SwitchSetLamp(tp, 1);
    SwitchSetTimer(twp);
    SwitchSetBlink(tp);
  } else if (SwitchIsOn(tp) == 1) {
    Uint16 *blink = &GetWork(tp)->blink;

    if (*blink != 0) {
      (*blink)--;
    } else {
      SwitchSetLamp(tp, 1 - twp->smode);
      SwitchSetBlink(tp);
    }
    if (twp->wtimer != 0) {
      twp->wtimer--;
      if (twp->wtimer == 0) {
        SwitchTurn(tp, 0);
        SwitchSetLamp(tp, 0);
      }
    }
  } else {
    SwitchSetLamp(tp, 0);
  }
}

static void SwitchSetTimer(taskwk *twp) {
  Float t = twp->scl.z;

  if (t > 5.0f && t < 30000.0f) {
    twp->wtimer = t;
  } else {
    twp->wtimer = 5;
  }
}

static void SwitchSetBlink(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 n = twp->wtimer - 40;

  if (n > 0) {
    if (twp->smode != 0) {
      GetWork(tp)->blink = (Sint32)(0.06f * (Float)n) + 1;
    } else {
      GetWork(tp)->blink = (Sint32)(0.04f * (Float)n) + 1;
    }
  } else {
    GetWork(tp)->blink = 1;
  }
}

static void SwitchModeOnce(task *tp) {
  if (SwitchIsPushed(tp)) {
    SwitchTurn(tp, 1);
  }
  SwitchSetLamp(tp, SwitchIsOn(tp));
}

static void SwitchSetLamp(task *tp, Sint32 on) {
  taskwk *twp = tp->twp;

  if (on) {
    twp->smode = 1;
    GetWork(tp)->lamp = 0xFF;
  } else {
    twp->smode = 0;
  }
}

static void SwitchFadeLamp(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->smode == 0) {
    Uint8 *lamp = &GetWork(tp)->lamp;

    if (*lamp >= 0x32) {
      *lamp = *lamp - 0x1E;
    } else {
      *lamp = 0x14;
    }
  }
}

static void SwitchTurn(task *tp, Sint32 on) {
  STACK_PAD_VAR(2);

  if (on) {
    if (!GetSwitchOnOff(GetWork(tp)->no)) {
      SE_Call(0x800E, NULL, 0, 0);
    }
    SetSwitchOnOff(GetWork(tp)->no, 1);
  } else {
    if (GetSwitchOnOff(GetWork(tp)->no)) {
      SE_Call(0x1002, NULL, 0, 0);
    }
    SetSwitchOnOff(GetWork(tp)->no, 0);
  }
}

static Sint32 SwitchIsOn(task *tp) {
  return GetSwitchOnOff(GetWork(tp)->no);
}

static Sint32 SwitchIsPushed(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->btimer != 0) {
    return 1;
  }
  return 0;
}

void SetSwitchOnOff(Sint32 no, Sint32 on) {
  if (on) {
    lbl_802B99E0[no] |= 1;
  } else {
    lbl_802B99E0[no] &= ~1;
  }
  lbl_802B99E0[no] |= 2;
}

Sint32 GetSwitchOnOff(Sint32 no) {
  if (lbl_802B99E0[no] & 1) {
    return 1;
  }
  return 0;
}

Sint32 GetSwitchChanged(Sint32 no) {
  if (lbl_802B99E0[no] & 2) {
    return 1;
  }
  return 0;
}

// dead in most stages, kept by a few
void InitSwitchOnOff(void) {
  Uint32 i;

  for (i = 0; i < sizeof(lbl_802B99E0); i++) {
    lbl_802B99E0[i] = 0;
  }
}

void ObjectSSS(task *tp) {
  taskwk *twp = tp->twp;
  motionwk *mwp = tp->mwp;

  CCL_Init(tp, _rename_sss_colli_info, 2, CID_OBJECT);
  ObjectMovableInitialize(twp, mwp, 11);
  AddSwitchAvoidPos(&twp->pos);
  tp->dest = SSSDest;
  tp->exec = SSSExec;
  tp->disp = SSSDisp;
}

static void SSSExec(task *tp) {
  STACK_PAD_VAR(2);

  if (!CheckRangeOut(tp)) {
    fn_800DDF88(tp);
  }
}

static void SSSDest(task *tp) { RemoveSwitchAvoidPos(&tp->twp->pos); }

static void SSSDisp(task *tp) {
  taskwk *twp = tp->twp;
  NJS_POINT3 pos;
  Angle3 ang;
  NJS_VECTOR scl;

  pos.x = twp->pos.x;
  pos.y = 5.0f + twp->pos.y;
  pos.z = twp->pos.z;
  ang.x = 0;
  ang.y = twp->ang.y;
  ang.z = 0;
  scl.x = 5.0f;
  scl.y = 10.0f;
  scl.z = 5.0f;
  fn_80014650(&pos, &ang, &scl, 2);
}
