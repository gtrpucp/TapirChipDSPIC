/* 
 * File:   pines.h
 * Author: HENDRIX
 *
 * Created on 23 de marzo de 2016, 08:38 PM
 */

#ifndef PINES_H
#define	PINES_H

#include <xc.h>


#define TRIS_TX_UART1       TRISBbits.TRISB3
#define TRIS_RX_UART1       TRISBbits.TRISB5

#define TRIS_TEST_BUTTON    TRISAbits.TRISA0
#define PU_TEST_BUTTON      CNPUAbits.CNPUA0
#define TRIGGER_BUTTON         PORTAbits.RA0

#define TRIS_TEST_LED       TRISCbits.TRISC1
#define TEST_LED            LATCbits.LATC1

#define TRIS_PIR    TRISBbits.TRISB7
#define LAT_PIR     LATBbits.LATB7
#define PIR         PORTBbits.RB7
#define TRIS_FLASH  TRISAbits.TRISA3
#define FLASH       LATAbits.LATA3
#define TRIS_ADC    TRISAbits.TRISA1
#define ADC         LATAbits.LATA1


#define TRIS_SDCARD_CS  TRISCbits.TRISC4
#define SDCARD_CS       LATCbits.LATC4
#define SDCARD_MISO     PORTAbits.RA9

//Digital I/O used to control de OV5642
#define TRIS_CAM_XCLK   TRISBbits.TRISB6
#define TRIS_CAM_SDA    TRISAbits.TRISA8
#define TRIS_CAM_SCL    TRISBbits.TRISB4
#define TRIS_CAM_PWDN   TRISCbits.TRISC0
#define CAM_PWDN        LATCbits.LATC0
#define TRIS_CAM_HREF   TRISAbits.TRISA7
#define CAM_HREF        PORTAbits.RA7
#define TRIS_CAM_VSYNC  TRISCbits.TRISC9
#define CAM_VSYNC       PORTCbits.RC9

//Digital I/O to control de FIFO memory
#define TRIS_MEM_DO0    TRISBbits.TRISB8
#define TRIS_MEM_DO1    TRISBbits.TRISB9
#define TRIS_MEM_DO2    TRISBbits.TRISB10
#define TRIS_MEM_DO3    TRISBbits.TRISB11
#define TRIS_MEM_DO4    TRISBbits.TRISB12
#define TRIS_MEM_DO5    TRISBbits.TRISB13 
#define TRIS_MEM_DO6    TRISBbits.TRISB14
#define TRIS_MEM_DO7    TRISBbits.TRISB15

#define MEM_DO0         PORTBbits.RB8
#define MEM_DO1         PORTBbits.RB9
#define MEM_DO2         PORTBbits.RB10
#define MEM_DO3         PORTBbits.RB11
#define MEM_DO4         PORTBbits.RB12
#define MEM_DO5         PORTBbits.RB13
#define MEM_DO6         PORTBbits.RB14
#define MEM_DO7         PORTBbits.RB15

#define TRIS_MEM_RESET  TRISAbits.TRISA2
#define MEM_RESET       LATAbits.LATA2

//Writing control
#define TRIS_MEM_WEN    TRISAbits.TRISA10
#define MEM_WEN         LATAbits.LATA10

//Reading control
#define TRIS_MEM_RCLK   TRISCbits.TRISC6
#define MEM_RCLK        LATCbits.LATC6
//#define MEM_RCLK_READ   PORTCbits.RC6

#define TRIS_MEM_RRST   TRISCbits.TRISC7
#define MEM_RRST        LATCbits.LATC7

#define TRIS_MEM_OE     TRISCbits.TRISC8
#define MEM_OE          LATCbits.LATC8

//#define TRIS_MEM_WRST   TRISCbits.TRISC2
//#define MEM_WRST        LATCbits.LATC2

#define TRIS_MEM_ORDY   TRISBbits.TRISB2
#define MEM_ORDY        PORTBbits.RB2
#define MEM_ORDY_PU     CNPUBbits.CNPUB2

#define TX_IN_PROGRESS  PORTCbits.RC2
#define TRIS_TAKING_PHOTO   TRISCbits.TRISC5
// PIN conectado al ESP8266
#define TAKING_PHOTO    LATCbits.LATC5

#endif	/* PINES_H */