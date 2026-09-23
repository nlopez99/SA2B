#include "EFFECT/ef_wphole.h"

#include "samt/ninja/njmath.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "types.h"

#include "fabsf.h"

extern Float logf(Float x);
extern Float powf(Float x, Float y);

extern void gjSetFog(void);
extern void njDisableFog(void);
extern void njEnableFog(void);
extern void __njColorBlendingMode(Int, Int);

// flare texlist and its two spin rates, owned by the object that opens the hole
extern NJS_TEXLIST _rename_wphole_flare_texlist;
extern Sint32 _rename_wphole_flare_spin0;
extern Sint32 _rename_wphole_flare_spin1;

extern void fn_80034234(Sint32);
extern void fn_80034214(void);
extern void fn_80033CE4(NJS_POINT3 *pos, Sint32 num, Float ang, Float sclx,
                        Float scly, Uint32 argb, Sint32 attr);
extern void fn_80033620(NJS_POINT3 *, Sint32, Sint32, Angle, Float, Float,
                        Sint32, Sint32, Float, Float);

extern Sint32 lbl_803ADAD0; // screen being drawn

// per-screen camera work; the hole is billboarded with its angle
typedef struct camwk {
  /* 0x00 */ Uint8 unk_00[0xC];
  /* 0x0C */ Angle3 ang;
} camwk;

extern camwk *lbl_80175378[4];

// ^ extern
// v in this file

static void WpHoleSetPoint(NJS_POINT3 *pos, Uint16 *timer, Float r, Uint8 flag);
static void WpHoleExec(task *tp);
static void WpHoleDisp(task *tp);
static void WpHoleDest(task *tp);
static Bool WpHoleParticleExec(particle_info *info, particle *p);

typedef struct wpholewk // sizeof=0x10
{
  /* 0x00 */ NJS_POINT3 *pos;   // one point per speck of dust
  /* 0x04 */ Uint16 *timer;     // frames left before that speck is respawned
  /* 0x08 */ Sint32 num;
  /* 0x0C */ task **owner;      // slot the opener keeps this task in
} wpholewk;

#define GetWork(task) ((wpholewk *)task->mwp)

// the hole keeps two floats in the work pointers it never allocates
#define GetR(task) (*(Float *)&task->fwp)
#define GetScl(task) (*(Float *)&task->awp)

static NJS_TEXNAME wphole_texname[] = {
    {"sikake_37_32"},
};
NJS_TEXLIST wphole_texlist = {wphole_texname, ARRAY_COUNT(wphole_texname)};

// each frame a speck shrinks by this much and turns by these angles, so it
// spirals in
static Float wphole_shrink = 0.96f;
static Float wphole_rot_y = -1.0f;
static Float wphole_rot_x = 0.5f;
static Float wphole_rot_z = 1.9f;

// the cloud is drawn wphole_trail_num times, each copy one step back and darker
static Float wphole_trail_fade = 0.75f;

// a speck respawns this far in, and starts in the outer wphole_spread of r
static Float wphole_inner = 0.07f;
static Float wphole_spread = 0.2f;

static Uint32 wphole_trail_num = 2;

// places a speck on a random sphere and sets its frames until it falls in;
// flag & 1 starts it fallen, flag & 2 spreads it over the whole radius
static void WpHoleSetPoint(NJS_POINT3 *pos, Uint16 *timer, Float r,
                           Uint8 flag) {
  Float len;
  Float min = wphole_inner * r;
  Float life;

  if (flag & 2) {
    len = min + r * ParticleRandom();
  } else {
    len = min + r * (1.0f - wphole_spread + wphole_spread * ParticleRandom());
  }
  pos->x = 0.0f;
  pos->y = len;
  pos->z = 0.0f;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njRotateZ(NULL, ParticleDegAng(360.0f * ParticleRandom()));
  njRotateX(NULL, ParticleDegAng(360.0f * ParticleRandom()));
  njCalcPoint(NULL, pos, pos);
  njPopMatrix(1);
  life = logf(min / len) / logf(wphole_shrink);
  if (life > 65000.0f) {
    life = 65000.0f;
  }
  if (life < 1.0f) {
    life = 1.0f;
  }
  *timer = life;
  if (flag & 1) {
    pos->x *= min / len;
    pos->y *= min / len;
    pos->z *= min / len;
  }
}

task *CreateWpHole(NJS_POINT3 *pos, Float r, Sint32 num, Uint8 flag, Float scl,
                   task **owner) {
  task *tp = CreateFundamentalTask(IM_TWK, LEV_3, WpHoleExec);
  Sint32 i;

  // nested to match
  if (tp != NULL) {
    tp->twp->pos = *pos;
    if (CheckRangeOut(tp)) {
      return NULL;
    }
    tp->mwp = syCalloc(1, sizeof(wpholewk));
    if (tp->mwp == NULL) {
      DestroyTask(tp);
      return NULL;
    }
    GetWork(tp)->pos = syCalloc(num, sizeof(NJS_POINT3));
    GetWork(tp)->timer = syCalloc(num, sizeof(Uint16));
    if (GetWork(tp)->pos == NULL || GetWork(tp)->timer == NULL) {
      syFree(GetWork(tp)->pos);
      syFree(GetWork(tp)->timer);
      syFree(tp->mwp);
      tp->mwp = NULL;
      DestroyTask(tp);
      return NULL;
    }
    tp->disp_sort = WpHoleDisp;
    tp->exec = WpHoleExec;
    tp->dest = WpHoleDest;
    GetR(tp) = r;
    GetScl(tp) = scl;
    GetWork(tp)->num = num;
    if (flag != 0) {
      flag = 1;
    }
    tp->twp->mode = flag;
    for (i = 0; i < num; i++) {
      WpHoleSetPoint(&GetWork(tp)->pos[i], &GetWork(tp)->timer[i], r, flag | 2);
    }
    GetWork(tp)->owner = owner;
    if (owner != NULL) {
      *owner = tp;
    }
    return tp;
  }
  return NULL;
}

static void WpHoleExec(task *tp) {
  Sint32 i;
  Sint32 num;
  Uint16 *timer;

  if (CheckRangeOut(tp)) {
    return;
  }
  njPushMatrixEx();
  njUnitMatrix(NULL);
  if (tp->twp->mode != 0) {
    njRotateZ(NULL, ParticleDegAng(-wphole_rot_z));
    njRotateX(NULL, ParticleDegAng(-wphole_rot_x));
    njRotateY(NULL, ParticleDegAng(-wphole_rot_y));
    njScale(NULL, 1.0f / wphole_shrink, 1.0f / wphole_shrink,
            1.0f / wphole_shrink);
  } else {
    njRotateZ(NULL, ParticleDegAng(wphole_rot_z));
    njRotateX(NULL, ParticleDegAng(wphole_rot_x));
    njRotateY(NULL, ParticleDegAng(wphole_rot_y));
    njScale(NULL, wphole_shrink, wphole_shrink, wphole_shrink);
  }
  njCalcPoints(NULL, GetWork(tp)->pos, GetWork(tp)->pos, GetWork(tp)->num);
  njPopMatrix(1);
  num = GetWork(tp)->num;
  timer = GetWork(tp)->timer;
  for (i = 0; i < num; i++, timer++) {
    if ((*timer)-- == 0) {
      WpHoleSetPoint(&GetWork(tp)->pos[i], timer, GetR(tp), tp->twp->mode);
    }
  }
}

static void WpHoleDisp(task *tp) {
  Sint32 i;
  Uint32 argb0;
  Uint32 argb1;
  Float scl;

  njDisableFog();
  gjSetFog();
  njPushMatrixEx();
  njTranslateV(NULL, &tp->twp->pos);
  njRotateY(NULL, lbl_80175378[lbl_803ADAD0]->ang.y);
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 10);
  njSetTexture(&wphole_texlist);
  njSetTextureNum(0);
  for (i = 0; i < wphole_trail_num; i++) {
    if (i > 0) {
      if (tp->twp->mode != 0) {
        njRotateZ(NULL, ParticleDegAng(-wphole_rot_z));
        njRotateX(NULL, ParticleDegAng(-wphole_rot_x));
        njRotateY(NULL, ParticleDegAng(-wphole_rot_y));
        njScale(NULL, 1.0f / wphole_shrink, 1.0f / wphole_shrink,
                1.0f / wphole_shrink);
      } else {
        njRotateZ(NULL, ParticleDegAng(wphole_rot_z));
        njRotateX(NULL, ParticleDegAng(wphole_rot_x));
        njRotateY(NULL, ParticleDegAng(wphole_rot_y));
        njScale(NULL, wphole_shrink, wphole_shrink, wphole_shrink);
      }
    }
    scl = GetScl(tp) * powf(wphole_trail_fade,
                            (Float)(wphole_trail_num - i - 1));
    argb0 = 255 - (wphole_trail_num - i - 1) * 32;
    argb0 = 0xFF000000 + (argb0 << 16) + (argb0 << 8) + argb0;
    fn_80034234(1);
    fn_80033CE4(GetWork(tp)->pos, GetWork(tp)->num, 0.0f, scl, scl, argb0, 0);
    fn_80034214();
  }
  njPopMatrix(1);
  if (tp->work.f > 0.01f) {
    // argb1 doubles as the alpha byte before it is packed
    argb1 = (Uint32)(255.0f * tp->work.f);
    if ((Uint32)(255.0f * tp->work.f) > 255) {
      argb1 = 255;
    }
    argb0 = (argb1 << 24) | 0x00FFFFFF;
    argb1 = (Uint32)(255.0f * tp->work.f *
                     (1.0f + 0.3f * -fabsf(njSin(lbl_801CC168._7C << 8))));
    if (argb1 > 255) {
      argb1 = 255;
    }
    argb1 = (argb1 << 24) | 0x00FFFFFF;
    njSetTexture(&_rename_wphole_flare_texlist);
    if (tp->twp->smode != 0) {
      fn_80033620(&tp->twp->pos, 0, 0x4000,
                  -(lbl_801CC168._7C * _rename_wphole_flare_spin0),
                  10.0f * tp->work.f, 10.0f * tp->work.f, argb0, 1, 0.0f, 0.0f);
      fn_80033620(&tp->twp->pos, 0, 0x4000,
                  lbl_801CC168._7C * _rename_wphole_flare_spin1,
                  10.0f * tp->work.f, 10.0f * tp->work.f, argb1, 0x11, 0.0f,
                  0.0f);
    } else {
      fn_80033620(&tp->twp->pos, 0, 0x4000,
                  lbl_801CC168._7C * _rename_wphole_flare_spin0,
                  10.0f * tp->work.f, 10.0f * tp->work.f, argb0, 1, 0.0f, 0.0f);
      fn_80033620(&tp->twp->pos, 0, 0x4000,
                  -(lbl_801CC168._7C * _rename_wphole_flare_spin1),
                  10.0f * tp->work.f, 10.0f * tp->work.f, argb1, 0x11, 0.0f,
                  0.0f);
    }
  }
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  njEnableFog();
  gjSetFog();
}

static void WpHoleDest(task *tp) {
  task **owner = GetWork(tp)->owner;

  if (owner != NULL && *owner == tp) {
    *owner = NULL;
  }
  syFree(GetWork(tp)->pos);
  syFree(GetWork(tp)->timer);
  tp->fwp = NULL;
  tp->awp = NULL;
  syFree(tp->mwp);
  tp->mwp = NULL;
}

static particle_info wphole_particle_info = {
    1,    &wphole_texlist,     0,        1,          0.0f, 0.975f, 0.0018f,
    -0.002f, WpHoleParticleExec, 75000.0f,
};

particle *CreateWpHoleParticle(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&wphole_particle_info);
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

static Bool WpHoleParticleExec(particle_info *info, particle *p) {
  Uint32 alpha;

  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  if (ParticleRandom() < 0.03f) {
    p->spd.x += 0.2f * (ParticleRandom() - 0.5f);
    p->spd.z += 0.2f * (ParticleRandom() - 0.5f);
  }
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  alpha = p->argb >> 24;
  if (alpha < 10 || p->scl < 0.0f) {
    return FALSE;
  }
  p->argb = (p->argb & 0x00FFFFFF) | ((alpha - 2) << 24);
  // no return value, as in the original
  return;
}
