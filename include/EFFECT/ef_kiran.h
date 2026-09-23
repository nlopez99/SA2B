#ifndef _EF_KIRAN_H_
#define _EF_KIRAN_H_

#include "EFFECT/ef_particle.h"

particle *CreateKiran(NJS_POINT3 *pos, NJS_VECTOR *spd, Float scl);
Bool KiranExec(particle_info *info, particle *p);

#endif // !_EF_KIRAN_H_
