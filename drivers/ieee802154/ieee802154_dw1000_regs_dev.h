/* ieee802154_dw1000_regs.h - Registers definition for DECAWAVE DW1000 */

/*
 * Copyright (c) 2018 hackin zhao.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IEEE802154_DW1000_REGS_H__
#define __IEEE802154_DW1000_REGS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DW1000_OFFSET(x) x

/* resgister base address */
#define DW1000_DEV_ID_ID 0x00
#define DW1000_EUI_64_ID 0x01
#define DW1000_PANADR_ID 0x03
#define DW1000_SYS_CFG_ID 0x04
#define DW1000_SYS_TIME_ID 0x06
#define DW1000_TX_FCTRL_ID 0x08
#define DW1000_TX_BUFFER_ID 0x09
#define DW1000_DX_TIME_ID 0x0A
#define DW1000_RX_FWTO_ID 0x0C
#define DW1000_SYS_CTRL_ID 0x0D
#define DW1000_SYS_MASK_ID 0x0E
#define DW1000_SYS_STATUS_ID 0x0F
#define DW1000_RX_FINFO_ID 0x10
#define DW1000_RX_BUFFER_ID 0x11
#define DW1000_RX_FQUAL_ID 0x12
#define DW1000_RX_TTCKI_ID 0x13
#define DW1000_RX_TTCKO_ID 0x14
#define DW1000_RX_TIME_ID 0x15
#define DW1000_TX_TIME_ID 0x17
#define DW1000_TX_ANTD_ID 0x18
#define DW1000_SYS_STATE_ID 0x19
#define DW1000_ACK_RESP_T_ID 0x1A
#define DW1000_RX_SNIFF_ID 0x1D
#define DW1000_TX_POWER_ID 0x1E
#define DW1000_CHAN_CTRL_ID 0x1F
#define DW1000_USR_SFD_ID 0x21
#define DW1000_AGC_CTRL_ID 0x23
#define DW1000_EXT_SYNC_ID 0x24
#define DW1000_ACC_MEM_ID 0x25
#define DW1000_GPIO_CTRL_ID 0x26
#define DW1000_DRX_CONF_ID 0x27
#define DW1000_RF_CONF_ID 0x28
#define DW1000_TX_CAL_ID 0x2A
#define DW1000_FS_CTRL_ID 0x2B
#define DW1000_AON_ID 0x2C
#define DW1000_OTP_IF_ID 0x2D
#define DW1000_LDE_IF_ID 0x2E
#define DW1000_DIG_DIAG_ID 0x2F
#define DW1000_PMSC_ID 0x36

/* reserved resgister */
#define DW1000_REG_02_ID_RESERVED 0x02
#define DW1000_REG_05_ID_RESERVED 0x05
#define DW1000_REG_07_ID_RESERVED 0x07
#define DW1000_REG_0B_ID_RESERVED 0x0B
#define DW1000_REG_16_ID_RESERVED 0x16
#define DW1000_REG_1B_ID_RESERVED 0x1B
#define DW1000_REG_1C_ID_RESERVED 0x1C
#define DW1000_REG_20_ID_RESERVED 0x20
#define DW1000_REG_22_ID_RESERVED 0x22
#define DW1000_REG_29_ID_RESERVED 0x29
#define DW1000_REG_30_ID_RESERVED 0x30
#define DW1000_REG_31_ID_RESERVED 0x31
#define DW1000_REG_32_ID_RESERVED 0x32
#define DW1000_REG_33_ID_RESERVED 0x33
#define DW1000_REG_34_ID_RESERVED 0x34
#define DW1000_REG_35_ID_RESERVED 0x35
#define DW1000_REG_37_ID_RESERVED 0x37
#define DW1000_REG_38_ID_RESERVED 0x38
#define DW1000_REG_39_ID_RESERVED 0x39
#define DW1000_REG_3A_ID_RESERVED 0x3A
#define DW1000_REG_3B_ID_RESERVED 0x3B
#define DW1000_REG_3C_ID_RESERVED 0x3C
#define DW1000_REG_3D_ID_RESERVED 0x3D
#define DW1000_REG_3E_ID_RESERVED 0x3E
#define DW1000_REG_3F_ID_RESERVED 0x3F

/* regsiter offset */
/* TODO: for test api */
#define DW1000_DEV_ID_REV (BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define DW1000_DEV_ID_VER (BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define DW1000_DEV_ID_MODEL 0xff
#define DW1000_DEV_ID_RIDTAG 0xffff

#ifdef __cplusplus
}
#endif

#endif

/*
 ***********************************************************************
 * note:
 * 
 * 1. 所有的寄存器的访问方式均以字节(byte)为单位访问，所有的偏移计算也均以字节
 *    为单位进行计算
 * 
 ************************************************************************
 */
