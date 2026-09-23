#ifndef _CCL_H_
#define _CCL_H_

#include "samt/core.h"
#include "samt/sonic/task.h"
#include "samt/sonic/c_colli.h"

// void CCL_ClearInfo(taskwk *);
void CCL_CalcColliCenterC(taskwk *, CCL_INFO *, NJS_POINT3 *);
void CCL_CalcColliCenterR(taskwk *, CCL_INFO *, NJS_POINT3 *);
void CCL_ClearSearch(void);
void CCL_CalcRange(taskwk *);
void CCL_GetHitFaceNormal(taskwk *, NJS_POINT3 *);
// void CCL_IsPushed(taskwk *);
void CCL_IsHit(taskwk *);
// void CCL_IsHitKindEx(taskwk *, Uint8);
void CCL_IsHitKindWithNumEx(taskwk *, Sint32, Uint8);
void CCL_IsHitWithNumEx(taskwk *, Sint32);
void CCL_GetNearestKindDir(taskwk *, Sint32, Sint32, Sint16);
void CCL_GetForm(taskwk *, Sint32);
// void CCL_Enable(taskwk *, Sint32);
// void CCL_Disable(taskwk *, Sint32);
void CCL_GetInfo(taskwk *, Sint32);
void CCL_InitShare(task *tp, CCL_INFO *info, Sint32 nbInfo, Uint8 id);
void CCL_ClearAll(void);
CCL_HIT_INFO *CCL_IsHitPlayerEx(task *tp);
void CCL_IsHitBulletEx(taskwk *);
void CCL_IsHitEnemyEx(taskwk *);
void CCL_IsHitEnemy2Ex(taskwk *);
void CCL_IsHitObjectEx(taskwk *);
// void CCL_IsHitKind(taskwk *, Uint8);
// void CCL_IsHitKind2(taskwk *, Uint8);
void CCL_IsHitPlayerWithNumEx(taskwk *, Sint32);
void CCL_IsHitObjectWithNumEx(taskwk *, Sint32);
void CCL_IsHitKindWithNum2(taskwk *, Sint32, Uint8);
void CCL_IsHitWithNum(taskwk *, Sint32);
void CCL_Entry(task *);
// void CCL_Init(task *, CCL_INFO *, Sint32, Uint8);
// void CCL_IsHitPlayer(taskwk *);
task* CCL_IsHitPlayer(task *);

void CCL_IsHitBullet(taskwk *);
void CCL_IsHitEnemy(taskwk *);
void CCL_IsHitEnemy2(taskwk *);
void CCL_IsHitPlayerWithNum(taskwk *, Sint32);
void CCL_IsHitPlayerWithNum2(taskwk *, Sint32);
void CCL_IsHitObjectWithNum(taskwk *, Sint32);
// void CCL_Analyze(void);

// ^ known symbols, unknown return type
// v unknown calling

void CCL_CheckHoming();
void CCL_CheckColliRange();
void CCL_CheckPushOut();
void CCL_CheckDamage();
void CCL_ExchangeInfo();
void CCL_SetRideFlag();
void CCL_CalcColliCenterSEX();
void CCL_CalcColliVectorC();
void CCL_ColliPlayer2Player();
void CCL_GetRectSideNum();
void CCL_ColliPlayer2Rectangle();
void CCL_ColliPlayer2Cylinder2();
void CCL_ColliPlayer2PlaneWall();
void CCL_ColliPlayer2CircleWall();
void CCL_ColliSphere2Sphere();
void CCL_ColliSphere2Cylinder2();
void CCL_ColliSphere2Capsule();
void CCL_ColliLine2Sphere();
void CCL_CalcColliPW();
void CCL_CalcColliCenterP();
void CCL_ColliPlayer2Cylinder();
void CCL_ColliPlayer2Rectangle2();
void CCL_ColliPlayer2Rectangle3();
void CCL_ColliSphere2Cylinder();
void CCL_ColliSphere2Rectangle();
void CCL_ColliCylinder2Cylinder();
void CCL_ColliCylinder2Rectangle();
void CCL_SelectColliFunction();
void CCL_CalcColli();
void CCL_SelectColliFunctionPONormal();
void CCL_CalcColliPO();
void CCL_PCheckColli();
void CCL_BCheckColli();
void CCL_CCheckColli();
void CCL_E2CheckColli();
void CCL_OCheckColli();
void CCL_O2CheckColli();

#endif // !_CCL_H_
