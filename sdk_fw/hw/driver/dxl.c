/*
 * dxl.c
 *
 *  Created on: 2021. 3. 6.
 *      Author: baram
 */


#include "dxl.h"
#include "cli.h"
#include "dxl/dxl_uart.h"



enum
{
  DXL_PKT_HEADER_1 = 0,
  DXL_PKT_HEADER_2,
  DXL_PKT_HEADER_3,
  DXL_PKT_RESERVED,
  DXL_PKT_ID,
  DXL_PKT_LENGTH_1,
  DXL_PKT_LENGTH_2,
  DXL_PKT_INST,
  DXL_PKT_ERR,
};

enum
{
  DXL_STATE_HEADER_1,
  DXL_STATE_HEADER_2,
  DXL_STATE_HEADER_3,
  DXL_STATE_RESERVED,
  DXL_STATE_ID,
  DXL_STATE_LENGTH_1,
  DXL_STATE_LENGTH_2,
  DXL_STATE_INST,
  DXL_STATE_ERR,
  DXL_STATE_PARAM,
  DXL_STATE_CRC_L,
  DXL_STATE_CRC_H,
};



static bool dxlSendInst(dxl_t *p_dxl, uint8_t id,  uint8_t inst, uint8_t *p_param, uint16_t param_len);
static bool dxlReceivePacket(dxl_t *p_dxl);
static uint16_t dxlUpdateCrc(uint16_t crc_accum, uint8_t *data_blk_ptr, uint16_t data_blk_size);




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

bool dxlLoadDriver(dxl_t *p_dxl, bool (*load_func)(dxl_driver_t *))
{
  bool ret;

  ret = load_func(&p_dxl->driver);

  return ret;
}

bool dxlOpen(dxl_t *p_dxl, uint8_t dxl_ch, uint32_t baud)
{
  bool ret = true;

  if (p_dxl->driver.is_init == false)
  {
    return false;
  }

  p_dxl->ch    = dxl_ch;
  p_dxl->baud  = baud;
  p_dxl->state = DXL_STATE_HEADER_1;
  p_dxl->pre_time     = millis();
  p_dxl->packet.param = NULL;
  p_dxl->is_open      = p_dxl->driver.open(dxl_ch, baud);

  ret = p_dxl->is_open;

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
  uint16_t packet_len;
  uint16_t crc = 0;
  uint16_t index;
  uint32_t  stuff_header;

  packet_len = param_len + 3;

  index = 0;
  p_dxl->packet_buf[DXL_PKT_HEADER_1] = 0xFF;
  p_dxl->packet_buf[DXL_PKT_HEADER_2] = 0xFF;
  p_dxl->packet_buf[DXL_PKT_HEADER_3] = 0xFD;
  p_dxl->packet_buf[DXL_PKT_RESERVED] = 0x00;
  p_dxl->packet_buf[DXL_PKT_ID]       = id;
  p_dxl->packet_buf[DXL_PKT_LENGTH_1] = (packet_len >> 0) & 0xFF;
  p_dxl->packet_buf[DXL_PKT_LENGTH_2] = (packet_len >> 8) & 0xFF;


  //-- Add Stuffing
  //
  index = DXL_PKT_INST;
  stuff_header = 0;

  p_dxl->packet_buf[index++] = inst;
  stuff_header = inst;
  p_dxl->packet.param = &p_dxl->packet_buf[8];

  for (int i=0; i<param_len; i++)
  {
    p_dxl->packet_buf[index++] = p_param[i];

    // Add Byte Stuffing
    stuff_header <<= 8;
    stuff_header  |= p_param[i];
    stuff_header  &= 0x00FFFFFF;

    if (stuff_header == 0x00FFFFFD)
    {
      p_dxl->packet_buf[index++] = 0xFD;
      packet_len++;
    }
  }

  p_dxl->packet_buf[DXL_PKT_LENGTH_1] = (packet_len >> 0) & 0xFF;
  p_dxl->packet_buf[DXL_PKT_LENGTH_2] = (packet_len >> 8) & 0xFF;


  crc = dxlUpdateCrc(0, p_dxl->packet_buf, index);

  p_dxl->packet_buf[index++] = (crc >> 0) & 0xFF;
  p_dxl->packet_buf[index++] = (crc >> 8) & 0xFF;


  p_dxl->driver.write(p_dxl->ch, p_dxl->packet_buf, index);

  return ret;
}

bool dxlReceivePacket(dxl_t *p_dxl)
{
  bool ret = false;
  uint8_t rx_data;
  uint16_t crc;
  uint16_t index;


  if (p_dxl->driver.available(p_dxl->ch) > 0)
  {
    rx_data = p_dxl->driver.read(p_dxl->ch);
  }
  else
  {
    return false;
  }

  if (millis()-p_dxl->pre_time >= 100)
  {
    p_dxl->state = DXL_STATE_HEADER_1;
  }
  p_dxl->pre_time = millis();


  switch(p_dxl->state)
  {
    case DXL_STATE_HEADER_1:
      if (rx_data == 0xFF)
      {
        p_dxl->packet_buf[DXL_PKT_HEADER_1] = rx_data;
        p_dxl->state = DXL_STATE_HEADER_2;
      }
      break;

    case DXL_STATE_HEADER_2:
      if (rx_data == 0xFF)
      {
        p_dxl->packet_buf[DXL_PKT_HEADER_2] = rx_data;
        p_dxl->state = DXL_STATE_HEADER_3;
      }
      else
      {
        p_dxl->state = DXL_STATE_HEADER_1;
      }
      break;

    case DXL_STATE_HEADER_3:
      if (rx_data == 0xFD)
      {
        p_dxl->packet_buf[DXL_PKT_HEADER_3] = rx_data;
        p_dxl->state = DXL_STATE_RESERVED;
      }
      else
      {
        p_dxl->state = DXL_STATE_HEADER_1;
      }
      break;

    case DXL_STATE_RESERVED:
      if (rx_data == 0x00)
      {
        p_dxl->packet_buf[DXL_PKT_RESERVED] = rx_data;
        p_dxl->state = DXL_STATE_ID;
      }
      else
      {
        p_dxl->state = DXL_STATE_HEADER_1;
      }
      break;

    case DXL_STATE_ID:
      p_dxl->packet_buf[DXL_PKT_ID] = rx_data;
      p_dxl->state = DXL_STATE_LENGTH_1;
      break;

    case DXL_STATE_LENGTH_1:
      p_dxl->packet_buf[DXL_PKT_LENGTH_1] = rx_data;
      p_dxl->state = DXL_STATE_LENGTH_2;
      break;

    case DXL_STATE_LENGTH_2:
      p_dxl->packet_buf[DXL_PKT_LENGTH_2] = rx_data;
      p_dxl->state = DXL_STATE_INST;
      break;

    case DXL_STATE_INST:
      p_dxl->packet_buf[DXL_PKT_INST] = rx_data;
      p_dxl->packet.length  = p_dxl->packet_buf[DXL_PKT_LENGTH_1] << 0;
      p_dxl->packet.length |= p_dxl->packet_buf[DXL_PKT_LENGTH_2] << 8;
      p_dxl->packet.param_index = 0;

      if (rx_data == 0x55)
      {
        p_dxl->is_status_packet = true;
        p_dxl->packet.param_len = p_dxl->packet.length - 4;
        p_dxl->index = DXL_PKT_ERR + 1;
        p_dxl->state = DXL_STATE_ERR;
      }
      else
      {
        p_dxl->is_status_packet = false;
        p_dxl->packet.param_len = p_dxl->packet.length - 3;
        p_dxl->index = DXL_PKT_INST + 1;
        if (p_dxl->packet.param_len > 0)
        {
          p_dxl->state = DXL_STATE_PARAM;
        }
        else
        {
          p_dxl->state = DXL_STATE_CRC_L;
        }
      }
      p_dxl->packet.param = &p_dxl->packet_buf[p_dxl->index];
      break;

    case DXL_STATE_ERR:
      p_dxl->packet_buf[DXL_PKT_ERR] = rx_data;
      if (p_dxl->packet.param_len > 0)
      {
        p_dxl->state = DXL_STATE_PARAM;
      }
      else
      {
        p_dxl->state = DXL_STATE_CRC_L;
      }
      break;

    case DXL_STATE_PARAM:
      index = p_dxl->index + p_dxl->packet.param_index;
      p_dxl->packet.param_index++;
      p_dxl->packet_buf[index] = rx_data;
      if (p_dxl->packet.param_index >= p_dxl->packet.param_len)
      {
        p_dxl->index += p_dxl->packet.param_len;
        p_dxl->state = DXL_STATE_CRC_L;
      }
      break;

    case DXL_STATE_CRC_L:
      p_dxl->packet.crc = rx_data;
      p_dxl->state = DXL_STATE_CRC_H;
      break;

    case DXL_STATE_CRC_H:
      p_dxl->packet.crc |= rx_data<<8;

      crc = dxlUpdateCrc(0, p_dxl->packet_buf, p_dxl->index);

      if (crc == p_dxl->packet.crc)
      {
        ret = true;
      }

      p_dxl->state = DXL_STATE_HEADER_1;
      break;
  }


  if (ret == true)
  {
    p_dxl->packet.id   = p_dxl->packet_buf[DXL_PKT_ID];
    p_dxl->packet.inst = p_dxl->packet_buf[DXL_PKT_INST];
    p_dxl->packet.err  = p_dxl->packet_buf[DXL_PKT_ERR];

    //-- Remove Stuffing
    //
    uint16_t stuff_len;
    uint16_t stuff_index;
    uint32_t stuff_header;

    stuff_len = p_dxl->packet.length - 2;

    stuff_header = 0;
    stuff_index = DXL_PKT_INST;
    for (int i=0; i<stuff_len; i++)
    {
      stuff_header |= p_dxl->packet_buf[DXL_PKT_INST + i];
      if (stuff_header == 0xFFFFFDFD)
      {
        p_dxl->packet.length--;
        p_dxl->packet.param_len--;
        i++;
      }

      p_dxl->packet_buf[stuff_index] = p_dxl->packet_buf[DXL_PKT_INST + i];
      stuff_index++;

      stuff_header <<= 8;
    }

    p_dxl->packet_buf[DXL_PKT_LENGTH_1] = p_dxl->packet.length >> 0;
    p_dxl->packet_buf[DXL_PKT_LENGTH_2] = p_dxl->packet.length >> 8;
  }


  return ret;
}

uint16_t dxlUpdateCrc(uint16_t crc_accum, uint8_t *data_blk_ptr, uint16_t data_blk_size)
{
    uint16_t  i, j;
    const uint16_t  crc_table[256] = {
        0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
        0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
        0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
        0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
        0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
        0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
        0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
        0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
        0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,
        0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
        0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,
        0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
        0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
        0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
        0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
        0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
        0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,
        0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
        0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,
        0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
        0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
        0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
        0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,
        0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
        0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
        0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
        0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
        0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
        0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
        0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
        0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,
        0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202
    };

    for(j = 0; j < data_blk_size; j++)
    {
      i = ((uint16_t)(crc_accum >> 8) ^ data_blk_ptr[j]) & 0xFF;
      crc_accum = (crc_accum << 8) ^ crc_table[i];
    }

    return crc_accum;
}




bool dxlInstPing(dxl_t *p_dxl, uint8_t id, dxl_ping_t *p_inst, uint32_t timeout)
{
  bool ret;
  uint32_t pre_time;
  uint16_t max_cnt = 1;

  if (id == DXL_BROADCAST_ID)
  {
    max_cnt = (sizeof(p_inst->buf) - 11) / sizeof(dxl_ping_resp_t);
    if (max_cnt > 253)
    {
      max_cnt = 253;
    }
  }

  p_inst->resp_cnt = 0;
  p_inst->p_resp = (dxl_ping_resp_t *)p_inst->buf;


  ret = dxlSendInst(p_dxl, id, DXL_INST_PING, NULL, 0);
  if (ret == true)
  {
    pre_time = millis();
    while(millis()-pre_time < timeout)
    {
      ret = dxlReceivePacket(p_dxl);
      if (ret == true)
      {
        p_inst->p_resp[p_inst->resp_cnt].id            = p_dxl->packet.id;
        p_inst->p_resp[p_inst->resp_cnt].model_number  = p_dxl->packet.param[0] << 0;
        p_inst->p_resp[p_inst->resp_cnt].model_number |= p_dxl->packet.param[1] << 8;
        p_inst->p_resp[p_inst->resp_cnt].firm_version  = p_dxl->packet.param[2];

        p_inst->resp_cnt++;
        if (p_inst->resp_cnt >= max_cnt)
        {
          break;
        }
        pre_time = millis();
      }
    }
  }

  if (p_inst->resp_cnt > 0)
  {
    ret = true;
  }

  return ret;
}

bool dxlInstRead(dxl_t *p_dxl, uint8_t id, uint16_t addr, uint8_t *p_data, uint16_t length, uint32_t timeout)
{
  bool ret;
  uint32_t pre_time;
  uint8_t tx_buf[4];

  tx_buf[0] = addr >> 0;
  tx_buf[1] = addr >> 8;
  tx_buf[2] = length >> 0;
  tx_buf[3] = length >> 8;

  ret = dxlSendInst(p_dxl, id, DXL_INST_READ, tx_buf, 4);

  if (ret == true)
  {
    pre_time = millis();
    while(millis()-pre_time < timeout)
    {
      ret = dxlReceivePacket(p_dxl);
      if (ret == true)
      {
        break;
      }
    }
  }

  if (ret == true && p_data != NULL)
  {
    for (int i=0; i<length; i++)
    {
      p_data[i] = p_dxl->packet.param[i];
    }
  }

  return ret;
}

bool dxlInstWrite(dxl_t *p_dxl, uint8_t id, uint16_t addr, uint8_t *p_data, uint16_t length, uint32_t timeout)
{
  bool ret;
  uint32_t pre_time;
  uint8_t tx_buf[4 + length];

  tx_buf[0] = addr >> 0;
  tx_buf[1] = addr >> 8;

  for (int i=0; i<length; i++)
  {
    tx_buf[2+i] = p_data[i];
  }

  ret = dxlSendInst(p_dxl, id, DXL_INST_WRITE, tx_buf, 2 + length);

  if (ret == true)
  {
    pre_time = millis();
    while(millis()-pre_time < timeout)
    {
      ret = dxlReceivePacket(p_dxl);
      if (ret == true)
      {
        break;
      }
    }
  }

  return ret;
}

bool dxlInstSyncRead(dxl_t *p_dxl, dxl_sync_read_t *p_inst, uint32_t timeout)
{
  bool ret;
  uint8_t id;
  uint16_t index;
  uint32_t pre_time;
  uint16_t resp_max_cnt;
  uint16_t resp_size;
  dxl_sync_read_resp_t *p_resp;

  id = DXL_BROADCAST_ID;


  resp_max_cnt = p_inst->param.id_cnt;
  resp_size = sizeof(dxl_sync_read_resp_t) + p_inst->param.length;

  if (resp_size*resp_max_cnt >= DXL_PACKET_BUF_MAX)
  {
    return false;
  }

  p_inst->resp_cnt = 0;

  index = 0;
  p_inst->buf[index++] = (p_inst->param.addr >> 0) & 0xFF;
  p_inst->buf[index++] = (p_inst->param.addr >> 8) & 0xFF;
  p_inst->buf[index++] = (p_inst->param.length >> 0) & 0xFF;
  p_inst->buf[index++] = (p_inst->param.length >> 8) & 0xFF;

  for (int i=0; i<p_inst->param.id_cnt; i++)
  {
    p_inst->buf[index++] = p_inst->param.id[i];
  }


  ret = dxlSendInst(p_dxl, id, DXL_INST_SYNC_READ, p_inst->buf, index);
  if (ret == true)
  {
    pre_time = millis();
    while(millis()-pre_time < timeout)
    {
      ret = dxlReceivePacket(p_dxl);
      if (ret == true)
      {
        p_resp = &p_inst->resp[p_inst->resp_cnt];
        p_resp->p_data = &p_inst->buf[p_inst->resp_cnt * resp_size];


        p_resp->id     = p_dxl->packet.id;
        p_resp->addr   = p_inst->param.addr;
        p_resp->length = p_inst->param.length;
        for (int i=0; i<p_resp->length; i++)
        {
          p_resp->p_data[i] = p_dxl->packet.param[i];
        }

        p_inst->resp_cnt++;
        if (p_inst->resp_cnt >= resp_max_cnt)
        {
          break;
        }
        pre_time = millis();
      }
    }
  }

  if (p_inst->resp_cnt == resp_max_cnt)
  {
    ret = true;
  }

  return ret;

}


#ifdef _USE_HW_CLI

static dxl_t cli_dxl;

static void cliDxl(cli_args_t *args)
{
  bool ret = false;
  uint32_t pre_time;
  uint32_t exe_time;


  if (args->argc == 2 && args->isStr(0, "open"))
  {
    uint32_t baud;

    baud = args->getData(1);

    dxlLoadDriver(&cli_dxl, dxlUartDriver);
    dxlOpen(&cli_dxl, _DEF_DXL1, baud);

    cliPrintf("dxlOpen ch%d %d bps\n", _DEF_DXL1, baud);
    ret = true;
  }

  if (args->isStr(0, "ping"))
  {
    uint8_t dxl_id;
    uint8_t dxl_ret;
    dxl_ping_t ping_resp;

    if (args->argc == 2)
    {
      dxl_id = (uint8_t)args->getData(1);
    }
    else
    {
      dxl_id = DXL_BROADCAST_ID;
    }

    pre_time = millis();
    dxl_ret = dxlInstPing(&cli_dxl, dxl_id, &ping_resp, 100);
    exe_time = millis()-pre_time;
    if (dxl_ret == true)
    {
      cliPrintf("%d ms\n", exe_time);

      for (int i=0; i<ping_resp.resp_cnt; i++)
      {
        cliPrintf("ID : %d\n", ping_resp.p_resp[i].id);
        cliPrintf("   Model Number : 0x%X(%d)\n", ping_resp.p_resp[i].model_number, ping_resp.p_resp[i].model_number);
        cliPrintf("   Firm Version : 0x%X(%d)\n", ping_resp.p_resp[i].firm_version, ping_resp.p_resp[i].firm_version);
      }
    }
    else
    {
      cliPrintf("dxlInstPing Fail : 0x%X\n", cli_dxl.packet.err);
    }

    ret = true;
  }

  if (args->argc == 4 && args->isStr(0, "read"))
  {
    uint8_t  dxl_id;
    uint16_t dxl_addr;
    uint16_t dxl_len;
    uint8_t  dxl_ret;

    dxl_id   = (uint8_t)args->getData(1);
    dxl_addr = (uint8_t)args->getData(2);
    dxl_len  = (uint8_t)args->getData(3);


    dxl_ret = dxlInstRead(&cli_dxl, dxl_id, dxl_addr, NULL, dxl_len, 100);
    if (dxl_ret == true)
    {
      for (int i=0; i<cli_dxl.packet.param_len; i++)
      {
        cliPrintf("rx %d:0x%02X (%d)\n",
                  dxl_addr + i,
                  cli_dxl.packet.param[i],
                  cli_dxl.packet.param[i]);
      }
    }
    else
    {
      cliPrintf("dxlInstRead Fail : 0x%X\n", cli_dxl.packet.err);
    }

    ret = true;
  }

  if (args->argc == 5 && args->isStr(0, "write"))
  {
    uint8_t  dxl_id;
    uint16_t dxl_addr;
    uint16_t dxl_len;
    uint8_t  dxl_ret;
    uint32_t dxl_data;

    dxl_id   = (uint8_t)args->getData(1);
    dxl_addr = (uint8_t)args->getData(2);
    dxl_data = (uint8_t)args->getData(3);
    dxl_len  = (uint8_t)args->getData(4);

    if (dxl_len > 4) dxl_len = 4;


    dxl_ret = dxlInstWrite(&cli_dxl, dxl_id, dxl_addr, (uint8_t *)&dxl_data, dxl_len, 100);
    if (dxl_ret == true)
    {
      cliPrintf("dxlInstWrite OK\n");
    }
    else
    {
      cliPrintf("dxlInstWrite Fail : 0x%X\n", cli_dxl.packet.err);
    }

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "led_test"))
  {
    uint8_t dxl_id;
    uint8_t dxl_data;
    bool dxl_ret;

    dxl_id = args->getData(1);

    dxl_data = 0;
    while(cliKeepLoop())
    {
      dxl_data ^= 1;
      dxl_ret = dxlInstWrite(&cli_dxl, dxl_id, 65, (uint8_t *)&dxl_data, 1, 100);
      if (dxl_ret == false)
      {
        cliPrintf("dxlInstWrite Fail\n");
        break;
      }
      delay(500);
    }

    ret = true;
  }

  if (args->argc >= 4 && args->isStr(0, "sync_read"))
  {
    uint8_t dxl_ret;
    uint8_t id_cnt;
    dxl_sync_read_t sync_read;


    id_cnt = args->argc - 3;
    sync_read.param.id_cnt = id_cnt;
    sync_read.param.addr   = (uint16_t)args->getData(1);
    sync_read.param.length = (uint16_t)args->getData(2);
    for (int i=0; i<id_cnt; i++)
    {
      sync_read.param.id[i] = (uint8_t)args->getData(3+i);
    }


    pre_time = millis();
    dxl_ret = dxlInstSyncRead(&cli_dxl, &sync_read, 100);
    exe_time = millis()-pre_time;
    if (dxl_ret == true)
    {
      cliPrintf("%d ms\n", exe_time);

      for (int i=0; i<sync_read.resp_cnt; i++)
      {
        cliPrintf("ID : %d\n", sync_read.resp[i].id);
        for (int j=0; j<sync_read.resp[i].length; j++)
        {
          cliPrintf("   data[%d] : 0x%X(%d)\n",
                    j,
                    sync_read.resp[i].p_data[j],
                    sync_read.resp[i].p_data[j]
                    );
        }
      }
    }
    else
    {
      cliPrintf("dxlInstSyncRead Fail : 0x%X\n", cli_dxl.packet.err);
    }

    ret = true;
  }


  if (ret == false)
  {
    cliPrintf("dxl open baud\n");
    cliPrintf("dxl ping id\n");
    cliPrintf("dxl read id addr len\n");
    cliPrintf("dxl write id addr data len(~4)\n");
    cliPrintf("dxl sync_read addr len id1 id2 ...\n");
    cliPrintf("dxl led_test id\n");
  }
}
#endif
