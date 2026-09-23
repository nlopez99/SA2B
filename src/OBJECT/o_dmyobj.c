#include "OBJECT/o_dmyobj.h"

#include "samt/ninja/njchunk.h"
#include "samt/ninja/njdef.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"
#include "samt/shinobi/sg_maloc.h"
#include "samt/sonic/njctrl.h"
#include "samt/sonic/player.h"
#include "set.h"
#include "types.h"

// request block, answered through 'ack'
typedef struct dmyobjcmd // sizeof=0x20
{
  /* 0x00 */ Uint32 req; // (id << 16) | command
  /* 0x04 */ Uint32 ack;
  /* 0x08 */ Uint32 nbtex;
  /* 0x0C */ Uint32 disp;
  /* 0x10 */ Uint32 unk_10;
  /* 0x14 */ Uint32 hold;
  /* 0x18 */ Uint32 busy;
  /* 0x1C */ Uint32 unk_1C;
} dmyobjcmd;

// transfer buffer, size first
typedef struct dmyobjbuf {
  /* 0x00 */ Uint32 size;
  /* 0x04 */ Uint8 data[1];
} dmyobjbuf;

// read by fn_80073028, which rewrites the UVs of a model every few frames;
// arrives with its four inner pointers stored as offsets
typedef struct uvanim_info // sizeof=0x24
{
  /* 0x00 */ Uint32 flag;
  /* 0x04 */ Uint32 frame_num;
  /* 0x08 */ Uint32 frame_time;
  /* 0x0C */ void *unkC;
  /* 0x10 */ void *uv;
  /* 0x14 */ void *unk14;
  /* 0x18 */ void *unk18;
  /* 0x1C */ Uint32 unk1C;
  /* 0x20 */ Uint32 unk20;
} uvanim_info;

// one entry of the texture memory list fn_801184C4 wires into a texlist
typedef struct dmyobjtexmem // sizeof=0x20
{
  /* 0x00 */ Uint8 unk_00[0x20];
} dmyobjtexmem;

extern dmyobjcmd lbl_801CD8B0;
extern void *lbl_803AD588;  // transfer buffer
extern Sint32 lbl_803ADC08; // DMYOBJs alive in the stage

extern void njMemCopy(void *dst, const void *src, Uint32 size);
extern void fn_800155E0(const NJS_POINT3 *st, const NJS_POINT3 *ed);
extern void fn_80073028(NJS_CNK_MODEL *model, void *info, Uint32 frame);
extern void fn_801166F0(NJS_TEXLIST *texlist);
extern void fn_801184C4(NJS_TEXLIST *texlist, NJS_TEXNAME *names,
                        dmyobjtexmem *mem, Uint32 nbtex);
extern void fn_80118FC0(void *pvm, NJS_TEXLIST *texlist);
extern void *fn_80119284(void *nj, Uint32 *offset, Uint32 *magic);
extern void fn_8011E17C(NJS_CNK_OBJECT *object);

// ^ extern
// v in this file

static void ObjectDmyObjDest(task *tp);
static void ObjectDmyObjExec(task *tp);
static void ObjectDmyObjDisp(task *tp);

#define DMYOBJ_TEX 0
#define DMYOBJ_MODEL 1
#define DMYOBJ_UVANIM 2

typedef struct dmyobjnode // sizeof=0x20
{
  /* 0x00 */ struct dmyobjnode *next;
  /* 0x04 */ Uint32 id;
  /* 0x08 */ Sint32 kind;
  /* 0x0C */ union {
    Uint32 magic;        // DMYOBJ_MODEL
    Uint32 nbtex;        // DMYOBJ_TEX
    uvanim_info *uvanim; // DMYOBJ_UVANIM
  } u;
  /* 0x10 */ union {
    NJS_CNK_OBJECT *object; // DMYOBJ_MODEL
    NJS_TEXNAME *names;     // DMYOBJ_TEX
  } p;
  /* 0x14 */ NJS_TEXLIST texlist;
  /* 0x1C */ dmyobjtexmem *texmem;
} dmyobjnode;

// received nodes, oldest first; drawn in that order
static dmyobjnode *dmyobj_list;

// releases whatever 'nd' owns, frees it, and hands back the rest of the list
#define FreeNodeBody(nd, nx)                                                   \
  (nx) = (nd)->next;                                                           \
  switch ((nd)->kind) {                                                        \
  case DMYOBJ_MODEL:                                                           \
    syFree((nd)->p.object);                                                    \
    break;                                                                     \
  case DMYOBJ_TEX:                                                             \
    fn_801166F0(&(nd)->texlist);                                               \
    syFree((nd)->texmem);                                                      \
    syFree((nd)->p.names);                                                     \
    break;                                                                     \
  case DMYOBJ_UVANIM:                                                          \
    syFree((nd)->u.uvanim);                                                    \
    break;                                                                     \
  }                                                                            \
  syFree(nd);

static dmyobjnode *DmyObjFreeNode(dmyobjnode *nd) {
  dmyobjnode *nx;

  FreeNodeBody(nd, nx);
  return nx;
}

static dmyobjnode *DmyObjAddNode(dmyobjnode *nd) {
  dmyobjnode *nn;
  dmyobjnode *last;

  nn = syCalloc(1, sizeof(dmyobjnode));
  last = dmyobj_list;
  while (last != NULL && last->next != NULL) {
    last = last->next;
  }
  if (last != NULL) {
    last->next = nn;
  } else {
    dmyobj_list = nn;
  }

  *nn = *nd;
  nn->next = NULL;
  return nn;
}

static dmyobjnode *DmyObjReplaceNode(dmyobjnode *nd) {
  dmyobjnode *nn;
  dmyobjnode *cur;
  dmyobjnode *prev;
  Uint32 id;

  nn = NULL;
  id = nd->id;
  if (id != 0) {
    prev = NULL;
    cur = dmyobj_list;
    while (cur != NULL) {
      if (cur->id == id) {
        break;
      }
      prev = cur;
      cur = cur->next;
    }
    if (cur != NULL) {
      nn = syCalloc(1, sizeof(dmyobjnode));
      *nn = *nd;
      nn->next = DmyObjFreeNode(cur);
      if (prev != NULL) {
        prev->next = nn;
      } else {
        dmyobj_list = nn;
      }
    }
  }
  return nn;
}

void ObjectDmyObj(task *tp) {
  taskwk *twp = tp->twp;

  if (!CheckRangeOut(tp)) {
    tp->disp = ObjectDmyObjDisp;
    tp->exec = ObjectDmyObjExec;
    tp->dest = ObjectDmyObjDest;
    twp->smode = 0;
    lbl_801CD8B0.ack = 1;
    lbl_803ADC08++;
  }
}

static void ObjectDmyObjDest(task *tp) {
  lbl_803ADC08--;
  if (lbl_803ADC08 == 0) {
    while (dmyobj_list != NULL &&
           (dmyobj_list = DmyObjFreeNode(dmyobj_list)) != NULL) {
      ;
    }
  }
}

static void ObjectDmyObjExec(task *tp) {
  if (CheckRangeOut(tp)) {
    return;
  }

  if (lbl_801CD8B0.hold != 0) {
    while (dmyobj_list != NULL &&
           (dmyobj_list = DmyObjFreeNode(dmyobj_list)) != NULL) {
      ;
    }
  }

  while (lbl_801CD8B0.hold != 0) {
    Uint32 req;
    Uint32 id;

    lbl_801CD8B0.busy = 1;
    req = lbl_801CD8B0.req;
    id = req >> 16;

    switch (req & 0xFFFF) {
    case 1: {
      NJS_CNK_OBJECT *object;
      Uint32 offset;
      Uint32 magic;
      dmyobjnode nn;

      offset = 0;
      lbl_801CD8B0.ack = 0;
      while ((object = fn_80119284(lbl_803AD588, &offset, &magic)) != NULL) {
        nn.id = id;
        nn.kind = DMYOBJ_MODEL;
        nn.p.object = object;
        nn.u.magic = magic;
        if (DmyObjReplaceNode(&nn) == NULL) {
          DmyObjAddNode(&nn);
        }
      }
      lbl_801CD8B0.req = 0;
      lbl_801CD8B0.ack = 1;
    } break;

    case 2: {
      dmyobjnode *last;
      dmyobjnode nn;

      lbl_801CD8B0.ack = 0;

      // walks to the tail and never uses it, as in the original
      last = dmyobj_list;
      while (last != NULL && last->next != NULL) {
        last = last->next;
      }

      nn.id = id;
      nn.kind = DMYOBJ_TEX;
      nn.u.nbtex = lbl_801CD8B0.nbtex;
      nn.p.names = syCalloc(1, lbl_801CD8B0.nbtex * sizeof(NJS_TEXNAME));
      nn.texmem = syCalloc(1, lbl_801CD8B0.nbtex * sizeof(dmyobjtexmem));
      fn_801184C4(&nn.texlist, nn.p.names, nn.texmem, nn.u.nbtex);
      fn_80118FC0(lbl_803AD588, &nn.texlist);
      if (DmyObjReplaceNode(&nn) == NULL) {
        DmyObjAddNode(&nn);
      }
      lbl_801CD8B0.req = 0;
      lbl_801CD8B0.ack = 1;
    } break;

    case 4: {
      uvanim_info *uvanim;
      Uint8 *data;
      Uint32 size;
      dmyobjnode nn;

      size = ((dmyobjbuf *)lbl_803AD588)->size;
      data = ((dmyobjbuf *)lbl_803AD588)->data;
      lbl_801CD8B0.ack = 0;

      uvanim = syCalloc(1, size);
      njMemCopy(uvanim, data, size);
      if (uvanim->unkC != NULL) {
        uvanim->unkC = (Uint8 *)uvanim->unkC + (Uint32)uvanim;
      }
      if (uvanim->uv != NULL) {
        uvanim->uv = (Uint8 *)uvanim->uv + (Uint32)uvanim;
      }
      if (uvanim->unk14 != NULL) {
        uvanim->unk14 = (Uint8 *)uvanim->unk14 + (Uint32)uvanim;
      }
      if (uvanim->unk18 != NULL) {
        uvanim->unk18 = (Uint8 *)uvanim->unk18 + (Uint32)uvanim;
      }

      nn.id = id;
      nn.kind = DMYOBJ_UVANIM;
      nn.u.uvanim = uvanim;
      if (DmyObjReplaceNode(&nn) == NULL) {
        DmyObjAddNode(&nn);
      }
      lbl_801CD8B0.req = 0;
      lbl_801CD8B0.ack = 1;
    } break;

    case 3: {
      dmyobjnode *nd;
      dmyobjnode *nx;

      STACK_PAD_VAR(3); // unused locals
      lbl_801CD8B0.ack = 0;
      nd = dmyobj_list;
      while (nd != NULL) {
        FreeNodeBody(nd, nx);
        nd = nx;
      }
      dmyobj_list = NULL;
      lbl_801CD8B0.req = 0;
      lbl_801CD8B0.ack = 1;
    } break;
    }
  }
  lbl_801CD8B0.busy = 0;
}

static void ObjectDmyObjDisp(task *tp) {
  taskwk *twp = tp->twp;

  if (twp->scl.x >= 1.0f) {
    NJS_POINT3 st;
    NJS_POINT3 ed;
    Sint32 i;

    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateZ(NULL, twp->ang.z);
    njRotateY(NULL, twp->ang.y);
    njRotateX(NULL, twp->ang.x);

    for (i = 0; i < 20; i++) {
      Float f = 10.0f * i - 100.0f;

      st.y = f;
      st.x = -100.0f;
      st.z = 0.0f;
      ed.y = f;
      ed.x = 100.0f;
      ed.z = 0.0f;
      fn_800155E0(&st, &ed);
    }
    for (i = 0; i < 20; i++) {
      Float f = 10.0f * i - 100.0f;

      st.x = f;
      st.z = -100.0f;
      st.y = 0.0f;
      ed.x = f;
      ed.z = 100.0f;
      ed.y = 0.0f;
      fn_800155E0(&st, &ed);

      st.z = f;
      st.x = -100.0f;
      st.y = 0.0f;
      ed.z = f;
      ed.x = 100.0f;
      ed.y = 0.0f;
      fn_800155E0(&st, &ed);
    }
    njPopMatrixEx();
  }

  if (lbl_801CD8B0.disp != 0) {
    NJS_CNK_OBJECT *last;
    dmyobjnode *nd;

    last = NULL;
    njPushMatrixEx();
    njTranslateEx(&twp->pos);
    njRotateZ(NULL, twp->ang.z);
    njRotateY(NULL, twp->ang.y);
    njRotateX(NULL, twp->ang.x);
    OnControl3D(0x2400);

    for (nd = dmyobj_list; nd != NULL; nd = nd->next) {
      switch (nd->kind) {
      case DMYOBJ_MODEL:
        switch (nd->u.magic) {
        case iff_NJBM:
          break;
        case iff_NJCM:
          fn_8011E17C(nd->p.object);
          last = nd->p.object;
          break;
        }
        break;
      case DMYOBJ_TEX:
        njSetTexture(&nd->texlist);
        break;
      case DMYOBJ_UVANIM:
        if (last != NULL) {
          fn_80073028(last->model, nd->u.uvanim, lbl_801CC168._7C);
        }
        break;
      }
    }

    OffControl3D(0x2400);
    njPopMatrixEx();
  }
}
