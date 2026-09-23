#ifndef _EF_ROCKETTHRUST_H_
#define _EF_ROCKETTHRUST_H_

#include "EFFECT/ef_particle.h"

particle *CreateRocketThrust(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool RocketThrustExec(particle_info *info, particle *p);
particle *CreateRocketThrustGround(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl,
                                   Float ground_y);
Bool RocketThrustGroundExec(particle_info *info, particle *p);
particle *CreateRocketThrustGroundFade(NJS_POINT3 *pos, NJS_VECTOR *spd,
                                       Float scl, Float ground_y);
Bool RocketThrustGroundFadeExec(particle_info *info, particle *p);

#endif // !_EF_ROCKETTHRUST_H_
