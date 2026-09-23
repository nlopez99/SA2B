#include "EFFECT/ef_spark.h"

#include "samt/ninja/njmatrix.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/player.h"
#include "samt/sonic/task.h"
#include "stdlib.h"

extern NJS_VECTOR lbl_801E5624; // gravity, in world units per frame
extern void fn_80014D7C(NJS_POINT3 *, Uint32 *, Sint32);

// ^ extern
// v in this file

// rand() scaled to 0..1
#define SPARK_RAND (0.000030517578f * (Float)rand())

// one spark, drawn as a short line along its direction of travel
typedef struct spark // sizeof=0x20
{
  /* 0x00 */ Sint16 _00; // unused
  /* 0x02 */ Sint16 timer;
  /* 0x04 */ Uint32 col;
  /* 0x08 */ NJS_POINT3 pos;
  /* 0x14 */ NJS_VECTOR spd;
} spark;

#define GetSpark(tp) ((spark *)(tp)->awp)

static void SparkExecute(task *tp);
static void SparkDisplay(task *tp);

static void SparkExecute(task *tp) {
  spark *sp = GetSpark(tp);
  NJS_VECTOR v;

  if (lbl_801CC168._37) {
    return;
  }
  if (sp->timer-- <= 0) {
    FreeTask(tp);
    return;
  }

  njAddVector(&sp->pos, &sp->spd);

  v = lbl_801E5624;
  v.x *= 0.02f;
  v.y *= 0.02f;
  v.z *= 0.02f;
  njAddVector(&sp->spd, &v);
}

static void SparkDisplay(task *tp) {
  spark *sp = GetSpark(tp);
  NJS_POINT3 pos[2];
  Uint32 col[2];

  // the line runs from the spark back along where it came from
  col[1] = sp->col;
  col[0] = col[1];
  pos[0] = sp->spd;
  pos[1] = sp->pos;
  njUnitVector(&pos[0]);
  pos[0].x *= 0.6f;
  pos[0].y *= 0.6f;
  pos[0].z *= 0.6f;
  njAddVector(&pos[0], &sp->pos);
  fn_80014D7C(pos, col, 5);
}

void CreateSpark(Sint32 num, NJS_POINT3 *pos, NJS_VECTOR *spd) {
  Sint32 i;
  task *tp;
  spark *sp;

  for (i = 0; i < num; i++) {
    tp = CreateElementalTask(0, LEV_3, SparkExecute, "SparkExecute");
    if (tp == NULL) {
      return;
    }
    sp = syCalloc(1, sizeof(spark));
    if (sp == NULL) {
      FreeTask(tp);
      return;
    }

    sp->pos = *pos;
    sp->pos.x += 0.5f * (0.5f - SPARK_RAND);
    sp->pos.y += 0.5f * (0.5f - SPARK_RAND);
    sp->pos.z += 0.5f * (0.5f - SPARK_RAND);

    if (spd != NULL) {
      // thrown along spd; its lifetime comes only from the colour below
      sp->spd = *spd;
      sp->spd.x += 1.5f * (0.5f - SPARK_RAND);
      sp->spd.y += 0.5f * SPARK_RAND;
      sp->spd.z += 1.5f * (0.5f - SPARK_RAND);
      sp->timer = 0;
    } else {
      // scattered from a standstill, and it always flies upwards
      sp->spd.x = 2.6f * (0.5f - SPARK_RAND);
      sp->spd.y = 1.7f * SPARK_RAND;
      sp->spd.z = 2.6f * (0.5f - SPARK_RAND);
      sp->timer = 30;
    }

    switch (i & 3) {
    case 0:
    case 2:
      sp->col = 0xFFE03000;
      sp->timer += 12;
      break;
    case 1:
      sp->col = 0xFFFF6010;
      sp->timer += 15;
      break;
    case 3:
      sp->col = 0xFFA04030;
      sp->timer += 18;
      break;
    default:
      sp->col = 0xFF1050FF;
      sp->timer += 18;
      break;
    }

    tp->awp = (anywk *)sp;
    tp->disp = SparkDisplay;
  }
}
