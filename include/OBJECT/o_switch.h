#ifndef _O_SWITCH_H_
#define _O_SWITCH_H_

#include "sa2b_types.h"
#include "samt/ninja/njcommon.h"
#include "samt/sonic/task.h"

void ObjectSwitch(task *tp);
void ObjectSSS(task *tp);

NJS_POINT3 **AddSwitchAvoidPos(NJS_POINT3 *pos);
NJS_POINT3 **RemoveSwitchAvoidPos(NJS_POINT3 *pos);

void SetSwitchOnOff(Sint32 no, Sint32 on);
Sint32 GetSwitchOnOff(Sint32 no);
Sint32 GetSwitchChanged(Sint32 no);
void InitSwitchOnOff(void);

#endif // !_O_SWITCH_H_
