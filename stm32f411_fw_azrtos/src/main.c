/*
 * main.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "main.h"


TX_THREAD main_thread;



int main(void)
{
  hwInit();

  tx_kernel_enter();

  return 0;
}

void mainThread(ULONG thread_input)
{
  (void) thread_input;

  apInit();
  apMain();
}


VOID tx_application_define(VOID *first_unused_memory)
{
  azureInit();

  CHAR *pointer;


  if (tx_byte_allocate(azureGetPool(), (VOID **) &pointer,
                       _HW_DEF_RTOS_THREAD_MEM_MAIN, TX_NO_WAIT) != TX_SUCCESS)
  {
    return;
  }

  if (tx_thread_create(&main_thread, "mainThread", mainThread, 0,
                       pointer, _HW_DEF_RTOS_THREAD_MEM_MAIN,
                       _HW_DEF_RTOS_THREAD_PRI_MAIN, _HW_DEF_RTOS_THREAD_PRI_MAIN,
                       TX_1_ULONG, TX_AUTO_START) != TX_SUCCESS)
  {
    return;
  }
}
