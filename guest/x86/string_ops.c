/*
 * Test for x86 cache and memory instructions
 *
 * Copyright (c) 2015 Red Hat Inc
 *
 * Authors:
 *  Eduardo Habkost <ehabkost@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "vmalloc.h"
#include "alloc_page.h"

#define MSR_IA32_PAT                        0x00000277
#define MSR_IA32_MISC_ENABLE		    0x000001a0
#define MAX_PHY_ADDR			    0x20000000
#define TEST_STRLEN 17
#define SHIFT_OFF 1
const char movsb_str[TEST_STRLEN] = "0123456789012345";
static inline uint64_t cpu_msr_read(uint32_t reg)
{
        uint32_t  msrl, msrh;

        asm volatile (" rdmsr ":"=a"(msrl), "=d"(msrh) : "c" (reg));
        return (((uint64_t)msrh << 32U) | msrl);
}

/* Write MSR */
static inline void cpu_msr_write(uint32_t reg, uint64_t msr_val)
{
        asm volatile (" wrmsr " : : "c" (reg), "a" ((uint32_t)msr_val), "d" ((uint32_t)(msr_val >> 32U)));
}


static inline void movsb_test(void *dest, void *src, size_t n)
{
	void *dest_p;
	dest_p = dest - SHIFT_OFF;
	printf("move %ld bytes here:\n", n);
	printf("SRC:%s\t DES:%s\n", movsb_str, (char *)dest_p);
	asm volatile("wbinvd":::"memory");
	asm volatile("mfence");
	asm volatile("cld");
        asm volatile("rep ; movsb"
                        : "+D"(dest), "+S"(src)
                        : "c"(n));
	asm volatile("wbinvd":::"memory");
	asm volatile("mfence");
	printf("DES:%s\n", (char *)dest_p);
	memcpy((void *)(dest_p), (void *)movsb_str, TEST_STRLEN);
}

static void test_last_page(void)
{
	phys_addr_t phy_addr;
	void *test_vaddr, *src_vaddr;
	/* Hard code the address of the last page  */
	phy_addr = (phys_addr_t) (MAX_PHY_ADDR-PAGE_SIZE);
	test_vaddr = vmap(phy_addr, PAGE_SIZE);
	printf("Do movsb testing using the last page\n");
	printf("phy_addr=0x%lx, test_vaddr=%p\n",phy_addr, test_vaddr);
	src_vaddr = (void *)(phy_addr+PAGE_SIZE-TEST_STRLEN);
	test_vaddr = test_vaddr+PAGE_SIZE-TEST_STRLEN;
	memcpy(src_vaddr, (void *)movsb_str, TEST_STRLEN);

	movsb_test((test_vaddr+1), src_vaddr, 15);
}

int main(int ac, char **av)
{
	uint64_t tmp64 = 0UL;
	phys_addr_t phy_addr;
	void *test_vaddr;
	void *no_overlap_addr;
        tmp64 = cpu_msr_read(MSR_IA32_PAT);
        printf("MSR_IA32_PAT=0x%lx\n", tmp64);
	tmp64 = cpu_msr_read(MSR_IA32_MISC_ENABLE);
	printf("MSR_IA32_MISC_ENABLE=0x%lx\n", tmp64);

	setup_vm();

	test_last_page();

	phy_addr = (phys_addr_t) alloc_page();
	printf("phy_addr=0x%lx\n", phy_addr);
	/* alias mappings, memory type is WB  */
	test_vaddr = vmap(phy_addr, PAGE_SIZE);
	printf("test_vaddr=%p\n", test_vaddr);
	memcpy((void *)phy_addr, (void *)movsb_str, 17);
	printf("source string:%s\n", (char *)phy_addr);

	no_overlap_addr = (void *)phy_addr+0x100;
	printf("No overlap case:\n");
	memcpy((void *)no_overlap_addr, (void *)movsb_str, 17);
	movsb_test((void *)(no_overlap_addr+1), (void *)phy_addr, 3);
	movsb_test((void *)(no_overlap_addr+1), (void *)phy_addr, 7);
	movsb_test((void *)(no_overlap_addr+1), (void *)phy_addr, 13);
	movsb_test((void *)(no_overlap_addr+1), (void *)phy_addr, 15);

	printf("Overlap case:source addess is larger than dest address\n");
	movsb_test((void *)(test_vaddr+1), (void *)(phy_addr+2), 7);
	printf("source addess is smaller than dest address\n");
	movsb_test((void *)(test_vaddr+1), (void *)phy_addr, 3);
	movsb_test((void *)(test_vaddr+1), (void *)phy_addr, 7);
	movsb_test((void *)(test_vaddr+1), (void *)phy_addr, 13);
	movsb_test((void *)(test_vaddr+1), (void *)phy_addr, 15);

	tmp64 = tmp64 & 0xfffffffffffffffe;
	cpu_msr_write (MSR_IA32_MISC_ENABLE, tmp64);
	tmp64 = cpu_msr_read(MSR_IA32_MISC_ENABLE);
	printf("Disalbe fast string feature. MSR_IA32_MISC_ENABLE=0x%lx\n", tmp64);

	printf("Overlap case and fast string is disable\n");
	movsb_test((void *)(test_vaddr+1), (void *)phy_addr, 3);
	movsb_test((void *)(test_vaddr+1), (void *)phy_addr, 7);
	movsb_test((void *)(test_vaddr+1), (void *)phy_addr, 13);
	movsb_test((void *)(test_vaddr+1), (void *)phy_addr, 15);

	return 0;

}
