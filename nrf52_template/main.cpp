/**
 * Copyright (c) 2014 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Wire.h"
#include "app_uart.h"
#include "app_util.h"
#include "boards.h"
#include "bsp.h"
#include "delay.h"
#include "nrf.h"
#include "nrf_VL53L0X.h"
#include "nrf_delay.h"
#include "nrf_error.h"
#include "nrf_esb.h"
#include "nrf_esb_error_codes.h"
#include "nrf_gpio.h"
#include "nrf_uart.h"
#include "sdk_common.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static TwoWire Wire(NRF_TWIM1, NRF_TWIS1, SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn, SDA_PIN_NUMBER, SCL_PIN_NUMBER);

#define MAX_TEST_DATA_BYTES (15U) /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256      /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256      /**< UART RX buffer size. */

void uart_error_handle(app_uart_evt_t *p_event) {
  if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
    APP_ERROR_HANDLER(p_event->data.error_communication);
  } else if (p_event->evt_type == APP_UART_FIFO_ERROR) {
    APP_ERROR_HANDLER(p_event->data.error_code);
  }
}

// Note had to modiy sdk to have matching order of parameters to make this work.
static nrf_esb_payload_t tx_payload = NRF_ESB_CREATE_PAYLOAD(0, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00);

static nrf_esb_payload_t rx_payload;

void nrf_esb_event_handler(nrf_esb_evt_t const *p_event) {
  switch (p_event->evt_id) {
  case NRF_ESB_EVENT_TX_SUCCESS:
    NRF_LOG_DEBUG("TX SUCCESS EVENT\n");
    break;
  case NRF_ESB_EVENT_TX_FAILED:
    NRF_LOG_DEBUG("TX FAILED EVENT\n");
    (void)nrf_esb_flush_tx();
    (void)nrf_esb_start_tx();
    break;
  case NRF_ESB_EVENT_RX_RECEIVED:
    NRF_LOG_DEBUG("RX RECEIVED EVENT\n");
    while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS) {
      if (rx_payload.length > 0) {
        NRF_LOG_DEBUG("RX RECEIVED PAYLOAD\n");
      }
    }
    break;
  }
}

void clocks_start(void) {
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;

  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    ;
}

void gpio_init(void) {
  bsp_board_init(BSP_INIT_LEDS);
  nrf_gpio_range_cfg_input( NRF_GPIO_PIN_MAP(0,9), NRF_GPIO_PIN_MAP(0,10),NRF_GPIO_PIN_PULLUP);
}

uint32_t  uart_init(void) {
  uint32_t err_code;

  const app_uart_comm_params_t comm_params =
  {
    RX_PIN_NUMBER,
    TX_PIN_NUMBER,
    RTS_PIN_NUMBER,
    CTS_PIN_NUMBER,
    APP_UART_FLOW_CONTROL_DISABLED,
    false,
#if defined(UART_PRESENT)
    NRF_UART_BAUDRATE_115200
#else
    NRF_UARTE_BAUDRATE_115200
#endif
  };

  APP_UART_FIFO_INIT(&comm_params,
      UART_RX_BUF_SIZE,
      UART_TX_BUF_SIZE,
      uart_error_handle,
      APP_IRQ_PRIORITY_LOWEST,
      err_code);

  VERIFY_SUCCESS(err_code);

  return err_code;
}

uint32_t esb_init(void) {
  uint32_t err_code;
  uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
  uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
  uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};

  nrf_esb_config_t nrf_esb_config = NRF_ESB_DEFAULT_CONFIG;
  nrf_esb_config.protocol = NRF_ESB_PROTOCOL_ESB_DPL;
  nrf_esb_config.retransmit_delay = 600;
  nrf_esb_config.bitrate = NRF_ESB_BITRATE_2MBPS;
  nrf_esb_config.event_handler = nrf_esb_event_handler;
  nrf_esb_config.mode = NRF_ESB_MODE_PTX;
  nrf_esb_config.selective_auto_ack = false;

  err_code = nrf_esb_init(&nrf_esb_config);

  VERIFY_SUCCESS(err_code);

  err_code = nrf_esb_set_base_address_0(base_addr_0);
  VERIFY_SUCCESS(err_code);

  err_code = nrf_esb_set_base_address_1(base_addr_1);
  VERIFY_SUCCESS(err_code);

  err_code = nrf_esb_set_prefixes(addr_prefix, NRF_ESB_PIPE_COUNT);
  VERIFY_SUCCESS(err_code);

  return err_code;
}

void i2c_init() {
  init_clock();
  Wire.setTimeout(2000);
  Wire.begin();
}

int main(void) {
  ret_code_t err_code;

  gpio_init();

  i2c_init();

  VL53L0X sensor;
  sensor.setWire(&Wire);
  sensor.setMeasurementTimingBudget(200000); //= 200ms, def is 33
  sensor.init();
  sensor.startContinuous(499);

  err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_DEFAULT_BACKENDS_INIT();

  clocks_start();
  // Warning - this will fail on a Nano2 if uart tx is not hooked up (loop back wire works)
//
  err_code = uart_init();
  APP_ERROR_CHECK(err_code);

  err_code = esb_init();
  APP_ERROR_CHECK(err_code);

  printf("\r\nEnhanced ShockBurst Transmitter Example started.\r\n");
      NRF_LOG_ERROR("Start!\n");
      uint8_t cal;

  while (true) {
    printf("Transmitting packet %02x\r\n", tx_payload.data[1]);
    NRF_LOG_ERROR(".");
    tx_payload.noack = false;
    if (nrf_esb_write_payload(&tx_payload) == NRF_SUCCESS) {
      // Toggle one of the LEDs.
      nrf_gpio_pin_write(LED_1, !(tx_payload.data[1] % 8 > 0 && tx_payload.data[1] % 8 <= 4));
#ifdef LED_4
      nrf_gpio_pin_write(LED_2, !(tx_payload.data[1] % 8 > 1 && tx_payload.data[1] % 8 <= 5));
      nrf_gpio_pin_write(LED_3, !(tx_payload.data[1] % 8 > 2 && tx_payload.data[1] % 8 <= 6));
      nrf_gpio_pin_write(LED_4, !(tx_payload.data[1] % 8 > 3));
#endif
      tx_payload.data[1]++;
    } else {
      printf("Sending packet failed\r\n");
    }

    if (tx_payload.data[1] % 16 == 0) {
      uint16_t distance = sensor.readRangeContinuousMillimeters(false);
      if (distance == 65535) {
          nrf_delay_us(500000);
        //failed once;
        printf("Failed Read\r\n");
      } else {

        // all good!
        printf("Got Distance:%" PRIu16 "\r\n", distance);
      }
    }

    nrf_delay_us(50000);
  }
}