#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>

#include "global.h"

#undef SysBase
#undef DOSBase
#undef UtilityBase
#undef MiscBase
#undef CIAABase
#undef CiaBase
#undef TimerBase

struct Library *SysBase;
struct Library *DOSBase;
struct Library *UtilityBase;
struct Library *MiscBase;
struct Library *CIAABase;
struct Library *CiaBase;
struct Library *TimerBase;

void plipbox_sync_global_bases(struct PLIPBase *pb)
{
   SysBase = pb->pb_SysBase;
   DOSBase = pb->pb_DOSBase;
   UtilityBase = pb->pb_UtilityBase;
   MiscBase = pb->pb_HWBase.hwb_MiscBase;
   CIAABase = pb->pb_HWBase.hwb_CIAABase;
   CiaBase = pb->pb_HWBase.hwb_CIAABase;
   TimerBase = pb->pb_HWBase.hwb_TimerBase;
}

void *memcpy(void *dst, const void *src, size_t n)
{
   unsigned char *d = (unsigned char *)dst;
   const unsigned char *s = (const unsigned char *)src;

   while (n--)
      *d++ = *s++;

   return dst;
}

void *memset(void *dst, int c, size_t n)
{
   unsigned char *d = (unsigned char *)dst;

   while (n--)
      *d++ = (unsigned char)c;

   return dst;
}

static unsigned long plipbox_udivmod(unsigned long n, unsigned long d, unsigned long *rem)
{
   unsigned long q = 0;
   unsigned long bit = 1;

   if (d == 0)
   {
      if (rem)
         *rem = 0;
      return 0;
   }

   while ((d & 0x80000000UL) == 0 && d < n)
   {
      d <<= 1;
      bit <<= 1;
   }

   while (bit)
   {
      if (n >= d)
      {
         n -= d;
         q |= bit;
      }
      d >>= 1;
      bit >>= 1;
   }

   if (rem)
      *rem = n;

   return q;
}

long __divsi3(long a, long b)
{
   int neg = 0;
   unsigned long ua;
   unsigned long ub;
   unsigned long q;

   if (a < 0)
   {
      ua = (unsigned long)(-a);
      neg = !neg;
   }
   else
      ua = (unsigned long)a;

   if (b < 0)
   {
      ub = (unsigned long)(-b);
      neg = !neg;
   }
   else
      ub = (unsigned long)b;

   q = plipbox_udivmod(ua, ub, NULL);
   return neg ? -(long)q : (long)q;
}

long __modsi3(long a, long b)
{
   unsigned long rem;
   unsigned long ua;
   unsigned long ub;

   ua = (a < 0) ? (unsigned long)(-a) : (unsigned long)a;
   ub = (b < 0) ? (unsigned long)(-b) : (unsigned long)b;
   plipbox_udivmod(ua, ub, &rem);
   return (a < 0) ? -(long)rem : (long)rem;
}
