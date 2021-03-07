/*
 * dxl.h
 *
 *  Created on: 2021. 3. 6.
 *      Author: baram
 */

#ifndef SRC_COMMON_HW_INCLUDE_DXL_H_
#define SRC_COMMON_HW_INCLUDE_DXL_H_

#ifdef __cplusplus
 extern "C" {
#endif



#include "hw_def.h"

#ifdef _USE_HW_DXL

#define DXL_PACKET_BUF_MAX        HW_DXL_PACKET_BUF_MAX



#define DXL_BROADCAST_ID     254


enum
{
  DXL_INST_PING           = 0x01,
  DXL_INST_READ           = 0x02,
  DXL_INST_WRITE          = 0x03,
  DXL_INST_REG_WRITE      = 0x04,
  DXL_INST_ACTION         = 0x05,
  DXL_INST_FACTORY_RESET  = 0x06,
  DXL_INST_REBOOT         = 0x08,
  DXL_INST_CLEAR          = 0x10,
  DXL_INST_STATUS         = 0x55,
  DXL_INST_SYNC_READ      = 0x82,
  DXL_INST_SYNC_WRITE     = 0x83,
  DXL_INST_BULK_READ      = 0x92,
  DXL_INST_BULK_WRITE     = 0x93,
};


typedef struct
{
  uint8_t  header[3];
  uint8_t  reserved;
  uint8_t  id;
  uint16_t length;
  uint8_t  inst;
  uint8_t  err;
  uint16_t param_len;
  uint16_t param_index;
  uint8_t  *param;
  uint16_t crc;
} dxl_packet_t;


typedef struct
{
  bool     is_open;
  uint8_t  ch;
  uint32_t baud;
  uint8_t  state;
  uint32_t pre_time;
  uint16_t index;
  bool     is_status_packet;

  dxl_packet_t packet;

  uint8_t  packet_buf[DXL_PACKET_BUF_MAX];
} dxl_t;


bool dxlInit(void);
bool dxlOpen(dxl_t *p_dxl, uint8_t dxl_ch, uint32_t baud);
bool dxlClose(dxl_t *p_dxl);
bool dxlSendInst(dxl_t *p_dxl, uint8_t id,  uint8_t inst, uint8_t *p_param, uint16_t param_len);
bool dxlReceivePacket(dxl_t *p_dxl);




typedef struct
{
  uint16_t model_number;
  uint8_t  firm_version;
} dxl_inst_ping_resp_t;




bool dxlInstPing(dxl_t *p_dxl, uint8_t id, dxl_inst_ping_resp_t *p_resp, uint32_t timeout);
bool dxlInstRead(dxl_t *p_dxl, uint8_t id, uint16_t addr, uint8_t *p_data, uint16_t length, uint32_t timeout);
bool dxlInstWrite(dxl_t *p_dxl, uint8_t id, uint16_t addr, uint8_t *p_data, uint16_t length, uint32_t timeout);



#endif


#ifdef __cplusplus
}
#endif


#endif /* SRC_COMMON_HW_INCLUDE_DXL_H_ */
