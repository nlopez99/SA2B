#ifndef _EF_SPSPARK_H_
#define _EF_SPSPARK_H_

#include "sa2b_types.h"

extern NJS_TEXLIST spspark_texlist;

void CreateSpSpark(NJS_POINT3 *pos, NJS_VECTOR *spd, Float spread,
                   Float ground_y);

#endif // !_EF_SPSPARK_H_
