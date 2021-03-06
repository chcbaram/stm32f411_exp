/*
 * dxl.c
 *
 *  Created on: 2021. 3. 6.
 *      Author: baram
 */


#include "dxl.h"
#include "cli.h"



#ifdef _USE_HW_CLI
static void cliDxl(cli_args_t *args);
#endif

bool dxlInit(void)
{
  bool ret = true;


#ifdef _USE_HW_CLI
  cliAdd("dxl", cliDxl);
#endif

  return ret;
}

bool dxlOpen(dxl_t *p_dxl, uint8_t ch, uint32_t baud)
{
  bool ret = true;


  return ret;
}

bool dxlClose(dxl_t *p_dxl)
{
  bool ret = true;


  return ret;
}

bool dxlSendInst(dxl_t *p_dxl, uint8_t id,  uint8_t inst, uint8_t *p_param, uint16_t param_len)
{
  bool ret = true;


  return ret;
}

bool dxlReceivePacket(dxl_t *p_dxl)
{
  bool ret = true;


  return ret;
}




#ifdef _USE_HW_CLI
static void cliDxl(cli_args_t *args)
{
  bool ret = false;




  if (ret == false)
  {
    cliPrintf("dxl open 0~X\n");
  }
}
#endif
