/*
 * Copyright (c) 2018, hackin, zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <device.h>
#include <gpio.h>
#include <init.h>
#include <kernel.h>
#include <sys_io.h>

/* Driverlib includes */
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <hw_ints.h>
#include <inc/hw_gpio.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>

//#undef __GPIO_H__ /* Zephyr and TM4C123xx SDK gpio.h conflict */
#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>

#include "gpio_utils.h"

/* Note: Zephyr uses exception numbers, vs the IRQ #s used by the TM4C123 SDK */
#define EXCEPTION_GPIOF0 30 /* (INT_GPIOF - 16) = (46-16) */
#define EXCEPTION_GPIOF1 30 /* (INT_GPIOF - 16) = (46-16) */
#define EXCEPTION_GPIOF2 30 /* (INT_GPIOF - 16) = (46-16) */
#define EXCEPTION_GPIOF3 30 /* (INT_GPIOF - 16) = (46-16) */
#define EXCEPTION_GPIOF4 30 /* (INT_GPIOF - 16) = (46-16) */

struct gpio_tm4c123_config {
    /* base address of GPIO port */
    unsigned long port_base;
    /* GPIO IRQ number */
    unsigned long irq_num;
};

struct gpio_tm4c123_data {
    /* list of registered callbacks */
    sys_slist_t callbacks;
    /* callback enable pin bitmask */
    u32_t pin_callback_enables;
};

#define DEV_CFG(dev) \
    ((const struct gpio_tm4c123_config*)(dev)->config->config_info)
#define DEV_DATA(dev) \
    ((struct gpio_tm4c123_data*)(dev)->driver_data)

static inline int gpio_tm4c123_config(struct device* port,
    int access_op, u32_t pin, int flags)
{
    const struct gpio_tm4c123_config* gpio_config = DEV_CFG(port);
    unsigned long port_base = gpio_config->port_base;
    unsigned long int_type;

    /*
	 * See pinmux_initialize(): which leverages TI's recommended
	 * method of using the PinMux utility for most pin configuration.
	 */

    if (access_op == GPIO_ACCESS_BY_PIN) {
        /* Just handle runtime interrupt type config here: */
        if (flags & GPIO_INT) {
            if (flags & GPIO_INT_EDGE) {
                if (flags & GPIO_INT_ACTIVE_HIGH) {
                    int_type = GPIO_RISING_EDGE;
                } else if (flags & GPIO_INT_DOUBLE_EDGE) {
                    int_type = GPIO_BOTH_EDGES;
                } else {
                    int_type = GPIO_FALLING_EDGE;
                }
            } else { /* GPIO_INT_LEVEL */
                if (flags & GPIO_INT_ACTIVE_HIGH) {
                    int_type = GPIO_HIGH_LEVEL;
                } else {
                    int_type = GPIO_LOW_LEVEL;
                }
            }
            MAP_GPIOIntTypeSet(port_base, (1 << pin), int_type);
            GPIOIntClear(port_base, (1 << pin));
            GPIOIntEnable(port_base, (1 << pin));
        }
    } else {
        return -ENOTSUP;
    }

    return 0;
}

static inline int gpio_tm4c123_write(struct device* port,
    int access_op, u32_t pin, u32_t value)
{
    const struct gpio_tm4c123_config* gpio_config = DEV_CFG(port);
    unsigned long port_base = gpio_config->port_base;

    if (access_op == GPIO_ACCESS_BY_PIN) {
        value = value << pin;
        /* Bitpack external GPIO pin number for GPIOPinWrite API: */
        pin = 1 << pin;

        MAP_GPIOPinWrite(port_base, (unsigned char)pin, value);
    } else {
        return -ENOTSUP;
    }

    return 0;
}

static inline int gpio_tm4c123_read(struct device* port,
    int access_op, u32_t pin, u32_t* value)
{
    const struct gpio_tm4c123_config* gpio_config = DEV_CFG(port);
    unsigned long port_base = gpio_config->port_base;
    long status;
    unsigned char pin_packed;

    if (access_op == GPIO_ACCESS_BY_PIN) {
        /* Bitpack external GPIO pin number for GPIOPinRead API: */
        pin_packed = 1 << pin;
        status = MAP_GPIOPinRead(port_base, pin_packed);
        *value = status >> pin;
    } else {
        return -ENOTSUP;
    }

    return 0;
}

static int gpio_tm4c123_manage_callback(struct device* dev,
    struct gpio_callback* callback, bool set)
{
    struct gpio_tm4c123_data* data = DEV_DATA(dev);

    _gpio_manage_callback(&data->callbacks, callback, set);

    return 0;
}

static int gpio_tm4c123_enable_callback(struct device* dev,
    int access_op, u32_t pin)
{
    struct gpio_tm4c123_data* data = DEV_DATA(dev);

    if (access_op == GPIO_ACCESS_BY_PIN) {
        data->pin_callback_enables |= (1 << pin);
    } else {
        data->pin_callback_enables = 0xFFFFFFFF;
    }

    return 0;
}

static int gpio_tm4c123_disable_callback(struct device* dev,
    int access_op, u32_t pin)
{
    struct gpio_tm4c123_data* data = DEV_DATA(dev);

    if (access_op == GPIO_ACCESS_BY_PIN) {
        data->pin_callback_enables &= ~(1 << pin);
    } else {
        data->pin_callback_enables = 0;
    }

    return 0;
}

static void gpio_tm4c123_port_isr(void* arg)
{
    struct device* dev = arg;
    const struct gpio_tm4c123_config* config = DEV_CFG(dev);
    struct gpio_tm4c123_data* data = DEV_DATA(dev);
    u32_t enabled_int, int_status;

    /* See which interrupts triggered: */
    int_status = (u32_t)GPIOIntStatus(config->port_base, 1);

    enabled_int = int_status & data->pin_callback_enables;

    /* Clear and Disable GPIO Interrupt */
    GPIOIntDisable(config->port_base, int_status);
    GPIOIntClear(config->port_base, int_status);

    /* Call the registered callbacks */
    _gpio_fire_callbacks(&data->callbacks, (struct device*)dev,
        enabled_int);

    /* Re-enable the interrupts */
    GPIOIntEnable(config->port_base, int_status);
}

static const struct gpio_driver_api api_funcs = {
    .config = gpio_tm4c123_config,
    .write = gpio_tm4c123_write,
    .read = gpio_tm4c123_read,
    .manage_callback = gpio_tm4c123_manage_callback,
    .enable_callback = gpio_tm4c123_enable_callback,
    .disable_callback = gpio_tm4c123_disable_callback,

};

#ifdef CONFIG_GPIO_TM4C123_F0
static const struct gpio_tm4c123_config gpio_tm4c123_f0_config = {
    .port_base = GPIO_PORTF_BASE,
    .irq_num = INT_GPIOF,
};

static struct device DEVICE_NAME_GET(gpio_tm4c123_f0);
static struct gpio_tm4c123_data gpio_tm4c123_f0_data;

static int gpio_tm4c123_f0_init(struct device* dev)
{
    ARG_UNUSED(dev);

    IRQ_CONNECT(EXCEPTION_GPIOF0, CONFIG_GPIO_TM4C123_F0_IRQ_PRI,
        gpio_tm4c123_port_isr, DEVICE_GET(gpio_tm4c123_f0), 0);

    MAP_IntPendClear(INT_GPIOF);
    irq_enable(EXCEPTION_GPIOF0);

    return 0;
}

DEVICE_AND_API_INIT(gpio_tm4c123_f0, CONFIG_GPIO_TM4C123_F0_NAME,
    &gpio_tm4c123_f0_init, &gpio_tm4c123_f0_data,
    &gpio_tm4c123_f0_config,
    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
    &api_funcs);

#endif

#ifdef CONFIG_GPIO_TM4C123_F1
static const struct gpio_tm4c123_config gpio_tm4c123_f1_config = {
    .port_base = GPIO_PORTF_BASE,
    .irq_num = INT_GPIOF,
};

static struct device DEVICE_NAME_GET(gpio_tm4c123_f1);
static struct gpio_tm4c123_data gpio_tm4c123_f1_data;

static int gpio_tm4c123_f1_init(struct device* dev)
{
    ARG_UNUSED(dev);

    IRQ_CONNECT(EXCEPTION_GPIOF1, CONFIG_GPIO_TM4C123_F1_IRQ_PRI,
        gpio_tm4c123_port_isr, DEVICE_GET(gpio_tm4c123_f1), 0);

    MAP_IntPendClear(INT_GPIOF);
    irq_enable(EXCEPTION_GPIOF1);

    return 0;
}

DEVICE_AND_API_INIT(gpio_tm4c123_f1, CONFIG_GPIO_TM4C123_F1_NAME,
    &gpio_tm4c123_f1_init, &gpio_tm4c123_f1_data,
    &gpio_tm4c123_f1_config,
    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
    &api_funcs);

#endif /* CONFIG_GPIO_TM4C123_F1 */

#ifdef CONFIG_GPIO_TM4C123_F2
static const struct gpio_tm4c123_config gpio_tm4c123_f2_config = {
    .port_base = GPIO_PORTF_BASE,
    .irq_num = INT_GPIOF,
};

static struct device DEVICE_NAME_GET(gpio_tm4c123_f2);
static struct gpio_tm4c123_data gpio_tm4c123_f2_data;

static int gpio_tm4c123_f2_init(struct device* dev)
{
    ARG_UNUSED(dev);

    IRQ_CONNECT(EXCEPTION_GPIOF2, CONFIG_GPIO_TM4C123_F2_IRQ_PRI,
        gpio_tm4c123_port_isr, DEVICE_GET(gpio_tm4c123_f2), 0);

    MAP_IntPendClear(INT_GPIOF);
    irq_enable(EXCEPTION_GPIOF2);

    return 0;
}

DEVICE_AND_API_INIT(gpio_tm4c123_f2, CONFIG_GPIO_TM4C123_F2_NAME,
    &gpio_tm4c123_f2_init, &gpio_tm4c123_f2_data,
    &gpio_tm4c123_f2_config,
    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
    &api_funcs);

#endif

#ifdef CONFIG_GPIO_TM4C123_F3
static const struct gpio_tm4c123_config gpio_tm4c123_f3_config = {
    .port_base = GPIO_PORTF_BASE,
    .irq_num = INT_GPIOF,
};

static struct device DEVICE_NAME_GET(gpio_tm4c123_f3);
static struct gpio_tm4c123_data gpio_tm4c123_f3_data;

static int gpio_tm4c123_f3_init(struct device* dev)
{
    ARG_UNUSED(dev);

    IRQ_CONNECT(EXCEPTION_GPIOF3, CONFIG_GPIO_TM4C123_F3_IRQ_PRI,
        gpio_tm4c123_port_isr, DEVICE_GET(gpio_tm4c123_f3), 0);

    MAP_IntPendClear(INT_GPIOF);
    irq_enable(EXCEPTION_GPIOF3);

    return 0;
}

DEVICE_AND_API_INIT(gpio_tm4c123_f3, CONFIG_GPIO_TM4C123_F3_NAME,
    &gpio_tm4c123_f3_init, &gpio_tm4c123_f3_data,
    &gpio_tm4c123_f3_config,
    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
    &api_funcs);

#endif

#ifdef CONFIG_GPIO_TM4C123_F4
static const struct gpio_tm4c123_config gpio_tm4c123_f4_config = {
    .port_base = GPIO_PORTF_BASE,
    .irq_num = INT_GPIOF,
};

static struct device DEVICE_NAME_GET(gpio_tm4c123_f4);
static struct gpio_tm4c123_data gpio_tm4c123_f4_data;

static int gpio_tm4c123_f4_init(struct device* dev)
{
    ARG_UNUSED(dev);

    IRQ_CONNECT(EXCEPTION_GPIOF4, CONFIG_GPIO_TM4C123_F4_IRQ_PRI,
        gpio_tm4c123_port_isr, DEVICE_GET(gpio_tm4c123_f4), 0);

    MAP_IntPendClear(INT_GPIOF);
    irq_enable(EXCEPTION_GPIOF4);

    return 0;
}

DEVICE_AND_API_INIT(gpio_tm4c123_f4, CONFIG_GPIO_TM4C123_F4_NAME,
    &gpio_tm4c123_f4_init, &gpio_tm4c123_f4_data,
    &gpio_tm4c123_f4_config,
    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
    &api_funcs);

#endif
