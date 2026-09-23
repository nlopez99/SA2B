#include "sonic/fog.h"

#include "stl/string.h"

extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern void njMemCopy(void *dst, const void *src, Sint32 size);

extern void fn_80119778(void *p, Sint32 c, Sint32 size); // memset
extern Sint32 fn_8001A16C(const char *fname, void *buf); // load a file
extern Uint32 fn_800334B0(Uint32 col0, Uint32 col1, Float ratio);
extern void fn_8011C2C8(Uint32 color);           // fog color
extern void fn_8011C2FC(Float near, Float far);  // fog range
extern void fn_8011C308(Sint32 type);            // GX fog type

// the loader keeps the name of the fog file it saw last here
extern char lbl_802B7000[0x20];

// FogWorkP: what the rest of the game reads the current fog through
extern FOG_WORK *lbl_803ADBC8;

// ^ extern
// v in this file

// a fog file is read into this fixed buffer before being copied to the caller
#define FOG_LOAD_BUF ((void *)0x80FFFE60)

// far value that means no fog
#define FOG_FAR_NONE 100000.0f

// GX fog types; a mode-less fog file gets GX_FOG_EXP2
#define FOG_GX_NONE 0
#define FOG_GX_EXP2 5

static task *FogTask;

// one FOG_WORK, addressed as separate objects; declared in reverse address
// order, keep it
task *FogManTask;
Sint32 FogWorkLock;
Float FogWorkRatio;
FOG_DATA *FogWorkB;
FOG_DATA *FogWorkA;

// FOG_MD_* -> GX fog type
static Sint32 fog_mode_tbl[6] = {0, 2, 4, 5, 6, 7};

void FogTaskExec(task *tp);
void FogTaskDest(task *tp);
void FogtaskManExec(task *tp);
void FogtaskManDest(task *tp);

void InitFogData(FOG_DATA *pFog) {
  fn_80119778(pFog, 0, sizeof(FOG_DATA));
  pFog->far = FOG_FAR_NONE;
}

Sint32 LoadFogFile(const char *fname, FOG_DATA *pFog) {
  strcpy(lbl_802B7000, fname);

  if (fn_8001A16C(fname, FOG_LOAD_BUF) == 0) {
    njMemCopy(pFog, FOG_LOAD_BUF, sizeof(FOG_DATA));
    return 1;
  }

  InitFogData(pFog);
  return 0;
}

void SetFog(FOG_DATA *pFog) {
  if (pFog->far >= FOG_FAR_NONE) {
    fn_8011C308(FOG_GX_NONE);
    fn_8011C2FC(-10.0f, pFog->far);
    fn_8011C2C8((pFog->color << 8) | (pFog->color >> 24));
    njDisableFog();
    return;
  }

  if (pFog->mode & 2) {
    fn_8011C308(fog_mode_tbl[(pFog->mode >> 16) & 0xF]);
    fn_8011C2FC(pFog->near, pFog->far);
    fn_8011C2C8((pFog->color << 8) | (pFog->color >> 24));
    njEnableFog();
    gjSetFog();
    return;
  }

  fn_8011C308(FOG_GX_EXP2);
  fn_8011C2FC(-10.0f, pFog->far);
  fn_8011C2C8((pFog->color << 8) | (pFog->color >> 24));
  njEnableFog();
  gjSetFog();
}

void SetMultiFog(FOG_DATA *pFogA, FOG_DATA *pFogB, Float ratio) {
  Sint32 color;
  Uint32 far;

  // neither end of the blend has fog: nothing to interpolate
  if (pFogA->far >= FOG_FAR_NONE && pFogB->far >= FOG_FAR_NONE) {
    fn_8011C308(FOG_GX_NONE);
    fn_8011C2FC(-10.0f, pFogA->far);
    fn_8011C2C8((pFogA->color << 8) | (pFogA->color >> 24));
    njDisableFog();
    return;
  }

  if (ratio > 1.0f) {
    ratio = 1.0f;
  }

  if (ratio < 0.0f) {
    ratio = 0.0f;
  }

  color = fn_800334B0(pFogA->color, pFogB->color, ratio);
  far = pFogA->far + ratio * (pFogB->far - pFogA->far);

  if (pFogA->mode & 2) {
    fn_8011C308(fog_mode_tbl[(pFogA->mode >> 16) & 0xF]);
    fn_8011C2FC(pFogA->near + ratio * (pFogB->near - pFogA->near), far);
    fn_8011C2C8(((color << 8) & 0xFFFFFF00) | ((Uint32)color >> 24));
    njEnableFog();
    gjSetFog();
    return;
  }

  fn_8011C308(FOG_GX_EXP2);
  fn_8011C2FC(-50.0f, far);
  fn_8011C2C8(((color << 8) & 0xFFFFFF00) | ((Uint32)color >> 24));
  njEnableFog();
  gjSetFog();
}

// pushes the fog work to the hardware once a frame
void FogTaskExec(task *tp) {
  if (FogTask != tp) {
    return;
  }

  if (FogWorkLock) {
    return;
  }

  FogWorkLock = 1;

  if (FogWorkA != NULL) {
    if (FogWorkB != NULL) {
      SetMultiFog(FogWorkA, FogWorkB, FogWorkRatio);
    } else {
      SetFog(FogWorkA);
    }
  }

  FogWorkLock = 0;
}

void FogTaskDest(task *tp) {
  if (tp == FogTask) {
    FogTask = NULL;
  }
}

// the manager task: keeps exactly one fog task alive under it
void FogtaskManExec(task *tp) {
  if (FogTask != NULL) {
    return;
  }

  FogWorkLock = 1;
  FogTask = CreateElementalTask(0, LEV_1, FogTaskExec, "FogTaskExec");

  if (FogTask != NULL) {
    FogTask->dest = FogTaskDest;
  }

  FogWorkLock = 0;
}

void FogtaskManDest(task *tp) {
  FogWorkLock = 1;

  if (FogTask != NULL) {
    DestroyTask(FogTask);
    FogTask = NULL;
  }

  if (FogManTask == tp) {
    fn_8011C308(FOG_GX_NONE);
    fn_8011C2FC(-10.0f, -100000.0f);
    fn_8011C2C8(0xFFFFFFFF);
    njDisableFog();
    njDisableFog();
    gjSetFog();
    lbl_803ADBC8 = NULL;
    FogManTask = NULL;
  }

  FogWorkLock = 0;
}

void FreeFogManager(void) {
  if (FogManTask != NULL) {
    DestroyTask(FogManTask);
    FogManTask = NULL;
    lbl_803ADBC8 = NULL;
  }

  fn_8011C308(FOG_GX_NONE);
  fn_8011C2FC(-10.0f, -100000.0f);
  fn_8011C2C8(0xFFFFFFFF);
  njDisableFog();
  njDisableFog();
  gjSetFog();
}

// stages a fog blend for the next frame; dead in most stages, kept by a few
void SetMultiFogWork(FOG_DATA *pFogA, FOG_DATA *pFogB, Float ratio) {
  FogWorkA = pFogA;
  FogWorkB = pFogB;
  FogWorkRatio = ratio;
  FogWorkLock = 0;
}

void CreateFogManagerEx(const char *fname, FOG_DATA *pFog) {
  if (FogManTask != NULL) {
    return;
  }

  FogWorkLock = 1;
  FogWorkA = pFog;
  FogWorkB = NULL;
  lbl_803ADBC8 = (FOG_WORK *)&FogWorkA;

  LoadFogFile(fname, pFog);
  njDisableFog();
  gjSetFog();

  FogManTask = CreateElementalTask(0, LEV_1, FogtaskManExec, "FogtaskManExec");

  if (FogManTask != NULL) {
    FogManTask->dest = FogtaskManDest;
  }

  FogWorkLock = 0;
}
