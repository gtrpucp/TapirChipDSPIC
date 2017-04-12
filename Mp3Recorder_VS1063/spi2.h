/* 
 * File:   spi2.h
 * Author: admin
 *
 * Created on 10 de noviembre de 2016, 07:15 PM
 */

#ifndef SPI2_H
#define	SPI2_H

#include <xc.h>
#include "GlobalDataType.h"
#include "pic_io.h" 

// Primary prescaler 4:1, secondary prescaler 8:1
#define	SPI2_BUS_SET_TO_LOW_SPEED       SPI2STATbits.SPIEN=0;SPI2CON1bits.PPRE=0b10;SPI2CON1bits.SPRE=0b000;SPI2STATbits.SPIEN=1;
                                        
// Primary prescaler 4:1, secondary prescaler 5:1
#define	SPI2_BUS_SET_TO_FULL_SPEED_SD	SPI2STATbits.SPIEN=0;SPI2CON1bits.PPRE=0b10;SPI2CON1bits.SPRE=0b011;SPI2STATbits.SPIEN=1;


void spi2_init(void);
BYTE xchg_spi2(BYTE dat);
void xmit_spi2_multi(const BYTE* buff, unsigned int cnt);
void rcvr_spi2_multi(BYTE* buff, unsigned int cnt);

#endif	/* SPI2_H */

