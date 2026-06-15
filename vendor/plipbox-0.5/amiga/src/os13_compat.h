#ifndef PLIPBOX_OS13_COMPAT_H
#define PLIPBOX_OS13_COMPAT_H

#ifdef PLIPBOX_OS13

#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif
#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef CLIB_EXEC_PROTOS_H
#include <clib/exec_protos.h>
#endif
#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif
#ifndef CLIB_DOS_PROTOS_H
#include <clib/dos_protos.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
 * Compatibility helpers for building plipbox.device for Kickstart 1.3.
 *
 * The original 0.6 source targets an OS 2.x+ style environment.  Keep the
 * normal build untouched, and map only the APIs known to be unavailable or
 * unsafe on plain 1.3 when PLIPBOX_OS13 is explicitly defined.
 */

static APTR plipbox_os13_alloc_vec(ULONG size, ULONG flags)
{
   UBYTE *mem;
   ULONG total = size + 8;

   mem = AllocMem(total, flags);
   if (!mem)
      return NULL;

   *((ULONG *)mem) = total;
   return (APTR)(mem + 8);
}

static VOID plipbox_os13_free_vec(APTR ptr)
{
   UBYTE *mem;
   ULONG total;

   if (!ptr)
      return;

   mem = ((UBYTE *)ptr) - 8;
   total = *((ULONG *)mem);
   FreeMem(mem, total);
}

static BYTE plipbox_os13_alloc_port_signal(void)
{
   BYTE sig;
   BYTE try_sig;

   /* Prefer non-break, non-sign-bit signals.  AllocSignal(-1) often
   ** returns bit 31 on Kickstart 1.3, which is awkward for old code paths
   ** and made MsgPort wakeups unreliable on the PLIP server task.
   */
   for (try_sig = 8; try_sig <= 11; ++try_sig)
   {
      sig = AllocSignal(try_sig);
      if (sig != -1)
         return sig;
   }
   for (try_sig = 4; try_sig <= 7; ++try_sig)
   {
      sig = AllocSignal(try_sig);
      if (sig != -1)
         return sig;
   }
   for (try_sig = 16; try_sig <= 30; ++try_sig)
   {
      sig = AllocSignal(try_sig);
      if (sig != -1)
         return sig;
   }

   return AllocSignal(-1);
}

static struct MsgPort *plipbox_os13_create_msg_port(void)
{
   struct MsgPort *port;
   BYTE sig;

   port = AllocMem(sizeof(*port), MEMF_CLEAR | MEMF_PUBLIC);
   if (!port)
      return NULL;

   sig = plipbox_os13_alloc_port_signal();
   if (sig == -1)
   {
      FreeMem(port, sizeof(*port));
      return NULL;
   }

   port->mp_Node.ln_Type = NT_MSGPORT;
   port->mp_Flags = PA_SIGNAL;
   port->mp_SigBit = sig;
   port->mp_SigTask = FindTask(NULL);
   NewList(&port->mp_MsgList);

   return port;
}

static VOID plipbox_os13_delete_msg_port(struct MsgPort *port)
{
   if (!port)
      return;

   FreeSignal(port->mp_SigBit);

   FreeMem(port, sizeof(*port));
}

static APTR plipbox_os13_create_io_request(struct MsgPort *port, ULONG size)
{
   struct IORequest *ior;

   ior = AllocMem(size, MEMF_CLEAR | MEMF_PUBLIC);
   if (!ior)
      return NULL;

   ior->io_Message.mn_Node.ln_Type = NT_MESSAGE;
   ior->io_Message.mn_ReplyPort = port;
   ior->io_Message.mn_Length = size;

   return (APTR)ior;
}

static VOID plipbox_os13_delete_io_request(APTR ior)
{
   struct IORequest *req = (struct IORequest *)ior;

   if (!req)
      return;

   FreeMem(req, req->io_Message.mn_Length);
}

static ULONG plipbox_os13_get_tag_data(ULONG tag, ULONG default_data, struct TagItem *tags)
{
   struct TagItem *ti = tags;

   while (ti)
   {
      switch (ti->ti_Tag)
      {
      case TAG_DONE:
         return default_data;
      case TAG_MORE:
         ti = (struct TagItem *)ti->ti_Data;
         continue;
      case TAG_SKIP:
         ti += ti->ti_Data + 1;
         continue;
      case TAG_IGNORE:
         ti++;
         continue;
      default:
         if (ti->ti_Tag == tag)
            return ti->ti_Data;
         ti++;
         continue;
      }
   }

   return default_data;
}

extern ULONG ServerTaskSegList[];

static struct Process *plipbox_os13_create_server_process(char *name, ULONG stack_size)
{
   struct MsgPort *mp;

   mp = CreateProc((CONST_STRPTR)name, 0, ((ULONG)ServerTaskSegList) >> 2, stack_size);
   if (!mp)
      return NULL;

   return (struct Process *)((UBYTE *)mp - sizeof(struct Task));
}

#define AllocVec(size, flags) plipbox_os13_alloc_vec((size), (flags))
#define FreeVec(ptr) plipbox_os13_free_vec((ptr))
#define CreateMsgPort() plipbox_os13_create_msg_port()
#define DeleteMsgPort(port) plipbox_os13_delete_msg_port((port))
#define CreateIORequest(port, size) plipbox_os13_create_io_request((port), (size))
#define DeleteIORequest(ior) plipbox_os13_delete_io_request((ior))
#define GetTagData(tag, def, tags) plipbox_os13_get_tag_data((tag), (ULONG)(def), (tags))

/*
 * Exec signal semaphores are not a Kickstart 1.3 facility.  plipbox uses them
 * only to protect short list operations between the device entry points and
 * the server task, so Forbid/Permit is the conservative 1.3 replacement.
 */
#define InitSemaphore(sem) ((VOID)0)
#define ObtainSemaphore(sem) Forbid()
#define ObtainSemaphoreShared(sem) Forbid()
#define ReleaseSemaphore(sem) Permit()

#endif /* PLIPBOX_OS13 */

#endif /* PLIPBOX_OS13_COMPAT_H */
