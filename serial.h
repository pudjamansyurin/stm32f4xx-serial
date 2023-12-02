/*
 * serial.h
 *
 *  Created on: Apr 19, 2022
 *      Author: pudja
 */

#ifndef STM32F4XX_SERIAL_H_
#define STM32F4XX_SERIAL_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* Exported macros
 * ---------------------------------------------------------------------------*/
typedef void (*stdout_locker_t)(uint8_t u8_lock);
typedef void (*stdin_reader_t)(uint8_t *p_buf, uint16_t u16_size);

/* Exported enum/struct types
 * ---------------------------------------------------------------------------*/
typedef struct
{
    uint8_t *p_buf;
    uint16_t u16_size;
    uint16_t u16_pos;
} circular_buffer_t;

typedef struct
{
    UART_HandleTypeDef *p_uart;
    stdout_locker_t locker;
    stdin_reader_t reader;
    circular_buffer_t rx;
} serial_t;

/* Public function prototypes
 * ---------------------------------------------------------------------------*/
void serial_init(UART_HandleTypeDef *p_uart, stdout_locker_t locker);
HAL_StatusTypeDef serial_start(stdin_reader_t reader, uint8_t *p_buf,
    uint16_t u16_size);
HAL_StatusTypeDef serial_stop(void);
void serial_write(void *p_buf, uint16_t u16_size);
void serial_irq_dma(void);
void serial_irq_uart(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32F4XX_SERIAL_H_ */
