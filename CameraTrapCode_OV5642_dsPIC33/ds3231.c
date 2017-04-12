/*
 * File:   Sensors.c
 * Author: Joel Palomino
 *
 * Created on 2 of february of 2017, 10:30 AM
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "highlevel_i2c2.h"
#include "DS3231.h"

#define FOSC 119762500  //Hz ~ 120MHz
#define FCY  FOSC/2     //Hz ~ 60MIPS

#include <libpic30.h>

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

/**
 * Initlialize the DS3231 module
 */
void DS3231_Init(void) {
    uint8_t ctReg = 0;
    static uint8_t hrReg = 0;
    
    ctReg = 0b00011100;
    if(LDByteWriteI2C2(DS3231_ADDRESS, DS3231_CONTROL_REG, ctReg))
        printf("Error while writing I2C bus\r\n");
    
    __delay_ms(10);
    
    if(LDByteReadI2C2(DS3231_ADDRESS, DS3231_HOUR_REG, &hrReg, 1))
        printf("Error while reading I2C bus\r\n");
    
    hrReg &= 0b10111111;
    __delay_ms(100);
    
    if(LDByteWriteI2C2(DS3231_ADDRESS, DS3231_HOUR_REG, hrReg))
        printf("Error while writing I2C bus\r\n");
    
    printf("RTC initialized correctly\r\n");
}

/**
 * Read the Hours and Date Registers
 * 
 * @param regs  Registers were the values will be stored
 */
void DS3231_DateHour(uint8_t *regs) {
    
    if(LDByteReadI2C2(DS3231_ADDRESS, DS3231_SEC_REG, regs, 8))
        printf("Error while reading I2C bus\r\n");
    
    regs[0] = bcd2bin(regs[0]);     // Seconds
    regs[1] = bcd2bin(regs[1]);     // Minutes
    regs[2] = bcd2bin(regs[2] & ~0b11000000);   // Hours
    regs[4] = bcd2bin(regs[4]);     // Days
    regs[5] = bcd2bin(regs[5]);     // Month
    regs[6] = bcd2bin(regs[6]);     // Year
}

/**
 * Enable peridically interruptions on pin INT of the DS3231
 * 
 * @param periodicity   Indicates the periodicity of interruption
 *                      can be EveryMinute, EverySecond, EveryHour
 */
void DS3231_EnableInt(uint8_t periodicity) {
    uint8_t ctReg = 0;
    
    ctReg = 0b00011101;
    if(LDByteWriteI2C2(DS3231_ADDRESS, DS3231_CONTROL_REG, ctReg))
        printf("Error while writing I2C bus\r\n");
    
    switch(periodicity) {
        case EverySecond:
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1SEC_REG, 0b10000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1MIN_REG, 0b10000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1HOUR_REG, 0b10000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1WDAY_REG, 0b10000000);
            break;
        case EveryMinute:
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1SEC_REG, 0b00000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1MIN_REG, 0b10000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1HOUR_REG, 0b10000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1WDAY_REG, 0b10000000);
            break;
        case EveryHour:
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1SEC_REG, 0b00000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1MIN_REG, 0b00000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1HOUR_REG, 0b10000000);
            LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1WDAY_REG, 0b10000000);
            break;
    }
}

/**
 * Enable an interrupt at a specific hour in the INT pin of the DS3231
 * 
 * @param hh24  Hour of the Interrupt in 24 Hour Format
 * @param mm    Minute of the Interrupt
 * @param ss    Second of the Interrupt
 */
void DS3231_EnableInt2(uint8_t hh24, uint8_t mm, uint8_t ss){
    uint8_t ctReg = 0;
    
    ctReg = 0b00011101;
    if(LDByteWriteI2C2(DS3231_ADDRESS, DS3231_CONTROL_REG, ctReg))
        printf("Error while writing I2C bus\r\n");
    
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1SEC_REG, bin2bcd(ss));
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1MIN_REG, bin2bcd(mm));
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1HOUR_REG, (bin2bcd(hh24) & 0b10111111));
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_AL1WDAY_REG, 0b10000000);
}

/**
 * Clear the Interrupr Flag of the DS3231, this is necesary to set the
 * INT pin to high again begore and after an interrupt has ocurred
 */
void DS3231_ClearInt(void){
    uint8_t statusReg = 0;
    
    if(LDByteReadI2C2(DS3231_ADDRESS, DS3231_STATUS_REG, &statusReg, 1))
        printf("Error while reading I2C bus\r\n");
    statusReg &= 0b11111110;
    
    if(LDByteWriteI2C2(DS3231_ADDRESS, DS3231_STATUS_REG, statusReg))
        printf("Error while writing I2C bus\r\n");
}

/**
 * Adjust the time in the DS3231 module by giving the current time
 * in UNIX format
 * 
 * @param time  Time in UNIX format
 */
void DS321_Adjust(long time){
    uint8_t ss = 0, mm = 0, hh = 0;
    uint16_t days = 0;
    uint8_t leap = 0, yOff = 0, m = 0;
    uint8_t dd = 0, DaysInMonth = 0;
    
    ss = time % 60;
    time /= 60;
    mm = time % 60;
    time /= 60;
    hh = time % 24;
    days = time / 24;
    for(yOff = 0; ; ++yOff){
        leap = yOff % 4 == 0;
        if(days < 365 + leap)
            break;
        days -= 365 + leap;
    }
    for(m = 1; ; m++){
        DaysInMonth = DaysPerMonth[m - 1];
        if(leap && m == 2)
            ++DaysInMonth;
        if(days < DaysInMonth)
            break;
        days -= DaysInMonth;
    }
    dd = days + 1;
    
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_SEC_REG, bin2bcd(ss));
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_MIN_REG, bin2bcd(mm));
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_HOUR_REG, bin2bcd(hh));
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_WDAY_REG, 0);
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_MDAY_REG, bin2bcd(dd));
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_MONTH_REG, bin2bcd(m));
    LDByteWriteI2C2(DS3231_ADDRESS, DS3231_YEAR_REG, bin2bcd(yOff));
}

/**
 * Read the Temperature from the HDC1000 Sensor
 * @return Temperature(float) in Celsius Degrees (°C)
 */
float HDC1000_ReadTemp(){
    static uint8_t confReg[2] = {0}, Reg_Value[2] = {0};
    uint16_t Temp = 0;
    
    //Set Temperature or Humidity Acquisition and High Resolution
    confReg[1] = 0; //Must be '0' according to datasheet
    confReg[0] = (0 << HDC1000_MODE_BIT)|(0<<HDC1000_TEMP_RES_BIT)|(0<<HDC1000_HUM_RES_BIT);
    
    if(LDPageWriteI2C2(HDC1000_ADDRESS, HDC1000_CONFIGURATION_REG, confReg, 2))
        printf("Error while writing I2C bus\r\n");
    
    if(LDByteReadI2C2(HDC1000_ADDRESS, HDC1000_TEMPERATURE_REG, Reg_Value, 2))
        printf("Error while reading I2C bus\r\n");
    
    Temp = ( Reg_Value[0] << 8 )| Reg_Value[1];
    return ( (float)Temp / 65536 ) * 165 - 40;
}

/**
 * Read the Humidity from the HDC1000 Sensor
 * @return Humidity(float) in Relative Humidity (%RH)
 */
float HDC1000_ReadHum() {
    static uint8_t confReg[2] = {0}, Reg_Value[2] = {0};
    uint16_t Hum = 0;
    
    //Set Temperature or Humidity Acquisition and High Resolution
    confReg[1] = 0; //Must be '0' according to datasheet
    confReg[0] = (0 << HDC1000_MODE_BIT)|(0<<HDC1000_TEMP_RES_BIT)|(0<<HDC1000_HUM_RES_BIT);
        
    if(LDPageWriteI2C2(HDC1000_ADDRESS, HDC1000_CONFIGURATION_REG, confReg, 2))
        printf("Error while writing I2C bus\r\n");
    
    if(LDByteReadI2C2(HDC1000_ADDRESS, HDC1000_HUMIDITY_REG, Reg_Value, 2))
        printf("Error while reading I2C bus\r\n");
    
    Hum = ( Reg_Value[0] << 8 )| Reg_Value[1];
    return ( (float)Hum / 65536 ) * 100;
    
}

/**
 * Initalize the MAX17043 fuel gauge after power-up
 */
void MAX17043_Init(void) {
    uint8_t confReg[2] = {0};
    
    confReg[0] = 0x54;
    confReg[1] = 0x00;
    if(LDPageWriteI2C2(MAX17043_ADDRESS, MAX17043_COMAND_REG, confReg, 2))
        printf("Error while writing I2C bus\r\n");
    
    confReg[0] = 0x97;
    confReg[1] = 0x00;
    if(LDPageWriteI2C2(MAX17043_ADDRESS, MAX17043_CONFIG_REG, confReg, 2))
        printf("Error while writing I2C bus\r\n");
    
    // Wait for one second to get correct readings
    __delay_ms(1000);
}

/**
 * Reads the current battery voltage measured by the fuel gauge
 * @return Voltage(float)   Battery current voltage
 */
float MAX17043_ReadVoltage(void) {
    static uint8_t Reg_Value[2] = {0};
    int value = 0;
    
    if(LDByteReadI2C2(MAX17043_ADDRESS, MAX17043_VCELL_REG, Reg_Value, 2))
        printf("Error while reading I2C bus\r\n");
    
    value = ((Reg_Value[0]<<8)|Reg_Value[1]);
    value = (value>>4)&0xFFF;
    return value*0.00125;
}

/**
 * Reads the current battery state of charge in % calculated from the fuel gauge
 * @return SoC(float)   Battery current state of charge
 */
float MAX17043_ReadSoC(void) {
    static uint8_t Reg_Value[2] = {0};
    float decimal = 0;
    
    if(LDByteReadI2C2(MAX17043_ADDRESS, MAX17043_SOC_REG, Reg_Value, 2))
        printf("Error while reading I2C bus\r\n");
    
    decimal = (float)Reg_Value[1]/256;
    return Reg_Value[0] + decimal;
}

/**
 * Read the version of the MAX17043 device
 * @return Version(int)
 */
int MAX17043_GetVersion(void) {
    static uint8_t Reg_Value[2] = {0};
    
    if(LDByteReadI2C2(MAX17043_ADDRESS, MAX17043_VERSION_REG, Reg_Value, 2))
        printf("Error while reading I2C bus\r\n");
    
    return (Reg_Value[1]<<8)|Reg_Value[0];
}

/**
 * Configure the MAX17043 fuel gauge to sleep mode
 */
void MAX17043_Sleep() {
    static uint8_t Reg_Value[2] = {0};
    
    if(LDByteReadI2C2(MAX17043_ADDRESS, MAX17043_CONFIG_REG, Reg_Value, 2))
        printf("Error while reading I2C bus\r\n");
    
    Reg_Value[1] = Reg_Value[1] | 0x80;
    if(LDPageWriteI2C2(MAX17043_ADDRESS, MAX17043_COMAND_REG, Reg_Value, 2))
        printf("Error while writing I2C bus\r\n");
}

/**
 * Configure the MAX17043 fuel gauge to exit sleep mode
 */
void MAX17043_WakeUp() {
    static uint8_t Reg_Value[2] = {0};
    
    if(LDByteReadI2C2(MAX17043_ADDRESS, MAX17043_CONFIG_REG, Reg_Value, 2))
        printf("Error while reading I2C bus\r\n");
    
    Reg_Value[1] = Reg_Value[1] & 0x7F;
    if(LDPageWriteI2C2(MAX17043_ADDRESS, MAX17043_COMAND_REG, Reg_Value, 2))
        printf("Error while writing I2C bus\r\n");
}