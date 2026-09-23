#ifndef _SONIC_FOG_H_
#define _SONIC_FOG_H_

#include "sa2b_types.h"
#include "samt/sonic/task.h"

// fog density mode, stored in bits 16..19 of FOG_DATA::mode
enum {
  FOG_MD_OFF,
  FOG_MD_LINEAR,
  FOG_MD_EXPONENT,
  FOG_MD_EXPONENT2,
  FOG_MD_REVEXPONENT,
  FOG_MD_REVEXPONENT2,
};

// a whole fog file: the header is followed by the 128 entry density table
typedef struct fogdata // sizeof=0x210
{
  /* 0x000 */ Uint32 mode;
  /* 0x004 */ Uint32 color;
  /* 0x008 */ Float far;
  /* 0x00C */ Float near;
  /* 0x010 */ Float fogtbl[128];
} FOG_DATA;

// what the fog task reads every frame; pFogB set means interpolate A -> B
typedef struct fogwork // sizeof=0x10
{
  /* 0x00 */ FOG_DATA *pFogA;
  /* 0x04 */ FOG_DATA *pFogB;
  /* 0x08 */ Float ratio;
  /* 0x0C */ Sint32 lock;
} FOG_WORK;

// the live fog work; the four together form one FOG_WORK
extern FOG_DATA *FogWorkA;
extern FOG_DATA *FogWorkB;
extern Float FogWorkRatio;
extern Sint32 FogWorkLock;

extern task *FogManTask;

// returns 1 on success, 0 on failure (pFog is reset to "no fog" on failure)
Sint32 LoadFogFile(const char *fname, FOG_DATA *pFog);

void SetFog(FOG_DATA *pFog);
void SetMultiFog(FOG_DATA *pFogA, FOG_DATA *pFogB, Float ratio);

void SetMultiFogWork(FOG_DATA *pFogA, FOG_DATA *pFogB, Float ratio);

void CreateFogManagerEx(const char *fname, FOG_DATA *pFog);
void FreeFogManager(void);

#endif // !_SONIC_FOG_H_
