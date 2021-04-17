/*
 * azure_rtos.c
 *
 *  Created on: 2021. 4. 17.
 *      Author: baram
 */
#include "azure_rtos.h"





static UCHAR tx_byte_pool_buffer[TX_APP_MEM_POOL_SIZE];
static TX_BYTE_POOL tx_app_byte_pool;



bool azureInit(void)
{
  if (tx_byte_pool_create(&tx_app_byte_pool, "Tx App memory pool", tx_byte_pool_buffer, TX_APP_MEM_POOL_SIZE) != TX_SUCCESS)
  {
    while(1)
    {
      //
    }
  }

  return true;
}

TX_BYTE_POOL *azureGetPool(void)
{
  return &tx_app_byte_pool;
}
