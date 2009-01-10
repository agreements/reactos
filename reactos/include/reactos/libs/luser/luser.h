#ifndef _NTOSKRNL_LUSER_OVERRIDE_H
#define _NTOSKRNL_LUSER_OVERRIDE_H

#include "lunix.h"
#include "lstorage.h"

extern void LuserSetGlobalDescriptorTable(void *);
extern void LuserGetGlobalDescriptorTable(void *);
extern void LuserSetLocalDescriptorTable(int);
extern int  LuserGetLocalDescriptorTable();
extern void LuserSetInterruptDescriptorTable(void *);
extern void LuserGetInterruptDescriptorTable(void *);
extern void LuserSetTR(int tr);
extern int LuserGetTR();
extern void LuserWriteCR(int cr, unsigned long newCR);
extern unsigned long LuserReadCR(int cr);
extern void LuserWriteDR(int dr, unsigned long newDR);
extern unsigned long LuserReadDR(int dr);
extern void LuserInvalidatePage(void *addr);
extern void LuserWrmsr(unsigned long Msr, unsigned long ValHi, unsigned long ValLo);
extern void LuserRdmsr(unsigned long Msr, unsigned long *ValHi, unsigned long *ValLo);
extern void LuserGetTrapEntry(int Trap, void *IdtEntry);
extern int LuserGetTSS(void *Tss);
extern unsigned int LuserInByte(int dx);
extern unsigned int LuserInWord(int dx);
extern unsigned int LuserInDWord(int dx);
extern void LuserOutByte(int dx, int val);
extern void LuserOutWord(int dx, int val);
extern void LuserOutDWord(int dx, int val);
extern int LuserReadInterrupt();
extern void LuserHaltProcessor();

extern void Printf(const char *fmt, ...);

typedef void (*LUSER_PAGE_FAULT_HANDLER)(siginfo_t *info, void *addr);
extern LUSER_PAGE_FAULT_HANDLER LuserPageFaultHandler;
void LuserDefaultHandlePageFault(siginfo_t *info, void *addr);

void LuserRegisterSigstack();
void LuserRegisterSegv();

#ifdef Ke386SetGlobalDescriptorTable
#undef Ke386SetGlobalDescriptorTable
#endif 
#define Ke386SetGlobalDescriptorTable(x) LuserSetGlobalDescriptorTable(&x)

#ifdef Ke386GetGlobalDescriptorTable
#undef Ke386GetGlobalDescriptorTable
#endif
#define Ke386GetGlobalDescriptorTable(x) LuserGetGlobalDescriptorTable(&x)

#ifdef Ke386SetLocalDescriptorTable
#undef Ke386SetLocalDescriptorTable
#endif 
#define Ke386SetLocalDescriptorTable(x) LuserSetLocalDescriptorTable(x)

#ifdef Ke386GetLocalDescriptorTable
#undef Ke386GetLocalDescriptorTable
#endif
#define Ke386GetLocalDescriptorTable(x) (x) = LuserGetLocalDescriptorTable()

#ifdef Ke386SetInterruptDescriptorTable
#undef Ke386SetInterruptDescriptorTable
#endif
#define Ke386SetInterruptDescriptorTable(x) LuserSetInterruptDescriptorTable(&x)

#ifdef Ke386GetInterruptDescriptorTable
#undef Ke386GetInterruptDescriptorTable
#endif
#define Ke386GetInterruptDescriptorTable(x) LuserGetInterruptDescriptorTable(&x)

#ifdef Ke386SaveFlags
#undef Ke386SaveFlags
#endif
#define Ke386SaveFlags(x)        __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)

#ifdef Ke386RestoreFlags
#undef Ke386RestoreFlags
#endif
#define Ke386RestoreFlags(x)     __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")

static inline void Ki386Cpuid(unsigned long Op, unsigned long *Eax, unsigned long *Ebx, unsigned long *Ecx, unsigned long *Edx)
{
    __asm__("cpuid"
	    : "=a" (*Eax), "=b" (*Ebx), "=c" (*Ecx), "=d" (*Edx)
	    : "0" (Op));
}

#ifdef Ke386Wrmsr
#undef Ke386Wrmsr
#endif
#define Ke386Wrmsr LuserWrmsr

#ifdef Ke386Rdmsr
#undef Ke386Rdmsr
#endif
#define Ke386Rdmsr(msr, hi, lo) LuserRdmsr((msr), (unsigned long *)&(hi), (unsigned long *)&(lo))

#ifdef Ke386FnInit
#undef Ke386FnInit
#endif
#define Ke386FnInit() 

#ifdef Ke386SetTr
#undef Ke386SetTr
#endif
#define Ke386SetTr LuserSetTR

#ifdef Ke386GetTr
#undef Ke386GetTr
#endif
#define Ke386GetTr(x) (x = LuserGetTR())

#ifdef __readcr0
#undef __readcr0
#endif
#define __readcr0() LuserReadCR(0)

#ifdef __writecr0
#undef __writecr0
#endif
#define __writecr0(x) LuserWriteCR(0,x)

#ifdef __readcr1
#undef __readcr1
#endif
#define __readcr1() LuserReadCR(1)

#ifdef __writecr1
#undef __writecr1
#endif
#define __writecr1(x) LuserWriteCR(1,x)

#ifdef __readcr2
#undef __readcr2
#endif
#define __readcr2() LuserReadCR(2)

#ifdef __writecr2
#undef __writecr2
#endif
#define __writecr2(x) LuserWriteCR(2,x)

#ifdef Ke386SetCr2
#undef Ke386SetCr2
#endif
#define Ke386SetCr2 __writecr2

#ifdef __readcr3
#undef __readcr3
#endif
#define __readcr3() LuserReadCR(3)

#ifdef __writecr3
#undef __writecr3
#endif
#define __writecr3(x) LuserWriteCR(3,x)

#ifdef __readcr4
#undef __readcr4
#endif
#define __readcr4() LuserReadCR(4)

#ifdef __writecr4
#undef __writecr4
#endif
#define __writecr4(x) LuserWriteCR(4,x)

#ifdef __readcr5
#undef __readcr5
#endif
#define __readcr5() LuserReadCR(5)

#ifdef __writecr5
#undef __writecr5
#endif
#define __writecr5(x) LuserWriteCR(5,x)

#ifdef __readcr6
#undef __readcr6
#endif
#define __readcr6() LuserReadCR(6)

#ifdef __writecr6
#undef __writecr6
#endif
#define __writecr6(x) LuserWriteCR(6,x)

#ifdef __readcr7
#undef __readcr7
#endif
#define __readcr7() LuserReadCR(7)

#ifdef __writecr7
#undef __writecr7
#endif
#define __writecr7(x) LuserWriteCR(7,x)

#ifdef __readdr0
#undef __readdr0
#endif
#define __readdr0() LuserReadDR(0)

#ifdef Ke386GetDr0
#undef Ke386GetDr0
#endif
#define Ke386GetDr0 __readdr0

#ifdef __writedr0
#undef __writedr0
#endif
#define __writedr0(x) LuserWriteDR(0,x)

#ifdef Ke386SetDr0
#undef Ke386SetDr0
#endif
#define Ke386SetDr0 __writedr0

#ifdef __readdr1
#undef __readdr1
#endif
#define __readdr1() LuserReadDR(1)

#ifdef Ke386GetDr1
#undef Ke386GetDr1
#endif
#define Ke386GetDr1 __readdr1

#ifdef __writedr1
#undef __writedr1
#endif
#define __writedr1(x) LuserWriteDR(1,x)

#ifdef Ke386SetDr1
#undef Ke386SetDr1
#endif
#define Ke386SetDr1 __writedr1

#ifdef __readdr2
#undef __readdr2
#endif
#define __readdr2() LuserReadDR(2)

#ifdef Ke386GetDr2
#undef Ke386GetDr2
#endif
#define Ke386GetDr2 __readdr2

#ifdef __writedr2
#undef __writedr2
#endif
#define __writedr2(x) LuserWriteDR(2,x)

#ifdef Ke386SetDr2
#undef Ke386SetDr2
#endif
#define Ke386SetDr2 __writedr2

#ifdef __readdr3
#undef __readdr3
#endif
#define __readdr3() LuserReadDR(3)

#ifdef Ke386GetDr3
#undef Ke386GetDr3
#endif
#define Ke386GetDr3 __readdr3

#ifdef __writedr3
#undef __writedr3
#endif
#define __writedr3(x) LuserWriteDR(3,x)

#ifdef Ke386SetDr3
#undef Ke386SetDr3
#endif
#define Ke386SetDr3 __writedr3

#ifdef __readdr4
#undef __readdr4
#endif
#define __readdr4() LuserReadDR(4)

#ifdef Ke386GetDr4
#undef Ke386GetDr4
#endif
#define Ke386GetDr4 __readdr4

#ifdef __writedr4
#undef __writedr4
#endif
#define __writedr4(x) LuserWriteDR(4,x)

#ifdef Ke386SetDr4
#undef Ke386SetDr4
#endif
#define Ke386SetDr4 __writedr4

#ifdef __readdr5
#undef __readdr5
#endif
#define __readdr5() LuserReadDR(5)

#ifdef Ke386GetDr5
#undef Ke386GetDr5
#endif
#define Ke386GetDr5 __readdr5

#ifdef __writedr5
#undef __writedr5
#endif
#define __writedr5(x) LuserWriteDR(5,x)

#ifdef Ke386SetDr5
#undef Ke386SetDr5
#endif
#define Ke386SetDr5 __writedr5

#ifdef __readdr6
#undef __readdr6
#endif
#define __readdr6() LuserReadDR(6)

#ifdef Ke386GetDr6
#undef Ke386GetDr6
#endif
#define Ke386GetDr6 __readdr6

#ifdef __writedr6
#undef __writedr6
#endif
#define __writedr6(x) LuserWriteDR(6,x)

#ifdef Ke386SetDr6
#undef Ke386SetDr6
#endif
#define Ke386SetDr6 __writedr6

#ifdef __readdr7
#undef __readdr7
#endif
#define __readdr7() LuserReadDR(7)

#ifdef Ke386GetDr7
#undef Ke386GetDr7
#endif
#define Ke386GetDr7 __readdr7

#ifdef __writedr7
#undef __writedr7
#endif
#define __writedr7(x) LuserWriteDR(7,x)

#ifdef Ke386SetDr7
#undef Ke386SetDr7
#endif
#define Ke386SetDr7 __writedr7

#ifdef __invlpg
#undef __invlpg
#endif
#define __invlpg LuserInvalidatePage

#endif/*_NTOSKRNL_LUSER_OVERRIDE_H*/
