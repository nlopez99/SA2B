#include "sa2b_types.h"
#include "samt/sonic/task.h"
#include "set.h"

static task *current_bg_tp = NULL;

void CityEscapeBgExec(task *tp) {}

void CityEscapeBgExecDest(task *tp) {
  if (current_bg_tp == tp) {
    current_bg_tp = NULL;
  }
}

void CityEscapeBgExecDisp(task *tp) {}

void CityEscapeBgExecDispSort(task *tp) {}
