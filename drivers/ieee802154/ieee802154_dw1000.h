/* ieee802154_dw1000.h - Registers definition for TI dw1000 */

/*
 * Copyright (c) 2018 hackin zhao.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IEEE802154_dw1000_H__
#define __IEEE802154_dw1000_H__

#include <atomic.h>
#include <linker/sections.h>
#include <spi.h>

/* Runtime context structure
 ***************************
 */

struct dw1000_context {
    struct net_if* iface;
    /**************************/
    struct dw1000_gpio_configuration* gpios;
    struct gpio_callback rx_tx_cb;
    struct spi_config spi;
    u8_t mac_addr[8];
    /************RF************/
    const struct dw1000_rf_registers_set* rf_settings;
    /************TX************/
    struct k_sem tx_sync;
    atomic_t tx;
    atomic_t tx_start;
    /************RX************/
    K_THREAD_STACK_MEMBER(rx_stack,
        CONFIG_IEEE802154_dw1000_RX_STACK_SIZE);
    struct k_thread rx_thread;
    struct k_sem rx_lock;
    atomic_t rx;
};

#include "ieee802154_dw1000_regs.h"

/* Registers useful routines
 ***************************
 */

bool _dw1000_access_reg(struct spi_config* spi, bool read, u8_t addr,
    void* data, size_t length, bool extended, bool burst);

static inline u8_t _dw1000_read_single_reg(struct spi_config* spi,
    u8_t addr, bool extended)
{
    u8_t val;

    if (_dw1000_access_reg(spi, true, addr, &val, 1, extended, false)) {
        return val;
    }

    return 0;
}

static inline bool _dw1000_write_single_reg(struct spi_config* spi,
    u8_t addr, u8_t val, bool extended)
{
    return _dw1000_access_reg(spi, false, addr, &val, 1, extended, false);
}

static inline bool _dw1000_instruct(struct spi_config* spi, u8_t addr)
{
    return _dw1000_access_reg(spi, false, addr, NULL, 0, false, false);
}

#define DEFINE_REG_READ(__reg_name, __reg_addr, __ext)               \
    static inline u8_t read_reg_##__reg_name(struct spi_config* spi) \
    {                                                                \
        return _dw1000_read_single_reg(spi, __reg_addr, __ext);      \
    }

#define DEFINE_REG_WRITE(__reg_name, __reg_addr, __ext)               \
    static inline bool write_reg_##__reg_name(struct spi_config* spi, \
        u8_t val)                                                     \
    {                                                                 \
        return _dw1000_write_single_reg(spi, __reg_addr,              \
            val, __ext);                                              \
    }

DEFINE_REG_WRITE(iocfg3, dw1000_REG_IOCFG3, false)
DEFINE_REG_WRITE(iocfg2, dw1000_REG_IOCFG2, false)
DEFINE_REG_WRITE(iocfg0, dw1000_REG_IOCFG0, false)
DEFINE_REG_WRITE(pa_cfg1, dw1000_REG_PA_CFG1, false)
DEFINE_REG_WRITE(pkt_len, dw1000_REG_PKT_LEN, false)

DEFINE_REG_READ(fs_cfg, dw1000_REG_FS_CFG, false)
DEFINE_REG_READ(rssi0, dw1000_REG_RSSI0, true)
DEFINE_REG_READ(pa_cfg1, dw1000_REG_PA_CFG1, false)
DEFINE_REG_READ(num_txbytes, dw1000_REG_NUM_TXBYTES, true)
DEFINE_REG_READ(num_rxbytes, dw1000_REG_NUM_RXBYTES, true)

/* Instructions useful routines
 ******************************
 */

#define DEFINE_STROBE_INSTRUCTION(__ins_name, __ins_addr)            \
    static inline bool instruct_##__ins_name(struct spi_config* spi) \
    {                                                                \
        /*SYS_LOG_DBG("");*/                                         \
        return _dw1000_instruct(spi, __ins_addr);                    \
    }

DEFINE_STROBE_INSTRUCTION(sres, dw1000_INS_SRES)
DEFINE_STROBE_INSTRUCTION(sfstxon, dw1000_INS_SFSTXON)
DEFINE_STROBE_INSTRUCTION(sxoff, dw1000_INS_SXOFF)
DEFINE_STROBE_INSTRUCTION(scal, dw1000_INS_SCAL)
DEFINE_STROBE_INSTRUCTION(srx, dw1000_INS_SRX)
DEFINE_STROBE_INSTRUCTION(stx, dw1000_INS_STX)
DEFINE_STROBE_INSTRUCTION(sidle, dw1000_INS_SIDLE)
DEFINE_STROBE_INSTRUCTION(safc, dw1000_INS_SAFC)
DEFINE_STROBE_INSTRUCTION(swor, dw1000_INS_SWOR)
DEFINE_STROBE_INSTRUCTION(spwd, dw1000_INS_SPWD)
DEFINE_STROBE_INSTRUCTION(sfrx, dw1000_INS_SFRX)
DEFINE_STROBE_INSTRUCTION(sftx, dw1000_INS_SFTX)
DEFINE_STROBE_INSTRUCTION(sworrst, dw1000_INS_SWORRST)
DEFINE_STROBE_INSTRUCTION(snop, dw1000_INS_SNOP)

#endif /* __IEEE802154_dw1000_H__ */
