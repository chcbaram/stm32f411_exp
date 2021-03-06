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



LCD_IMAGE_DEF(img_logo);
LCD_IMAGE_DEF(img_logo2);

void cliBoot(cli_args_t *args);
void lcdMain(args_t *p_args);
void sdMain(args_t *p_args);


void apInit(void)
{
  cliOpen(_DEF_UART1, 57600);   // USB
  cliAdd("boot", cliBoot);

  uartOpen(_DEF_UART2, 57600);  // XL-330
  uartOpen(_DEF_UART3, 115200); // ESP8266
}

void apMain(void)
{
  uint32_t pre_time;
  args_t args;
  uint8_t  buf[128];
  uint16_t buf_len;
  uint32_t baud;

  baud = uartGetBaud(_DEF_UART1);


  args.mode = 0;
  args.x_time = 0;
  args.pre_time = millis();

  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 1500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    if (args.mode == 0)
    {
      cliMain();
    }
    else
    {
      if (baud != uartGetBaud(_DEF_UART1))
      {
        baud = uartGetBaud(_DEF_UART1);
        uartOpen(_DEF_UART2, baud);
      }

      // USB -> XL-330
      buf_len = uartAvailable(_DEF_UART1);
      if (buf_len > 0)
      {
        if (buf_len > 128)
        {
          buf_len = 128;
        }
        for (int i=0; i<buf_len; i++)
        {
          buf[i] = uartRead(_DEF_UART1);
        }
        uartWrite(_DEF_UART2, &buf[0], buf_len);
      }
      // XL-330 -> USB
      buf_len = uartAvailable(_DEF_UART2);
      if (buf_len > 0)
      {
        if (buf_len > 128)
        {
          buf_len = 128;
        }
        for (int i=0; i<buf_len; i++)
        {
          buf[i] = uartRead(_DEF_UART2);
        }
        uartWrite(_DEF_UART1, &buf[0], buf_len);
      }
    }

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
      lcdPrintfResize(0,16, green, 32, "U2D2 Mode");
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
