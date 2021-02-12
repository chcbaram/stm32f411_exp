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
}

void apMain(void)
{
  uint32_t pre_time;


  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    cliMain();
    lcdMain();
  }
}

void lcdMain(void)
{
  static uint32_t pre_time;
  McpMode mode;
  McpBaud baud;
  int16_t x;
  int16_t y;

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

    mode = mcp2515GetMode();
    baud = mcp2515GetBaud();

    x = 0;
    y = 18 + 12*0;
    switch(mode)
    {
      case MCP_MODE_NORMAL:
        lcdPrintf(x, y, white, "Mode : Normal");
        break;
      case MCP_MODE_SLEEP:
        lcdPrintf(x, y, white, "Mode : Sleep");
        break;
      case MCP_MODE_LOOPBACK:
        lcdPrintf(x, y, white, "Mode : Loopback");
        break;
      case MCP_MODE_LISTEN:
        lcdPrintf(x, y, white, "Mode : Listen");
        break;
      case MCP_MODE_CONFIG:
        lcdPrintf(x, y, white, "Mode : Config");
        break;
    }

    x = 0;
    y = 18 + 12*1;
    switch(baud)
    {
      case MCP_BAUD_125K:
        lcdPrintf(x, y, white, "Baud : 125Kbps");
        break;
      case MCP_BAUD_250K:
        lcdPrintf(x, y, white, "Baud : 250Kbps");
        break;
      case MCP_BAUD_500K:
        lcdPrintf(x, y, white, "Baud : 500Kbps");
        break;
      case MCP_BAUD_1000K:
        lcdPrintf(x, y, white, "Baud : 1Mbps");
        break;
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
