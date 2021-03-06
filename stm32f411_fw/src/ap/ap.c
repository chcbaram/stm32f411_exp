/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"


LCD_IMAGE_DEF(img_logo);
LCD_IMAGE_DEF(img_logo2);

void cliBoot(cli_args_t *args);
void lcdMain(void);

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
  sd_state_t sd_state;
  uint8_t  buf[128];
  uint16_t buf_len;
  uint32_t baud;

  baud = uartGetBaud(_DEF_UART1);

  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 1500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);

      buf[0] = 0xFF;
      buf[1] = 0xFF;
      buf[2] = 0xFD;
      buf[3] = 0x00;
      buf[4] = 0x01;
      buf[5] = 0x03;
      buf[6] = 0x00;
      buf[7] = 0x01;
      buf[8] = 0x19;
      buf[9] = 0x4E;

      logPrintf("\n");
      uartWrite(_DEF_UART2, &buf[0], 10);
    }

#if 1

    if (uartAvailable(_DEF_UART2) > 0)
    {
      logPrintf("rx : 0x%X\n", uartRead(_DEF_UART2));
    }


#else

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
#endif


    //cliMain();
    lcdMain();

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
}

void lcdMain(void)
{
  static uint32_t pre_time;

	if (lcdIsInit() != true)
	{
	  return;
	}

	if (millis()-pre_time >= (1000/30) && lcdDrawAvailable() == true)
	{
	  pre_time = millis();

	  lcdClearBuffer(black);

#if 1
	  int16_t x1 = 0;
	  int16_t x2 = 0;

	  static int16_t x_time = 0;
	  static float font_size = 1;
	  static uint8_t mode = 0;

	  if (buttonGetPressed(_DEF_BUTTON1))
	  {
	    mode = (mode + 1)%3;
	    delay(200);
	  }
	  if (mode == 0)
	  {
      x_time += 2;

      x1 = x_time;
      x1 %= (LCD_WIDTH-img_logo.header.w);;

      x2 = x_time;
      x2 %= (LCD_WIDTH-img_logo.header.w);
      x2 = LCD_WIDTH - img_logo.header.w - x2;

      lcdDrawImage(x1, 0, &img_logo);
      lcdDrawImage(x2, 0, &img_logo2);
	  }

	  if (mode == 1)
	  {
      lcdPrintfResize(0, 0, green, font_size, "폰트시험");
      font_size += 1;
      if (font_size >= 60)
      {
        font_size = 1;
      }
	  }

    if (mode == 2)
    {
      lcdPrintfResize(0, 0, green, 16, "폰트시험");
      lcdPrintfResize(0,16, green, 24, "폰트시험");
      lcdPrintfResize(0,40, green, 32, "폰트시험");
    }
#else
    McpMode mode;
    McpBaud baud;
    int16_t x = 0;
    int16_t y = 6;

	  lcdSetFont(LCD_FONT_HAN);
	  lcdPrintf(24,16*0, green, "[CAN 통신]");

	  lcdSetFont(LCD_FONT_07x10);

	  for (int ch=0; ch<MCP2515_MAX_CH; ch++)
	  {
      mode = mcp2515GetMode(ch);
      baud = mcp2515GetBaud(ch);

      x = 0;
      y += 12;
      switch(mode)
      {
        case MCP_MODE_NORMAL:
        lcdPrintf(x, y, white, "ch%d Mode : Normal", ch);
        break;
        case MCP_MODE_SLEEP:
        lcdPrintf(x, y, white, "ch%d Mode : Sleep", ch);
        break;
        case MCP_MODE_LOOPBACK:
        lcdPrintf(x, y, white, "ch%d Mode : Loopback", ch);
        break;
        case MCP_MODE_LISTEN:
        lcdPrintf(x, y, white, "ch%d Mode : Listen", ch);
        break;
        case MCP_MODE_CONFIG:
        lcdPrintf(x, y, white, "ch%d Mode : Config", ch);
        break;
      }

      x = 0;
      y += 12;
      switch(baud)
      {
        case MCP_BAUD_100K:
        lcdPrintf(x, y, white, "    Baud : 100Kbps", ch);
        break;
        case MCP_BAUD_125K:
        lcdPrintf(x, y, white, "    Baud : 125Kbps", ch);
        break;
        case MCP_BAUD_250K:
        lcdPrintf(x, y, white, "    Baud : 250Kbps", ch);
        break;
        case MCP_BAUD_500K:
        lcdPrintf(x, y, white, "    Baud : 500Kbps", ch);
        break;
        case MCP_BAUD_1000K:
        lcdPrintf(x, y, white, "    Baud : 1Mbps", ch);
        break;
        default:
        lcdPrintf(x, y, white, "    Baud : unknown", ch);
        break;
      }
	  }
#endif

	  lcdRequestDraw();
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
