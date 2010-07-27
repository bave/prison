#ifndef _RDTSC_H_
#define _RDTSC_H_

#include <stdint.h>

uint64_t __tsc;

#ifdef __x86_64
#define __edxpush  asm volatile("pushq %rdx")
#define __eaxpush  asm volatile("pushq %rax")
#else
#define __edxpush  asm volatile("pushl %edx")
#define __eaxpush  asm volatile("pushl %eax")
#endif

#define __rdtsc    asm volatile("rdtsc")
#define __tsc_HIGH asm volatile("movl %edx, __tsc+4(%rip)")
#define __tsc_LOW  asm volatile("movl %eax, __tsc(%rip)")

#ifdef __x86_64
#define __eaxpop   asm volatile("popq %rax")
#define __edxpop   asm volatile("popq %rdx")
#else
#define __eaxpop   asm volatile("popl %eax")
#define __edxpop   asm volatile("popl %edx")
#endif

inline uint64_t tsc(void)
{
    __edxpush;
    __eaxpush;
    __rdtsc;
    __tsc_HIGH;
    __tsc_LOW;
    __eaxpop;
    __edxpop;
    return __tsc;
}

#endif //_RDTSC_H_
