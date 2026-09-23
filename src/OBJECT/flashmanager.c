#include "OBJECT/flashmanager.h"

#include "EFFECT/ef_spspark.h"
#include "samt/sonic/task.h"
#include "stdlib.h"

extern particle_info _rename_flash_info;

// light slots for the display hook, one per entry point
extern Uint32 _rename_flash_light0[3];
extern Uint32 _rename_flash_light1[3];
extern Uint32 _rename_flash_light2[4];

// light hook called for each flash, may be NULL
extern void (*lbl_803ADC30)(NJS_POINT3 *pos, Uint32 *light, Float scl,
                            Float f);

// ^ extern
// v in this file

static particle *CreateFlashParticle(NJS_POINT3 *pos, NJS_VECTOR *spd,
                                     Float scl, Float phase);
static void FlashManagerFlash(taskwk *twp);
static void FlashManagerDest(task *tp);
static void FlashManagerSpark(task *tp);
static void FlashManager(task *tp);

enum {
  MD_FLASH,
  MD_SPARK,
  MD_END,
};

// the task has no motion work, so its timer lives in that slot
#define GetTimer(task) (*(Float *)&(task)->mwp)

static particle *CreateFlashParticle(NJS_POINT3 *pos, NJS_VECTOR *spd,
                                     Float scl, Float phase) {
  particle *p = fn_80032B78(&_rename_flash_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(360.0f * ParticleRandom());
    p->phase.f = phase;
  }
  return p;
}

Bool FlashParticleExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  if ((Sint16)p->frame >= info->frame_num) {
    return FALSE;
  }
  return TRUE;
}

static void FlashManagerFlash(taskwk *twp) {
  NJS_VECTOR spd;

  spd.x = 0.0f;
  spd.y = 0.0f;
  spd.z = 0.0f;
  CreateFlashParticle(&twp->pos, &spd, twp->scl.x, twp->scl.y);
  twp->mode = MD_SPARK;
}

static void FlashManagerDest(task *tp) { tp->mwp = NULL; }

// the sparks keep coming for as long as the flash lasts
static void FlashManagerSpark(task *tp) {
  taskwk *twp = tp->twp;
  NJS_VECTOR spd;
  Sint32 i;

  GetTimer(tp) += 0.4f;
  if (GetTimer(tp) > 7.0f) {
    twp->mode = MD_END;
    return;
  }
  for (i = 0; i < twp->btimer; i++) {
    spd.x = 0.05f * (twp->scl.x * (5.5f * ParticleRandom() - 2.75f));
    spd.y = 0.05f * (twp->scl.x * (5.5f * ParticleRandom() - 2.0f));
    spd.z = 0.05f * (twp->scl.x * (5.5f * ParticleRandom() - 2.75f));
    CreateSpSpark(&twp->pos, &spd, 0.0f, -100000.0f);
  }
}

static void FlashManager(task *tp) {
  taskwk *twp = tp->twp;

  switch (twp->mode) {
  case MD_FLASH:
    GetTimer(tp) = 0.0f;
    tp->dest = FlashManagerDest;
    FlashManagerFlash(twp);
    break;
  case MD_SPARK:
    FlashManagerSpark(tp);
    break;
  case MD_END:
    FreeTask(tp);
    return;
  }
}

void CreateFlash(Float x, Float y, Float z, Float scl) {
  NJS_POINT3 pos;
  task *tp;

  if (lbl_803ADC30 != NULL) {
    pos.x = x;
    pos.y = y;
    pos.z = z;
    lbl_803ADC30(&pos, _rename_flash_light0, scl, 0.0f);
  }
  tp = CreateElementalTask(IM_TWK, LEV_3, FlashManager, "FlashManager");
  if (tp != NULL) {
    taskwk *twp = tp->twp;

    twp->pos.x = x;
    twp->pos.y = y;
    twp->pos.z = z;
    twp->scl.x = scl;
    twp->btimer = 3;
  }
}

// as CreateFlash, but the caller says how many sparks it throws
void CreateFlashNum(Float x, Float y, Float z, Float scl, Uint8 num) {
  NJS_POINT3 pos;
  task *tp;

  if (lbl_803ADC30 != NULL) {
    pos.x = x;
    pos.y = y;
    pos.z = z;
    lbl_803ADC30(&pos, _rename_flash_light1, scl, 0.0f);
  }
  tp = CreateElementalTask(IM_TWK, LEV_3, FlashManager, "FlashManager");
  if (tp != NULL) {
    taskwk *twp = tp->twp;

    twp->pos.x = x;
    twp->pos.y = y;
    twp->pos.z = z;
    twp->scl.x = scl;
    twp->btimer = num;
  }
}

// phase goes to the flash particle's phase, read only by the displayer
void CreateFlashNumPhase(Float x, Float y, Float z, Float scl, Uint8 num,
                         Float phase) {
  NJS_POINT3 pos;
  task *tp;

  if (lbl_803ADC30 != NULL) {
    pos.x = x;
    pos.y = y;
    pos.z = z;
    lbl_803ADC30(&pos, _rename_flash_light2, scl, 0.0f);
  }
  tp = CreateElementalTask(IM_TWK, LEV_3, FlashManager, "FlashManager");
  if (tp != NULL) {
    taskwk *twp = tp->twp;

    twp->pos.x = x;
    twp->pos.y = y;
    twp->pos.z = z;
    twp->scl.x = scl;
    twp->btimer = num;
    twp->scl.y = phase;
  }
}
