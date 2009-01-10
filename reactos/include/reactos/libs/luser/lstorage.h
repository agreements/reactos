#ifndef _NTOSKRNL_LUSER_STORAGE_H
#define _NTOSKRNL_LUSER_STORAGE_H

#ifdef __linux__
#include <stdint.h>
typedef uint16_t USHORT;
typedef uint32_t ULONG;
typedef uint64_t ULONGLONG;
#endif

typedef struct { USHORT pad; USHORT Limit; ULONG Base; } LDESCRIPTOR;

struct LuserCPUData {
    LDESCRIPTOR LuserGdt;
    LDESCRIPTOR LuserIdt;
    LDESCRIPTOR LuserLdt;
    ULONGLONG LuserMsr[32];
    ULONG LuserCr[8], LuserDr[8];
    ULONG LuserDs, LuserEs, LuserFs, LuserSs, LuserTr, LuserLdtSel;
    ULONG MemSize;
    ULONG IntPipeRead, DevPipeRead, DevPipeWrite;
    ULONG VideoFd;
} __attribute__((packed));

void ReadLuserCPUData(struct LuserCPUData *data);
void WriteLuserCPUData(struct LuserCPUData *data);

#endif/*_NTOSKRNL_LUSER_STORAGE_H*/
