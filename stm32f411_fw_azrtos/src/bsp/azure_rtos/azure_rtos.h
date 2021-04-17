/*
 * azure_rtos.h
 *
 *  Created on: 2021. 4. 17.
 *      Author: baram
 */

#ifndef SRC_BSP_AZURE_RTOS_AZURE_RTOS_H_
#define SRC_BSP_AZURE_RTOS_AZURE_RTOS_H_

#include "def.h"
#include "tx_api.h"
#include "tx_thread.h"

#define TX_APP_MEM_POOL_SIZE                     16 * 1024


bool azureInit(void);
TX_BYTE_POOL *azureGetPool(void);


#endif /* SRC_BSP_AZURE_RTOS_AZURE_RTOS_H_ */
