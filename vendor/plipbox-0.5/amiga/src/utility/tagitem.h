#ifndef UTILITY_TAGITEM_H
#define UTILITY_TAGITEM_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

typedef ULONG Tag;

struct TagItem
{
   Tag ti_Tag;
   ULONG ti_Data;
};

#define TAG_DONE   (0UL)
#define TAG_END    (0UL)
#define TAG_IGNORE (1UL)
#define TAG_MORE   (2UL)
#define TAG_SKIP   (3UL)
#define TAG_USER   ((ULONG)(1UL<<31))

#endif
