/* 
 * File:   Eeprom_i2c.h
 * Author: HENDRIX
 *
 * Created on 24 de marzo de 2015, 02:53 PM
 */

#ifndef EEPROM_I2C_H
#define	EEPROM_I2C_H

#include <xc.h>
#include <stdint.h>

uint8_t custom_IdleI2C1(void);
uint8_t custom_AckI2C1(void);
uint8_t custom_NoAckI2C1(void);
uint8_t custom_StartI2C1(void);
uint8_t custom_RestartI2C1(void);
uint8_t custom_WriteI2C1(uint8_t data);
uint8_t custom_ReadI2C1(void);
uint8_t custom_StopI2C1(void);
uint8_t custom_BusySlave1(uint8_t controlByte);

//void I2C_Inicializa(void);

uint8_t LDByteReadI2C1(uint8_t controlByte, uint8_t address, uint8_t *data, uint8_t len);
uint8_t LDByteWriteI2C1(uint8_t controlByte, uint8_t address, uint8_t data);
uint8_t LDPageWriteI2C1(uint8_t controlByte, uint8_t address, uint8_t *data, uint8_t len);

uint8_t HDByteReadI2C1(uint8_t controlByte, uint16_t address, uint8_t *data, uint16_t len);
uint8_t HDByteWriteI2C1(uint8_t controlByte, uint16_t address, uint8_t data);
uint8_t HDPageWriteI2C1(uint8_t controlByte, uint16_t address, uint8_t *data, uint16_t len);

#endif	/* EEPROM_I2C_H */

