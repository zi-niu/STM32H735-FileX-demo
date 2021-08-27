/* USER CODE BEGIN Header */
/**
******************************************************************************
* @file    app_filex.c
* @author  MCD Application Team
* @brief   FileX applicative file
******************************************************************************
* @attention
*
* <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
* All rights reserved.</center></h2>
*
* This software component is licensed by ST under Ultimate Liberty license
* SLA0044, the "License"; You may not use this file except in compliance with
* the License. You may obtain a copy of the License at:
*                             www.st.com/SLA0044
*
******************************************************************************
*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_filex.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32h735g_discovery.h"
#include "micro_test.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define DEFAULT_STACK_SIZE               (2 * 1024)
#define DEFAULT_QUEUE_LENGTH             16

/* fx_sd_thread priority */
#define DEFAULT_THREAD_PRIO              10
/* fx_sd_thread preemption priority */
#define DEFAULT_PREEMPTION_THRESHOLD     DEFAULT_THREAD_PRIO

/* Message content*/
typedef enum {
CARD_STATUS_CHANGED             = 99,
CARD_STATUS_DISCONNECTED        = 88,
CARD_STATUS_CONNECTED           = 77
} SD_ConnectionStateTypeDef;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* Buffer for FileX FX_MEDIA sector cache. this should be 32-Bytes
aligned to avoid cache maintenance issues */
ALIGN_32BYTES (uint32_t media_memory[DEFAULT_SECTOR_SIZE / sizeof(uint32_t)]);

/* Define FileX global data structures.  */
FX_MEDIA        sdio_disk;
FX_FILE         fx_file;
/* Define ThreadX global data structures.  */
TX_THREAD       fx_app_thread;
TX_QUEUE        tx_msg_queue;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

void fx_thread_entry(ULONG thread_input);

void Error_Handler(void);

/* USER CODE END PFP */

/**
  * @brief  Application FileX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_FileX_Init(VOID *memory_ptr)
{
  UINT ret = FX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN App_FileX_MEM_POOL */
  (void)byte_pool;
  /* USER CODE END App_FileX_MEM_POOL */

  /* USER CODE BEGIN App_FileX_Init */

  VOID *pointer;

  /* Allocate memory for the main thread's stack */
  ret = tx_byte_allocate(byte_pool, &pointer, DEFAULT_STACK_SIZE, TX_NO_WAIT);

  if (ret != FX_SUCCESS)
  {
    /* Failed at allocating memory */
    Error_Handler();
  }

  /* Create the main thread.  */
  tx_thread_create(&fx_app_thread, "FileX App Thread", fx_thread_entry, 0, pointer, DEFAULT_STACK_SIZE,
                   DEFAULT_THREAD_PRIO, DEFAULT_PREEMPTION_THRESHOLD, TX_NO_TIME_SLICE, TX_AUTO_START);

  /* Allocate memory for the message queue */
  ret = tx_byte_allocate(byte_pool, &pointer, DEFAULT_QUEUE_LENGTH * sizeof(ULONG), TX_NO_WAIT);

  if (ret != FX_SUCCESS)
  {
    /* Failed at allocating memory */
    Error_Handler();
  }

  /* Create the message queue */
  tx_queue_create(&tx_msg_queue, "sd_event_queue", 1, pointer, DEFAULT_QUEUE_LENGTH * sizeof(ULONG));

  /* Initialize FileX.  */
  fx_system_initialize();

  /* USER CODE END App_FileX_Init */

  return ret;
}

/* USER CODE BEGIN 1 */

void fx_thread_entry(ULONG thread_input)
{

  UINT status;
  ULONG r_msg;
  ULONG s_msg = CARD_STATUS_CHANGED;
  ULONG last_status = CARD_STATUS_DISCONNECTED;
  ULONG bytes_read;
  CHAR read_buffer[32];
  CHAR data[] = "This is FileX working on STM32";

  if(BSP_SD_IsDetected(SD_INSTANCE) == SD_PRESENT)
  {
    /* SD card is already inserted, place the info into the queue */
    tx_queue_send(&tx_msg_queue, &s_msg, TX_NO_WAIT);
  }
  else
  {
    /* Indicate that SD card is not inserted from start */
    BSP_LED_On(LED_RED);
  }

  printf("fx_thread_entry\r\n");
  /* Infinite Loop */
  for( ;; )
  {
    /* We wait here for a valid SD card insertion event, if it is not inserted already */
    while(1)
    {

      while(tx_queue_receive(&tx_msg_queue, &r_msg, TX_TIMER_TICKS_PER_SECOND / 2) != TX_SUCCESS)
      {
        /* Toggle GREEN LED to indicate idle state after a successful operation */
        if(last_status == CARD_STATUS_CONNECTED)
        {
          BSP_LED_Toggle(LED_GREEN);
        }
      }

      /* check if we received the correct event message */
      if(r_msg == CARD_STATUS_CHANGED)
      {
        /* reset the status */
        r_msg = 0;

        /* for debouncing purpose we wait a bit till it settles down */
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2);

        if(BSP_SD_IsDetected(SD_INSTANCE) == SD_PRESENT)
        {
          /* We have a valid SD insertion event, start processing.. */
          /* Update last known status */
          last_status = CARD_STATUS_CONNECTED;
          BSP_LED_Off(LED_RED);
          break;
        }
        else
        {
          /* Update last known status */
          last_status = CARD_STATUS_DISCONNECTED;
          BSP_LED_Off(LED_GREEN);
          BSP_LED_On(LED_RED);
        }
      }

    }

    /* Open the SD disk driver.  */
    status =  fx_media_open(&sdio_disk, "STM32_SDIO_DISK", fx_stm32_sd_driver, 0,(VOID *) media_memory, sizeof(media_memory));

    /* Check the media open status.  */
    if (status != FX_SUCCESS)
    {
      Error_Handler();
    }

    /* Create a file called STM32.TXT in the root directory.  */
    status =  fx_file_create(&sdio_disk, "STM32.TXT");

    /* Check the create status.  */
    if (status != FX_SUCCESS)
    {
      /* Check for an already created status. This is expected on the
      second pass of this loop!  */
      if (status != FX_ALREADY_CREATED)
      {
        /* Create error, call error handler.  */
        Error_Handler();
      }
    }

    /* Open the test file.  */
    MICRO_EXPECT_EQ(FX_SUCCESS,
    				fx_file_open(&sdio_disk, &fx_file, "STM32.TXT", FX_OPEN_FOR_WRITE) );

    /* Seek to the beginning of the test file.  */
    MICRO_EXPECT_EQ(FX_SUCCESS,
    				fx_file_seek(&fx_file, 0) );

    /* Write a string to the test file.  */
    MICRO_EXPECT_EQ(FX_SUCCESS,
    				fx_file_write(&fx_file, data, sizeof(data)) );

    /* Close the test file.  */
    MICRO_EXPECT_EQ(FX_SUCCESS,
    				fx_file_close(&fx_file) );

    MICRO_EXPECT_EQ(FX_SUCCESS,
    				fx_media_flush(&sdio_disk) );

    /* Open the test file.  */
    MICRO_EXPECT_EQ(FX_SUCCESS,
    		fx_file_open(&sdio_disk, &fx_file, "STM32.TXT", FX_OPEN_FOR_READ) );

    /* Seek to the beginning of the test file.  */
    MICRO_EXPECT_EQ(FX_SUCCESS,
    				fx_file_seek(&fx_file, 0) );

    /* Read the first 28 bytes of the test file.  */
    status =  fx_file_read(&fx_file, read_buffer, sizeof(data), &bytes_read);

    /* Check the file read status.  */
    if ((status != FX_SUCCESS) || (bytes_read != sizeof(data)))
    {
      /* Error reading file, call error handler.  */
      Error_Handler();
    }

    /* Close the test file.  */
    MICRO_EXPECT_EQ(FX_SUCCESS,
    				fx_file_close(&fx_file) );

    /* Close the media.  */
    MICRO_EXPECT_EQ(FX_SUCCESS,
    				fx_media_close(&sdio_disk) );
  }
}

void BSP_SD_DetectCallback(uint32_t Instance, uint32_t Status)
{
  ULONG s_msg = CARD_STATUS_CHANGED;

  if(Instance == SD_INSTANCE)
  {
    tx_queue_send(&tx_msg_queue, &s_msg, TX_NO_WAIT);
  }

  UNUSED(Status);
}



/* USER CODE END 1 */
