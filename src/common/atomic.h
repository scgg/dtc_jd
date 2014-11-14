#ifndef __ATOMIC__T
#define __ATOMIC__T

#include <stdint.h>

#if __GNUC__ < 4
#include "atomic_asm.h"
#else
#include "atomic_gcc.h"
#endif

#if __WORDSIZE==64 || __GNUC__ >= 5 || (__GNUC__==4 && __GNUC_MINOR__>=3)
#define HAS_ATOMIC8	1
#include "atomic_gcc8.h"
#else
#define HAS_ATOMIC8	1
#include "atomic_asm8.h"
#endif

#endif