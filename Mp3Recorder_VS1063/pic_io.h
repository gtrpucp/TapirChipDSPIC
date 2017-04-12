/* 
 * File:   pines.h
 * Author: HENDRIX
 *
 * Created on 23 de marzo de 2016, 08:38 PM
 */

#ifndef PIC_IO_H
#define	PIC_IO_H

#include <xc.h>

//#define PLACA_CAMARA_PUCP

////////////////////////////////////////////////////////////////////////////////
// PUERTO SERIAL
#define TRIS_TX_UART1       TRISBbits.TRISB7
#define TRIS_RX_UART1       TRISBbits.TRISB5

////////////////////////////////////////////////////////////////////////////////

// button
#define TRIS_TEST_BUTTON    TRISBbits.TRISB3
#define PU_TEST_BUTTON      CNPUBbits.CNPUB3
#define TEST_BUTTON         PORTBbits.RB3

// buzzer
#define TRIS_BUZZER         TRISCbits.TRISC1
#define BUZZER              LATCbits.LATC1

// pin de test
#define TRIS_PROBE_TEST     TRISAbits.TRISA0
#define PROBE_TEST          LATAbits.LATA0

////////////////////////////////////////////////////////////////////////////////
// sdcard
#define TRIS_SDCARD_CS  TRISCbits.TRISC4
#define SDCARD_CS       LATCbits.LATC4

//#define TRIS_SDCARD_CD  TRISCbits.TRISC5
//#define SDCARD_CD       PORTCbits.RC5

#define SDCARD_MISO     PORTAbits.RA9

////////////////////////////////////////////////////////////////////////////////
// Pines para el VS1063
// MISO RB9/RP41
// MOSI RB10/RP42
// SCK  RB11/RP43
// CS   RB12/RPI44  OUTPUT
// DCS  RB13/RPI45  OUTPUT
// DREQ RB14/RP46   INPUT
// RST  RB15/RPI47  OUTPUT

// Control Chip Select Pin (for accessing SPI Control/Status registers)
#define TRIS_VS_XCS  TRISBbits.TRISB12
#define VS_XCS       LATBbits.LATB12

// Data Chip Select / BSYNC
#define TRIS_VS_XDCS TRISBbits.TRISB13
#define VS_XDCS      LATBbits.LATB13

// Data Request Pin: Player asks for more data
#define TRIS_VS_DREQ    TRISBbits.TRISB14
#define VS_DREQ         PORTBbits.RB14

// XRESET pin 
#define TRIS_VS_XRESET TRISBbits.TRISB15
#define VS_XRESET      LATBbits.LATB15

////////////////////////////////////////////////////////////////////////////////

void io_config();

#endif	/* PIC_IO_H */

