/* 
 * File:   Sensors
 * Author: Joel Palomino
 * Comments: Here are defined the functions to initiliaze the Trap Camera Sensors
 * Revision history: 02/02/2017
 */

#ifndef SENSORS_H
#define	SENSORS_H

#include <xc.h> // include processor files - each processor file is guarded.

#define DS3231_ADDRESS 0xD0//0x68

#define DS3231_SEC_REG        0x00  
#define DS3231_MIN_REG        0x01  
#define DS3231_HOUR_REG       0x02
#define DS3231_WDAY_REG       0x03
#define DS3231_MDAY_REG       0x04
#define DS3231_MONTH_REG      0x05
#define DS3231_YEAR_REG       0x06

#define DS3231_AL1SEC_REG     0x07
#define DS3231_AL1MIN_REG     0x08
#define DS3231_AL1HOUR_REG    0x09
#define DS3231_AL1WDAY_REG    0x0A

#define DS3231_AL2MIN_REG     0x0B
#define DS3231_AL2HOUR_REG    0x0C
#define DS3231_AL2WDAY_REG    0x0D

#define DS3231_CONTROL_REG          0x0E
#define DS3231_STATUS_REG           0x0F
#define DS3231_AGING_OFFSET_REG     0x0F
#define DS3231_TMP_UP_REG           0x11
#define DS3231_TMP_LOW_REG          0x12

#define EverySecond     0x01
#define EveryMinute     0x02
#define EveryHour       0x03

// DS3231 - Real Time Clock Variables
static const int DaysPerMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// DS3231 - Real time Clock Functions
void DS3231_Init(void);
void DS3231_DateHour(uint8_t *regs);
void DS3231_EnableInt(uint8_t periodicity);
void DS3231_EnableInt2(uint8_t hh24, uint8_t mm, uint8_t ss);
void DS3231_ClearInt(void);
void DS3231_Adjust(long time);

/////////////////////////////////////////////////
// HDC1000 - Humidity & Temperature Sensor Register Address
#define HDC1000_ADDRESS 0x80 //0x40

#define HDC1000_TEMPERATURE_REG     0x00
#define HDC1000_HUMIDITY_REG        0x01
#define HDC1000_CONFIGURATION_REG   0x02
#define HDC1000_SERIAL_ID_MSB_REG   0xFB
#define HDC1000_SERIAL_ID_MID_REG   0xFC
#define HDC1000_SERIAL_ID_LSB_REG   0xFD
#define HDC1000_MANUFACTURER_ID_REG 0xFE
#define HDC1000_DEVICE_ID_REG       0xFF

#define HDC1000_MODE_BIT             4
#define HDC1000_TEMP_RES_BIT         2
#define HDC1000_HUM_RES_BIT          0

// HDC1000 - Humidity & Temperature Sensor Functions
float HDC1000_ReadTemp(void);
float HDC1000_ReadHum(void);

///////////////////////////////////////////////////
// MAX17043 - MAXIM Fuel Gauge Register Address
#define MAX17043_ADDRESS        0x6C//0x36

#define MAX17043_VCELL_REG      0x02
#define MAX17043_SOC_REG        0x04
#define MAX17043_MODE_REG       0x06
#define MAX17043_VERSION_REG    0x08
#define MAX17043_CONFIG_REG     0x0C
#define MAX17043_COMAND_REG     0xFE

static const float MIN_BATT_V = 3.23; //Minimum Battery Voltage Level

// MAX17043 - MAXIM Fuel Gauge Functions
void MAX17043_Init();
float MAX17043_ReadVoltage(void);
float MAX17043_ReadSoC(void);
int MAX17043_GetVersion(void);
void MAX17043_Sleep(void);
void MAX17043_WakeUp(void);

#endif	/* XC_HEADER_TEMPLATE_H */