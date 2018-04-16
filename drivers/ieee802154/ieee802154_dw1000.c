#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "dev/dw1000"
#include <logging/sys_log.h>

#include <errno.h>

#include <arch/cpu.h>
#include <kernel.h>

#include <board.h>
#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <misc/byteorder.h>
#include <random/rand32.h>
#include <string.h>

#include <gpio.h>
#include <ieee802154/dw1000.h>

#include "ieee802154_dw1000.h"

#define _usleep(usec) k_busy_wait(usec)

static struct dw1000_gpio_configuration dw1000_gpios[DW1000_GPIO_IDX_MAX] = {
	{
		.dev = NULL,
		.pin = CONFIG_DW1000_GPIO_IDX_ISR_PIN,
	},
	{
		.dev = NULL,
		.pin = CONFIG_DW1000_GPIO_IDX_WAKEUP_PIN,
	},
	{
		.dev = NULL,
		.pin = CONFIG_DW1000_GPIO_IDX_RST_PIN,
	},
	{
		.dev = NULL,
		.pin = CONFIG_DW1000_GPIO_IDX_EXTON_PIN,
	},
	{
		.dev = NULL,
		.pin = CONFIG_DW1000_GPIO_IDX_GPIO_5_PIN, /* SPI POL */
	},
	{
		.dev = NULL,
		.pin = CONFIG_DW1000_GPIO_IDX_GPIO_6_PIN, /* SPI PHA */
	},
};

struct dw1000_gpio_configuration *dw1000_configure_gpios(void)
{
	const int flags_noint_out = GPIO_DIR_OUT;
	const int flags_noint_in = GPIO_DIR_IN;
	const int flags_int_in = (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);
	struct device *gpio;

	gpio = device_get_binding(CONFIG_IEEE802154_DW1000_GPIO_ISR_PORT_NAME);
	gpio_pin_configure(gpio, dw1000_gpios[DW1000_GPIO_IDX_ISR].pin,
			   flags_noint_out);
	dw1000_gpios[DW1000_GPIO_IDX_ISR].dev = gpio;

	gpio = device_get_binding(CONFIG_IEEE802154_DW1000_GPIO_WAKEUP_PORT_NAME);
	gpio_pin_configure(gpio, dw1000_gpios[DW1000_GPIO_IDX_WAKEUP].pin,
			   flags_noint_out);
	dw1000_gpios[DW1000_GPIO_IDX_WAKEUP].dev = gpio;

	gpio = device_get_binding(CONFIG_IEEE802154_DW1000_GPIO_RST_PORT_NAME);
	gpio_pin_configure(gpio, dw1000_gpios[DW1000_GPIO_IDX_RST].pin,
			   flags_noint_out);
	dw1000_gpios[DW1000_GPIO_IDX_RST].dev = gpio;

	gpio = device_get_binding(CONFIG_IEEE802154_DW1000_GPIO_EXTON_PORT_NAME);
	gpio_pin_configure(gpio, dw1000_gpios[DW1000_GPIO_IDX_EXTON].pin,
			   flags_noint_out);
	dw1000_gpios[DW1000_GPIO_IDX_EXTON].dev = gpio;

	gpio = device_get_binding(CONFIG_IEEE802154_DW1000_GPIO_GPIO_5_PORT_NAME);
	gpio_pin_configure(gpio, dw1000_gpios[DW1000_GPIO_IDX_GPIO_5].pin,
			   flags_noint_out);
	dw1000_gpios[DW1000_GPIO_IDX_GPIO_5].dev = gpio;

	gpio = device_get_binding(CONFIG_IEEE802154_DW1000_GPIO_GPIO_6_PORT_NAME);
	gpio_pin_configure(gpio, dw1000_gpios[DW1000_GPIO_IDX_GPIO_6].pin,
			   flags_noint_out);
	dw1000_gpios[DW1000_GPIO_IDX_GPIO_6].dev = gpio;

	return dw1000_gpios;
}

static inline int configure_spi(struct device *dev)
{
	struct dw1000_context *dw1000 = dev->driver_data;

	dw1000->spi = device_get_binding(
		CONFIG_IEEE802154_DW1000_SPI_DRV_NAME);
	if (!dw1000->spi) {
		SYS_LOG_ERR("Unable to get SPI device");
		return -ENODEV;
	}

#if defined(CONFIG_IEEE802154_DW1000_GPIO_SPI_CS)
	cs_ctrl.gpio_dev = device_get_binding(
		CONFIG_IEEE802154_DW1000_GPIO_SPI_CS_DRV_NAME);
	if (!cs_ctrl.gpio_dev) {
		SYS_LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	cs_ctrl.gpio_pin = CONFIG_IEEE802154_DW1000_GPIO_SPI_CS_PIN;
	cs_ctrl.delay = 0;

	dw1000->spi_cfg.cs = &cs_ctrl;

	SYS_LOG_DBG("SPI GPIO CS configured on %s:%u",
		    CONFIG_IEEE802154_DW1000_GPIO_SPI_CS_DRV_NAME,
		    CONFIG_IEEE802154_DW1000_GPIO_SPI_CS_PIN);
#endif  /* CONFIG_IEEE802154_DW1000_GPIO_SPI_CS */

	dw1000->spi_cfg.frequency = CONFIG_IEEE802154_DW1000_SPI_FREQ;
	dw1000->spi_cfg.operation = SPI_WORD_SET(8);
	dw1000->spi_cfg.slave = CONFIG_IEEE802154_DW1000_SPI_SLAVE;
}

static inline void set_reset(struct device *dev)
{
	struct dw1000_context *dw1000 = dev->driver_data;

	gpio_pin_write(dw1000->gpios[DW1000_GPIO_IDX_RST].dev,
		       dw1000->gpios[DW1000_GPIO_IDX_RST].pin, 0);

	_usleep(2000);

	gpio_pin_configure(gpio, dw1000_gpios[DW1000_GPIO_IDX_RST].pin,
			   GPIO_DIR_IN);
}

static inline void set_exton(struct device *dev, u32_t value)
{
	struct dw1000_context *dw1000 = dev->driver_data;

	gpio_pin_write(dw1000->gpios[DW1000_GPIO_IDX_RST].dev,
		       dw1000->gpios[DW1000_GPIO_IDX_RST].pin, value);
}

static inline void set_wakeup(struct device *dev, u32_t value)
{
	struct dw1000_context *dw1000 = dev->driver_data;

	gpio_pin_write(dw1000->gpios[DW1000_GPIO_IDX_RST].dev,
		       dw1000->gpios[DW1000_GPIO_IDX_RST].pin, value);
}

static int power_on_and_setup(struct device *dev)
{
	struct dw1000_context *dw1000 = dev->driver_data;

	/* Switching to LPM2 mode */
	set_reset(dev, 0);
	_usleep(150);

	set_vreg_en(dev, 0);
	_usleep(250);

	/* Then to ACTIVE mode */
	set_vreg_en(dev, 1);
	_usleep(250);

	set_reset(dev, 1);
	_usleep(150);

	if (!verify_osc_stabilization(cc2520)) {
		return -EIO;
	}

	/* Default settings to always write (see chapter 28 part 1) */
	if (!write_reg_txpower(cc2520, CC2520_TXPOWER_DEFAULT) ||
	    !write_reg_ccactrl0(cc2520, CC2520_CCACTRL0_DEFAULT) ||
	    !write_reg_mdmctrl0(cc2520, CC2520_MDMCTRL0_DEFAULT) ||
	    !write_reg_mdmctrl1(cc2520, CC2520_MDMCTRL1_DEFAULT) ||
	    !write_reg_rxctrl(cc2520, CC2520_RXCTRL_DEFAULT) ||
	    !write_reg_fsctrl(cc2520, CC2520_FSCTRL_DEFAULT) ||
	    !write_reg_fscal1(cc2520, CC2520_FSCAL1_DEFAULT) ||
	    !write_reg_agcctrl1(cc2520, CC2520_AGCCTRL1_DEFAULT) ||
	    !write_reg_adctest0(cc2520, CC2520_ADCTEST0_DEFAULT) ||
	    !write_reg_adctest1(cc2520, CC2520_ADCTEST1_DEFAULT) ||
	    !write_reg_adctest2(cc2520, CC2520_ADCTEST2_DEFAULT)) {
		return -EIO;
	}

	/* EXTCLOCK0: Disabling external clock
	 * FRMCTRL0: AUTOACK and AUTOCRC enabled
	 * FRMCTRL1: SET_RXENMASK_ON_TX and IGNORE_TX_UNDERF
	 * FRMFILT0: Frame filtering (setting CC2520_FRAME_FILTERING)
	 * FIFOPCTRL: Set TX threshold (setting CC2520_TX_THRESHOLD)
	 */
	if (!write_reg_extclock(cc2520, 0) ||
	    !write_reg_frmctrl0(cc2520, CC2520_AUTOMATISM) ||
	    !write_reg_frmctrl1(cc2520, FRMCTRL1_IGNORE_TX_UNDERF |
				FRMCTRL1_SET_RXENMASK_ON_TX) ||
	    !write_reg_frmfilt0(cc2520, FRMFILT0_FRAME_FILTER_EN |
				FRMFILT0_MAX_FRAME_VERSION(3)) ||
	    !write_reg_frmfilt1(cc2520, FRMFILT1_ACCEPT_ALL) ||
	    !write_reg_srcmatch(cc2520, SRCMATCH_DEFAULTS) ||
	    !write_reg_fifopctrl(cc2520,
				 FIFOPCTRL_FIFOP_THR(CC2520_TX_THRESHOLD))) {
		return -EIO;
	}

	/* Cleaning up TX fifo */
	instruct_sflushtx(cc2520);

	setup_gpio_callbacks(dev);

	_cc2520_print_gpio_config(dev);

	return 0;
}

static int dw1000_init(struct device *dev)
{
	struct dw1000_context *dw1000 = dev->driver_data;

	atomic_set(&dw1000->tx, 0);
	atomic_set(&dw1000->tx_start, 0);
	atomic_set(&dw1000->rx, 0);
	k_sem_init(&dw1000->rx_lock, 0, 1);
	k_sem_init(&dw1000->tx_sync, 0, 1);

	dw1000->gpios = dw1000_configure_gpios();
	if (!dw1000->gpios) {
		SYS_LOG_ERR("Configuring GPIOS failed");
		return -EIO;
	}

	if (configure_spi(dev) != 0) {
		SYS_LOG_ERR("Configuring SPI failed");
		return -EIO;
	}

	SYS_LOG_DBG("GPIO and SPI configured");

	if (power_on_and_setup(dev) != 0) {
		SYS_LOG_ERR("Configuring DW1000 failed");
		return -EIO;
	}

	k_thread_create(&dw1000->rx_thread, dw1000->rx_stack,
			CONFIG_IEEE802154_DW1000_RX_STACK_SIZE,
			(k_thread_entry_t)dw1000_rx,
			dev, NULL, NULL, K_PRIO_COOP(2), 0, 0);

	SYS_LOG_INF("DW1000 initialized");

	return 0;
}

static struct dw1000_context dw1000_context_data;

static struct ieee802154_radio_api dw1000_radio_api = {
	.iface_api.init = dw1000_iface_init,
	.iface_api.send = ieee802154_radio_send,
	.get_capabilities = dw1000_get_capabilities,
	.cca = dw1000_cca,
	.set_channel = dw1000_set_channel,
	.set_txpower = dw1000_set_txpower,
	.tx = dw1000_tx,
	.start = dw1000_start,
	.stop = dw1000_stop,
	.get_subg_channel_count = dw1000_get_channel_count,
};

NET_DEVICE_INIT(dw1000, CONFIG_IEEE802154_dw1000_DRV_NAME,
		dw1000_init, &dw1000_context_data, NULL,
		CONFIG_IEEE802154_dw1000_INIT_PRIO,
		&dw1000_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
