/*
 * bsp.h
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */

#ifndef SRC_BSP_BSP_H_
#define SRC_BSP_BSP_H_


#include "def.h"


#define _USE_LOG_PRINT    1

#if _USE_LOG_PRINT
#define logPrintf(fmt, ...)     printf(fmt, ##__VA_ARGS__)
#else
#define logPrintf(fmt, ...)
#endif


#include "stm32f4xx_hal.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include "azure_rtos.h"


void bspInit(void);

void delay(uint32_t ms);
uint32_t millis(void);

void Error_Handler(void);


#endif /* SRC_BSP_BSP_H_ */
