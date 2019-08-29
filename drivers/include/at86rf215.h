/*
 * Copyright (C) 2019 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    drivers_at86rf215 AT86RF215 based drivers
 * @ingroup     drivers_netdev
 *
 * This module contains a driver for the Atmel AT86RF215 radio.
 *
 * @{
 *
 * @file
 * @brief       Interface definition for AT86RF215 based drivers
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 */

#ifndef AT86RF215_H
#define AT86RF215_H

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "periph/spi.h"
#include "periph/gpio.h"
#include "net/netdev.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/nettype.h"
#include "xtimer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct at86rf215_RF_regs at86rf215_RF_regs_t;
typedef struct at86rf215_BBC_regs at86rf215_BBC_regs_t;

/**
 * @brief   Maximum possible packet size in byte
 */
#define AT86RF215_MAX_PKT_LENGTH        (2047)

/**
 * @name    Channel configuration
 * @{
 */
#define AT86RF215_DEFAULT_CHANNEL        (IEEE802154_DEFAULT_CHANNEL)
#define AT86RF215_DEFAULT_SUBGHZ_CHANNEL (IEEE802154_DEFAULT_SUBGHZ_CHANNEL)
/** @} */

/**
 * @brief   Default TX power (0dBm)
 */
#define AT86RF215_DEFAULT_TXPOWER       (IEEE802154_DEFAULT_TXPOWER)

/**
 * @name    Flags for device internal states (see datasheet)
 * @{
 */
typedef enum {
    AT86RF215_STATE_OFF,
    AT86RF215_STATE_IDLE,       /**< idle state, listening */
    AT86RF215_STATE_RX,         /**< receiving frame, sending ACK */
    AT86RF215_STATE_TX_PREP,    /**< preparing to send frame */
    AT86RF215_STATE_TX,         /**< sending frame, wait for ACK */
    AT86RF215_STATE_SLEEP       /**< sleep mode, not listening */
} at86rf215_state_t;
/** @} */

/**
 * @name    Internal device option flags
 * @{
 */
#define AT86RF215_OPT_TELL_TX_START  (0x0001)       /**< notify MAC layer on TX
                                                     *   start */
#define AT86RF215_OPT_TELL_TX_END    (0x0002)       /**< notify MAC layer on TX
                                                     *   finished */
#define AT86RF215_OPT_TELL_RX_START  (0x0004)       /**< notify MAC layer on RX
                                                     *   start */
#define AT86RF215_OPT_TELL_RX_END    (0x0008)       /**< notify MAC layer on RX
                                                     *   finished */
#define AT86RF215_OPT_CSMA           (0x0010)       /**< CSMA active */
#define AT86RF215_OPT_PROMISCUOUS    (0x0020)       /**< promiscuous mode
                                                     *   active */
#define AT86RF215_OPT_PRELOADING     (0x0040)       /**< preloading enabled */
#define AT86RF215_OPT_AUTOACK        (0x0080)       /**< Auto ACK active */
#define AT86RF215_OPT_ACK_REQUESTED  (0x0100)       /**< ACK requested for current frame */
#define AT86RF215_OPT_SUBGHZ         (0x0200)       /**< Operate in the Sub-GHz band */
/** @} */

/**
 * @brief   struct holding all params needed for device initialization
 */
typedef struct at86rf215_params {
    spi_t spi;              /**< SPI bus the device is connected to */
    spi_clk_t spi_clk;      /**< SPI clock speed to use */
    spi_cs_t cs_pin;        /**< GPIO pin connected to chip select */
    gpio_t int_pin;         /**< GPIO pin connected to the interrupt pin */
    gpio_t reset_pin;       /**< GPIO pin connected to the reset pin */
} at86rf215_params_t;

/**
 * @brief   Device descriptor for AT86RF215 radio devices
 *
 * @extends netdev_ieee802154_t
 */
typedef struct at86rf215 {
    netdev_ieee802154_t netdev;             /**< netdev parent struct */
    /* device specific fields */
    at86rf215_params_t params;              /**< parameters for initialization */
    struct at86rf215 *sibling;              /**< The other radio */
    const at86rf215_RF_regs_t  *RF;         /**< Radio Frontend Registers */
    const at86rf215_BBC_regs_t *BBC;        /**< Baseband Registers */
    xtimer_t ack_timer;                     /**< timer for ACK timeout */
    msg_t ack_msg;                          /**< message for ACK timeout */
    uint16_t flags;                         /**< Device specific flags */
    uint16_t num_chans;                     /**< Number of legal channel at current modulation */
    uint16_t tx_frame_len;                  /**< length of the current TX frame */
    uint16_t ack_timeout_usec;              /**< time to wait before retransmission in µs */
    uint8_t ack_timeout;                    /**< 1 if ack timeout was reached, 0 otherwise */
    uint8_t page;                           /**< currently used channel page */
    uint8_t state;                          /**< current state of the radio */
    uint8_t retries_max;                    /**< number of retries until ACK is received */
    uint8_t retries;                        /**< retries left */
    uint8_t csma_retries_max;               /**< number of retries until channel is clear */
    uint8_t csma_retries;                   /**< CSMA retries left */
} at86rf215_t;

/**
 * @brief   Setup an AT86RF215 based device state
 *
 * @param[out] dev_09       sub-GHz device descriptor
 * @param[out] dev_24       2.4 GHz device descriptor
 * @param[in]  params       parameters for device initialization
 */
void at86rf215_setup(at86rf215_t *dev_09, at86rf215_t *dev_24, const at86rf215_params_t *params);

/**
 * @brief   Trigger a hardware reset and configure radio with default values
 *
 * @param[in,out] dev       device to reset
 */
void at86rf215_reset(at86rf215_t *dev);

/**
 * @brief   Get the short address of the given device
 *
 * @param[in] dev           device to read from
 *
 * @return                  the currently set (2-byte) short address
 */
uint16_t at86rf215_get_addr_short(const at86rf215_t *dev);

/**
 * @brief   Set the short address of the given device
 *
 * @param[in,out] dev       device to write to
 * @param[in] addr          (2-byte) short address to set
 */
void at86rf215_set_addr_short(at86rf215_t *dev, uint16_t addr);

/**
 * @brief   Get the configured long address of the given device
 *
 * @param[in] dev           device to read from
 *
 * @return                  the currently set (8-byte) long address
 */
uint64_t at86rf215_get_addr_long(const at86rf215_t *dev);

/**
 * @brief   Set the long address of the given device
 *
 * @param[in,out] dev       device to write to
 * @param[in] addr          (8-byte) long address to set
 */
void at86rf215_set_addr_long(at86rf215_t *dev, uint64_t addr);

/**
 * @brief   Get the configured channel number of the given device
 *
 * @param[in] dev           device to read from
 *
 * @return                  the currently set channel number
 */
uint8_t at86rf215_get_chan(const at86rf215_t *dev);

/**
 * @brief   Set the channel number of the given device
 *
 * @param[in,out] dev       device to write to
 * @param[in] chan          channel number to set
 */
void at86rf215_set_chan(at86rf215_t *dev, uint16_t chan);

/**
 * @brief   Get the configured channel page of the given device
 *
 * @param[in] dev           device to read from
 *
 * @return                  the currently set channel page
 */
uint8_t at86rf215_get_page(const at86rf215_t *dev);

/**
 * @brief   Set the channel page of the given device
 *
 * @param[in,out] dev       device to write to
 * @param[in] page          channel page to set
 */
void at86rf215_set_page(at86rf215_t *dev, uint8_t page);

/**
 * @brief   Get the configured PAN ID of the given device
 *
 * @param[in] dev           device to read from
 *
 * @return                  the currently set PAN ID
 */
uint16_t at86rf215_get_pan(const at86rf215_t *dev);

/**
 * @brief   Set the PAN ID of the given device
 *
 * @param[in,out] dev       device to write to
 * @param[in] pan           PAN ID to set
 */
void at86rf215_set_pan(at86rf215_t *dev, uint16_t pan);

/**
 * @brief   Get the configured transmission power of the given device [in dBm]
 *
 * @param[in] dev           device to read from
 *
 * @return                  configured transmission power in dBm
 */
int16_t at86rf215_get_txpower(const at86rf215_t *dev);

/**
 * @brief   Set the transmission power of the given device [in dBm]
 *
 * If the device does not support the exact dBm value given, it will set a value
 * as close as possible to the given value. If the given value is larger or
 * lower then the maximal or minimal possible value, the min or max value is
 * set, respectively.
 *
 * @param[in] dev           device to write to
 * @param[in] txpower       transmission power in dBm
 */
void at86rf215_set_txpower(const at86rf215_t *dev, int16_t txpower);

/**
 * @brief   Get the CCA threshold value
 *
 * @param[in] dev           device to read value from
 *
 * @return                  the current CCA threshold value
 */
int8_t at86rf215_get_cca_threshold(const at86rf215_t *dev);

/**
 * @brief   Set the CCA threshold value
 *
 * @param[in] dev           device to write to
 * @param[in] value         the new CCA threshold value
 */
void at86rf215_set_cca_threshold(const at86rf215_t *dev, int8_t value);

/**
 * @brief   Get the latest ED level measurement
 *
 * @param[in] dev           device to read value from
 *
 * @return                  the last ED level
 */
int8_t at86rf215_get_ed_level(at86rf215_t *dev);

/**
 * @brief   Enable or disable driver specific options
 *
 * @param[in,out] dev       device to set/clear option flag for
 * @param[in] option        option to enable/disable
 * @param[in] state         true for enable, false for disable
 */
void at86rf215_set_option(at86rf215_t *dev, uint16_t option, bool state);

/**
 * @brief   Set the state of the given device (trigger a state change)
 *
 * @param[in,out] dev       device to change state of
 * @param[in] state         the targeted new state
 *
 * @return                  the previous state before the new state was set
 */
uint8_t at86rf215_set_state(at86rf215_t *dev, uint8_t state);

/**
 * @brief   Convenience function for simply sending data
 *
 * @note This function ignores the PRELOADING option
 *
 * @param[in,out] dev       device to use for sending
 * @param[in] data          data to send (must include IEEE802.15.4 header)
 * @param[in] len           length of @p data
 *
 * @return                  number of bytes that were actually send
 * @return                  0 on error
 */
size_t at86rf215_send(at86rf215_t *dev, const uint8_t *data, size_t len);

/**
 * @brief   Prepare for sending of data
 *
 * This function puts the given device into the TX state, so no receiving of
 * data is possible after it was called.
 *
 * @param[in,out] dev        device to prepare for sending
 */
void at86rf215_tx_prepare(at86rf215_t *dev);

/**
 * @brief   Load chunks of data into the transmit buffer of the given device
 *
 * @param[in,out] dev       device to write data to
 * @param[in] data          buffer containing the data to load
 * @param[in] len           number of bytes in @p buffer
 * @param[in] offset        offset used when writing data to internal buffer
 *
 * @return                  offset + number of bytes written
 */
size_t at86rf215_tx_load(at86rf215_t *dev, const uint8_t *data,
                         size_t len, size_t offset);

/**
 * @brief   Trigger sending of data previously loaded into transmit buffer
 *
 * @param[in] dev           device to trigger
 */
void at86rf215_tx_exec(at86rf215_t *dev);

/**
 * @brief   Signal that the transfer of the frame (and optional ACK reception)
 *          has finished.
 *
 * @param[in] dev           device to use
 */
void at86rf215_tx_done(at86rf215_t *dev);

/**
 * @brief   Perform one manual channel clear assessment (CCA)
 *
 * The CCA mode and threshold level depends on the current transceiver settings.
 *
 * @param[in]  dev          device to use
 *
 * @return                  true if channel is determined clear
 * @return                  false if channel is determined busy
 */
bool at86rf215_cca(at86rf215_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* AT86RF215_H */
/** @} */
