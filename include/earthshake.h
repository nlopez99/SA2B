#ifndef _EARTHSHAKE_H_
#define _EARTHSHAKE_H_

#include "sa2b_types.h"

struct task;

// starts a quake centred on pos that fades out past radius
struct task *CreateEsShakeRad(NJS_POINT3 *pos, Float radius, Float power);

#endif // !_EARTHSHAKE_H_
