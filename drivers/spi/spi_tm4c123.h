/*
 * Copyright (c) 2018 hackin, zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SPI_TM4C123_H_
#define _SPI_TM4C123_H_

#include "spi_context.h"

typedef void (*irq_config_func_t)(struct device* port);

struct spi_tm4c123_config {
    unsigned long spi_clk;
    unsigned long spi_base;
    unsigned long peripheral;
#ifdef CONFIG_SPI_TM4C123_INTERRUPT
    irq_config_func_t irq_config;
#endif
};

struct spi_tm4c123_data {
    struct spi_context ctx;
};

#endif /* _SPI_TM4C123_H_ */