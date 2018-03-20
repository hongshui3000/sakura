/*
 * Copyright (c) 2017, hackin, zhao.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* Push button switch 1 */
#define SW1_GPIO_PIN 6 /* GPIO22/Pin15 */
#define SW1_GPIO_NAME "GPIO_F4"

/* Push button switch 2 */
#define SW2_GPIO_PIN 5 /* GPIO13/Pin4 */
#define SW2_GPIO_NAME "GPIO_F0"

/* Onboard GREEN LED */
#define LEDG_GPIO_PIN 3 /*GPIO11/Pin2 */
#define LEDG_GPIO_PORT "GPIO_F3"

/* Onboard RED LED */
#define LEDR_GPIO_PIN 3 /*GPIO11/Pin2 */
#define LEDR_GPIO_PORT "GPIO_F1"

/* Onboard BLUE LED */
#define LEDB_GPIO_PIN 3 /*GPIO11/Pin2 */
#define LEDB_GPIO_PORT "GPIO_F2"

#endif /* __INC_BOARD_H */
