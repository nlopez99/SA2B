#include "field.h"

#include "samt/ninja/njmath.h"

// the list is threaded through the slots a field task never uses itself:
// 'mwp' holds the callback, 'awp' the next entry
#define GetFieldFunc(tp) ((field_func)(tp)->mwp)
#define GetFieldNext(tp) ((task *)(tp)->awp)

extern task *_rename_field_list;

// ^ extern
// v in this file

void FieldEntry(task *tp, field_func func) {
  if (tp->awp != NULL) {
    for (;;) {
    }
  }
  tp->awp = (anywk *)_rename_field_list;
  tp->mwp = (motionwk *)func;
  _rename_field_list = tp;
}

void FieldExit(task *tp) {
  task *cur;
  task *prev;

  cur = _rename_field_list;
  prev = NULL;
  for (; cur != NULL; cur = GetFieldNext(cur)) {
    if (cur == tp) {
      if (cur == _rename_field_list) {
        _rename_field_list = GetFieldNext(cur);
      } else {
        prev->awp = cur->awp;
      }
      break;
    }
    prev = cur;
  }

  tp->awp = NULL;
  tp->mwp = NULL;
}

void GetFieldInfo(NJS_POINT3 *pos, fieldinfo *info) {
  task *tp;
  Float *p;
  Float len;
  Float scl;

  info->flag = 0;
  info->vel.x = 0.0f;
  info->vel.y = 0.0f;
  info->vel.z = 0.0f;
  info->dir = info->vel;

  for (tp = _rename_field_list; tp != NULL; tp = GetFieldNext(tp)) {
    GetFieldFunc(tp)(tp, pos, info);
  }

  if (info->flag & FIELD_DIR) {
    len = njScalor(&info->dir);
    if (len > 0.0001f) {
      scl = 1.0f / len;
      p = &info->dir.x;
      *p++ *= scl;
      *p++ *= scl;
      *p *= scl;
    }
  }
}
