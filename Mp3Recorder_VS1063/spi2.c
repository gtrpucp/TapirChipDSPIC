
#include "spi2.h"

void spi2_init(void) {
   
    SPI2STAT = 0b0000000000000000; //enable SPI pins
    SPI2CON2 = 0b0000000000000000; //framed SPI support disabled
    
    RPINR22bits.SDI2R = 41;     //RP41 -> MISO
    RPINR22bits.SCK2R = 43;     //RP43 -> SCK IN
    
    RPOR4bits.RP43R = 0b001001; //RP43 -> SCK
    RPOR4bits.RP42R = 0b001000; //RP42 -> MOSI
        
    //master mode
    SPI2STATbits.SPIEN = 0;
    SPI2CON1bits.DISSCK = 0; // Internal serial clock is enabled
    SPI2CON1bits.DISSDO = 0; // SDOx pin is controlled by the module
    SPI2CON1bits.MODE16 = 0; // Communication is word-wide (8 bits)
    SPI2CON1bits.MSTEN = 1; // Master mode enabled
    SPI2CON1bits.SMP = 0;   // Input data is sampled at the middle of data output time
    SPI2CON1bits.CKE = 1;   // Serial output data changes on transition from active clock state to idle clock state
    SPI2CON1bits.CKP = 0;   // Idle state for clock is a low level;

    SPI2_BUS_SET_TO_LOW_SPEED;
//    SPI2CON1bits.PPRE = 0b10;   // Primary prescaler 4:1
//    SPI2CON1bits.SPRE = 0b000;  // Secondary prescaler 8:1
    SPI2STATbits.SPIEN = 1; // Enable SPI module    
}


BYTE xchg_spi2(BYTE dat) {
    IFS2bits.SPI2IF = 0; //check
    SPI2BUF = dat;
    //while (!_SPIRBF) ;
    while (!IFS2bits.SPI2IF);
    return (BYTE) SPI2BUF;
}


void xmit_spi2_multi(
        const BYTE* buff, /* Data to be sent */
        unsigned int cnt /* Number of bytes to send */
        ) {
    do { 
        IFS2bits.SPI2IF = 0; //check
        SPI2BUF = *buff++;
        while (!IFS2bits.SPI2IF);
        SPI2BUF;
        IFS2bits.SPI2IF = 0; //check
        SPI2BUF = *buff++;
        while (!IFS2bits.SPI2IF);
        SPI2BUF;
    } while (cnt -= 2);
}


void rcvr_spi2_multi(
        BYTE* buff, /* Buffer to store received data */
        unsigned int cnt /* Number of bytes to receive */
        ) {
    do {
        IFS2bits.SPI2IF = 0; //check
        SPI2BUF = 0xFF;
        while (!IFS2bits.SPI2IF);
        *buff++ = SPI2BUF;
        IFS2bits.SPI2IF = 0; //check
        SPI2BUF = 0xFF;
        while (!IFS2bits.SPI2IF);
        *buff++ = SPI2BUF;
    } while (cnt -= 2);
}

