/*
 * serial.h
 *
 *  Created on: Apr 19, 2022
 *      Author: pudja
 */

#ifndef STM32F4XX_SERIAL_H_
#define STM32F4XX_SERIAL_H_

#include "stm32f4xx_hal.h"

/* Exported types
 * --------------------------------------------*/
typedef void (*serial_reader_t)(uint8_t *buffer, uint16_t size);

typedef struct
{
	UART_HandleTypeDef *huart;
	serial_reader_t reader;
	struct {
		uint8_t* buffer;
		uint16_t size;
		uint16_t pos;
	} rx;
} serial_t;

/* Public function prototypes
 * --------------------------------------------*/
void serial_init(UART_HandleTypeDef *uart);
HAL_StatusTypeDef serial_start(serial_reader_t reader, uint8_t *buffer,
	uint16_t size);
HAL_StatusTypeDef serial_stop(void);
void serial_write(void *p_buffer, uint16_t size);
void serial_irq_dma(void);
void serial_irq_uart(void);

#endif /* STM32F4XX_SERIAL_H_ */
