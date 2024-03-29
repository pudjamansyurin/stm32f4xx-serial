# STM32F4xx serial module using UART interface

## Module description
- Receive using circular DMA mode (non-blocking)
- Transmit using polling mode (blocking)
- Optional stdout locking mechanism

#### **`main.c`**
```c
#include "stm32f4xx-serial/serial.h"

/* External variables */
extern UART_HandleTypeDef huart1;

/* Private variables */
static uint8_t u8_buf[256];

/* Private function definitions */
static void stdin_reader(uint8_t *u8p_buf, uint16_t u16_size)
{
    /* do something with received data */
}

/* Entry Point */
int main(void)
{
    /* Initialize peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
  
    /* Initialize serial layer */  
    serial_init(&huart1, NULL);
    serial_start(stdin_reader, u8_buf, sizeof(u8_buf));

    /* simulate in-direct stdout */
    printf("Hello World\n");

    /* Super loop */
    while(1) {
        /* do other concurrent task */

        HAL_Delay(1);
    }
}
```


#### **`stm32f4xx_it.c`**
```c
/* USER CODE BEGIN Includes */
#include "stm32f4xx-serial/serial.h"
/* USER CODE END Includes */

/**
  * @brief This function handles DMA1 stream5 global interrupt.
  */
void DMA1_Stream5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
    /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */
    serial_irq_dma();
    /* USER CODE END DMA1_Stream5_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
    /* USER CODE BEGIN USART1_IRQn 1 */
    serial_irq_uart();
    /* USER CODE END USART1_IRQn 1 */
}

```