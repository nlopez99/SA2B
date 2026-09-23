#include "sa2b_types.h"
#include "earthshake.h"

#include "fabsf.h"
#include "samt/ninja/njcollision.h"
#include "samt/ninja/njmatrix.h"
#include "samt/sonic/task.h"
#include "stdlib.h"

// offsets a screen's camera by shake, and takes the offset back off again
extern Sint32 fn_800C36F4(Sint32 pno, NJS_VECTOR *shake);
extern void fn_800C36A4(Sint32 pno);

// camera of each screen
extern NJS_POINT3 *lbl_80175378[4];
// number of screens
extern Sint32 lbl_803ADAD4;

#define RAND_RATE (1.0f / 32768.0f)

static NJS_VECTOR ZeroVector = { 0.0f, 0.0f, 0.0f };

// the one task that owns the camera offsets; its work holds a frame of shake
// per screen: pos for screen 0, scl for screen 1
static task *EarthShakeMan;

static void EarthShakeManExec(task *tp)
{
  taskwk *twp = tp->twp;

  if (njScalor2(&twp->pos) < 0.001f && njScalor2(&twp->scl) < 0.001f) {
    DestroyTask(tp);
    return;
  }
  if (lbl_803ADAD4 >= 1) {
    fn_800C36F4(0, &twp->pos);
  }
  twp->pos = ZeroVector;
  if (lbl_803ADAD4 >= 2) {
    fn_800C36F4(1, &twp->scl);
  }
  twp->scl = ZeroVector;
}

static void EarthShakeManDest(task *tp)
{
  Sint32 i;

  for (i = 0; i < lbl_803ADAD4; i++) {
    fn_800C36A4(i);
  }
  if (EarthShakeMan == tp) {
    EarthShakeMan = NULL;
  }
}

static void AddEarthShake(Sint32 pno, NJS_VECTOR *v)
{
  taskwk *twp;
  Float *d;
  Float *s;

  if (EarthShakeMan == NULL) {
    EarthShakeMan = CreateElementalTask(2, 3, EarthShakeManExec, "EarthShakeManExec");
    if (EarthShakeMan != NULL) {
      EarthShakeMan->dest = EarthShakeManDest;
    }
  }
  if (EarthShakeMan == NULL) {
    return;
  }
  twp = EarthShakeMan->twp;
  s = &v->x;
  if (pno == 0) {
    d = &twp->pos.x;
    *d++ += *s++;
    *d++ += *s++;
    *d += *s;
    return;
  }
  d = &twp->scl.x;
  *d++ += *s++;
  *d++ += *s++;
  *d += *s;
}

static Float ShakeDamp = 0.98f;
static Float ShakeBounce = 0.9f;
static Float ShakeSpring = 0.5f;

// one quake, a damped bounce: pos = epicentre, scl.x = velocity, scl.y = height,
// scl.z = radius, ang.y = throw direction, wtimer = how fast it turns
static void EsShakeRadExec(task *tp)
{
  taskwk *twp = tp->twp;
  Float dist;
  Float rate;
  Float amp;
  NJS_VECTOR v;
  Sint32 i;

  twp->scl.x = -twp->scl.y * (ShakeSpring + tp->work.f) + twp->scl.x * ShakeDamp;
  twp->scl.y = twp->scl.y + twp->scl.x;
  if (twp->scl.y < 0.0f) {
    twp->scl.y = 0.0f;
    twp->scl.x = ShakeBounce * fabsf(twp->scl.x);
  }
  for (i = 0; i < lbl_803ADAD4; i++) {
    dist = njDistanceP2P(lbl_80175378[i], &twp->pos);
    if (dist < twp->scl.z) {
      rate = 1.0f - dist / twp->scl.z;
      amp = rate * fabsf(twp->scl.x);
      rate *= twp->scl.y; // rate is the vertical throw from here on
      v.y = rate;
      v.x = v.z = 0.35f * amp;
      njPushMatrixEx();
      njUnitMatrix(NULL);
      njRotateY(NULL, twp->ang.y);
      njCalcVector(NULL, &v, &v);
      njPopMatrixEx();
      AddEarthShake(i, &v);
    }
  }
  twp->ang.y += twp->wtimer;
  if (fabsf(twp->scl.x) < 0.1f) {
    DestroyTask(tp);
  }
}

task *CreateEsShakeRad(NJS_POINT3 *pos, Float radius, Float power)
{
  task *tp;
  taskwk *twp;

  tp = CreateElementalTask(2, 3, EsShakeRadExec, "EsShakeRad");
  if (tp != NULL) {
    twp = tp->twp;
    twp->pos = *pos;
    twp->scl.z = radius;
    twp->scl.x = power;
    twp->wtimer = (Sint32)(128.0f * (RAND_RATE * (Float)rand())) + 0x300;
    tp->work.f = 0.05f * (RAND_RATE * (Float)rand());
  }
  return tp;
}
