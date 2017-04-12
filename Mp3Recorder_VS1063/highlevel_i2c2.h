#ifndef EEPROM_I2C_H
#define	EEPROM_I2C_H

#include <xc.h>
#include <stdint.h>

uint8_t custom_IdleI2C(void);
uint8_t custom_AckI2C(void);
uint8_t custom_NoAckI2C(void);
uint8_t custom_StartI2C(void);
uint8_t custom_RestartI2C(void);
uint8_t custom_WriteI2C(uint8_t data);
uint8_t custom_ReadI2C(void);
uint8_t custom_StopI2C(void);
uint8_t custom_BusySlave(uint8_t controlByte);

//void I2C_Inicializa(void);

uint8_t LDByteReadI2C2(uint8_t controlByte, uint8_t address, uint8_t *data, uint8_t len);
uint8_t LDByteWriteI2C2(uint8_t controlByte, uint8_t address, uint8_t data);
uint8_t LDPageWriteI2C2(uint8_t controlByte, uint8_t address, uint8_t *data, uint8_t len);

uint8_t HDByteReadI2C2(uint8_t controlByte, uint16_t address, uint8_t *data, uint16_t len);
uint8_t HDByteWriteI2C2(uint8_t controlByte, uint16_t address, uint8_t data);
uint8_t HDPageWriteI2C2(uint8_t controlByte, uint16_t address, uint8_t *data, uint16_t len);

#endif	/* EEPROM_I2C_H */