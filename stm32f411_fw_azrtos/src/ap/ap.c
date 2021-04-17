/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"


typedef struct
{
  uint32_t pre_time;
  uint16_t x_time;
  uint8_t  mode;
} args_t;


TX_THREAD led_thread;


LCD_IMAGE_DEF(img_logo);
LCD_IMAGE_DEF(img_logo2);

void cliBoot(cli_args_t *args);
void lcdMain(args_t *p_args);
void sdMain(args_t *p_args);
void ledThread(ULONG thread_input);


void apInit(void)
{
  cliOpen(_DEF_UART1, 57600);   // USB
  cliAdd("boot", cliBoot);

  uartOpen(_DEF_UART2, 57600);  // USART1
  uartOpen(_DEF_UART3, 115200); // ESP8266

  CHAR *pointer;


  if (tx_byte_allocate(azureGetPool(), (VOID **) &pointer,
                       _HW_DEF_RTOS_THREAD_MEM_LED, TX_NO_WAIT) != TX_SUCCESS)
  {
    return;
  }

  if (tx_thread_create(&led_thread, "ledThread", ledThread, 0,
                       pointer, _HW_DEF_RTOS_THREAD_MEM_LED,
                       _HW_DEF_RTOS_THREAD_PRI_LED, _HW_DEF_RTOS_THREAD_PRI_LED,
                       TX_1_ULONG, TX_AUTO_START) != TX_SUCCESS)
  {
    return;
  }
}

void apMain(void)
{
  uint32_t pre_time;
  args_t args;

  args.mode = 0;
  args.x_time = 0;
  args.pre_time = millis();


  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 1500)
    {
      pre_time = millis();
      //ledToggle(_DEF_LED1);
    }

    if (uartAvailable(_DEF_UART2) > 0)
    {
      uartPrintf(_DEF_UART2, "Rx : 0x%x\n", uartRead(_DEF_UART2));
    }
    cliMain();

    lcdMain(&args);
    sdMain(&args);
  }
}

void lcdMain(args_t *p_args)
{
  if (lcdIsInit() != true)
  {
    return;
  }

  if (millis()-p_args->pre_time >= (1000/30) && lcdDrawAvailable() == true)
  {
    p_args->pre_time = millis();

    lcdClearBuffer(black);

    uint16_t x1 = 0;
    uint16_t x2 = 0;


    if (buttonGetPressed(_DEF_BUTTON1))
    {
      p_args->mode = (p_args->mode + 1)%2;
      delay(200);
    }
    if (p_args->mode == 0)
    {
      p_args->x_time += 2;

      x1 = p_args->x_time;
      x1 %= (LCD_WIDTH-img_logo.header.w);;

      x2 = p_args->x_time;
      x2 %= (LCD_WIDTH-img_logo.header.w);
      x2 = LCD_WIDTH - img_logo.header.w - x2;

      lcdDrawImage(x1, 0, &img_logo);
      lcdDrawImage(x2, 0, &img_logo2);
    }

    if (p_args->mode == 1)
    {
      lcdPrintfResize(0,16, green, 32, "Mode 1");
    }

    lcdRequestDraw();
  }
}

void sdMain(args_t *p_args)
{
  sd_state_t sd_state;


  sd_state = sdUpdate();
  if (sd_state == SDCARD_CONNECTED)
  {
    logPrintf("\nSDCARD_CONNECTED\n");
  }
  if (sd_state == SDCARD_DISCONNECTED)
  {
    logPrintf("\nSDCARD_DISCONNECTED\n");
  }
}

void cliBoot(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    firm_version_t *p_boot_ver = (firm_version_t *)(FLASH_ADDR_BOOT_VER);


    cliPrintf("boot ver   : %s\n", p_boot_ver->version);
    cliPrintf("boot name  : %s\n", p_boot_ver->name);
    cliPrintf("boot param : 0x%X\n", rtcBackupRegRead(0));

    cliPrintf("PCLK2 : %d\n",HAL_RCC_GetPCLK2Freq());

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "jump_boot") == true)
  {
    resetToBoot(0);
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "jump_fw") == true)
  {
    rtcBackupRegWrite(0, 0);
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("boot info\n");
  }
}

void ledThread(ULONG thread_input)
{
  (void) thread_input;

  while(1)
  {
    ledToggle(_DEF_LED1);
    delay(500);
  }
}
