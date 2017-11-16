/*
 * Copyright (c) 2017, clspring team.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>

static int ti_tm4c123_init(struct device* arg)
{
    ARG_UNUSED(arg);

    MAP_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

    //SystemInit();

    return 0;
}

SYS_INIT(ti_tm4c123_init, PRE_KERNEL_1, 0);
