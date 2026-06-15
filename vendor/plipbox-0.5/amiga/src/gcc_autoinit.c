#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/nodes.h>
#include <devices/sana2.h>

#include "global.h"
#include "compiler.h"

PUBLIC ASM SAVEDS struct Device *DevInit(REG(d0) BASEPTR,
                                         REG(a0) BPTR seglist,
                                         REG(a6) struct Library *_SysBase);
PUBLIC ASM SAVEDS LONG DevOpen(REG(a1) struct IOSana2Req *ios2,
                               REG(d0) ULONG unit,
                               REG(d1) ULONG flags,
                               REG(a6) BASEPTR);
PUBLIC ASM SAVEDS BPTR DevClose(REG(a1) struct IOSana2Req *ior,
                                REG(a6) BASEPTR);
PUBLIC ASM SAVEDS BPTR DevExpunge(REG(a6) BASEPTR);
PUBLIC ASM SAVEDS VOID DevBeginIO(REG(a1) struct IOSana2Req *ios2,
                                  REG(a6) BASEPTR);
PUBLIC ASM SAVEDS LONG DevAbortIO(REG(a1) struct IOSana2Req *ior,
                                  REG(a6) BASEPTR);

const char plipbox_device_name[] = "plipbox.device";
const char plipbox_id_string[] = "plipbox.device 0.5-os13 (plipbox-0.5 protocol)\r\n";

static ULONG stub(void)
{
   return 0;
}

ULONG plipbox_device_vectors[] =
{
   (ULONG)DevOpen,
   (ULONG)DevClose,
   (ULONG)DevExpunge,
   (ULONG)stub,
   (ULONG)DevBeginIO,
   (ULONG)DevAbortIO,
   (ULONG)-1
};

ULONG plipbox_auto_init_tables[] =
{
   sizeof(struct PLIPBase),
   (ULONG)plipbox_device_vectors,
   0,
   (ULONG)DevInit
};
