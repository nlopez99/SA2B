#include "EFFECT/ef_bombfire.h"

#include "samt/ninja/njchunk.h"
#include "samt/ninja/njmatrix.h"
#include "samt/ninja/njtexture.h"

// blends two colors
extern Uint32 fn_800334B0(Uint32 col0, Uint32 col1, Float ratio);

extern void _rename_CnkDrawModelColor2(NJS_CNK_MODEL *model, Uint32 col);

extern NJS_TEXLIST   _rename_bombfire_texlist;
extern NJS_CNK_MODEL _rename_bombfire_model;
extern Uint32        _rename_bombfire_col0;
extern Uint32        _rename_bombfire_col1;

// ^ extern
// v in this file

void _rename_DrawExplosion(Float r, Float rate) {
  Float scl;
  Float model_r;

  // widen the model's radius to cover the scaled ball
  model_r = _rename_bombfire_model.r;
  _rename_bombfire_model.r = 10.0f + r;
  scl = r / model_r;

  njSetTexture(&_rename_bombfire_texlist);
  njPushMatrixEx();
  njScale(NULL, scl, scl, scl);
  _rename_CnkDrawModelColor2(
      &_rename_bombfire_model,
      fn_800334B0(_rename_bombfire_col0, _rename_bombfire_col1, rate));
  njPopMatrixEx();

  _rename_bombfire_model.r = model_r;
}
