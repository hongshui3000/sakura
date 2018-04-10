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

#include "ieee802154_dw1000.h"

static int dw1000_init(struct device* dev)
{
    struct dw1000_context* dw1000 = dev->driver_data;

    atomic_set(&dw1000->tx, 0);
    atomic_set(&dw1000->tx_start, 0);
    atomic_set(&dw1000->rx, 0);
    k_sem_init(&dw1000->rx_lock, 0, 1);
    k_sem_init(&dw1000->tx_sync, 0, 1);

    dw1000->gpios = cc1200_configure_gpios();
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
        SYS_LOG_ERR("Configuring CC1200 failed");
        return -EIO;
    }

    k_thread_create(&cc1200->rx_thread, cc1200->rx_stack,
        CONFIG_IEEE802154_CC1200_RX_STACK_SIZE,
        (k_thread_entry_t)cc1200_rx,
        dev, NULL, NULL, K_PRIO_COOP(2), 0, 0);

    SYS_LOG_INF("CC1200 initialized");

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
