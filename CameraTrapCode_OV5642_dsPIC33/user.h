#include "highlevel_i2c2.h"

void Camara_EnviaStringUART1(uint8_t *buffer);
void Camara_EnviaBytesI2C(uint16_t dir, uint8_t dat);
void InicializaREFOCLK(void);
void InicializaTIMER1(void);
void InicializaTIMER2(void);
void InicializaTIMER4_5(void);
void InicializaSPI1(void);
void InicializaDMA_SPI(void);
void InicializaI2C(void);
void InicializaUART1(void);
void InicializaIO(void);
void PIR_IntConfig(void);
void NoAhorroEnergia(void);
void AhorroEnergia(void);
void initADC(void);
uint16_t readADC(void);
uint8_t custom_IdleI2C(void);
uint8_t custom_AckI2C(void);
uint8_t custom_NoAckI2C(void);
uint8_t custom_StartI2C(void);
uint8_t custom_RestartI2C(void);
uint8_t custom_WriteI2C(uint8_t data);
uint8_t custom_ReadI2C(void);
uint8_t custom_StopI2C(void);