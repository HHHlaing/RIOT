/*
 * Copyright 2008-2009, Freie Universitaet Berlin (FUB). All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_arm7_common
 * @{
 */

/**
 * @file
 * @internal
 * @brief       ARM bootloader
 *
 * @author      Heiko Will <hwill@inf.fu-berlin.de>
 * @author      Michael Baar <michael.baar@fu-berlin.de>
 * @version     $Revision$
 * @since       19.08.2008
 *
 * @note        $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

#include "log.h"

void FIQ_Routine(void)   __attribute__((interrupt("FIQ")));
//void SWI_Routine (void)   __attribute__((interrupt("SWI")));
void UNDEF_Routine(void) __attribute__((interrupt("UNDEF")));

/*-----------------------------------------------------------------------------------*/
void FIQ_Routine(void)
{
    LOG_ERROR("Kernel Panic,\nEarly FIQ call\n");

    while (1) {};
}
/*-----------------------------------------------------------------------------------*/
void SWI_Routine(void)
{
    LOG_ERROR("Kernel Panic,\nEarly SWI call\n");

    while (1) {};
}
/*-----------------------------------------------------------------------------------*/
volatile int arm_abortflag = 0;

void abtorigin(const char *vector, unsigned long *lnk_ptr1)
{
    register unsigned long *lnk_ptr2;
    register unsigned long *sp;
    register unsigned int   cpsr, spsr;

    __asm__ __volatile__("mrs %0, cpsr" : "=r"(cpsr));          // copy current mode
    __asm__ __volatile__("mrs %0, spsr" : "=r"(spsr));          // copy dabt generating mode
    __asm__ __volatile__("msr cpsr_c, %0" :: "r"(spsr));        // switch to dabt generating mode
    __asm__ __volatile__("mov %0, lr" : "=r"(lnk_ptr2));        // copy lr
    __asm__ __volatile__("mov %0, sp" : "=r"(sp));              // copy sp
    __asm__ __volatile__("msr cpsr_c, %0" :: "r"(cpsr));        // switch back to abt mode

    LOG_ERROR("#!%s abort at %p (0x%08lX) originating from %p (0x%08lX) in mode 0x%X\n",
           vector, (void *)lnk_ptr1, *(lnk_ptr1), (void *)lnk_ptr2, *(lnk_ptr2), spsr
          );

    while (1) {};
}
/*-----------------------------------------------------------------------------------*/
void UNDEF_Routine(void)
{
    /* cppcheck-suppress variableScope
     * (reason: used within __asm__ which cppcheck doesn't pick up) */
    register unsigned long *lnk_ptr;
    __asm__ __volatile__("sub %0, lr, #8" : "=r"(lnk_ptr));     // get aborting instruction

    if (arm_abortflag == 0) {
        arm_abortflag = 1;                                          // remember state (if printing should fail again)
        abtorigin("undef", lnk_ptr);
    }

    while (1) {};
}
/*-----------------------------------------------------------------------------------*/
void PABT_Routine(void)
{
    /* cppcheck-suppress variableScope
     * (reason: used within __asm__ which cppcheck doesn't pick up) */
    register unsigned long *lnk_ptr;
    __asm__ __volatile__("sub %0, lr, #8" : "=r"(lnk_ptr));     // get aborting instruction

    if (arm_abortflag == 0) {
        arm_abortflag = 1;                                          // remember state (if printing should fail again)
        abtorigin("pabt", lnk_ptr);
    }

    while (1) {};
}
/*-----------------------------------------------------------------------------------*/
void DABT_Routine(void)
{
    /* cppcheck-suppress variableScope
     * (reason: used within __asm__ which cppcheck doesn't pick up) */
    register unsigned long *lnk_ptr;
    __asm__ __volatile__("sub %0, lr, #8" : "=r"(lnk_ptr));     // get aborting instruction

    if (arm_abortflag == 0) {
        arm_abortflag = 1;                                          // remember state (if printing should fail again)
        abtorigin("data", lnk_ptr);
    }

    while (1) {};
}
/*-----------------------------------------------------------------------------------*/
static inline void
bl_init_data(void)
{
    extern unsigned int _etext;
    extern unsigned int _data;
    extern unsigned int _edata;
    extern unsigned int __bss_start;
    extern unsigned int __bss_end;

    register unsigned int *p1;
    register unsigned int *p2;
    register unsigned int *p3;

    // initialize data from flash
    // (linker script ensures that data is 32-bit aligned)
    p1 = &_etext;
    p2 = &_data;
    p3 = &_edata;

    while (p2 < p3) {
        *p2++ = *p1++;
    }

    // clear bss
    // (linker script ensures that bss is 32-bit aligned)
    p1 = &__bss_start;
    p2 = &__bss_end;

    while (p1 < p2) {
        *p1++ = 0;
    }
}
/*-----------------------------------------------------------------------------------*/
void bootloader(void)
{
    extern void bl_init_ports(void);
    extern void bl_init_clks(void);

    /* board specific setup of clocks */
    bl_init_clks();

    /* initialize bss and data */
    bl_init_data();

    /* board specific setup of i/o pins */
    bl_init_ports();

#ifdef MODULE_NEWLIB
    extern void __libc_init_array(void);
    __libc_init_array();
#endif
}

/** @} */
