#ifndef __FABSF_H_
#define __FABSF_H_

#ifndef __MWERKS__
// mwerks intrinsic
inline float __fabsf(float f)
{
    return f < 0 ? -f : f;
}
#endif

inline float fabsf(float f)
{
    return __fabsf(f);
}

#endif // !__FABSF_H_
