/*
 * hw.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "hw.h"



extern uint32_t _flash_tag_addr;
extern uint32_t __isr_vector_addr;


__attribute__((section(".tag"))) firm_tag_t firm_tag =
    {
        .magic_number = 0xAAAA5555,

        //-- fw info
        //
        .addr_tag = (uint32_t)&_flash_tag_addr,
        .addr_fw  = (uint32_t)&__isr_vector_addr,
        .size_tag = 1024,


        //-- tag info
        //
    };


__attribute__((section(".version"))) firm_version_t firm_ver =
    {
        "V210211R2",
        "STM32F411_EXP_BD"
    };


void hwInit(void)
{
  bspInit();

  rtcInit();
  resetInit();
  cliInit();
  ledInit();
  usbInit();
  uartInit();
  buttonInit();
  gpioInit();
  flashInit();
  spiInit();
  i2cInit();
  i2sInit();
  eepromInit();
  adcInit();
  pwmInit();

  if (sdInit() == true)
  {
    fatfsInit();
  }

  lcdInit();
  mcp2515Init();

  usbBegin(USB_CDC_MODE);
  espInit();
}
