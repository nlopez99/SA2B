#ifndef _FIELD_H_
#define _FIELD_H_

#include "sa2b_types.h"
#include "samt/ninja/njcommon.h"
#include "samt/sonic/task.h"

// a field task is asked for its contribution at a position and ORs its flags in
#define FIELD_DIR (1 << 0) // 'dir' is set, GetFieldInfo normalises it
#define FIELD_VEL (1 << 1) // 'vel' is set, the caller drifts towards it
#define FIELD_KILL (1 << 3) // the caller destroys itself

typedef struct fieldinfo // sizeof=0x1C
{
  /* 0x00 */ Sint32 flag;
  /* 0x04 */ NJS_VECTOR dir; // gravity/down direction, normalised
  /* 0x10 */ NJS_VECTOR vel; // flow velocity
} fieldinfo;

typedef void (*field_func)(task *tp, NJS_POINT3 *pos, fieldinfo *info);

void FieldEntry(task *tp, field_func func);
void FieldExit(task *tp);
void GetFieldInfo(NJS_POINT3 *pos, fieldinfo *info);

#endif // !_FIELD_H_
