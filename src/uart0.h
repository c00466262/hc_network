/*
 * File:		uart.h
 * Purpose:     Provide common ColdFire UART routines for polled serial IO
 *
 * Notes:
 */

#ifndef __UART0_H__
#define __UART0_H__

/********************************************************************/

void uart0_init(int baud);
void uart0_put_char (char ch);
void uart0_put_str(char *str);
BOOL uart0_get_char (char* ch);
void uart0_interrupt(void);
void uart0_interrupt_err(void);

/********************************************************************/

#endif /* __UART_H__ */
