/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/portio.c
 * PURPOSE:         I/O Functions for access to ports
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

//
// HAL Port to Inlined Port
//
#define H2I(Port) PtrToUshort(Port)

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
READ_PORT_BUFFER_UCHAR(IN PUCHAR Port,
                       OUT PUCHAR Buffer,
                       IN ULONG Count)
{
#ifndef LUSER
    __inbytestring(H2I(Port), Buffer, Count);
#else
    int i;
    for (i = 0; i < Count; i++)
        Buffer[i] = __inbyte(H2I(Port));
#endif
}

VOID
NTAPI
READ_PORT_BUFFER_USHORT(IN PUSHORT Port,
                        OUT PUSHORT Buffer,
                        IN ULONG Count)
{
#ifndef LUSER
    __inwordstring(H2I(Port), Buffer, Count);
#else
    int i;
    for (i = 0; i < Count; i++)
        Buffer[i] = __inword(H2I(Port));
#endif

}

VOID
NTAPI
READ_PORT_BUFFER_ULONG(IN PULONG Port,
                       OUT PULONG Buffer,
                       IN ULONG Count)
{
#ifndef LUSER
    __indwordstring(H2I(Port), Buffer, Count);
#else
    int i;
    for (i = 0; i < Count; i++)
        Buffer[i] = __indword(H2I(Port));
#endif
}

UCHAR
NTAPI
READ_PORT_UCHAR(IN PUCHAR Port)
{
    return __inbyte(H2I(Port));
}

USHORT
NTAPI
READ_PORT_USHORT(IN PUSHORT Port)
{
    return __inword(H2I(Port));
}

ULONG
NTAPI
READ_PORT_ULONG(IN PULONG Port)
{
    return __indword(H2I(Port));
}

VOID
NTAPI
WRITE_PORT_BUFFER_UCHAR(IN PUCHAR Port,
                        IN PUCHAR Buffer,
                        IN ULONG Count)
{
#ifndef LUSER
    __outbytestring(H2I(Port), Buffer, Count);
#else
    int i;
    for (i = 0; i < Count; i++)
        __outbyte(H2I(Port), Buffer[i]);
#endif
}

VOID
NTAPI
WRITE_PORT_BUFFER_USHORT(IN PUSHORT Port,
                         IN PUSHORT Buffer,
                         IN ULONG Count)
{
#ifndef LUSER
    __outwordstring(H2I(Port), Buffer, Count);
#else
    int i;
    for (i = 0; i < Count; i++)
        __outword(H2I(Port), Buffer[i]);
#endif
}

VOID
NTAPI
WRITE_PORT_BUFFER_ULONG(IN PULONG Port,
                        IN PULONG Buffer,
                        IN ULONG Count)
{
#ifndef LUSER
    __outdwordstring(H2I(Port), Buffer, Count);
#else
    int i;
    for (i = 0; i < Count; i++)
        __outdword(H2I(Port), Buffer[i]);
#endif
}

VOID
NTAPI
WRITE_PORT_UCHAR(IN PUCHAR Port,
                 IN UCHAR Value)
{
    __outbyte(H2I(Port), Value);
}

VOID
NTAPI
WRITE_PORT_USHORT(IN PUSHORT Port,
                  IN USHORT Value)
{
    __outword(H2I(Port), Value);
}

VOID
NTAPI
WRITE_PORT_ULONG(IN PULONG Port,
                 IN ULONG Value)
{
    __outdword(H2I(Port), Value);
}

/* EOF */
