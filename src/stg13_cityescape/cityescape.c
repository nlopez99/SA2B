#include "sa2b_types.h"
#include "samt/sonic/adx.h"
#include "samt/sonic/file_ctl.h"
#include "samt/sonic/light.h"
#include "samt/sonic/player.h"
#include "samt/sonic/task.h"
#include "set.h"

extern void CityEscapeBgExec(task *tp);
extern void CityEscapeBgExecDest(task *tp);
extern void CityEscapeBgExecDisp(task *tp);
extern void CityEscapeBgExecDispSort(task *tp);
extern void fn_80011A04(u32);
extern void fn_80011DF4(void);
extern s32 fn_8001CD40(void);
extern void fn_8002506C(u32, float *);
extern void fn_8003D7D4(mtnwk *mp);
extern void fn_8005523C(taskwk *tp, motionwk *mp, playerwk *pwp);
extern void fn_8005D0EC(void *);

typedef struct unk_struct {
  s16 unk_0x0, unk_0x2;
  void *unk_0x4;
} unk_struct_t;

extern unk_struct_t lbl_801E5D28[];
extern void *lbl_13_data_200DF0[];

// ^ extern
// v in this file

void CityEscapeProlog();
void CityEscapeEpilog();
void CityEscapeExec(task *tp);
void CityEscapeBgmStart(task *tp);

float lbl_13_data_202228[16] = {0.3f,  0.1f, 1.0f, 1.0f, 0.45f, 1.0f,
                                1.0f,  1.0f, 0.3f, 0.1f, 1.0f,  0.5f,
                                0.43f, 1.0f, 1.0f, 1.0f};
SUBPRG_HEADER _rename_CityEscapeSubprgHeader = {
    "STG13   ", 0, {CityEscapeProlog, CityEscapeEpilog, CityEscapeExec}};
unk_struct_t lbl_13_data_1FD2B0[57];
s32 lbl_13_bss_80 = 0;

void fn_13_116C4(void) {
  unk_struct_t *s, *table = lbl_13_data_1FD2B0, *entry;
  for (s = table;; s++) {
    if (s->unk_0x0 == -1)
      break; // this feels fake as hell but i have zero clue what it could be
    if (s->unk_0x0 < 0 || s->unk_0x0 >= 300)
      continue;
    entry = &lbl_801E5D28[s->unk_0x0];
    if (entry->unk_0x4 == NULL) {
      void *ptr = s->unk_0x4;
      entry->unk_0x2 = s->unk_0x2;
      lbl_801E5D28[s->unk_0x0].unk_0x4 = ptr;
    }
  }
}

void fn_13_11738(void) {
  unk_struct_t *s, *table = lbl_13_data_1FD2B0, *entry;
  playerwk *pwp = playerpwp[0];
  if (pwp) {
    fn_8003D7D4(&pwp->m);
    playerpwp[pwp->player]->m.flag |= 4;
    fn_8005523C(playertwp[0], playermwp[0], pwp);
  }
  pwp = playerpwp[1];
  if (pwp) {
    fn_8003D7D4(&pwp->m);
    playerpwp[pwp->player]->m.flag |= 4;
    fn_8005523C(playertwp[1], playermwp[1], pwp);
  }

  for (s = table;; s++) {
    if (s->unk_0x0 == -1)
      break; // see above comment
    if (s->unk_0x0 < 0 || s->unk_0x0 >= 300)
      continue;
    entry = &lbl_801E5D28[s->unk_0x0];
    if (entry->unk_0x4 == s->unk_0x4) {
      lbl_801E5D28[s->unk_0x0].unk_0x4 = NULL;
      lbl_801E5D28[s->unk_0x0].unk_0x2 = 0;
    }
  }
}

void _prolog() { _rename_LoadSubprogram(&_rename_CityEscapeSubprgHeader); }

void _epilog() {}

void CityEscapeProlog() {
  task *bgexec;
  if (lbl_801CC168.unk_0x0 == 0 && lbl_801CC168.TWO_PLAYER) {
    CreateElementalTask(2, LEV_0, CityEscapeBgmStart, "BgmStart");
  }
  // +0x274
  fn_8002506C(0, lbl_13_data_202228);
  fn_8002506C(1, &lbl_13_data_202228[8]);
  fn_8002506C(2, lbl_13_data_202228);
  fn_8002506C(3, lbl_13_data_202228);
  // +0x334
  LoadLightFile("stg13_light.bin");
  if (lbl_801CC168.unk_0x0 == 0) {
    if (!lbl_801CC168.TWO_PLAYER) {
      fn_80011A04(0);
      BGM_SetFile("c_escap1.adx");
    }
  } else {
    fn_80011A04(0);
    BGM_SetFile("t9_sonic.adx"); // this file does not exist
  }
  fn_8005D0EC(lbl_13_data_200DF0);
  bgexec = CreateElementalTask(2, LEV_1, CityEscapeBgExec, "BgExec");
  if (bgexec) {
    // idk why it's like this and bgexec's ctor doesn't do it
    bgexec->disp = CityEscapeBgExecDisp;
    bgexec->disp_sort = CityEscapeBgExecDispSort;
    bgexec->dest = CityEscapeBgExecDest;
  }
}

void CityEscapeEpilog() {}

// TODO this might be a TU split? not sure
void CityEscapeBgmStart(task *tp) {
  if (fn_8001CD40() == 7) {
    fn_80011DF4();
    lbl_13_bss_80 = 0;
  } else if (fn_8001CD40() == 16) {
    if (playertwp[0]->mode == 54 && playertwp[1]->mode == 54) {
      fn_80011DF4();
      lbl_13_bss_80 = 0;
    } else if (lbl_13_bss_80++ == 9) {
      fn_80011A04(0);
      BGM_SetFile("c_escap1.adx");
    } else if (lbl_13_bss_80 < 9) {
      fn_80011DF4();
    }
  }
}

void CityEscapeExec(task *tp) {}
