#include "highlevel_i2c2.h"

uint8_t custom_IdleI2C2(void) {

    uint32_t timeReloj = 60000;

    //while (((SSP1CON2 & 0x1F) | (SSP1STATbits.R_W)) && (timeReloj--)); //60 milisegundos
    while (((I2C2CON & 0x001F) | (I2C2STATbits.TRSTAT)) && (timeReloj--)); //60 milisegundos

    if (timeReloj == 0xFFFFFFFF) {
        return 1;
    }
    return 0;

}

uint8_t custom_AckI2C2(void) {

    uint32_t timeReloj = 60000;

    //SSP1CON2bits.ACKDT = 0;           // set acknowledge bit state for ACK
    //SSP1CON2bits.ACKEN = 1;           // initiate bus acknowledge sequence
    I2C2CONbits.ACKDT = 0;           // set acknowledge bit state for ACK
    I2C2CONbits.ACKEN = 1;           // initiate bus acknowledge sequence

    //while ((SSP1CON2bits.ACKEN) && (timeReloj--));
    while ((I2C2CONbits.ACKEN) && (timeReloj--));

    if (timeReloj == 0xFFFFFFFF) {
        return 1;
    }
    return 0;

}

uint8_t custom_NoAckI2C2(void) {

    uint32_t timeReloj = 60000;

    //SSP1CON2bits.ACKDT = 1;          // set acknowledge bit for not ACK
    //SSP1CON2bits.ACKEN = 1;          // initiate bus acknowledge sequence
    I2C2CONbits.ACKDT = 1;           // set acknowledge bit state for ACK
    I2C2CONbits.ACKEN = 1;           // initiate bus acknowledge sequence

    //while ((SSP1CON2bits.ACKEN) && (timeReloj--));
    while ((I2C2CONbits.ACKEN) && (timeReloj--));
    
    //SSP1CON2bits.ACKDT = 0;			//Set for NotACk
    I2C2CONbits.ACKDT = 0;			//Set for NotACk

    if (timeReloj == 0xFFFFFFFF) {
        return 1;
    }
    return 0;

}


uint8_t custom_StartI2C2(void) {

    uint32_t timeReloj = 60000;

    //SSP1CON2bits.SEN = 1;
    I2C2CONbits.SEN = 1;
    
    //while ((SSP1CON2bits.SEN) && (timeReloj--));
    while ((I2C2CONbits.SEN) && (timeReloj--));

    if (timeReloj == 0xFFFFFFFF) {
        return 1;
    }
    return 0;

}

uint8_t custom_RestartI2C2(void) {

    uint32_t timeReloj = 60000;

    //SSP1CON2bits.RSEN = 1;
    I2C2CONbits.RSEN = 1;
    
    //while ((SSP1CON2bits.RSEN) && (timeReloj--));
    while ((I2C2CONbits.RSEN) && (timeReloj--));

    if (timeReloj == 0xFFFFFFFF) {
        return 1;
    }
    return 0;

}

uint8_t custom_WriteI2C2(uint8_t data) {

    uint32_t timeReloj = 60000;

    //SSP1BUF = data;
    I2C2TRN = data;
    
    //if (SSP1CON1bits.WCOL) { // test if write collision occurred
    if (I2C2STATbits.IWCOL) { // test if write collision occurred
        return 1;           // if WCOL bit is set return negative #
        
    } else {        
        //while ((SSP1STATbits.BF) && (timeReloj--));
        while ((I2C2STATbits.TBF) && (timeReloj--));
        
        if (timeReloj == 0xFFFFFFFF) {
            return 1;
        }
        timeReloj = 60000;
        //while (((SSP1CON2 & 0x1F) | (SSP1STATbits.R_W)) && (timeReloj--)); //60 milisegundos
        while (((I2C2CON & 0x001F) | (I2C2STATbits.TRSTAT)) && (timeReloj--)); //60 milisegundos

        if (timeReloj == 0xFFFFFFFF) {
            return 1;
        }
        
        //return SSP1CON2bits.ACKSTAT; // Si ack fue recibido devuelve 0
//        printf("ack/nack: %d\n",I2C2STATbits.ACKSTAT);
        return I2C2STATbits.ACKSTAT; // Si ack fue recibido devuelve 0
       
    }
}

uint8_t custom_ReadI2C2(void) {

    uint32_t timeReloj = 60000;
    uint8_t dato;

    //SSP1CON2bits.RCEN = 1;
    I2C2CONbits.RCEN = 1;
    Nop();    
    
    //while (!SSP1STATbits.BF);
    while (!I2C2STATbits.RBF);
    
    //dato = SSP1BUF;
    dato = I2C2RCV;

    //while ((SSP1CON2bits.RCEN) && (timeReloj--));
    while ((I2C2CONbits.RCEN) && (timeReloj--));

    if (timeReloj == 0xFFFFFFFF) {
        return 0xFF;
    }
    return dato;

}

uint8_t custom_StopI2C2(void) {

    uint32_t timeReloj = 60000;

    //SSP1CON2bits.PEN = 1;
    I2C2CONbits.PEN = 1;
    
    //while ((SSP1CON2bits.PEN) && (timeReloj--));
    while ((I2C2CONbits.PEN) && (timeReloj--));

    if (timeReloj == 0xFFFFFFFF) {
        return 1;
    }
    return 0;

}

uint8_t custom_BusySlave2(uint8_t controlByte)
{
    uint8_t error=0;
    
    custom_StartI2C2();
    error = custom_WriteI2C2(controlByte); // write 1 byte - R/W bit should be 0
    custom_StopI2C2();
    
    return error;
}

/********************************************************************
 *     Function Name:    LDByteReadI2C                               *
 *     Parameters:       EE memory ControlByte, address, pointer and *
 *                       length bytes.                               *
 *     Description:      Reads data string from I2C EE memory        *
 *                       device. This routine can be used for any I2C*
 *                       EE memory device, which only uses 1 byte of *
 *                       address data as in the 24LC01B/02B/04B/08B. *
 *                                                                   *
 ********************************************************************/

uint8_t LDByteReadI2C2(uint8_t controlByte, uint8_t address, uint8_t *data, uint8_t len) {

    uint8_t buffer;
    uint8_t error = 0;
    uint8_t aux;

    error += custom_IdleI2C2(); // ensure module is idle
    error += custom_StartI2C2(); // initiate START condition
    error += custom_WriteI2C2(controlByte); // write 1 byte
    error += custom_WriteI2C2(address); // WRITE word address to EEPROM
    error += custom_RestartI2C2(); // generate I2C bus restart condition

    error += custom_WriteI2C2(controlByte | 0x01); // WRITE 1 byte - R/W bit should be 1 for read
    for (aux = 0; aux < len; aux++, data++) {
        buffer = custom_ReadI2C2();
        *data = buffer;
        error += custom_AckI2C2();
    }
    *data = custom_ReadI2C2();
    error += custom_NoAckI2C2();
    error += custom_StopI2C2();
    
    return (0); // return with no error

}

/************************************************************************
 *     Function Name:    LDByteWriteI2C                                  *
 *     Parameters:       EE memory ControlByte, address and data         *
 *     Description:      Writes data one byte at a time to I2C EE        *
 *                       device. This routine can be used for any I2C    *
 *                       EE memory device, which only uses 1 byte of     *
 *                       address data as in the 24LC01B/02B/04B/08B/16B. *
 *                                                                       *
 ************************************************************************/

uint8_t LDByteWriteI2C2(uint8_t controlByte, uint8_t address, uint8_t data) {

    uint8_t error = 0;

    error += custom_IdleI2C2(); // ensure module is idle
    error += custom_StartI2C2(); // initiate START condition
    error += custom_WriteI2C2(controlByte); // write 1 byte - R/W bit should be 0
    error += custom_WriteI2C2(address); // write address byte to EEPROM
    error += custom_WriteI2C2(data); // Write data byte to EEPROM
    error += custom_StopI2C2(); // send STOP condition

    return (0); // return with no error

}

/********************************************************************
 *     Function Name:    LDPageWriteI2C                              *
 *     Parameters:       EE memory ControlByte, address and pointer  *
 *     Description:      Writes data string to I2C EE memory         *
 *                       device. This routine can be used for any I2C*
 *                       EE memory device, which uses 2 bytes of     *
 *                       address data as in the 24LC32A/64/128/256.  *
 *                                                                   *
 ********************************************************************/

uint8_t LDPageWriteI2C2(uint8_t controlByte, uint8_t address, uint8_t *data, uint8_t len) {

    uint8_t error = 0;
    uint8_t aux;

    error += custom_IdleI2C2(); // ensure module is idle
    error += custom_StartI2C2(); // initiate START condition
    error += custom_WriteI2C2(controlByte); // write 1 byte - R/W bit should be 0
    error += custom_WriteI2C2(address); // write LowAdd byte to EEPROM
    for (aux = 0; aux < len; aux++, data++) {
        error += custom_WriteI2C2(*data);
    }
    error += custom_StopI2C2(); // send STOP condition

    return 0; // return with no error

}

/********************************************************************
 *     Function Name:    HDByteReadI2C                               *
 *     Parameters:       EE memory ControlByte, address, pointer and *
 *                       length bytes.                               *
 *     Description:      Reads data string from I2C EE memory        *
 *                       device. This routine can be used for any I2C*
 *                       EE memory device, which only uses 1 byte of *
 *                       address data as in the 24LC01B/02B/04B/08B. *
 *                                                                   *
 ********************************************************************/

uint8_t HDByteReadI2C2(uint8_t controlByte, uint16_t address, uint8_t *data, uint16_t len) {

    uint8_t buffer;
    uint8_t error = 0;
    uint16_t aux;

    error += custom_IdleI2C2(); // ensure module is idle
    error += custom_StartI2C2(); // initiate START condition
    error += custom_WriteI2C2(controlByte); // write 1 byte
    error += custom_WriteI2C2((address >> 8) & 0x00FF); // WRITE word address to EEPROM
    error += custom_WriteI2C2(address & 0x00FF); // WRITE word address to EEPROM

    error += custom_RestartI2C2(); // generate I2C bus restart condition
    error += custom_WriteI2C2(controlByte | 0x01); // WRITE 1 byte - R/W bit should be 1 for read

    if (len > 1) {
        for (aux = 0; aux < len; aux++, data++) {
            buffer = custom_ReadI2C2();
            *data = buffer;
            error += custom_AckI2C2();
        }
    }
    
    *data = custom_ReadI2C2();
    error += custom_NoAckI2C2();
    error += custom_StopI2C2();

    return 0; // return with no error
}

/************************************************************************
 *     Function Name:    HDByteWriteI2C                                  *
 *     Parameters:       EE memory ControlByte, address and data         *
 *     Description:      Writes data one byte at a time to I2C EE        *
 *                       device. This routine can be used for any I2C    *
 *                       EE memory device, which only uses 1 byte of     *
 *                       address data as in the 24LC01B/02B/04B/08B/16B. *
 *                                                                       *
 ************************************************************************/

//uint8_t HDByteWriteI2C(uint8_t ControlByte, uint8_t HighAdd, uint8_t LowAdd, uint8_t data) {
uint8_t HDByteWriteI2C2(uint8_t controlByte, uint16_t address, uint8_t data) {
    
    uint8_t error = 0;
    uint16_t timeOut = 500;

    while(custom_BusySlave2(controlByte) && --timeOut);
    
    if (timeOut == 0) {
        error++;
    } else {
        error += custom_IdleI2C2(); // ensure module is idle
        error += custom_StartI2C2(); // initiate START condition
        error += custom_WriteI2C2(controlByte); // write 1 byte - R/W bit should be 0
        //error += custom_WriteI2C(HighAdd); // WRITE word address to EEPROM
        //error += custom_WriteI2C(LowAdd); // WRITE word address to EEPROM
        error += custom_WriteI2C2((address >> 8) & 0x00FF); // WRITE word address to EEPROM
        error += custom_WriteI2C2(address & 0x00FF); // WRITE word address to EEPROM
        error += custom_WriteI2C2(data);
        error += custom_StopI2C2(); // send STOP condition
    }
    
    return error; // return with no error
}

/********************************************************************
 *     Function Name:    HDPageWriteI2C                              *
 *     Parameters:       EE memory ControlByte, address and pointer  *
 *     Description:      Writes data string to I2C EE memory         *
 *                       device. This routine can be used for any I2C*
 *                       EE memory device, which uses 2 bytes of     *
 *                       address data as in the 24LC32A/64/128/256.  *
 *                                                                   *
 ********************************************************************/

//uint8_t HDPageWriteI2C(uint8_t ControlByte, uint8_t HighAdd, uint8_t LowAdd, uint8_t *wrptr, uint16_t len) {
uint8_t HDPageWriteI2C2(uint8_t controlByte, uint16_t address, uint8_t *data, uint16_t len) {

    uint8_t error = 0;
    uint16_t aux;

    error += custom_IdleI2C2(); // ensure module is idle
    error += custom_StartI2C2(); // initiate START condition
    error += custom_WriteI2C2(controlByte); // write 1 byte - R/W bit should be 0
    //error += custom_WriteI2C(HighAdd); // WRITE word address to EEPROM
    //error += custom_WriteI2C(LowAdd); // WRITE word address to EEPROM
    error += custom_WriteI2C2((address >> 8) & 0x00FF); // WRITE word address to EEPROM
    error += custom_WriteI2C2(address & 0x00FF); // WRITE word address to EEPROM
    for (aux = 0; aux < len; aux++, data++) {
        error += custom_WriteI2C2(*data);
    }
    error += custom_StopI2C2(); // send STOP condition

    return 0; // return with no error
}
