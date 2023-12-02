/*
 * serial.c
 *
 *  Created on: Apr 19, 2022
 *      Author: pudja
 */
#include "serial.h"
#include "retarget.h"

/* Private variables
 * ---------------------------------------------------------------------------*/
static serial_t hserial;

/* Private function prototypes
 * ---------------------------------------------------------------------------*/
static void stdout_lock(uint8_t u8_lock);
static HAL_StatusTypeDef stdin_listen(void);
static void check_buffer(uint16_t u16_new);
static void fill_buffer(uint16_t u16_pos, uint16_t u16_size);

/* Public function implementations
 * ---------------------------------------------------------------------------*/
void serial_init(UART_HandleTypeDef *p_uart, stdout_locker_t locker)
{
    /* set properties */
    hserial.p_uart = p_uart;
    hserial.locker = locker;

    /* Enable interrupts */
    __HAL_UART_ENABLE_IT(p_uart, UART_IT_IDLE);
    __HAL_DMA_ENABLE_IT(p_uart->hdmarx, DMA_IT_TC);
    __HAL_DMA_ENABLE_IT(p_uart->hdmarx, DMA_IT_HT);

    /* initialize stdout */
    stdout_init();
}

HAL_StatusTypeDef serial_start(stdin_reader_t reader, uint8_t *p_buf,
    uint16_t u16_size)
{
    HAL_StatusTypeDef status;
    circular_buffer_t *p_rx;

    p_rx = &(hserial.rx);

    /* set properties */
    p_rx->p_buf = p_buf;
    p_rx->u16_pos = 0;
    p_rx->u16_size = u16_size;
    hserial.reader = reader;

    /* Start receiving UART in DMA mode */
    status = stdin_listen();
    return (status);
}

HAL_StatusTypeDef serial_stop(void)
{
    HAL_StatusTypeDef status;

    status = HAL_UART_DMAStop(hserial.p_uart);
    return (status);
}

void serial_write(void *p_buf, uint16_t u16_size)
{
    uint8_t *u8p_buf;

    u8p_buf = (uint8_t*) p_buf;

    stdout_lock(1);
    HAL_UART_Transmit(hserial.p_uart, u8p_buf, u16_size, HAL_MAX_DELAY);
    stdout_lock(0);
}

void serial_irq_dma(void)
{
    DMA_HandleTypeDef *p_dmarx;

    p_dmarx = hserial.p_uart->hdmarx;

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

    p_dmarx = hserial.p_uart->hdmarx;

    if (__HAL_UART_GET_FLAG(hserial.p_uart, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(hserial.p_uart);
        check_buffer(__HAL_DMA_GET_COUNTER(p_dmarx));
    }
}

/* Private function implementations
 * ---------------------------------------------------------------------------*/
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
    circular_buffer_t *p_rx;

    p_rx = &(hserial.rx);

    status = HAL_UART_Receive_DMA(hserial.p_uart, p_rx->p_buf, p_rx->u16_size);
    return (status);
}

static void check_buffer(uint16_t u16_new)
{
    circular_buffer_t *p_rx;
    uint16_t u16_size;
    uint16_t u16_pos;

    p_rx = &(hserial.rx);

    /* Calculate current position in buffer */
    u16_pos = p_rx->u16_size - u16_new;

    /* Check change in received data */
    if (u16_pos != p_rx->u16_pos)
    {
        if (u16_pos > p_rx->u16_pos)
        {
            /* Current position is over previous one */
            /* We are in "linear" mode */
            /* Process data directly by subtracting "pointers" */
            u16_size = u16_pos - p_rx->u16_pos;
            fill_buffer(p_rx->u16_pos, u16_size);
        }
        else
        {
            /* We are in "overflow" mode */
            /* First process data to the end of buffer */
            u16_size = p_rx->u16_size - p_rx->u16_pos;
            fill_buffer(p_rx->u16_pos, u16_size);

            /* Check and continue with beginning of buffer */
            if (u16_pos > 0)
            {
                u16_size = u16_pos;
                fill_buffer(0, u16_size);
            }
        }
    }

    /* Check and manually update if we reached end of buffer */
    p_rx->u16_pos = (u16_pos == p_rx->u16_size) ? 0 : u16_pos;
}

static void fill_buffer(uint16_t u16_pos, uint16_t u16_size)
{
    if (NULL == hserial.reader)
    {
        return;
    }

    hserial.reader(&(hserial.rx.p_buf[u16_pos]), u16_size);
}

