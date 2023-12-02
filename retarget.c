/*
 * retarget.c
 *
 *  Created on: May 5, 2022
 *      Author: pujak
 */
#include "serial.h"
#include <stdio.h>

/* Public function definitions
 * ---------------------------------------------------------------------------*/
void stdout_init(void)
{
    /* use line buffering */
    setvbuf(stdout, NULL, _IOLBF, 0);
}

/* Replace weak syscalls routines */
int __io_putchar(int ch)
{
    serial_write((char*) &ch, 1);
    return (ch);
}

int _write(int file, char *ptr, int len)
{
    serial_write(ptr, len);
    return len;
}
