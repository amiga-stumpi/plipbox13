
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <devices/sana2.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>

#define DEVICE_NAME "plipbox.device"
#define PLIP13_BPS 100000UL
#define PLIP13_MTU 1500UL
#define PLIP13_RAW_MTU 1518UL
#ifndef PLIPBOX13_HW_STAGE
#define PLIPBOX13_HW_STAGE 1
#endif
#ifndef PLIPBOX13_HANDSHAKE_DIAG
#define PLIPBOX13_HANDSHAKE_DIAG 0
#endif

#ifndef S2ERR_NO_ERROR
#define S2ERR_NO_ERROR 0
#endif
#ifndef S2ERR_BAD_ARGUMENT
#define S2ERR_BAD_ARGUMENT 3
#endif
#ifndef S2ERR_OUTOFSERVICE
#define S2ERR_OUTOFSERVICE 5
#endif
#ifndef S2WERR_GENERIC_ERROR
#define S2WERR_GENERIC_ERROR 0
#endif
#ifndef S2WERR_UNIT_OFFLINE
#define S2WERR_UNIT_OFFLINE 2
#endif
#ifndef SANA2IOF_QUICK
#define SANA2IOF_QUICK IOF_QUICK
#endif
#ifndef SANA2IOF_RAW
#define SANA2IOF_RAW 0
#endif
#ifndef S2_BROADCAST
#define S2_BROADCAST (S2_START + 5)
#endif

typedef APTR BMFunc;
struct BufferManagement {
    struct MinNode bm_Node;
    BMFunc bm_CopyFromBuffer;
    BMFunc bm_CopyToBuffer;
};

extern int plipbox13_hw_online(const UBYTE *mac, const char *name);
extern void plipbox13_hw_offline(void);
extern int plipbox13_hw_send(const UBYTE *dst, const UBYTE *src, UWORD type, const UBYTE *payload, ULONG len);
extern int plipbox13_hw_handshake_test(void);
extern BOOL plipbox_call_bm(BMFunc func, void *dst, void *src, LONG size);

const char plipbox13_device_name[] = DEVICE_NAME;
const char plipbox13_id_string[] = DEVICE_NAME " v0.5-os13-sana by Marcel Jaehne (c)2026";

struct ExecBase *SysBase;

static BPTR g_seglist;
static struct List g_read_list;
static UBYTE g_mac[6] = {0x1a,0x11,0xa1,0xa0,0x47,0x11};
static UBYTE g_bcast[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
static UBYTE g_tx_payload[PLIP13_MTU];
static struct BufferManagement g_bm;
static int g_have_bm;
static int g_online;

static void mem_copy(void *dst, const void *src, ULONG len)
{
    UBYTE *d = (UBYTE *)dst;
    const UBYTE *s = (const UBYTE *)src;
    while (len--)
        *d++ = *s++;
}

static void list_init(struct List *list)
{
    list->lh_Head = (struct Node *)&list->lh_Tail;
    list->lh_Tail = 0;
    list->lh_TailPred = (struct Node *)&list->lh_Head;
}

static ULONG tag_get(ULONG tag, ULONG def, struct TagItem *tags)
{
    struct TagItem *ti = tags;
    while (ti) {
        if (ti->ti_Tag == TAG_DONE)
            return def;
        if (ti->ti_Tag == TAG_IGNORE) {
            ++ti;
            continue;
        }
        if (ti->ti_Tag == TAG_SKIP) {
            ti += ti->ti_Data + 1;
            continue;
        }
        if (ti->ti_Tag == TAG_MORE) {
            ti = (struct TagItem *)ti->ti_Data;
            continue;
        }
        if (ti->ti_Tag == tag)
            return ti->ti_Data;
        ++ti;
    }
    return def;
}

static int setup_bm(struct IOSana2Req *req)
{
    struct TagItem *tags = (struct TagItem *)req->ios2_BufferManagement;
    g_bm.bm_CopyToBuffer = (BMFunc)tag_get(S2_CopyToBuff, 0, tags);
    g_bm.bm_CopyFromBuffer = (BMFunc)tag_get(S2_CopyFromBuff, 0, tags);
    if (!g_bm.bm_CopyToBuffer || !g_bm.bm_CopyFromBuffer)
        return 0;
    req->ios2_BufferManagement = &g_bm;
    g_have_bm = 1;
    return 1;
}

static int node_is_linked(struct List *list, struct Node *node)
{
    struct Node *n;
    if (!list || !node)
        return 0;
    for (n = list->lh_Head; n && n->ln_Succ; n = n->ln_Succ) {
        if (n == node)
            return 1;
    }
    return 0;
}

static void abort_all_reads(void)
{
    struct Node *n;
    struct Node *next;
    struct IOSana2Req *req;

    Forbid();
    n = g_read_list.lh_Head;
    while (n && n->ln_Succ) {
        next = n->ln_Succ;
        Remove(n);
        req = (struct IOSana2Req *)n;
        req->ios2_Req.io_Error = IOERR_ABORTED;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        ReplyMsg(&req->ios2_Req.io_Message);
        n = next;
    }
    Permit();
}

static void device_query(struct IOSana2Req *req)
{
    struct Sana2DeviceQuery *q = (struct Sana2DeviceQuery *)req->ios2_StatData;
    if (!q) {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        return;
    }

    q->DevQueryFormat = 0;
    q->DeviceLevel = 0;
    if (q->SizeAvailable >= 18)
        q->AddrFieldSize = 48;
    if (q->SizeAvailable >= 22)
        q->MTU = PLIP13_MTU;
    if (q->SizeAvailable >= 26)
        q->BPS = PLIP13_BPS;
    if (q->SizeAvailable >= 30)
        q->HardwareType = S2WireType_Ethernet;
    q->SizeSupplied = q->SizeAvailable < 30 ? q->SizeAvailable : 30;
}

static struct Library *init_device(
    register APTR dev __asm("d0"),
    register BPTR seglist __asm("a0"),
    register struct ExecBase *sysbase __asm("a6"))
{
    struct Library *lib = (struct Library *)dev;
    (void)sysbase;

    SysBase = sysbase;
    g_seglist = seglist;
    list_init(&g_read_list);
    g_online = 0;

    lib->lib_Node.ln_Type = NT_DEVICE;
    lib->lib_Node.ln_Name = (char *)plipbox13_device_name;
    lib->lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    lib->lib_Version = 0;
    lib->lib_Revision = 5;
    lib->lib_IdString = (APTR)plipbox13_id_string;
    return lib;
}

static ULONG expunge(register struct Library *dev __asm("a6"))
{
    if (dev->lib_OpenCnt) {
        dev->lib_Flags |= LIBF_DELEXP;
        return 0;
    }
    Remove((struct Node *)dev);
    return (ULONG)g_seglist;
}

static void open_device(
    register struct Library *dev __asm("a6"),
    register struct IOSana2Req *req __asm("a1"),
    register ULONG unit __asm("d0"),
    register ULONG flags __asm("d1"))
{
    (void)flags;

    req->ios2_Req.io_Error = IOERR_OPENFAIL;
    req->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    req->ios2_WireError = S2WERR_GENERIC_ERROR;

    if (unit != 0 || dev->lib_OpenCnt != 0)
        return;

    list_init(&g_read_list);
    g_have_bm = 0;
    g_online = 0;
    if (!setup_bm(req)) {
        req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        return;
    }
    ++dev->lib_OpenCnt;
    dev->lib_Flags &= ~LIBF_DELEXP;
    req->ios2_Req.io_Device = (struct Device *)dev;
    req->ios2_Req.io_Unit = (struct Unit *)0;
    req->ios2_Req.io_Error = 0;
    req->ios2_WireError = S2WERR_GENERIC_ERROR;
}

static ULONG close_device(
    register struct Library *dev __asm("a6"),
    register struct IOSana2Req *req __asm("a1"))
{
    g_online = 0;
    plipbox13_hw_offline();
    abort_all_reads();

    if (dev->lib_OpenCnt > 0)
        --dev->lib_OpenCnt;
    req->ios2_Req.io_Device = 0;
    req->ios2_Req.io_Unit = 0;

    if (dev->lib_OpenCnt == 0 && (dev->lib_Flags & LIBF_DELEXP))
        return expunge(dev);
    return 0;
}

static void begin_io(
    register struct Library *dev __asm("a6"),
    register struct IOSana2Req *req __asm("a1"))
{
    (void)dev;

    req->ios2_Req.io_Error = S2ERR_NO_ERROR;
    req->ios2_WireError = S2WERR_GENERIC_ERROR;

    switch (req->ios2_Req.io_Command) {
    case CMD_READ:
        /* Stage-0 SANA skeleton: do not take ownership of caller requests yet.
         * Returning offline here lets TheWire13 exercise RX init without any
         * device-side list manipulation.
         */
        req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        req->ios2_WireError = S2WERR_UNIT_OFFLINE;
        break;

    case CMD_WRITE:
    case S2_BROADCAST:
#if PLIPBOX13_HW_STAGE < 3
        req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        req->ios2_WireError = S2WERR_UNIT_OFFLINE;
        break;
#endif
        if (!g_online || !g_have_bm) {
            req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
            req->ios2_WireError = S2WERR_UNIT_OFFLINE;
            break;
        }
        if (req->ios2_DataLength > PLIP13_MTU) {
            req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
            break;
        }
        if (!plipbox_call_bm(g_bm.bm_CopyFromBuffer, g_tx_payload, req->ios2_Data, req->ios2_DataLength)) {
            req->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
            break;
        }
        if (req->ios2_Req.io_Command == S2_BROADCAST)
            mem_copy(req->ios2_DstAddr, g_bcast, 6);
#if PLIPBOX13_HANDSHAKE_DIAG
        (void)plipbox13_hw_handshake_test();
        req->ios2_Req.io_Error = IOERR_ABORTED;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        break;
#endif
        if (!plipbox13_hw_send(req->ios2_DstAddr, g_mac, (UWORD)req->ios2_PacketType, g_tx_payload, req->ios2_DataLength)) {
            req->ios2_Req.io_Error = IOERR_ABORTED;
            req->ios2_WireError = S2WERR_GENERIC_ERROR;
        }
        break;

    case S2_CONFIGINTERFACE:
        mem_copy(g_mac, req->ios2_SrcAddr, 6);
        if (!plipbox13_hw_online(g_mac, plipbox13_device_name)) {
            req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
            req->ios2_WireError = S2WERR_UNIT_OFFLINE;
            break;
        }
        g_online = 1;
        break;

    case S2_ONLINE:
        if (!plipbox13_hw_online(g_mac, plipbox13_device_name)) {
            req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
            req->ios2_WireError = S2WERR_UNIT_OFFLINE;
            break;
        }
        g_online = 1;
        break;

    case S2_OFFLINE:
        g_online = 0;
        plipbox13_hw_offline();
        abort_all_reads();
        break;

    case S2_GETSTATIONADDRESS:
        mem_copy(req->ios2_SrcAddr, g_mac, 6);
        mem_copy(req->ios2_DstAddr, g_mac, 6);
        break;

    case S2_DEVICEQUERY:
        device_query(req);
        break;

    default:
        req->ios2_Req.io_Error = IOERR_NOCMD;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        break;
    }

    if (req) {
        if (req->ios2_Req.io_Flags & SANA2IOF_QUICK)
            req->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        else
            ReplyMsg(&req->ios2_Req.io_Message);
    }
}

static ULONG abort_io(
    register struct Library *dev __asm("a6"),
    register struct IOSana2Req *req __asm("a1"))
{
    int owned;
    (void)dev;

    Forbid();
    owned = node_is_linked(&g_read_list, &req->ios2_Req.io_Message.mn_Node);
    if (owned)
        Remove(&req->ios2_Req.io_Message.mn_Node);
    Permit();

    if (owned) {
        req->ios2_Req.io_Error = IOERR_ABORTED;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        ReplyMsg(&req->ios2_Req.io_Message);
    }
    return 0;
}

static ULONG device_vectors[] = {
    (ULONG)open_device,
    (ULONG)close_device,
    (ULONG)expunge,
    0,
    (ULONG)begin_io,
    (ULONG)abort_io,
    (ULONG)-1
};

ULONG plipbox13_auto_init_tables[] = {
    sizeof(struct Library),
    (ULONG)device_vectors,
    0,
    (ULONG)init_device
};
