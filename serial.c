/*
 * serial.c
 *
 *  Created on: Apr 19, 2022
 *      Author: pudja
 */
#include "serial.h"
#include "retarget.h"

/* Private variables */
static serial_t hserial;

/* Private function prototypes
 * --------------------------------------------*/
static void stdout_lock(uint8_t u8_lock);
static HAL_StatusTypeDef stdin_listen(void);
static void check_buffer(uint16_t new);
static void fill_buffer(uint16_t pos, uint16_t size);

/* Public function implementations
 * --------------------------------------------*/
void serial_init(UART_HandleTypeDef *p_uart, stdout_locker_t locker)
{
	/* set properties */
	hserial.huart = p_uart;
	hserial.locker = locker;

	/* Enable interrupts */
	__HAL_UART_ENABLE_IT(p_uart, UART_IT_IDLE);
	__HAL_DMA_ENABLE_IT(p_uart->hdmarx, DMA_IT_TC);
	__HAL_DMA_ENABLE_IT(p_uart->hdmarx, DMA_IT_HT);

	/* initialize stdout */
	stdout_init();
}

HAL_StatusTypeDef serial_start(stdin_reader_t rdr, uint8_t *buffer,
	uint16_t size)
{
	/* set properties */
	hserial.reader = rdr;
	hserial.rx.buffer = buffer;
	hserial.rx.size = size;
	hserial.rx.pos = 0;

	/* Start receiving UART in DMA mode */
	return (stdin_listen());
}

HAL_StatusTypeDef serial_stop(void)
{
	HAL_StatusTypeDef status;

	status = HAL_UART_DMAStop(hserial.huart);
	return (status);
}

void serial_write(void *p_buffer, uint16_t size)
{
	stdout_lock(1);
	HAL_UART_Transmit(hserial.huart, (uint8_t*)p_buffer, size, HAL_MAX_DELAY);
	stdout_lock(0);
}

void serial_irq_dma(void)
{
	DMA_HandleTypeDef *p_dmarx;

	p_dmarx = hserial.huart->hdmarx;

	/* Handle HT interrupt */
	if (__HAL_DMA_GET_IT_SOURCE(p_dmarx, DMA_IT_HT))
	{
		__HAL_DMA_CLEAR_FLAG(p_dmarx, __HAL_DMA_GET_HT_FLAG_INDEX(p_dmarx));
		check_buffer(__HAL_DMA_GET_COUNTER(p_dmarx));
	}

	/* Handle TC interrupt */
	else if (__HAL_DMA_GET_IT_SOURCE(p_dmarx, DMA_IT_TC))
	{
		__HAL_DMA_CLEAR_FLAG(p_dmarx, __HAL_DMA_GET_TC_FLAG_INDEX(p_dmarx));
		check_buffer(__HAL_DMA_GET_COUNTER(p_dmarx));
	}

	/* Handle ERROR interrupt */
	else
	{
		__HAL_DMA_CLEAR_FLAG(p_dmarx, __HAL_DMA_GET_TE_FLAG_INDEX(p_dmarx));
		__HAL_DMA_CLEAR_FLAG(p_dmarx, __HAL_DMA_GET_FE_FLAG_INDEX(p_dmarx));
		__HAL_DMA_CLEAR_FLAG(p_dmarx, __HAL_DMA_GET_DME_FLAG_INDEX(p_dmarx));

		stdin_listen();
	}
}

void serial_irq_uart(void)
{
	DMA_HandleTypeDef *p_dmarx;

	p_dmarx = hserial.huart->hdmarx;

	if (__HAL_UART_GET_FLAG(hserial.huart, UART_FLAG_IDLE))
	{
		__HAL_UART_CLEAR_IDLEFLAG(hserial.huart);
		check_buffer(__HAL_DMA_GET_COUNTER(p_dmarx));
	}
}

/* Private function implementations
 * --------------------------------------------*/
static void stdout_lock(uint8_t u8_lock)
{
	if (NULL != hserial.locker)
	{
		hserial.locker(u8_lock);
	}
}

static HAL_StatusTypeDef stdin_listen(void)
{
	HAL_StatusTypeDef status;

	status = HAL_UART_Receive_DMA(hserial.huart, hserial.rx.buffer,
		hserial.rx.size);

	return (status);
}

static void check_buffer(uint16_t new)
{
	uint16_t pos;

	/* Calculate current position in buffer */
	pos = hserial.rx.size - new;

	/* Check change in received data */
	if (pos != hserial.rx.pos)
	{
		if (pos > hserial.rx.pos)
		{
			/* Current position is over previous one */
			/* We are in "linear" mode */
			/* Process data directly by subtracting "pointers" */
			fill_buffer(hserial.rx.pos, pos - hserial.rx.pos);
		}
		else
		{
			/* We are in "overflow" mode */
			/* First process data to the end of buffer */
			fill_buffer(hserial.rx.pos, hserial.rx.size - hserial.rx.pos);

			/* Check and continue with beginning of buffer */
			if (pos > 0)
			{
				fill_buffer(0, pos);
			}
		}
	}

	/* Check and manually update if we reached end of buffer */
	hserial.rx.pos = (pos == hserial.rx.size) ? 0 : pos;
}

static void fill_buffer(uint16_t pos, uint16_t size)
{
	if (NULL != hserial.reader)
	{
		hserial.reader(&(hserial.rx.buffer[pos]), size);
	}
}

