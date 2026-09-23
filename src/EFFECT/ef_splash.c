#include "EFFECT/ef_splash.h"

#include "fabsf.h"
#include "samt/ninja/njmatrix.h"
#include "samt/sonic/game.h"
#include "samt/sonic/player.h"
#include "samt/sonic/task.h"

extern particle_info _rename_splash_info;
extern particle_info _rename_splash_ring_info;
extern particle_info _rename_splash_drop_info;

extern Sint32 _rename_GetStageNum(void);
extern void fn_8006B7EC(Sint32 tone, void *id, Sint32 pri, Sint32 volofs,
                        NJS_POINT3 *pos);

// ^ extern
// v in this file

// the water tint of the ring particles, picked once from the stage number
static Uint32 splash_ring_argb = 0xFF404458;
// one splash task per player
static task *player_splash_tp[2] = {NULL, NULL};

particle *CreateSplash(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_splash_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(80.0f * ParticleRandom() - 40.0f);
    p->ang_spd = ParticleDegAng(0.5f * ParticleRandom() - 0.25f);
    p->frame = 0.0f;
  }
  return p;
}

particle *CreateSplashDrop(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_splash_drop_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(80.0f * ParticleRandom() - 40.0f);
    p->ang_spd = ParticleDegAng(0.5f * ParticleRandom() - 0.25f);
    p->frame = 2.0f * ParticleRandom();
    p->argb = 0xFFA090FF;
  }
  return p;
}

// the ring left on the water; the only one in the stage's water colour
static particle *CreateSplashRing(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl) {
  particle *p = fn_80032B78(&_rename_splash_ring_info);
  if (p != NULL) {
    p->pos = *pos;
    p->spd = *spd;
    p->scl = scl;
    p->ang = ParticleDegAng(80.0f * ParticleRandom() - 40.0f);
    p->ang_spd = ParticleDegAng(0.5f * ParticleRandom() - 0.25f);
    p->frame = 0.0f;
    p->argb = splash_ring_argb;
  }
  return p;
}

Bool SplashExec(particle_info *info, particle *p) {
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

// num rings evenly spaced round a circle, each nudged a random part of a step
static void CreateSplashRingCircle(NJS_POINT3 *pos, NJS_VECTOR *spd,
                                   Float radius, Float scl, Sint32 num) {
  NJS_VECTOR v = {0.0f, 0.03f, 0.07f};
  NJS_POINT3 p = {0.0f, 0.0f, 0.0f};
  NJS_POINT3 ring_pos;
  NJS_VECTOR ring_spd;
  Uint8 i;
  Angle step = 0x10000 / num;
  Angle ang = 0;

  p.z = radius;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  v.y *= scl;
  v.z *= scl;
  for (i = 0; i < num; i++) {
    ang = (Angle)((Float)ang + 0.5f * ((Float)step * ParticleRandom()));
    njRotateY(NULL, ang);
    njCalcPoint(NULL, &p, &ring_pos);
    njCalcVector(NULL, &v, &ring_spd);
    ring_pos.x += pos->x;
    ring_pos.y += pos->y;
    ring_pos.z += pos->z;
    ring_spd.x += spd->x;
    ring_spd.y += spd->y;
    ring_spd.z += spd->z;
    CreateSplashRing(&ring_pos, &ring_spd, scl);
    ang += step;
  }
  njPopMatrixEx();
}

static void PlayerSplashDest(task *tp);
static void PlayerSplashExec(task *tp);

// one per player; pno rides in btimer
task *CreatePlayerSplash(Sint32 pno, task *ptp) {
  task *tp;
  task **slot;

  // nested to match
  if (pno < 2 && *(slot = &player_splash_tp[pno]) == NULL) {
    switch (_rename_GetStageNum()) {
    case STAGE_COALMINE:
      splash_ring_argb = 0xFF404458;
      break;
    case STAGE_JUNGLE:
      splash_ring_argb = 0xFF182928;
      break;
    default:
      splash_ring_argb = 0xFF8080A0;
      break;
    }
    if (ptp != NULL) {
      tp = CreateChildTask(IM_TWK, PlayerSplashExec, ptp);
    } else {
      // the original's name for PlayerSplashExec, keep it
      tp = CreateElementalTask(IM_TWK, LEV_1, PlayerSplashExec, "SplashExec");
    }
    if (tp != NULL) {
      taskwk *twp = tp->twp;

      tp->dest = PlayerSplashDest;
      twp->btimer = (Uint8)pno;
      *slot = tp;
      return tp;
    }
  }
  return NULL;
}

static void PlayerSplashDest(task *tp) {
  taskwk *twp = tp->twp;
  task **slot = &player_splash_tp[twp->btimer];

  if (*slot == tp) {
    *slot = NULL;
  }
}

static Float splash_ring_scl = 2.1f;
static Float splash_ring_spd_y = 0.15f;
static Float splash_ring_spd_y_max = 0.25f;
static Float splash_ring_radius = 1.4f;
static Float splash_ring_radius_scl = 0.3f;

static void PlayerSplashExec(task *tp) {
  taskwk *twp = tp->twp;
  taskwk *ptwp;
  Sint32 pno;
  Float nframe;
  Float radius;

  pno = twp->btimer;
  if ((ptwp = playertwp[pno]) == NULL) {
    DestroyTask(tp);
    return;
  }
  nframe = playerpwp[pno]->m.nframe;
  if ((playerpwp[pno]->shadow.Attr_top & 0x2002) &&
      playerpwp[pno]->shadow.y_top - playerpwp[pno]->shadow.y_bottom < 3.0f) {
    // the frame the player broke the surface on: one big crown
    if ((ptwp->flag & (twp->flag ^ ptwp->flag) & 1) && twp->wtimer > 12) {
      NJS_VECTOR spd;
      NJS_POINT3 pos;

      spd = playermwp[pno]->spd;
      spd.x *= 0.3f;
      spd.y = splash_ring_spd_y * twp->scl.y;
      spd.z *= 0.3f;
      radius = splash_ring_radius + splash_ring_radius_scl * twp->scl.y;
      if (spd.y > splash_ring_spd_y_max) {
        spd.y = splash_ring_spd_y_max;
        radius += spd.y - splash_ring_spd_y_max;
      }
      pos = ptwp->pos;
      pos.y = playerpwp[pno]->shadow.y_top;
      CreateSplashRingCircle(&pos, &spd, radius, splash_ring_scl, 12);
      twp->wtimer = 0;
      fn_8006B7EC(0x7000, NULL, 0, 0, &pos);
    }
    // running: the run cycle plants a foot at frame 0 and again at frame 15
    if ((ptwp->flag & 1) &&
        (ptwp->mode == 1 || ptwp->mode == 0x19 || ptwp->mode == 0xE ||
         ptwp->mode == 0x20) &&
        playerpwp[pno]->spd.x >= 0.1f &&
        ((nframe >= 0.0f && twp->scl.x > nframe) ||
         (nframe >= 15.0f && twp->scl.x < 15.0f && twp->scl.x < nframe)) &&
        twp->wtimer > 4) {
      NJS_VECTOR spd;
      NJS_POINT3 pos;

      spd = playermwp[pno]->spd;
      spd.x *= 0.3f;
      spd.y = splash_ring_spd_y + 0.002f * (9.0f + ParticleRandom());
      spd.z *= 0.3f;
      radius = splash_ring_radius;
      pos = ptwp->pos;
      pos.y = playerpwp[pno]->shadow.y_top;
      CreateSplashRingCircle(&pos, &spd, radius, splash_ring_scl, 5);
      twp->wtimer = 0;
      fn_8006B7EC(0x7000, NULL, 0, 0, &pos);
    }
    // walking: one ring per cycle
    else if ((ptwp->flag & 1) && (ptwp->mode == 0xC || ptwp->mode == 0x1B) &&
             nframe >= 0.0f && twp->scl.x > nframe && twp->wtimer > 4) {
      NJS_VECTOR spd;
      NJS_POINT3 pos;

      spd = playermwp[pno]->spd;
      spd.x *= 0.3f;
      spd.y = 0.1f;
      spd.z *= 0.3f;
      radius = splash_ring_radius;
      pos = ptwp->pos;
      pos.y = playerpwp[pno]->shadow.y_top;
      CreateSplashRingCircle(&pos, &spd, radius, 2.1f, 6);
      twp->wtimer = 0;
      fn_8006B7EC(0x7000, NULL, 0, 0, &pos);
    }
  }
  twp->flag = ptwp->flag;
  twp->scl.y = fabsf(playermwp[pno]->spd.y);
  twp->wtimer++;
  twp->scl.x = nframe;
}
