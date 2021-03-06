/*
 * hw.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "hw.h"


__attribute__((section(".version"))) firm_version_t boot_ver =
    {
        "B210213R1",
        "Bootloader"
    };



void hwInit(void)
{
  bspInit();

  rtcInit();
  resetInit();
  ledInit();
  usbInit();
  uartInit();
  buttonInit();
  flashInit();
}
