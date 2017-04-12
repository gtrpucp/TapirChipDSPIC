/* 
 * File:   delayms.h
 * Author: HENDRIX
 *
 * Created on 22 de marzo de 2016, 05:52 PM
 */

#ifndef DELAYMS_H
#define	DELAYMS_H

#include <xc.h>
#include <stdint.h>

#define FOSC 119762500  //Hz ~ 120MHz
#define FCY  FOSC/2     //Hz ~ 60MIPS

#include <libpic30.h>

void DelayMs(uint16_t tms);
void DelayUs(uint16_t tms);

#endif	/* DELAYMS_H */

