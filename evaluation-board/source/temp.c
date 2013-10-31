#include "bsp.h"
#include "adc.h"
#include "gptimer.h"
#include "sys_ctrl.h"
#include "hw_cctest.h"
#include "hw_memmap.h"
#include "hw_ints.h"
#include "hw_rfcore_xreg.h"
#include "interrupt.h"

#include "lcd.h"
#include "temp.h"

// Constants for converting to temerature
#define TEMP_CONST 0.58134 //(VREF / 2047) = (1190 / 2047), VREF from Datasheet
#define OFFSET_DATASHEET_25C 827 // 1422*TEMP_CONST, from Datasheet 
#define TEMP_COEFF (TEMP_CONST * 4.2) // From Datasheet
#define OFFSET_0C (OFFSET_DATASHEET_25C - (25 * TEMP_COEFF))

float convertToTemperature( uint16_t reading );

void tempInit( void ) {
    // Enable the RF Core
    SysCtrlPeripheralEnable( SYS_CTRL_PERIPH_RFC );
    // Connect temperature sensor to ADC
    HWREG( CCTEST_TR0 ) |= CCTEST_TR0_ADCTM;
    // Enable the temperature sensor
    HWREG( RFCORE_XREG_ATEST ) = 0x01;
    SOCADCSingleConfigure( SOCADC_12_BIT, SOCADC_REF_INTERNAL );
}

float convertToTemperature( uint16_t reading ) {
    double outputVoltage = reading * TEMP_CONST;
    return ( ( outputVoltage - OFFSET_0C ) / TEMP_COEFF );
}

void readTemperature( RetVal *retVal ) {
    uint16_t reading;
    float temperature;

    SOCADCSingleStart( SOCADC_TEMP_SENS );
    while( !SOCADCEndOfCOnversionGet() ) {}

    reading = SOCADCDataGet() >> SOCADC_12_BIT_RSHIFT;
    temperature = convertToTemperature( reading );
    
    retVal->retType = RET_TYPE_FLOAT;
    retVal->floatRet = temperature;
}