/* 
 * File:   delay.h
 * Author: admin
 *
 * Created on 26 de octubre de 2016, 09:59 PM
 */

#ifndef DELAY_H
#define	DELAY_H

#include <xc.h>
#include <stdint.h>


#define FOSC 119762500  //Hz ~ 120MHz
#define FCY  FOSC/2     //Hz ~ 60MIPS

#include <libpic30.h>

//void DelayMs(uint16_t tms);
//void DelayUs(uint16_t tms);

#define delay_cycles(x) _delay(x)

void delay_ms(uint16_t t);
void delay_us(uint16_t t);


#define DelayMs(x)  delay_ms(x)
#define DelayUs(x)  delay_us(x)


#endif	/* DELAY_H */

