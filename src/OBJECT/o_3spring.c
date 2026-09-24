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

extern NJS_TEXLIST _rename_3spring_TexList;
extern GJS_MODEL   _rename_3SpringGjModel1;
extern GJS_MODEL   _rename_3SpringGjModel2;

extern CCL_INFO    _rename_3spring_colli_info[1];
extern Float       _rename_3spring_k;
extern Float       _rename_3spring_touch_spd;
extern Float       _rename_3spring_damp;

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
  CCL_Init(tp, _rename_3spring_colli_info, ARYLEN(_rename_3spring_colli_info),
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
      GetWork(tp)->spd = _rename_3spring_touch_spd;
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
  GetWork(tp)->spd = _rename_3spring_damp *
                     (GetWork(tp)->spd - GetWork(tp)->pos * _rename_3spring_k);
  GetWork(tp)->pos += GetWork(tp)->spd;
}

static void Object3SpringDisp(task *tp) {
  taskwk *twp = tp->twp;
  Float f;
  njSetTexture(&_rename_3spring_TexList);
  njPushMatrixEx();
  njTranslateEx(&twp->pos);
  njRotateZ(NULL, twp->ang.z);
  njRotateX(NULL, twp->ang.x);
  njRotateY(NULL, twp->ang.y);
  gjDrawModel(&_rename_3SpringGjModel1);
  f = fabsf(GetWork(tp)->pos);
  njTranslate(NULL, 0.0f, f, f);
  gjDrawModel(&_rename_3SpringGjModel2);
  njPopMatrixEx();
}
