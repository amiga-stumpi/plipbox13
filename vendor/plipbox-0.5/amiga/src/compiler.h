#ifndef __COMPILER_H
#define __COMPILER_H

#ifdef __SASC
/* Implementation notes:
**
** SAS/C complains if a module contains only "static" and "extern" functions
** that there would be no exported symbol. Therefore, the PUBLIC macro
** is defined empty for SAS/C but should be "extern" if possible (for
** instance with GNU CC).
*/
#  define ASM     __asm              /* define registers for function args */
#  define REG(x)  register __ ## x   /* specify a register in arglist */
#  define INLINE  __inline           /* inline this function */
#  define STDARGS __stdargs          /* pass args to this function on stack */
#  define REGARGS __regargs          /* pass args to this function in regs */
#  define SAVEDS  __saveds           /* setup data segment reg. on entry */
#  define FAR     __far              /* reference this object in far mode */
#  define MIN     __builtin_min      /* min{} function */
#  define MAX     __builtin_max      /* max{} function */
#  define PUTREG __builtin_putreg    /* set a register to a certain value */
#  define REG_D0 0                   /* reg. number for PUTREG() */
#  define REG_D1 1                   /* reg. number for PUTREG() */
extern void PUTREG(int, long);       /* prototype */
#  define PUBLIC                     /* define a globally visible function */
#  define PRIVATE static             /* define a locally visible function */
#elif defined(__GNUC__)
/*
** Direct amitcp13 build path using bebbo/m68k-amigaos-gcc.
** The imported source keeps SAS/C-style register annotations in the prototype
** order.  The direct build compiles C functions with stack arguments and uses
** small assembly wrappers for the real Amiga register-based entry points.
*/
#  define ASM
#  define REG(x)
#  define INLINE inline
#  define STDARGS
#  define REGARGS
#  define SAVEDS
#  define FAR
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#  define PUTREG(r,v) ((void)0)
#  define REG_D0 0
#  define REG_D1 1
#  define PUBLIC
#  undef GLOBAL
#  define GLOBAL
#  define PRIVATE static
#else
#  error Please define the above macros for your compiler
#endif

#endif

