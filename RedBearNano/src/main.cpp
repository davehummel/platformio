#include <stdio.h>
#include "mbed.h"
#include "BLEDevice.h"

DigitalOut myled(P0_19);

BLEDevice  ble;

void disconnectionCallback(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    ble.startAdvertising(); // restart advertising
}

void periodicCallback(void)
{
    myled = !myled; /* Do blinky on LED1 while we're waiting for BLE events */

    /* Note that the periodicCallback() executes in interrupt context, so it is safer to do
     * heavy-weight sensor polling from the main thread. */
    triggerSensorPolling = true;
}


int main()
{

  Ticker ticker;
    ticker.attach(periodicCallback, 1);

    ble.init();
   ble.onDisconnection(disconnectionCallback);

   ble.startAdvertising();


      while (true) {
              ble.waitForEvent();
      }

    return 0;
}
