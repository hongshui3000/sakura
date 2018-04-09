/*
 * Copyright (c) 2018 hackin, zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include <board.h>
#include <errno.h>
#include <kernel.h>
#include <misc/util.h>
#include <spi.h>
#include <stdbool.h>
#include <stdint.h>
#include <toolchain.h>

/* Driverlib includes */
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/ssi.h>
#include <driverlib/sysctl.h>
#include <hw_ints.h>
#include <inc/hw_memmap.h>

#include "spi_tm4c123.h"

#define DEV_CFG(dev) \
    ((const struct spi_tm4c123_config* const)(dev->config->config_info))

#define DEV_DATA(dev) \
    ((struct spi_tm4c123_data * const)(dev->driver_data))

#ifdef CONFIG_SPI_TM4C123_INTERRUPT
static void spi_tm4c123_isr(void* arg)
{
    struct device* const dev = (struct device*)arg;
    const struct spi_tm4c123_config* cfg = dev->config->config_info;
    struct spi_tm4c123_data* data = dev->driver_data;
    int err;

    err = spi_tm4c123_get_err(spi);
    if (err) {
        spi_tm4c123_complete(data, spi, err);
        return;
    }

    if (spi_tm4c123_transfer_ongoing(data)) {
        err = spi_tm4c123_shift_frames(spi, data);
    }

    if (err || !spi_tm4c123_transfer_ongoing(data)) {
        spi_tm4c123_complete(data, spi, err);
    }
}
#endif

static int spi_tm4c123_release(struct device* dev,
    const struct spi_config* config)
{
    struct spi_tm4c123_data* data = DEV_DATA(dev);

    spi_context_unlock_unconditionally(&data->ctx);

    return 0;
}

static int spi_tm4c123_configure(struct device* dev, const struct spi_config* config)
{
    const struct spi_tm4c123_config* cfg = DEV_CFG(dev);
    struct spi_tm4c123_data* data = DEV_DATA(dev);
    u32_t protocol, mode, bit_rate;

    if (spi_context_configured(&data->ctx, config)) {
        /* Nothing to do */
        return 0;
    }

    if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
        if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
            protocol = SSI_FRF_MOTO_MODE_3;
        } else {
            protocol = SSI_FRF_MOTO_MODE_1;
        }
    } else {
        if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
            protocol = SSI_FRF_MOTO_MODE_2;
        } else {
            protocol = SSI_FRF_MOTO_MODE_0;
        }
    }

    bit_rate = config->frequency;
    if ((cfg->spi_clk / bit_rate) <= (254 * 256))
        return -ENOTSUP;

    if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
        mode = SSI_MODE_MASTER;
        if (bit_rate <= (cfg->spi_clk / 2))
            return -ENOTSUP;
    } else {
        mode = SSI_MODE_SLAVE;
        if (bit_rate <= (cfg->spi_clk / 12))
            return -ENOTSUP;
    }

    if ((SPI_WORD_SIZE_GET(config->operation) < 4)
        || (SPI_WORD_SIZE_GET(config->operation) > 16)) {
        return -ENOTSUP;
    }

    MAP_SSIDisable(cfg->spi_base);

    MAP_SSIConfigSetExpClk(cfg->spi_base, cfg->spi_clk,
        protocol, mode, bit_rate, (SPI_WORD_SIZE_GET(config->operation)));

    MAP_SSIEnable(cfg->spi_base);

    return 0;
}

static int transceive(struct device* dev,
    const struct spi_config* config,
    const struct spi_buf_set* tx_bufs,
    const struct spi_buf_set* rx_bufs,
    bool asynchronous, struct k_poll_signal* signal)
{
    struct spi_tm4c123_data* data = DEV_DATA(dev);
    int ret;

    if (!tx_bufs && !tx_bufs) {
        return 0;
    }

#ifndef CONFIG_SPI_TM4C123_INTERRUPT
    if (asynchronous) {
        return -ENOTSUP;
    }
#endif

    spi_context_lock(&data->ctx, asynchronous, signal);

    ret = spi_tm4c123_configure(dev, config);
    if (ret) {
        goto out;
    }

    /* Set buffers info */
    spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

    /* This is turned off in spi_tm4c123_complete(). */
    spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_SPI_TM4C123_INTERRUPT
/* TODO: send or receive use interrupt */
#else
/* TODO: send or receive not use interrupt */
#endif

    ret = spi_context_wait_for_completion(&data->ctx);

out:
    spi_context_release(&data->ctx, ret);

    return ret ? -EIO : 0;
}

static int spi_tm4c123_transceive(struct device* dev,
    const struct spi_config* config,
    const struct spi_buf_set* tx_bufs,
    const struct spi_buf_set* rx_bufs)
{
    return transceive(dev, config, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_tm4c123_transceive_async(struct device* dev,
    const struct spi_config* config,
    const struct spi_buf_set* tx_bufs,
    const struct spi_buf_set* rx_bufs,
    struct k_poll_signal* async)
{
    return transceive(dev, config, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static const struct spi_driver_api api_funcs = {
    .transceive = spi_tm4c123_transceive,
#ifdef CONFIG_SPI_ASYNC
    .transceive_async = spi_tm4c123_transceive_async,
#endif
    .release = spi_tm4c123_release,
};

static int spi_tm4c123_init(struct device* dev)
{
    const struct spi_tm4c123_config* cfg = DEV_CFG(dev);

    if (!MAP_SysCtlPeripheralPresent(cfg->peripheral)) {
        return -1;
    }

    MAP_SysCtlPeripheralEnable(cfg->peripheral);

    return 0;
}

#ifdef CONFIG_SPI_0

#ifdef CONFIG_SPI_TM4C123_INTERRUPT
static void spi_tm4c123_irq_config_func_0(struct device* port);
#endif

static const struct spi_tm4c123_config spi_tm4c123_cfg_0 = {
    .spi_clk = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
    .spi_base = SSI0_BASE,
    .peripheral = SYSCTL_PERIPH_SSI0,
#ifdef CONFIG_SPI_TM4C123_INTERRUPT
    .irq_config = spi_tm4c123_irq_config_func_0,
#endif
};

static struct spi_tm4c123_data spi_tm4c123_dev_data_0 = {
    SPI_CONTEXT_INIT_LOCK(spi_tm4c123_dev_data_0, ctx),
    SPI_CONTEXT_INIT_SYNC(spi_tm4c123_dev_data_0, ctx),
};

DEVICE_AND_API_INIT(spi_tm4c123_0, CONFIG_SPI_0_NAME, &spi_tm4c123_init,
    &spi_tm4c123_dev_data_0, &spi_tm4c123_cfg_0,
    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
    &api_funcs);

#ifdef CONFIG_SPI_TM4C123_INTERRUPT
static void spi_tm4c123_irq_config_func_0(struct device* dev)
{
    IRQ_CONNECT(CONFIG_SPI_0_IRQ, CONFIG_SPI_0_IRQ_PRI,
        spi_tm4c123_isr, DEVICE_GET(spi_tm4c123_0), 0);
    irq_enable(CONFIG_SPI_0_IRQ);
}
#endif

#endif /* CONFIG_SPI_0 */
