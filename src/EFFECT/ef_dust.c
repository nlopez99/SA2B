#include "EFFECT/ef_dust.h"

#include "samt/ninja/njmatrix.h"
#include "samt/sonic/player.h"
#include "samt/sonic/task.h"

extern NJS_TEXLIST _rename_dust_texlist;

// throws num puffs of dust at pno's position, spread over a vector
extern void fn_800BF6A4(Sint32 pno, Sint32 num, NJS_POINT3 *pos,
                        NJS_VECTOR *spd);

// ^ extern
// v in this file

static void BrokenDownSmokeDest(task *tp);
static void BrokenDownSmokeExec(task *tp);

// player joint positions in awp; playerwk in player.h has them 0x18 too low
typedef struct pljointwk {
  /* 0x000 */ Uint8      unused[0x200];
  /* 0x200 */ NJS_POINT3 joint[8];
  /* 0x260 */ Uint8      unused2[0x194];
  /* 0x3F4 */ NJS_POINT3 joint2[2];
} pljointwk;

static particle_init dust_init = {
    0, 1.0f, {0.0f, 0.0f, 0.0f}, 0xB0080810, 0.0f, 0x4000,
};

particle_info dust_info = {
    2,     &_rename_dust_texlist, 0,          8,       0.11f, 0.98f, 0.0065f,
    0.03f, DustExec,              75000.0f,   &dust_init,
};

particle_info dust_short_info = {
    2,     &_rename_dust_texlist, 0,          8,       0.06f, 0.98f, -0.001f,
    0.05f, DustShortExec,         22500.0f,   &dust_init,
};

particle *CreateDust(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&dust_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->phase.f = 16.0f * ParticleRandom();
    p->frame += ParticleRandom();
  }
  return p;
}

particle *CreateDustShort(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&dust_short_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(20.0f * ParticleRandom() - 10.0f);
    p->ang_spd = ParticleDegAng(ParticleRandom() - 0.5f);
    p->phase.f = 16.0f * ParticleRandom();
    p->frame += ParticleRandom();
  }
  return p;
}

// triangle wave: 0..n..0 with a period of 2n
static Sint32 DustTriangleWave(Uint32 x, Uint32 n) {
  Uint32 period = n << 1;
  Sint32 r = x % period;
  if (r >= n) {
    r = period - r;
  }
  return r;
}

Bool DustExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = ParticleDegAng(DustTriangleWave(p->phase.f, 16) - 8) + 0x4000;
  p->phase.f += 0.26666668f;
  if ((Sint16)p->frame >= info->frame_num) {
    p->frame = info->frame_num - 1;
    p->argb = (p->argb & 0x00FFFFFF) | ((p->argb >> 1) & 0xFF000000);
    if (p->argb & 0xFF000000) {
      return TRUE;
    }
    return FALSE;
  }
  return TRUE;
}

Bool DustShortExec(particle_info *info, particle *p) {
  p->ang += p->ang_spd;
  p->scl += info->scl_spd;
  p->pos.x += p->spd.x;
  p->pos.y += p->spd.y;
  p->pos.z += p->spd.z;
  p->spd.x *= info->friction;
  p->spd.y = info->gravity + p->spd.y * info->friction;
  p->spd.z *= info->friction;
  p->frame += info->frame_spd;
  p->ang2 = ParticleDegAng(DustTriangleWave(p->phase.f, 16) - 8) + 0x4000;
  p->phase.f += 0.26666668f;
  if ((Sint16)p->frame >= info->frame_num) {
    p->frame = info->frame_num - 1;
    return FALSE;
  }
  return TRUE;
}

// throws num dust particles outwards from a circle of radius r around pos
void CreateDustRing(NJS_POINT3 *pos, NJS_VECTOR *spd, Float r, Float scl,
                    Sint32 num) {
  NJS_VECTOR v = {0.0f, 0.03f, 0.05f};
  NJS_POINT3 p = {0.0f, 0.0f, 0.0f};
  NJS_POINT3 ppos;
  NJS_VECTOR pspd;
  Uint8 i;
  Angle step = 0x10000 / num;
  Angle ang = 0;

  p.z = r;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  v.y *= scl;
  v.z *= scl;
  for (i = 0; i < num; i++) {
    njRotateY(NULL, ang);
    njCalcPoint(NULL, &p, &ppos);
    njCalcVector(NULL, &v, &pspd);
    ppos.x += pos->x;
    ppos.y += pos->y;
    ppos.z += pos->z;
    pspd.x += spd->x;
    pspd.y += spd->y;
    pspd.z += spd->z;
    CreateDust(&ppos, &pspd, scl);
    ang += step;
  }
  njPopMatrixEx();
}

// as CreateDustRing, but short dust with each spoke nudged by up to half a step
void CreateDustRingShort(NJS_POINT3 *pos, NJS_VECTOR *spd, Float r, Float scl,
                         Sint32 num) {
  NJS_VECTOR v = {0.0f, 0.03f, 0.07f};
  NJS_POINT3 p = {0.0f, 0.0f, 0.0f};
  NJS_POINT3 ppos;
  NJS_VECTOR pspd;
  Uint8 i;
  Angle step = 0x10000 / num;
  Angle ang = 0;

  p.z = r;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  v.y *= scl;
  v.z *= scl;
  for (i = 0; i < num; i++) {
    ang += 0.5f * (step * ParticleRandom());
    njRotateY(NULL, ang);
    njCalcPoint(NULL, &p, &ppos);
    njCalcVector(NULL, &v, &pspd);
    ppos.x += pos->x;
    ppos.y += pos->y;
    ppos.z += pos->z;
    pspd.x += spd->x;
    pspd.y += spd->y;
    pspd.z += spd->z;
    CreateDustShort(&ppos, &pspd, scl);
    ang += step;
  }
  njPopMatrixEx();
}

// puts the generator on the joint mode names; the rest use the right foot
// plus the offset in scl
static void BlackSmokeSetPos(taskwk *twp) {
  pljointwk *pjp = (pljointwk *)playertp[twp->smode]->awp;

  switch (twp->mode) {
  case 0:
    twp->pos = pjp->joint[4];
    break;
  case 3:
    twp->pos = pjp->joint[0];
    break;
  case 4:
    twp->pos = pjp->joint[1];
    break;
  case 7:
    twp->pos = pjp->joint[5];
    break;
  case 8:
    twp->pos = pjp->joint[6];
    break;
  case 9:
    twp->pos = pjp->joint[7];
    break;
  case 10:
    twp->pos = pjp->joint2[0];
    break;
  case 11:
    twp->pos = pjp->joint2[1];
    break;
  default:
    twp->pos = pjp->joint[2];
    njAddVector(&twp->pos, &twp->scl);
    break;
  }
}

static void BlackSmokeGeneratorExec(task *tp) {
  taskwk *twp = tp->twp;
  particle *p; // unused
  NJS_VECTOR spd;
  Sint32 num;

  if (playerpwp[twp->smode] == NULL) {
    DestroyTask(tp);
    return;
  }
  if (lbl_801CC168._37 != 0) {
    return;
  }
  if (twp->mode <= 0) {
    BlackSmokeSetPos(twp);
    spd.x = 0.0f;
    spd.y = 0.45f;
    spd.z = 0.0f;
    CreateDustShort(&twp->pos, &spd, 0.2f);
    num = twp->btimer;
    num--;
    if (num <= 0) {
      FreeTask(tp);
      return;
    }
    twp->btimer = num;
    twp->mode = 8.0f + 24.0f * ParticleRandom();
  } else {
    twp->mode--;
  }
}

// follows pno around puffing black smoke out of a random joint, num times
void CreateBlackSmokeGenerator(Sint32 pno, Sint32 num) {
  task *tp;
  taskwk *twp;
  particle *p; // unused
  NJS_VECTOR spd;
  NJS_POINT3 pos;

  tp = CreateFundamentalTask(IM_TWK, LEV_5, BlackSmokeGeneratorExec);
  if (tp == NULL) {
    return;
  }
  twp = tp->twp;
  twp->smode = pno;
  twp->btimer = num;
  twp->mode = 16.0f * ParticleRandom();
  twp->scl.x = 2.0f * ParticleRandom();
  twp->scl.z = 2.0f * ParticleRandom();
  BlackSmokeSetPos(twp);
  if (ParticleRandom() >= 0.9f) {
    pos = twp->pos;
    spd.x = 0.0f;
    spd.y = 0.5f;
    spd.z = 0.0f;
    fn_800BF6A4(pno, 8, &pos, &spd);
  }
}

// one smoker per player, its player number in btimer
static task *smoke_task[2] = {NULL, NULL};

// smokes harder the less health pno has left
task *CreateBrokenDownSmoke(Sint32 pno, task *ptp) {
  task *tp;
  taskwk *twp;

  if (pno < 2 && smoke_task[pno] == NULL) {
    if (ptp != NULL) {
      tp = CreateChildTask(IM_TWK, BrokenDownSmokeExec, ptp);
    } else {
      tp = CreateFundamentalTask(IM_TWK, LEV_1, BrokenDownSmokeExec);
    }
    if (tp != NULL) {
      twp = tp->twp;
      tp->dest = BrokenDownSmokeDest;
      twp->btimer = pno;
      smoke_task[pno] = tp;
      return tp;
    }
  }
  return NULL;
}

static void BrokenDownSmokeDest(task *tp) {
  taskwk *twp = tp->twp;

  if (smoke_task[twp->btimer] == tp) {
    smoke_task[twp->btimer] = NULL;
  }
}

static void BrokenDownSmokeExec(task *tp) {
  taskwk *twp = tp->twp;
  Sint32 pno = twp->btimer;
  playerwk *pwp = playerpwp[pno];
  Sint32 num;

  if (pwp == NULL) {
    DestroyTask(tp);
    return;
  }
  if (lbl_801CC168._37 != 0) {
    return;
  }
  num = 1.6f * (10.0f - pwp->hp);
  if (num <= 0) {
    return;
  }
  if (lbl_801CC168._7C & 0xF) {
    return;
  }
  if (((lbl_801CC168._7C & 0xFF) >> 4) >= num) {
    return;
  }
  CreateBlackSmokeGenerator(pno, 2);
}
