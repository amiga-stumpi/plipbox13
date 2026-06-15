#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <hardware/cia.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include "global.h"
#include "hw.h"

extern BOOL hwsend(struct HWBase *hwb, struct HWFrame *frame);

#undef SysBase
#undef DOSBase
#undef UtilityBase
#undef MiscBase
#undef CIAABase
#undef CiaBase
#undef TimerBase

extern struct ExecBase *SysBase;
struct Library *DOSBase;
struct Library *UtilityBase;
struct Library *MiscBase;
struct Library *CIAABase;
struct Library *CiaBase;
struct Library *TimerBase;

static struct PLIPBase g_pb;
static struct HWFrame *g_frame;
static int g_hw_init;
static int g_hw_attached;

#ifndef PLIPBOX13_HW_STAGE
#define PLIPBOX13_HW_STAGE 1
#endif
#ifndef PLIPBOX13_SKIP_MAGIC
#define PLIPBOX13_SKIP_MAGIC 0
#endif
#ifndef PLIPBOX13_DIRECT_HWSEND
#define PLIPBOX13_DIRECT_HWSEND 0
#endif
#ifndef PLIPBOX13_HANDSHAKE_DIAG
#define PLIPBOX13_HANDSHAKE_DIAG 0
#endif
#ifndef PLIPBOX13_C_SEND
#define PLIPBOX13_C_SEND 0
#endif

#define CIAA_PTR ((volatile struct CIA *)0x00bfe001UL)
#define CIAB_PTR ((volatile struct CIA *)0x00bfd000UL)
#define PLIP_CMD_SEND 0x11U

static void shim_memset(void *dst, UBYTE v, ULONG n)
{
    UBYTE *d = (UBYTE *)dst;
    while (n--)
        *d++ = v;
}

static void shim_memcpy(void *dst, const void *src, ULONG n)
{
    UBYTE *d = (UBYTE *)dst;
    const UBYTE *s = (const UBYTE *)src;
    while (n--)
        *d++ = *s++;
}

void *memcpy(void *dst, const void *src, unsigned long n)
{
    shim_memcpy(dst, src, n);
    return dst;
}

void *memset(void *dst, int c, unsigned long n)
{
    shim_memset(dst, (UBYTE)c, n);
    return dst;
}

static ULONG diag_strlen(const char *s)
{
    ULONG n = 0;
    if (!s)
        return 0;
    while (s[n])
        ++n;
    return n;
}

static void diag_puts(const char *s)
{
    BPTR out;
    ULONG len;

    if (!DOSBase || !s)
        return;
    out = Output();
    len = diag_strlen(s);
    if (out && len)
        Write(out, (APTR)s, (LONG)len);
}

static void diag_hex_byte(UBYTE v)
{
    static const char hex[] = "0123456789abcdef";
    char out[3];
    out[0] = hex[(v >> 4) & 15];
    out[1] = hex[v & 15];
    out[2] = 0;
    diag_puts(out);
}

void plipbox_sync_global_bases(struct PLIPBase *pb)
{
    SysBase = (struct ExecBase *)pb->pb_SysBase;
    DOSBase = pb->pb_DOSBase;
    UtilityBase = pb->pb_UtilityBase;
    MiscBase = pb->pb_HWBase.hwb_MiscBase;
    CIAABase = pb->pb_HWBase.hwb_CIAABase;
    CiaBase = pb->pb_HWBase.hwb_CIAABase;
    TimerBase = pb->pb_HWBase.hwb_TimerBase;
}

int plipbox13_hw_handshake_test(void)
{
#if PLIPBOX13_HANDSHAKE_DIAG
    volatile struct CIA *ca = CIAA_PTR;
    volatile struct CIA *cb = CIAB_PTR;
    UBYTE before_pra;
    UBYTE before_ddra;
    UBYTE after_select;
    ULONG i;
    int rak_seen = 0;

    diag_puts("PLIP handshake diag begin\n");

    before_pra = cb->ciapra;
    before_ddra = cb->ciaddra;
    diag_puts("PLIP ciab pra before=0x");
    diag_hex_byte(before_pra);
    diag_puts(" ddra=0x");
    diag_hex_byte(before_ddra);
    diag_puts(" ciaa ddrb=0x");
    diag_hex_byte(ca->ciaddrb);
    diag_puts("\n");

    ca->ciaddrb = 0xff;
    ca->ciaprb = PLIP_CMD_SEND;

    cb->ciapra &= ~(1U << CIAB_PRTRPOUT);
    cb->ciapra |= (1U << CIAB_PRTRSEL);

    after_select = cb->ciapra;
    diag_puts("PLIP ciab pra after select=0x");
    diag_hex_byte(after_select);
    diag_puts("\n");

    for (i = 0; i < 50000UL; ++i) {
        if (cb->ciapra & (1U << CIAB_PRTRBUSY)) {
            rak_seen = 1;
            break;
        }
    }

    diag_puts("PLIP handshake rak=");
    diag_puts(rak_seen ? "seen" : "missing");
    diag_puts(" loops=0x");
    diag_hex_byte((UBYTE)((i >> 8) & 0xff));
    diag_hex_byte((UBYTE)(i & 0xff));
    diag_puts(" pra=0x");
    diag_hex_byte(cb->ciapra);
    diag_puts("\n");

    ca->ciaddrb = 0x00;
    cb->ciapra &= ~((1U << CIAB_PRTRPOUT) | (1U << CIAB_PRTRSEL));

    diag_puts("PLIP handshake diag end\n");
    return rak_seen;
#else
    return 1;
#endif
}


static int wait_rak_state(UBYTE state, ULONG limit)
{
    volatile struct CIA *cb = CIAB_PTR;
    ULONG i;
    UBYTE mask = (UBYTE)(1U << CIAB_PRTRBUSY);

    for (i = 0; i < limit; ++i) {
        if (((cb->ciapra & mask) ? 1 : 0) == state)
            return 1;
    }
    return 0;
}

static int plipbox13_c_send_frame(struct HWFrame *frame)
{
    volatile struct CIA *ca = CIAA_PTR;
    volatile struct CIA *cb = CIAB_PTR;
    UBYTE *p;
    ULONG total;
    ULONG i;
    UBYTE rak;
    UBYTE req_mask = (UBYTE)(1U << CIAB_PRTRPOUT);
    UBYTE sel_mask = (UBYTE)(1U << CIAB_PRTRSEL);
    UBYTE rak_mask = (UBYTE)(1U << CIAB_PRTRBUSY);

    if (!frame)
        return 0;

    total = (ULONG)frame->hwf_Size + 2UL;
    if (total < 3UL || total > (ULONG)PLIP_DEFMTU + 2UL + (ULONG)HW_ETH_HDR_SIZE)
        return 0;

    if (!wait_rak_state(0, 100000UL))
        return 0;

    p = (UBYTE *)frame;
    rak = (UBYTE)(cb->ciapra & rak_mask);

    ca->ciaddrb = 0xff;
    ca->ciaprb = PLIP_CMD_SEND;
    cb->ciapra &= ~req_mask;
    cb->ciapra |= sel_mask;

    for (i = 0; i < total; ++i) {
        ULONG loops;
        int toggled = 0;
        for (loops = 0; loops < 100000UL; ++loops) {
            UBYTE now = (UBYTE)(cb->ciapra & rak_mask);
            if (now != rak) {
                rak = now;
                toggled = 1;
                break;
            }
        }
        if (!toggled)
            goto fail;

        ca->ciaprb = *p++;
        cb->ciapra ^= req_mask;
    }

    for (i = 0; i < 100000UL; ++i) {
        UBYTE now = (UBYTE)(cb->ciapra & rak_mask);
        if (now != rak) {
            ca->ciaddrb = 0x00;
            cb->ciapra &= (UBYTE)~(req_mask | sel_mask);
            return 1;
        }
    }

fail:
    ca->ciaddrb = 0x00;
    cb->ciapra &= (UBYTE)~(req_mask | sel_mask);
    return 0;
}

int plipbox13_hw_online(const UBYTE *mac, const char *name)
{
    if (g_hw_attached)
        return 1;

    shim_memset(&g_pb, 0, sizeof(g_pb));
    g_pb.pb_DevNode.lib_Node.ln_Name = (char *)name;
    g_pb.pb_SysBase = (struct Library *)SysBase;
    if (!g_pb.pb_DOSBase)
        g_pb.pb_DOSBase = OpenLibrary((CONST_STRPTR)"dos.library", 0);
    g_pb.pb_Server = (struct Process *)FindTask(0);
    g_pb.pb_Task = FindTask(0);
    g_pb.pb_MTU = PLIP_DEFMTU;
    g_pb.pb_ReportBPS = PLIP_DEFBPS;
    g_pb.pb_HWBase.hwb_IntSig = (ULONG)-1;
    g_pb.pb_Retries = PLIP_DEFRETRIES;
    g_pb.pb_Timeout = PLIP_DEFTIMEOUT;
    g_pb.pb_CollisionDelay = PLIP_DEFDELAY;
    g_pb.pb_ArbitrationDelay = PLIP_DEFARBITRATIONDELAY;
    g_pb.pb_Flags = PLIPF_OFFLINE;
    shim_memcpy(g_pb.pb_CfgAddr, mac, 6);
    shim_memcpy(g_pb.pb_DefAddr, mac, 6);

    g_frame = (struct HWFrame *)AllocMem(sizeof(struct HWFrame) + PLIP_DEFMTU, MEMF_PUBLIC | MEMF_CLEAR);
    if (!g_frame)
        return 0;
    g_pb.pb_Frame = g_frame;

    if (!hw_init(&g_pb))
        goto fail;
    g_hw_init = 1;

#if PLIPBOX13_HW_STAGE < 2
    return 1;
#endif

    if (!hw_attach(&g_pb))
        goto fail;
    g_hw_attached = 1;
    g_pb.pb_Flags &= ~PLIPF_OFFLINE;

    (void)plipbox13_hw_handshake_test();

#if PLIPBOX13_HW_STAGE < 3 || PLIPBOX13_SKIP_MAGIC
    return 1;
#endif

    /* plipbox-0.5 online/magic frame. */
    g_frame->hwf_Size = HW_ETH_HDR_SIZE;
    g_frame->hwf_DstAddr[0] = 0;
    g_frame->hwf_DstAddr[1] = 5;
    g_frame->hwf_DstAddr[2] = 0;
    g_frame->hwf_DstAddr[3] = 0;
    g_frame->hwf_DstAddr[4] = 0;
    g_frame->hwf_DstAddr[5] = 0;
    shim_memcpy(g_frame->hwf_SrcAddr, mac, 6);
    g_frame->hwf_Type = 0xffffU;
    (void)hw_send_frame(&g_pb, g_frame);
    return 1;

fail:
    if (g_hw_attached) {
        hw_detach(&g_pb);
        g_hw_attached = 0;
    }
    if (g_hw_init) {
        hw_cleanup(&g_pb);
        g_hw_init = 0;
    }
    if (g_frame) {
        FreeMem(g_frame, sizeof(struct HWFrame) + PLIP_DEFMTU);
        g_frame = 0;
        g_pb.pb_Frame = 0;
    }
    if (g_pb.pb_DOSBase) {
        CloseLibrary(g_pb.pb_DOSBase);
        g_pb.pb_DOSBase = 0;
        DOSBase = 0;
    }
    return 0;
}

void plipbox13_hw_offline(void)
{
    if (g_hw_attached) {
        hw_detach(&g_pb);
        g_hw_attached = 0;
    }
    if (g_hw_init) {
        hw_cleanup(&g_pb);
        g_hw_init = 0;
    }
    if (g_frame) {
        FreeMem(g_frame, sizeof(struct HWFrame) + PLIP_DEFMTU);
        g_frame = 0;
        g_pb.pb_Frame = 0;
    }
    if (g_pb.pb_DOSBase) {
        CloseLibrary(g_pb.pb_DOSBase);
        g_pb.pb_DOSBase = 0;
        DOSBase = 0;
    }
}

int plipbox13_hw_send(const UBYTE *dst, const UBYTE *src, UWORD type, const UBYTE *payload, ULONG len)
{
    UBYTE *out;
    if (!g_hw_attached || !g_frame || len > PLIP_DEFMTU)
        return 0;
    g_frame->hwf_Size = (USHORT)(HW_ETH_HDR_SIZE + len);
    shim_memcpy(g_frame->hwf_DstAddr, dst, 6);
    shim_memcpy(g_frame->hwf_SrcAddr, src, 6);
    g_frame->hwf_Type = type;
    out = (UBYTE *)(g_frame + 1);
    shim_memcpy(out, payload, len);
#if PLIPBOX13_C_SEND
    return plipbox13_c_send_frame(g_frame);
#elif PLIPBOX13_DIRECT_HWSEND
    return hwsend(&g_pb.pb_HWBase, g_frame) ? 1 : 0;
#else
    return hw_send_frame(&g_pb, g_frame) ? 1 : 0;
#endif
}
