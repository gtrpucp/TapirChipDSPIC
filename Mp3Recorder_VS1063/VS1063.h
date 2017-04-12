
#ifndef VS1063_H
#define	VS1063_H

#include <stdint.h>
#include "pic_io.h"
#include "spi2.h"
#include "delay.h"  

#define VS_CLK_SLOW  SPI2_BUS_SET_TO_LOW_SPEED
#define VS_CLK_FAST  SPI2_BUS_SET_TO_FULL_SPEED_SD

#define vs_select_control()     VS_XCS = 0    
#define vs_deselect_control()   VS_XCS = 1 
#define vs_select_data()        VS_XDCS = 0 
#define vs_deselect_data()      VS_XDCS = 1

// should be followed by 200ms delays for the cap to charge/discharge
#define vs_assert_xreset()      VS_XRESET = 0 
#define vs_deassert_xreset()    VS_XRESET = 1 

// comandos de lectura y escritura de VS10xx 
#define VS_WRITE_COMMAND    0x02
#define VS_READ_COMMAND     0x03

inline void vs_wait(void);

#endif	/* VS1063_H */



