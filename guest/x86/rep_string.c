/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "vmalloc.h"
#include "alloc_page.h"

#define MSR_IA32_MISC_ENABLE                0x000001a0
#define LOW_MASK	(0xffffffff00000000ull)
#define ALL_MASK	(0xffffffffffffffffull)

#define TEST(opcode)								\
	do {									\
	asm volatile(".byte 0xf2 \n\t .byte 0x67 \n\t .byte " opcode "\n\t"	\
			: "=S"(s), "=c"(c), "=D"(d)				\
			: "S"(ALL_MASK), "c"(LOW_MASK), "D"(ALL_MASK));		\
	printf("opcode %s rcx=%llx rsi=%llx rdi=%llx\n",			\
			opcode, c, s, d);					\
	} while(0)

static inline uint64_t cpu_msr_read(uint32_t reg)
{
	 uint32_t  msrl, msrh;
	 asm volatile (" rdmsr ":"=a"(msrl), "=d"(msrh) : "c" (reg));
	 return (((uint64_t)msrh << 32U) | msrl);
}

static inline void cpu_msr_write(uint32_t reg, uint64_t msr_val)
{
	asm volatile (" wrmsr " : : "c" (reg), "a" ((uint32_t)msr_val), "d" ((uint32_t)(msr_val >> 32U)));
}

int main(int ac, char **av)
{
	unsigned long long s, d, c;
	unsigned long long tmp64;
	TEST("0x6c");
	TEST("0x6d");
	TEST("0x6e");
	TEST("0x6f");
	TEST("0xa4");
	TEST("0xa5");
	TEST("0xa6");
	TEST("0xa7");
	TEST("0xaa");
	TEST("0xab");
	TEST("0xae");
	TEST("0xaf");

	printf("disable fast string\n");
	tmp64 = cpu_msr_read(MSR_IA32_MISC_ENABLE);
	tmp64 = tmp64 & 0xfffffffffffffffe;
        cpu_msr_write (MSR_IA32_MISC_ENABLE, tmp64);
        

	TEST("0x6c");
        TEST("0x6d");
        TEST("0x6e");
        TEST("0x6f");
        TEST("0xa4");
        TEST("0xa5");
        TEST("0xa6");
        TEST("0xa7");
        TEST("0xaa");
        TEST("0xab");
        TEST("0xae");
        TEST("0xaf");


	return 0;

}
