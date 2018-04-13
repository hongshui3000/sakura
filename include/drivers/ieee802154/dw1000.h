/*
 * Copyright (c) 2018 hackin zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DW1000_H__
#define __DW1000_H__

#include <device.h>

enum dw1000_gpio_index {
    DW1000_GPIO_IDX_ISR = 0,
    DW1000_GPIO_IDX_WAKEUP,
    DW1000_GPIO_IDX_RST,
    DW1000_GPIO_IDX_EXTON,
    DW1000_GPIO_IDX_GPIO_0,
    DW1000_GPIO_IDX_GPIO_1,
    DW1000_GPIO_IDX_GPIO_2,
    DW1000_GPIO_IDX_GPIO_3,
    DW1000_GPIO_IDX_GPIO_4,
    DW1000_GPIO_IDX_GPIO_5, /* SPI POL */
    DW1000_GPIO_IDX_GPIO_6, /* SPI PHA */
    DW1000_GPIO_IDX_MAX,
};

struct dw1000_gpio_configuration {
    struct device* dev;
    u32_t pin;
};

struct dw1000_gpio_configuration* dw1000_configure_gpios(void);

#endif /* __DW1000_H__ */
