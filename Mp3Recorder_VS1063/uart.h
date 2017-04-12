/* 
 * File:   uart.h
 * Author: admin
 *
 * Created on 15 de noviembre de 2016, 07:45 PM
 */

#ifndef UART_H
#define	UART_H

#include <xc.h>
#include <stdint.h>


//#define Uart_BufferFull()  U1STAbits.URXDA
#define Uart_rxAvailable()  IFS0bits.U1RXIF

void InicializaUART1(void);
char Uart_getc();
void Uart_putc(char c);

#endif	/* UART_H */

