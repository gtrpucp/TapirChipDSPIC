
#include <string.h>
#include "vs1063.h"


#define spi_transfer(x)     xchg_spi2(x)

// wait for VS_DREQ to get HIGH before sending new data to SPI
inline void vs_wait(void)
{
    while (!VS_DREQ) {
    };
}

// read the 16-bit value of a VS10xx register
uint16_t ReadSci(const uint8_t address){
    uint16_t resultvalue = 0;
    uint16_t aux = 0;   
    
    vs_wait();              // Wait until DREQ is high
    vs_deselect_data();     // desactive xDCS
    vs_select_control();    // active XCS
    spi_transfer(VS_READ_COMMAND);  // read command code
    spi_transfer(address);          // SCI register number
    aux = spi_transfer(0xff);
    resultvalue = aux << 8;
    aux = spi_transfer(0xff);
    resultvalue |= aux;
    vs_deselect_control();  // Desactive xCS

    return resultvalue;
}

// write 2 bytes VS10xx register
void WriteSci(uint8_t address, uint16_t data){
    
    vs_wait();
    vs_deselect_data();     // Desactive XDCS
    vs_select_control();    // active XCS

    spi_transfer(VS_WRITE_COMMAND); // Write command code
    spi_transfer(address);
    spi_transfer((uint8_t)(data >> 8));
    spi_transfer((uint8_t)(data & 0xff));
    vs_deselect_control();  // Desactive XCS
}

// write data bytes 
int WriteSdi(const uint8_t *data, uint8_t bytes){
    uint8_t i;
    
    if(bytes > 32)
        return -1;
    
    vs_wait();
    vs_deselect_control();      // Desactive XCS
    vs_select_data();           // Active XDCS
    
    for(i = 0; i < bytes; i++){
        spi_transfer(*data++);
    }
    vs_deselect_data();         // Desactive XDCS
    
    return 0;
}

