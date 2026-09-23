#ifndef _OBJDUST_H_
#define _OBJDUST_H_

#include "EFFECT/ef_particle.h"

// dust and smoke thrown by breakable objects; texlist shared with ef_dust.c
particle *_rename_CreateSmoke(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
particle *_rename_CreateObjDust1(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
particle *CreateObjDust2(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                         Float phase);
particle *CreateObjDust3(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                         Float phase);

Uint32 PingPongFrame(Uint32 n, Uint32 half);

Bool SmokeExec(particle_info *info, particle *p);
Bool ObjDust1Exec(particle_info *info, particle *p);
Bool ObjDust2Exec(particle_info *info, particle *p);
Bool ObjDust3Exec(particle_info *info, particle *p);

// dust along the four edges of a box, in the box's own rotation
void _rename_CreateBoxDust(NJS_POINT3 *pos, Float x, Float z, Angle3 *ang,
                           Float scl, Float density, Float spd);
// one puff thrown off in a random direction
void _rename_CreateBreakSmoke(NJS_POINT3 *pos, Float rad, Float spd,
                              Float scl);
// num puffs evenly spaced round a circle, randomly phased
void _rename_PutDustCircle(NJS_POINT3 *pos, Float rad, Float scl, Sint32 num);

#endif // !_OBJDUST_H_
