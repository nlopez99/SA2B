#ifndef _O_CHAOKEY_H_
#define _O_CHAOKEY_H_

#include "sa2b_types.h"
#include "samt/ninja/njcommon.h"
#include "samt/sonic/task.h"

// used outside this file, keep it non-static
BOOL ChaoKeyIsActive(void);
task *CreateChaoKey(NJS_POINT3 *pos, Float ground_y, Float spd);
void CreateChaoKeyTask(NJS_POINT3 *pos);

#endif // !_O_CHAOKEY_H_
