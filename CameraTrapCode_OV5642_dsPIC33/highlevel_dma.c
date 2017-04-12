
#include "highlevel_dma.h"

extern uint8_t buff_CamCapture[];
extern uint8_t buff_DummyRx[];

void DMA0_SetConfig(const BYTE * buff) {
    
    //*************************************************************************    
    // Setup DMA Channel 0 for:    
    //*************************************************************************    
    DMA0CON = 0x6001;
    /*
    DMA0CONbits.CHEN = 0;//channel disabled
    DMA0CONbits.SIZE = 1;//byte 1 - word 0 
    DMA0CONbits.DIR = 1;//register to peripheral
    DMA0CONbits.HALF = 0;//full transfer interrupt
    DMA0CONbits.NULLW = 0;//normal data
    //DMA0CONbits.AMODE = 0b01;//register indirect without post-increment
    DMA0CONbits.AMODE = 0b00;//register indirect with post-increment
    DMA0CONbits.MODE = 0b01;//one shot, no pingpong
     */
    
    DMA0STAL = (unsigned int)buff;
    DMA0STAH = 0x0000;    

    DMA0PAD = (volatile unsigned int)&SPI1BUF;
    
    DMA0CNT = 512 - 1;//byte transfers
    //DMA0CNT = 256 - 1;//byte transfers

    DMA0REQ = 0x000A;//select SPI1 as DMA request source
        
    IFS0bits.DMA0IF = 0;//clear DMA interrupt flag     
    IEC0bits.DMA0IE = 1;//enable DMA interrupt isr
    
}

void DMA1_SetConfig(void) {

    //*************************************************************************    
    // Setup DMA Channel 1 for:    
    //*************************************************************************    
    DMA1CON = 0x4000;
    /*
    DMA1CONbits.CHEN = 0;//channel disabled
    DMA1CONbits.SIZE = 0;//byte 1 - word 0 
    DMA1CONbits.DIR = 0;//peripheral to register
    DMA1CONbits.HALF = 0;//full transfer interrupt
    DMA1CONbits.NULLW = 0;//normal data
    DMA1CONbits.AMODE = 0b00;//register indirect without post-increment
    DMA1CONbits.MODE = 0b00;//continuous, no pingpong
     */
    
    DMA1STAL = (unsigned int)buff_DummyRx;
    DMA1STAH = 0x0000;    

    DMA1PAD = (volatile unsigned int)&SPI1BUF;
    
    DMA1CNT = 0;//byte transfers

    DMA1REQ = 0x000A;//select SPI1 as DMA request source
           
    IFS0bits.DMA1IF = 0;//clear DMA interrupt flag 
    IEC0bits.DMA1IE = 1;//enable DMA interrupt isr
                   
}

void DMA_ForceTransfer(const BYTE * buff) {

    //SPI1STATbits.SPIEN = 0; // Enable SPI module        
    //SPI1CON1bits.MODE16 = 1; // Communication is word-wide (8 bits)    
    //SPI1STATbits.SPIEN = 1; // Enable SPI module            
    
    //DMA0_SetConfig(buff);
    //DMA1_SetConfig();
    
    DMA0STAL = (unsigned int)buff;    
        
    IFS0bits.DMA1IF = 0;//clear DMA interrupt flag 
    DMA1CONbits.CHEN = 1;//enable DMA channel      

    IFS0bits.DMA0IF = 0;//clear DMA interrupt flag 
    DMA0CONbits.CHEN = 1;//enable DMA channel    
    DMA0REQbits.FORCE = 1;//force first transfer
        
}


void DMA_SetDisable(void) {

    DMA0CONbits.CHEN = 0;//disable DMA channel
    IFS0bits.DMA0IF = 0;//clear DMA interrupt flag 

    DMA1CONbits.CHEN = 0;//disable DMA channel
    IFS0bits.DMA1IF = 0;//clear DMA interrupt flag 

    //SPI1STATbits.SPIEN = 0; // Enable SPI module        
    //SPI1CON1bits.MODE16 = 0; // Communication is word-wide (8 bits)
    //SPI1STATbits.SPIEN = 1; // Enable SPI module        
    
}

