/*
 * Copyright (c) 2018 hackin, zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include <stdint.h>
#include <uart.h>

/* Driverlib includes */
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/ssi.h>
#include <driverlib/sysctl.h>
#include <hw_ints.h>

#define CONFIG_CFG(cfg) \
    ((const struct spi_tm4c123_config* const)(cfg)->dev->config->config_info)

#define CONFIG_DATA(cfg) \
    ((struct spi_tm4c123_data * const)(cfg)->dev->driver_data)

/* Value to shift out when no application data needs transmitting. */
#define SPI_TM4C123_TX_NOP 0x00

#ifdef CONFIG_SPI_TM4C123_INTERRUPT
static void spi_tm4c123_isr(void* arg)
{
    struct device* const dev = (struct device*)arg;
    const struct spi_tm4c123_config* cfg = dev->config->config_info;
    struct spi_tm4c123_data* data = dev->driver_data;
    SPI_TypeDef* spi = cfg->spi;
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

static int spi_tm4c123_release(struct spi_config* config)
{
    struct spi_tm4c123_data* data = CONFIG_DATA(config);

    spi_context_unlock_unconditionally(&data->ctx);

    return 0;
}

static int spi_tm4c123_configure(struct spi_config* config)
{
    const struct spi_tm4c123_config* cfg = CONFIG_CFG(config);
    struct spi_tm4c123_data* data = CONFIG_DATA(config);
    u32_t protocol, mode, bit_rate;

    if (spi_context_configured(&data->ctx, config)) {
        /* Nothing to do */
        return 0;
    }

    //! \b SSI_FRF_MOTO_MODE_0, \b SSI_FRF_MOTO_MODE_1, \b SSI_FRF_MOTO_MODE_2,
    //! \b SSI_FRF_MOTO_MODE_3, \b SSI_FRF_TI, or \b SSI_FRF_NMW.

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

    if (SPI_OP_MODE_GET(operation) == SPI_OP_MODE_MASTER) {
        mode = SSI_MODE_MASTER;
    } else {
        mode = SSI_MODE_SLAVE;
    }

    if ((SPI_WORD_SIZE_GET(config->operation) < 4)
        || (SPI_WORD_SIZE_GET(config->operation) > 16)) {
        return -ENOTSUP;
    }

    MAP_SSIDisable(SSI0_BASE);

    MAP_SSIConfigSetExpClk(SSI0_BASE, CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
        protocol, mode,bit_rate,(SPI_WORD_SIZE_GET(config->operation));

    MAP_SSIEnable(SSI0_BASE);
}

static int transceive(struct spi_config* config,
    const struct spi_buf* tx_bufs, u32_t tx_count,
    struct spi_buf* rx_bufs, u32_t rx_count,
    bool asynchronous, struct k_poll_signal* signal)
{
    const struct spi_tm4c123_config* cfg = CONFIG_CFG(config);
    struct spi_tm4c123_data* data = CONFIG_DATA(config);
    SPI_TypeDef* spi = cfg->spi;
    int ret;

    if (!tx_count && !rx_count) {
        return 0;
    }

#ifndef CONFIG_SPI_TM4C123_INTERRUPT
    if (asynchronous) {
        return -ENOTSUP;
    }
#endif

    spi_context_lock(&data->ctx, asynchronous, signal);

    ret = spi_tm4c123_configure(config);
    if (ret) {
        return ret;
    }

    /* Set buffers info */
    spi_context_buffers_setup(&data->ctx, tx_bufs, tx_count,
        rx_bufs, rx_count, 1);

#if defined(CONFIG_SPI_TM4C123_HAS_FIFO)
    /* Flush RX buffer */
    while (LL_SPI_IsActiveFlag_RXNE(spi)) {
        (void)LL_SPI_ReceiveData8(spi);
    }
#endif

    LL_SPI_Enable(spi);

    /* This is turned off in spi_tm4c123_complete(). */
    spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_SPI_TM4C123_INTERRUPT
    LL_SPI_EnableIT_ERR(spi);

    if (rx_bufs) {
        LL_SPI_EnableIT_RXNE(spi);
    }

    LL_SPI_EnableIT_TXE(spi);

    ret = spi_context_wait_for_completion(&data->ctx);
#else
    do {
        ret = spi_tm4c123_shift_frames(spi, data);
    } while (!ret && spi_tm4c123_transfer_ongoing(data));

    spi_tm4c123_complete(data, spi, ret);
#endif

    spi_context_release(&data->ctx, ret);

    if (ret) {
        SYS_LOG_ERR("error mask 0x%x", ret);
    }

    return ret ? -EIO : 0;
}

static int spi_tm4c123_transceive(struct spi_config* config,
    const struct spi_buf* tx_bufs, u32_t tx_count,
    struct spi_buf* rx_bufs, u32_t rx_count)
{
    return transceive(config, tx_bufs, tx_count,
        rx_bufs, rx_count, false, NULL);
}

#ifdef CONFIG_POLL
static int spi_tm4c123_transceive_async(struct spi_config* config,
    const struct spi_buf* tx_bufs,
    size_t tx_count,
    struct spi_buf* rx_bufs,
    size_t rx_count,
    struct k_poll_signal* async)
{
    return transceive(config, tx_bufs, tx_count,
        rx_bufs, rx_count, true, async);
}
#endif /* CONFIG_POLL */

static const struct spi_driver_api api_funcs = {
    .transceive = spi_tm4c123_transceive,
#ifdef CONFIG_POLL
    .transceive_async = spi_tm4c123_transceive_async,
#endif
    .release = spi_tm4c123_release,
};

static int spi_tm4c123_init(struct device* dev)
{
    const struct spi_device_config* config = DEV_CFG(dev);

    if (!MAP_SysCtlPeripheralPresent(SYSCTL_PERIPH_SSI0)) {
        return false;
    }

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

#ifdef CONFIG_SPI_TM4C123_INTERRUPT

#endif

    return 0;
}

#ifdef CONFIG_SPI_1

#ifdef CONFIG_SPI_TM4C123_INTERRUPT
static void spi_tm4c123_irq_config_func_1(struct device* port);
#endif

static const struct spi_tm4c123_config spi_tm4c123_cfg_1 = {
    .spi = (SPI_TypeDef*)CONFIG_SPI_1_BASE_ADDRESS,
    .pclken = {
#ifdef CONFIG_SOC_SERIES_TM4C123F0X
        .enr = LL_APB1_GRP2_PERIPH_SPI1,
        .bus = TM4C123_CLOCK_BUS_APB1_2
#else
        .enr = LL_APB2_GRP1_PERIPH_SPI1,
        .bus = TM4C123_CLOCK_BUS_APB2
#endif
    },
#ifdef CONFIG_SPI_TM4C123_INTERRUPT
    .irq_config = spi_tm4c123_irq_config_func_1,
#endif
};

static struct spi_tm4c123_data spi_tm4c123_dev_data_1 = {
    SPI_CONTEXT_INIT_LOCK(spi_tm4c123_dev_data_1, ctx),
    SPI_CONTEXT_INIT_SYNC(spi_tm4c123_dev_data_1, ctx),
};

DEVICE_AND_API_INIT(spi_tm4c123_1, CONFIG_SPI_1_NAME, &spi_tm4c123_init,
    &spi_tm4c123_dev_data_1, &spi_tm4c123_cfg_1,
    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
    &api_funcs);

#ifdef CONFIG_SPI_TM4C123_INTERRUPT
static void spi_tm4c123_irq_config_func_1(struct device* dev)
{
    IRQ_CONNECT(CONFIG_SPI_1_IRQ, CONFIG_SPI_1_IRQ_PRI,
        spi_tm4c123_isr, DEVICE_GET(spi_tm4c123_1), 0);
    irq_enable(CONFIG_SPI_1_IRQ);
}
#endif

#endif /* CONFIG_SPI_1 */
