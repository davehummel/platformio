/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_timer.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_serial.h"

#include "app_error.h"
#include "app_util.h"
#include "boards.h"
#include "nrf_error.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "esb_radio.h"
#include "esb_mesh.h"

/** @file
 * @defgroup nrf_serial_example main.c
 * @{
 * @ingroup nrf_serial_example
 * @brief Example of @ref nrf_serial usage. Simple loopback.
 *
 */

//#define OP_QUEUES_SIZE 3
//#define APP_TIMER_PRESCALER NRF_SERIAL_APP_TIMER_PRESCALER
EsbRadio Radio;
EsbMesh Mesh;

//Timer code below
//APP_TIMER_DEF(m_led_a_timer_id);

static uint8_t data[8] = {0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00};

static uint32_t counter = 0;

// Timeout handler for the repeated timer
//static void timer_a_handler(void *p_context) {
//
//  counter++;
//  if (counter % 16 == 0) {
//    Radio.switchModes(NRF_ESB_MODE_PTX);
//  }
//  if (counter % 16 < 8) {
//    nrf_gpio_pin_write(LED_1, (data[1]++) % 2);
//    APP_ERROR_CHECK(Radio.transmit(0, 8, data));
//  }
//  if (counter % 16 == 8) {
//    APP_ERROR_CHECK(Radio.switchModes(NRF_ESB_MODE_PRX));
//  }
//  if (counter % 16 >= 8) {
//    nrf_gpio_pin_write(LED_3, (data[1]++) % 2);
//    APP_ERROR_CHECK(Radio.transmit(0, 8, data));
//  }
//}

// Create timers
//static void create_timers() {
//  uint32_t err_code;
//
//  // Create timers
//  err_code = app_timer_create(&m_led_a_timer_id,
//      APP_TIMER_MODE_REPEATED,
//      timer_a_handler);
//  APP_ERROR_CHECK(err_code);
//}
// Timer code abouve

static void sleep_handler(void) {
  __WFE();
  __SEV();
  __WFE();
}

NRF_SERIAL_DRV_UART_CONFIG_DEF(m_uart0_drv_config,
    RX_PIN_NUMBER, TX_PIN_NUMBER,
    RTS_PIN_NUMBER, CTS_PIN_NUMBER,
    NRF_UART_HWFC_DISABLED, NRF_UART_PARITY_EXCLUDED,
    NRF_UART_BAUDRATE_115200,
    UART_DEFAULT_CONFIG_IRQ_PRIORITY);

#define SERIAL_FIFO_TX_SIZE 32
#define SERIAL_FIFO_RX_SIZE 32

NRF_SERIAL_QUEUES_DEF(serial_queues, SERIAL_FIFO_TX_SIZE, SERIAL_FIFO_RX_SIZE);

#define SERIAL_BUFF_TX_SIZE 1
#define SERIAL_BUFF_RX_SIZE 1

NRF_SERIAL_BUFFERS_DEF(serial_buffs, SERIAL_BUFF_TX_SIZE, SERIAL_BUFF_RX_SIZE);

NRF_SERIAL_CONFIG_DEF(serial_config, NRF_SERIAL_MODE_IRQ,
    &serial_queues, &serial_buffs, NULL, sleep_handler);

NRF_SERIAL_UART_DEF(serial_uart, 0);

static nrf_esb_payload_t rx_payload;

void nrf_esb_event_handler(nrf_esb_evt_t const *p_event) {

  if (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS) {
    // Set LEDs identical to the ones on the PTX.
    nrf_gpio_pin_write(LED_2, (rx_payload.data[1]++) % 2);
    (void)nrf_serial_write(&serial_uart, rx_payload.data, rx_payload.length, NULL, 0);
    (void)nrf_serial_flush(&serial_uart, 0);
    NRF_LOG_INFO("Receiving packet: %02x", rx_payload.data[1]);
  }
}

int main(void) {

  ret_code_t ret;
  // Init logging
  ret = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(ret);
  NRF_LOG_DEFAULT_BACKENDS_INIT();

  NRF_LOG_INFO("Logging enabled.");
  // Init clocks and timers
  ret = nrf_drv_clock_init();
  APP_ERROR_CHECK(ret);
  nrf_drv_clock_lfclk_request(NULL);

  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;

  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    ;

  NRF_LOG_INFO("Clock driver enabled.");
  ret = app_timer_init();
  APP_ERROR_CHECK(ret);

  // Init power management
  ret = nrf_drv_power_init(NULL);
  APP_ERROR_CHECK(ret);
  NRF_LOG_INFO("Power driver enabled.");
  // Initialize LEDs and buttons.
  bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);
  // Init Serial on uart 0
  ret = nrf_serial_init(&serial_uart, &m_uart0_drv_config, &serial_config);
  APP_ERROR_CHECK(ret);

  NRF_LOG_INFO("Serial Enabled enabled.");

//  {
//    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
//    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
//    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};
//    APP_ERROR_CHECK(Radio.setPrimaryAddr(base_addr_0));
//    APP_ERROR_CHECK(Radio.setSecondaryAddr(base_addr_1));
//    APP_ERROR_CHECK(Radio.setAllAddrPrefix(8, addr_prefix));
//  }
//  Radio.setReceivedListener(nrf_esb_event_handler);
//
//  Radio.init(NRF_ESB_MODE_PTX);

  ret = Mesh.join(123,EsbMesh::ROLE_HIGH_POWER);


  NRF_LOG_INFO("Mesh enabled.");

  // Create application timers.
  //create_timers();
  NRF_LOG_INFO("T%u", APP_TIMER_TICKS(.5));
 // app_timer_stop(m_led_a_timer_id);
 // ret = app_timer_start(m_led_a_timer_id, APP_TIMER_TICKS(500), NULL);
  APP_ERROR_CHECK(ret);
  NRF_LOG_INFO("Timer enabled.");

  static char tx_message[] = "Serial Started!\n\r";

  ret = nrf_serial_write(&serial_uart,
      tx_message,
      strlen(tx_message),
      NULL,
      NRF_SERIAL_MAX_TIMEOUT);
  APP_ERROR_CHECK(ret);

  while (true) {
    if (NRF_LOG_PROCESS() == false) {
      __WFE();
    }
  }
}

/** @} */