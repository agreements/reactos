#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ndk/ntndk.h>
#include <luser.h>
#include <lunix.h>
#include <lstorage.h>
#include <unixmap.h>

void ReadLuserCPUData(struct LuserCPUData *data);

unsigned int LuserIn(int dx, int len)
{
    struct LuserCPUData cpu;
    int pipemsg[3] = { };
    pipemsg[0] = len;
    pipemsg[1] = dx;
    ReadLuserCPUData(&cpu);
    if (unix_write(cpu.DevPipeWrite, (char *)pipemsg, sizeof(pipemsg)) < 0)
        unix_abort();
    if (unix_read(cpu.DevPipeRead, (char *)pipemsg, sizeof(pipemsg)) < 0)
        unix_abort();
    return pipemsg[2];
}

void LuserOut(int dx, int val, int len)
{
    struct LuserCPUData cpu;
    int pipemsg[3] = { };
    pipemsg[0] = 0x80 | len;
    pipemsg[1] = dx;
    pipemsg[2] = val;
    ReadLuserCPUData(&cpu);
    if (unix_write(cpu.DevPipeWrite, (char *)pipemsg, sizeof(pipemsg)) < 0)
        unix_abort();
    if (unix_read(cpu.DevPipeRead, (char *)pipemsg, sizeof(pipemsg)) < 0)
        unix_abort();
}

unsigned int LuserInByte(int dx)
{
    return LuserIn(dx, 1);
}

unsigned int LuserInWord(int dx)
{
    return LuserIn(dx, 2);
}

unsigned int LuserInDWord(int dx)
{
    return LuserIn(dx, 4);
}

void LuserOutByte(int dx, int val)
{
    LuserOut(dx, val, 1);
}

void LuserOutWord(int dx, int val)
{
    LuserOut(dx, val, 2);
}

void LuserOutDWord(int dx, int val)
{
    LuserOut(dx, val, 4);
}

int LuserReadInterrupt()
{
    char interrupt;
    struct LuserCPUData cpu;
    ReadLuserCPUData(&cpu);
    if (unix_read(cpu.IntPipeRead, &interrupt, 1) == 1)
        return interrupt;
    else return -1;
}

void LuserHaltProcessor()
{
    struct timespec ts = { 1, 0 };
    unix_nanosleep(&ts, NULL);
}
