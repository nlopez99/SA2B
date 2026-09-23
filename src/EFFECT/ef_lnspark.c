#include "EFFECT/ef_lnspark.h"

#include "fabsf.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/task.h"
#include "stdlib.h"

extern void fn_80014D7C(NJS_POINT3 *, Uint32 *, Sint32);

// ^ extern
// v in this file

#define LNSPARK_MAX 255
#define LNSPARK_TRAIL 2

// pos[0] is the spark, pos[1] its next-frame position; drawn as a line
typedef struct lnspark // sizeof=0x2C
{
  /* 0x00 */ NJS_POINT3 pos[LNSPARK_TRAIL];
  /* 0x18 */ NJS_VECTOR spd;
  /* 0x24 */ Float ground_y;
  /* 0x28 */ Sint32 timer;
} lnspark;

// ring buffer shared by every spark, owned by one LnSparkExec task
typedef struct lnsparkwk // sizeof=0x2BDC
{
  /* 0x0000 */ lnspark spark[LNSPARK_MAX];
  /* 0x2BD4 */ Sint32 head;
  /* 0x2BD8 */ Sint32 num;
} lnsparkwk;

static lnsparkwk *lnspark_work;
static task *lnspark_task;

static Uint32 lnspark_color[LNSPARK_TRAIL] = {0xFF902000, 0xFFFFA030};

static Float lnspark_friction = 0.98f;
static Float lnspark_bound = 0.7f;
static Float lnspark_gravity = 0.08f;

static void LnSparkExec(task *tp);
static void LnSparkDisp(task *tp);
static void LnSparkDest(task *tp);

void CreateLnSpark(NJS_POINT3 *pos, NJS_VECTOR *spd, Float spread,
                   Float ground_y) {
  task *tp;
  lnspark *sp;
  Sint32 num;
  Float x, y, z;
  Float px, py, pz;
  Sint32 idx; // unused

  if (lnspark_task == NULL) {
    tp = CreateElementalTask(0, 2, LnSparkExec, "LnSparkExec");
    if (tp != NULL) {
      tp->disp = LnSparkDisp;
      tp->exec = LnSparkExec;
      tp->dest = LnSparkDest;
      lnspark_task = tp;
    }
  }
  if (lnspark_task != NULL && lnspark_work == NULL) {
    lnspark_work = syCalloc(1, sizeof(lnsparkwk));
  }
  if (lnspark_task != NULL && lnspark_work != NULL) {
    num = lnspark_work->num;
    if (num >= LNSPARK_MAX - 1) {
      lnspark_work->head = (lnspark_work->head + 1) % LNSPARK_MAX;
    } else {
      num++;
    }
    lnspark_work->num = num;
    num += lnspark_work->head;
    num--;
    if (num < 0) {
      num = LNSPARK_MAX - 1;
    }
    num %= LNSPARK_MAX;
    sp = &lnspark_work->spark[num];
    spread *= 2.0f;
    x = spread * (0.000030517578f * (Float)rand() - 0.5f);
    y = spread * (0.000030517578f * (Float)rand() - 0.5f);
    z = spread * (0.000030517578f * (Float)rand() - 0.5f);
    x += spd->x;
    y += spd->y;
    z += spd->z;
    sp->spd.x = x;
    sp->spd.y = y;
    sp->spd.z = z;
    px = pos->x;
    py = pos->y;
    pz = pos->z;
    sp->pos[0].x = px;
    sp->pos[0].y = py;
    sp->pos[0].z = pz;
    sp->pos[1].x = px;
    sp->pos[1].y = py;
    sp->pos[1].z = pz;
    sp->ground_y = ground_y;
    sp->timer = 80;
  } else {
    if (lnspark_work != NULL) {
      syFree(lnspark_work);
      lnspark_work = NULL;
    }
    if (lnspark_task != NULL) {
      DestroyTask(lnspark_task);
    }
  }
}

static void LnSparkExec(task *tp) {
  Sint32 num;
  Sint32 idx;
  Sint32 dead;
  NJS_POINT3 *pp;
  Sint32 i;
  Float *fp;
  Float px, py, pz;
  Float sx, sy, sz;
  Sint32 end;

  if (lnspark_work == NULL) {
    DestroyTask(tp);
    return;
  }
  num = lnspark_work->num;
  idx = lnspark_work->head;
  dead = -1;
  while (num-- != 0) {
    lnspark *sp = &lnspark_work->spark[idx];
    fp = &sp->spd.x;
    sx = *fp++;
    sy = *fp++;
    sz = *fp;
    fp = &sp->pos[0].x;
    px = *fp++;
    py = *fp++;
    pz = *fp;
    sx *= lnspark_friction;
    sy = sy * lnspark_friction - lnspark_gravity;
    sz *= lnspark_friction;
    px += sx;
    py += sy;
    pz += sz;
    if (py < sp->ground_y) {
      py = sp->ground_y;
      sy = lnspark_bound * fabsf(sy);
    }
    fp = &sp->spd.x;
    *fp++ = sx;
    *fp++ = sy;
    *fp = sz;
    fp = &sp->pos[0].x;
    *fp++ = px;
    *fp++ = py;
    *fp = pz;
    i = 1;
    pp = &sp->pos[1];
    for (; i < LNSPARK_TRAIL; i++) {
      sx *= 0.97f * lnspark_friction;
      sy = 0.97f * (sy * lnspark_friction) - lnspark_gravity;
      sz *= 0.97f * lnspark_friction;
      px += sx;
      py += sy;
      pz += sz;
      if (py < sp->ground_y) {
        py = sp->ground_y;
        sy = lnspark_bound * fabsf(sy);
      }
      fp = &pp->x;
      *fp++ = px;
      *fp++ = py;
      *fp = pz;
      pp++;
    }
    sp->timer--;
    if (sp->timer <= 0) {
      dead = idx;
    }
    idx++;
    if (idx >= LNSPARK_MAX) {
      idx -= LNSPARK_MAX;
    }
  }
  if (dead >= 0) {
    end = lnspark_work->head + lnspark_work->num;
    dead = (dead + 1) % LNSPARK_MAX;
    if (dead < lnspark_work->head) {
      lnspark_work->head = dead;
      dead += LNSPARK_MAX;
    } else {
      lnspark_work->head = dead;
    }
    lnspark_work->num = end - dead;
  }
  if (lnspark_work->num == 0) {
    DestroyTask(tp);
  }
}

static void LnSparkDisp(task *tp) {
  Sint32 num;
  Sint32 idx;
  lnspark *sp;
  // unused
  Sint32 i;
  Sint32 col;
  Float w;

  if (lnspark_work == NULL) {
    return;
  }
  num = lnspark_work->num;
  idx = lnspark_work->head;
  while (num-- != 0) {
    sp = &lnspark_work->spark[idx];
    idx++;
    if (idx >= LNSPARK_MAX) {
      idx -= LNSPARK_MAX;
    }
    fn_80014D7C(sp->pos, lnspark_color, 3);
  }
}

static void LnSparkDest(task *tp) {
  if (lnspark_work != NULL) {
    syFree(lnspark_work);
    lnspark_work = NULL;
  }
  if (lnspark_task == tp) {
    lnspark_task = NULL;
  }
}
