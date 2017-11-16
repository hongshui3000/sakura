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

    SystemInit();

    return 0;
}

SYS_INIT(ti_tm4c123_init, PRE_KERNEL_1, 0);
