#include "EFFECT/ef_spspark.h"

#include "fabsf.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/task.h"
#include "stdlib.h"

extern void fn_8011F4F8(Sint32, Uint32, Sint32);
extern void fn_8011F4FC(void);
extern void fn_8011F500(NJS_POINT3 *, Sint32, Float, Float);

// ^ extern
// v in this file

#define SpSparkColor(a, r, g, b) (((a) << 24) + ((r) << 16) + ((g) << 8) + (b))

#define SPSPARK_MAX 255
#define SPSPARK_TRAIL 6

// pos[0] is the spark, pos[1..] its next-frame positions; drawn as a strip
typedef struct spspark // sizeof=0x5C
{
  /* 0x00 */ NJS_POINT3 pos[SPSPARK_TRAIL];
  /* 0x48 */ NJS_VECTOR spd;
  /* 0x54 */ Float ground_y;
  /* 0x58 */ Sint32 timer;
} spspark;

// ring buffer shared by every spark, owned by one SpSparkExec task
typedef struct spsparkwk // sizeof=0x5BAC
{
  /* 0x0000 */ spspark spark[SPSPARK_MAX];
  /* 0x5BA4 */ Sint32 head;
  /* 0x5BA8 */ Sint32 num;
} spsparkwk;

static spsparkwk *spspark_work;
static task *spspark_task;
static Sint32 spspark_task_num;

static NJS_TEXNAME spspark_texname[] = {
    {"p_exp"},
};

NJS_TEXLIST spspark_texlist = {
    spspark_texname,
    ARRAY_COUNT(spspark_texname),
};

static Float spspark_friction = 0.98f;
static Float spspark_bound = 0.7f;
static Float spspark_gravity = 0.08f;
static Float spspark_width = 0.06f;

static void SpSparkExec(task *tp);
static void SpSparkDisp(task *tp);
static void SpSparkDest(task *tp);

void CreateSpSpark(NJS_POINT3 *pos, NJS_VECTOR *spd, Float spread,
                   Float ground_y) {
  task *tp;
  spspark *sp;
  Sint32 num;
  Float x, y, z;
  Float px, py, pz;
  NJS_VECTOR v; // unused

  if (spspark_task == NULL) {
    tp = CreateElementalTask(0, 2, SpSparkExec, "SpSparkExec");
    if (tp != NULL) {
      tp->disp = SpSparkDisp;
      tp->exec = SpSparkExec;
      tp->dest = SpSparkDest;
      spspark_task = tp;
      spspark_task_num++;
    }
  }
  if (spspark_task != NULL && spspark_work == NULL) {
    spspark_work = syCalloc(1, sizeof(spsparkwk));
  }
  if (spspark_task != NULL && spspark_work != NULL) {
    num = spspark_work->num;
    if (num >= SPSPARK_MAX - 1) {
      spspark_work->head = (spspark_work->head + 1) % SPSPARK_MAX;
    } else {
      num++;
    }
    spspark_work->num = num;
    num += spspark_work->head;
    num--;
    if (num < 0) {
      num = SPSPARK_MAX - 1;
    }
    num %= SPSPARK_MAX;
    sp = &spspark_work->spark[num];
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
    sp->pos[2].x = px;
    sp->pos[2].y = py;
    sp->pos[2].z = pz;
    sp->pos[3].x = px;
    sp->pos[3].y = py;
    sp->pos[3].z = pz;
    sp->pos[4].x = px;
    sp->pos[4].y = py;
    sp->pos[4].z = pz;
    sp->pos[5].x = px;
    sp->pos[5].y = py;
    sp->pos[5].z = pz;
    sp->ground_y = ground_y;
    sp->timer = 80;
  } else {
    if (spspark_work != NULL) {
      syFree(spspark_work);
      spspark_work = NULL;
    }
    if (spspark_task != NULL) {
      DestroyTask(spspark_task);
    }
  }
}

static void SpSparkExec(task *tp) {
  Sint32 num;
  Sint32 idx;
  Sint32 dead;
  NJS_POINT3 *pp;
  Sint32 i;
  Float *fp;
  Float sx, sy, sz;
  Float px, py, pz;
  Sint32 end;

  if (spspark_work == NULL) {
    DestroyTask(tp);
    return;
  }
  num = spspark_work->num;
  idx = spspark_work->head;
  dead = -1;
  while (num-- != 0) {
    spspark *sp = &spspark_work->spark[idx];
    fp = &sp->spd.x;
    sx = *fp++;
    sy = *fp++;
    sz = *fp;
    fp = &sp->pos[0].x;
    px = *fp++;
    py = *fp++;
    pz = *fp;
    sx *= spspark_friction;
    sy = sy * spspark_friction - spspark_gravity;
    sz *= spspark_friction;
    px += sx;
    py += sy;
    pz += sz;
    if (py < sp->ground_y) {
      py = sp->ground_y;
      sy = spspark_bound * fabsf(sy);
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
    for (; i < SPSPARK_TRAIL; i++) {
      sx *= 0.97f * spspark_friction;
      sy = 0.97f * (sy * spspark_friction) - spspark_gravity;
      sz *= 0.97f * spspark_friction;
      px += sx;
      py += sy;
      pz += sz;
      if (py < sp->ground_y) {
        py = sp->ground_y;
        sy = spspark_bound * fabsf(sy);
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
    idx = (idx + 1) % SPSPARK_MAX;
  }
  if (dead >= 0) {
    end = spspark_work->head + spspark_work->num;
    dead = (dead + 1) % SPSPARK_MAX;
    if (dead < spspark_work->head) {
      spspark_work->head = dead;
      dead += SPSPARK_MAX;
    } else {
      spspark_work->head = dead;
    }
    spspark_work->num = end - dead;
  }
  if (spspark_work->num == 0) {
    DestroyTask(tp);
  }
}

static void SpSparkDisp(task *tp) {
  Sint32 num;
  Sint32 idx;
  Sint32 i;
  Sint32 col;
  spspark *sp;
  Float w;

  if (spspark_work == NULL) {
    return;
  }
  num = spspark_work->num;
  idx = spspark_work->head;
  while (num-- != 0) {
    sp = &spspark_work->spark[idx];
    idx = (idx + 1) % SPSPARK_MAX;
    col = 0;
    w = spspark_width / 6.0f;
    for (i = 0; i < SPSPARK_TRAIL; i++) {
      w += spspark_width / 6.0f;
      njSetTexture(&spspark_texlist);
      fn_8011F4F8(0, SpSparkColor(0xFF, col, col, col), 0);
      if (w < 0.023f) {
        fn_8011F500(&sp->pos[i], 1, 0.023f, 0.023f);
      } else {
        fn_8011F500(&sp->pos[i], 1, w, w);
      }
      fn_8011F4FC();
      col += 0x33;
    }
  }
}

static void SpSparkDest(task *tp) {
  if (spspark_work != NULL) {
    syFree(spspark_work);
    spspark_work = NULL;
  }
  if (spspark_task == tp) {
    spspark_task = NULL;
  }
  spspark_task_num--;
}
