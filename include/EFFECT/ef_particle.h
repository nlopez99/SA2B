#ifndef _EF_PARTICLE_H_
#define _EF_PARTICLE_H_

#include "sa2b_types.h"
#include "stdlib.h"

typedef struct particle      particle;
typedef struct particle_init particle_init;
typedef struct particle_info particle_info;

// returns FALSE once the particle is dead
typedef Bool (*particle_exec)(particle_info *info, particle *p);

struct particle // sizeof=0x40
{
  /* 0x00 */ Angle      ang;
  /* 0x04 */ Float      scl;
  /* 0x08 */ NJS_POINT3 pos;
  /* 0x14 */ Uint32     argb;
  /* 0x18 */ Float      frame;
  /* 0x1C */ Angle      ang2;
  /* 0x20 */ NJS_VECTOR spd;
  /* 0x2C */ Angle      ang_spd;
  /* 0x30 */ particle  *next;
  /* 0x34 */ Uint32     flag;
  /* 0x38 */ union {
    Angle ang;
    Float f;
  } phase;
  /* 0x3C */ particle  *next_free;
};

// what fn_80032B78 copies into a fresh particle (all but next_free)
struct particle_init // sizeof=0x3C
{
  /* 0x00 */ Angle      ang;
  /* 0x04 */ Float      scl;
  /* 0x08 */ NJS_POINT3 pos;
  /* 0x14 */ Uint32     argb;
  /* 0x18 */ Float      frame;
  /* 0x1C */ Angle      ang2;
  /* 0x20 */ NJS_VECTOR spd;
  /* 0x2C */ Angle      ang_spd;
  /* 0x30 */ particle  *next;
  /* 0x34 */ Uint32     flag;
  /* 0x38 */ Float      phase;
};

struct particle_info // sizeof=0x38
{
  /* 0x00 */ Sint32         type;
  /* 0x04 */ NJS_TEXLIST   *texlist;
  /* 0x08 */ Uint32         frame_start;
  /* 0x0C */ Uint32         frame_num;
  /* 0x10 */ Float          frame_spd;
  /* 0x14 */ Float          friction;
  /* 0x18 */ Float          gravity;
  /* 0x1C */ Float          scl_spd;
  /* 0x20 */ particle_exec  exec;
  /* 0x24 */ Float          clip_dist;
  /* 0x28 */ particle_init *init;
  /* 0x2C */ Sint32         unk2C;
  /* 0x30 */ particle      *head;
  /* 0x34 */ particle_info *next;
};

#define ParticleRandom() (0.000030517578f * (Float)rand())
#define ParticleDegAng(n) ((Angle)(182.04445f * (n)))

// takes a particle from the pool, fills it from info->init and links it to info
extern particle *fn_80032B78(particle_info *info);

#endif // !_EF_PARTICLE_H_
