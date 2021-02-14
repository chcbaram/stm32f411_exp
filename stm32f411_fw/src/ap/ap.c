/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"




void cliBoot(cli_args_t *args);
void lcdMain(void);

void apInit(void)
{
  cliOpen(_DEF_UART1, 57600);
  cliAdd("boot", cliBoot);

  uartOpen(_DEF_UART2, 57600);
  uartOpen(_DEF_UART3, 115200);
}

void apMain(void)
{
  uint32_t pre_time;
  sd_state_t sd_state;


  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    if (uartAvailable(_DEF_UART2) > 0)
    {
      uint8_t rx_data;

      rx_data = uartRead(_DEF_UART2);

      if (rx_data == '1')
      {
        uartPrintf(_DEF_UART3, "AT\r\n");
      }
    }
    if (uartAvailable(_DEF_UART3) > 0)
    {
      uint8_t rx_data;
      rx_data = uartRead(_DEF_UART3);
      uartPrintf(_DEF_UART2, "%c", rx_data);
    }

    cliMain();
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
  McpMode mode;
  McpBaud baud;
  int16_t x = 0;
	int16_t y = 6;

	if (lcdIsInit() != true)
	{
	  return;
	}

	if (millis()-pre_time >= (1000/30) && lcdDrawAvailable() == true)
	{
	  pre_time = millis();

	  lcdClearBuffer(black);

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
