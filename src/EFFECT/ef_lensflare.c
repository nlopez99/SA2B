#include "EFFECT/ef_lensflare.h"

#include "samt/ninja/njdraw.h"
#include "samt/sonic/camera.h"

extern void njEnableFog(void);
extern void njDisableFog(void);
extern void gjSetFog(void);
extern void __njColorBlendingMode(Sint32 which, Sint32 mode);
extern void fn_800156FC(Float a, Float r, Float g, Float b);
extern void fn_8011E8E4(NJS_SPRITE *sp, Sint32 n, Float pri, Uint32 attr);

extern Sint32 lbl_803ADAD0; // screen being drawn
extern Sint32 lbl_803ADAD4; // number of screens

extern camposwk *lbl_80175378[4];

extern NJS_TEXLIST _rename_lensflare_texlist;
extern NJS_SPRITE _rename_lensflare_sprite;

// ^ extern
// v in this file

// camera the current screen is drawn through
#define LensFlareCam() (lbl_80175378[lbl_803ADAD0])

// ghost image on the line from the light to the screen centre; ends on tex -1
typedef struct lensflare // sizeof=0x18
{
  /* 0x00 */ Sint32 tex;
  /* 0x04 */ Float dist;
  /* 0x08 */ Float scl;
  /* 0x0C */ Float fade;
  /* 0x10 */ Float alpha;
  /* 0x14 */ Float pri;
} lensflare;

extern lensflare _rename_lensflare_ghost[];

// sun glare for the light at pos; in split screen x and x scale are doubled
void DrawLensFlare(NJS_POINT3 *pos) {
  NJS_POINT2 scr;
  NJS_VECTOR fwd;
  NJS_VECTOR dir;
  NJS_VECTOR v;
  lensflare *e;
  Float dot;
  Float dy;
  Float dim;
  Float glow;
  Float scl;
  Float raw;
  Float sun;
  STACK_PAD_VAR(2);

  dir.x = pos->x - LensFlareCam()->pos.x;
  dir.y = pos->y - LensFlareCam()->pos.y;
  dir.z = pos->z - LensFlareCam()->pos.z;
  njUnitVector(&dir);
  v.x = 0.0f;
  v.y = 0.0f;
  v.z = -1.0f;
  njPushMatrixEx();
  njUnitMatrix(NULL);
  njRotateY(NULL, LensFlareCam()->ang.y);
  njRotateX(NULL, LensFlareCam()->ang.x);
  njCalcVector(NULL, &v, &fwd);
  njPopMatrix(1);
  dot = njInnerProduct(&fwd, &dir);
  if (dot > 0.0f) {
    njSetTexture(&_rename_lensflare_texlist);
    __njColorBlendingMode(0, 8);
    __njColorBlendingMode(1, 10);
    njDisableFog();
    gjSetFog();
    njProjectScreen(NULL, pos, &scr);
    if (dot > 0.99f) {
      sun = 50.0f * (dot - 0.98f);
      sun = sun * sun;
      fn_800156FC(0.9f * sun, 1.0f, 1.0f, 1.0f);
      _rename_lensflare_sprite.p.x = scr.x;
      _rename_lensflare_sprite.p.y = scr.y;
      if (lbl_803ADAD4 > 1) {
        if (lbl_803ADAD0 > 0) {
          _rename_lensflare_sprite.p.x = 640.0f + 2.0f * scr.x;
        } else {
          _rename_lensflare_sprite.p.x = 2.0f * scr.x;
        }
        _rename_lensflare_sprite.sx = 2.0f * (20.0f * sun);
      } else {
        _rename_lensflare_sprite.sx = 20.0f * sun;
      }
      _rename_lensflare_sprite.sy = 20.0f * sun;
      fn_8011E8E4(&_rename_lensflare_sprite, 7, -12.0f, 0x22);
    }
    raw = (dot > 0.6f) ? 3.0f * (dot - 0.6f) : 0.0f;
    raw = raw * raw;
    glow = raw * raw;
    // dot reused for the x distance to the centre, as in the original
    dot = 320.0f - scr.x;
    dy = 240.0f - scr.y;
    fn_800156FC(1.0f, 1.0f, 1.0f, 1.0f);
    _rename_lensflare_sprite.p.x = scr.x;
    _rename_lensflare_sprite.p.y = scr.y;
    if (lbl_803ADAD4 > 1) {
      if (lbl_803ADAD0 > 0) {
        _rename_lensflare_sprite.p.x = 640.0f + 2.0f * scr.x;
      } else {
        _rename_lensflare_sprite.p.x = 2.0f * scr.x;
      }
      _rename_lensflare_sprite.sx = 1.0f;
    } else {
      _rename_lensflare_sprite.sx = 1.0f;
    }
    _rename_lensflare_sprite.sy = 1.0f;
    fn_8011E8E4(&_rename_lensflare_sprite, 0, -13.0f, 0x22);
    e = _rename_lensflare_ghost;
    dim = 1.0f - glow;
    while (e->tex != -1) {
      // clamp tests the expression, not scl, to match
      raw = e->scl * (1.0f - dim * e->fade);
      scl = raw;
      if (raw < 0.0f) {
        scl = 0.0f;
      }
      fn_800156FC(glow * e->alpha, 1.0f, 1.0f, 1.0f);
      _rename_lensflare_sprite.p.x = scr.x + dot * e->dist;
      _rename_lensflare_sprite.p.y = scr.y + dy * e->dist;
      if (lbl_803ADAD4 > 1) {
        if (lbl_803ADAD0 > 0) {
          _rename_lensflare_sprite.p.x = 640.0f + 2.0f * (scr.x + dot * e->dist);
        } else {
          _rename_lensflare_sprite.p.x = 2.0f * (scr.x + dot * e->dist);
        }
        _rename_lensflare_sprite.sx = 2.0f * scl;
      } else {
        _rename_lensflare_sprite.sx = scl;
      }
      _rename_lensflare_sprite.sy = scl;
      fn_8011E8E4(&_rename_lensflare_sprite, e->tex, e->pri - 10.0f, 0x22);
      e++;
    }
  }
  njEnableFog();
  gjSetFog();
  __njColorBlendingMode(0, 8);
  __njColorBlendingMode(1, 6);
  fn_800156FC(0.0f, 0.0f, 0.0f, 0.0f);
  _rename_lensflare_sprite.p.x = 0.0f;
  _rename_lensflare_sprite.p.y = 0.0f;
  _rename_lensflare_sprite.sx = 1.0f;
  _rename_lensflare_sprite.sy = 1.0f;
}
