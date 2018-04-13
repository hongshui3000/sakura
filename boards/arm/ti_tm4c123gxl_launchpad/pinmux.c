/**
 * 
 * Configure the device pins for different signals
 * 
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 * 
 *
 *  Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *    Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *    Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the
 *     distribution.
 * 
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/** 
 * This file was automatically generated on 2017/10/12 at am 12:34:13
 * by TI PinMux version 4.0.1496
 * (Then modified to meet Zephyr coding style)
 *
 */
#include <init.h>
#include <stdbool.h>
#include <stdint.h>

#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <inc/hw_gpio.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <pinmux.h>

/**
 *@brief Configures the device pins for the customer specific usage.
 */
int pinmux_initialize(struct device* port)
{

#if defined(CONFIG_UART_TM4C123) || defined(CONFIG_SPI_0)
    /* Enable Peripheral Clocks */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
#endif
#if defined(CONFIG_GPIO_TM4C123_F0) || defined(CONFIG_GPIO_TM4C123_F1) || defined(CONFIG_GPIO_TM4C123_F2) || defined(CONFIG_GPIO_TM4C123_F3) || defined(CONFIG_GPIO_TM4C123_F4)

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    /* Unlock the Port Pin and Set the Commit Bit */
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= GPIO_PIN_0;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0x0;
#endif
#ifdef CONFIG_GPIO_TM4C123_F4
    /* Configure the GPIO Pin Mux for PF4 for GPIO_PF4 */
    MAP_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    MAP_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
#endif
#ifdef CONFIG_GPIO_TM4C123_F0
    /* Configure the GPIO Pin Mux for PF0 for GPIO_PF0 */
    MAP_GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0);
    MAP_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
#endif
#ifdef CONFIG_GPIO_TM4C123_F1
    /* Configure the GPIO Pin Mux for PF1 for GPIO_PF1 */
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
#endif
#ifdef CONFIG_GPIO_TM4C123_F3
    /* Configure the GPIO Pin Mux for PF3 for GPIO_PF3 */
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);
#endif
#ifdef CONFIG_GPIO_TM4C123_F2
    /* Configure the GPIO Pin Mux for PF2 for GPIO_PF2 */
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
#endif
#ifdef CONFIG_UART_TM4C123

    /* Configure the GPIO Pin Mux for PA0 for U0RX */
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0);

    /* Configure the GPIO Pin Mux for PA1 for U0TX */
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1);
#endif

#ifdef CONFIG_SPI_0

    /* Configure the GPIO Pin Mux for PA4 for SSI0RX */
    MAP_GPIOPinConfigure(GPIO_PA4_SSI0RX);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_4);

    /* Configure the GPIO Pin Mux for PA3 for SSI0FSS */
    MAP_GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_3);

    /* Configure the GPIO Pin Mux for PA5 for SSI0TX */
    MAP_GPIOPinConfigure(GPIO_PA5_SSI0TX);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5);

    /* Configure the GPIO Pin Mux for PA2 for SSI0CLK */
    MAP_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2);
#endif
    return 0;
}

SYS_INIT(pinmux_initialize, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
