#include "uart.h"
#include "pic_io.h"

void InicializaUART1(void) {

    //Configuramos los pines RX y TX
    TRIS_TX_UART1 = 0; //TX
    TRIS_RX_UART1 = 1; //RX

    RPOR2bits.RP39R = 0b000001; //RP39 -> U1TX
    RPINR18bits.U1RXR = 37; //RP37 -> U1RX

    U1BRG = 129; //baudrate 115200
    U1MODE = 0x8008;
    U1STA = 0x0400;

    IFS0bits.U1RXIF = 0;
//    IEC0bits.U1RXIE = 1;
}

void Uart_putc(char c) {
    //transmit ONLY if TX buffer is empty
    while (U1STAbits.UTXBF == 1);
    U1TXREG = c;
}

char Uart_getc() {

    if (U1STAbits.FERR) {
        return 0;
    }
    if (U1STAbits.OERR) {
        U1STAbits.OERR = 0;
        return 0;
    }

    IFS0bits.U1RXIF = 0;

    //Trama esperada:
    return U1RXREG;
}
