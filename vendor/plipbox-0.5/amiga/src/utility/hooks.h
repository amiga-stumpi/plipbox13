#ifndef UTILITY_HOOKS_H
#define UTILITY_HOOKS_H

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

struct Hook
{
   struct MinNode h_MinNode;
   APTR h_Entry;
   APTR h_SubEntry;
   APTR h_Data;
};

#endif
