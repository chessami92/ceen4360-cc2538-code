#include "bsp.h"
#include "bsp_led.h"
#include "lcd_dogm128_6.h"
#include "als_sfh5711.h"
#include "adc.h"
#include "gptimer.h"
#include "sys_ctrl.h"
#include "hw_cctest.h"
#include "hw_memmap.h"
#include "hw_ints.h"
#include "hw_rfcore_xreg.h"
#include "interrupt.h"
#include <stdint.h>

// Constants for converting to temerature
#define TEMP_CONST 0.58134 //(VREF / 2047) = (1190 / 2047), VREF from Datasheet
#define OFFSET_DATASHEET_25C 827 // 1422*TEMP_CONST, from Datasheet 
#define TEMP_COEFF (TEMP_CONST * 4.2) // From Datasheet
#define OFFSET_0C (OFFSET_DATASHEET_25C - (25 * TEMP_COEFF))

void setup();
void refreshScreen();
void toggleLeds();
float convertToTemperature( uint16_t reading );
void displayTemperature();
void displayLight();

void setup() {
    // Set the clocking to run directly from the external crystal/oscillator.
    // (no ext 32k osc, no internal osc)
    SysCtrlClockSet( false, false, SYS_CTRL_SYSDIV_32MHZ );
    // Set IO clock to the same as system clock
    SysCtrlIOClockSet( SYS_CTRL_SYSDIV_32MHZ );
    // Enable the RF Core
    SysCtrlPeripheralEnable(SYS_CTRL_PERIPH_RFC);
  
    bspInit( BSP_SYS_CLK_SPD );
    bspSpiInit( BSP_SPI_CLK_SPD );
    lcdInit();
    
    SysCtrlPeripheralEnable(SYS_CTRL_PERIPH_GPT0);
    TimerConfigure( GPTIMER0_BASE, GPTIMER_CFG_SPLIT_PAIR | 
                    GPTIMER_CFG_A_PERIODIC | GPTIMER_CFG_B_PERIODIC );
    TimerPrescaleSet( GPTIMER0_BASE, GPTIMER_BOTH, 255 );
    TimerLoadSet( GPTIMER0_BASE, GPTIMER_A, SysCtrlClockGet() / 255 / 10 );
    TimerLoadSet( GPTIMER0_BASE, GPTIMER_B, SysCtrlClockGet() / 255 );
    TimerIntRegister( GPTIMER0_BASE, GPTIMER_A, refreshScreen );
    TimerIntRegister( GPTIMER0_BASE, GPTIMER_B, toggleLeds );
    TimerIntEnable( GPTIMER0_BASE, GPTIMER_TIMA_TIMEOUT | GPTIMER_TIMB_TIMEOUT );
    
    TimerEnable( GPTIMER0_BASE, GPTIMER_BOTH );
    
    bspLedInit();
    bspLedSet( BSP_LED_1 | BSP_LED_3 );
    
    alsInit();
    
    // Connect temperature sensor to ADC
    HWREG( CCTEST_TR0 ) |= CCTEST_TR0_ADCTM;
    // Enable the temperature sensor
    HWREG( RFCORE_XREG_ATEST ) = 0x01;
    SOCADCSingleConfigure( SOCADC_12_BIT, SOCADC_REF_INTERNAL );
    
    IntEnable( INT_TIMER0A );
    IntEnable( INT_TIMER0B );
    IntMasterEnable();
}

void refreshScreen() {
    TimerIntClear( GPTIMER0_BASE, GPTIMER_TIMA_TIMEOUT );
    lcdSendBuffer( 0 );
}

void toggleLeds() {
    TimerIntClear( GPTIMER0_BASE, GPTIMER_TIMB_TIMEOUT );
    bspLedToggle( BSP_LED_1 | BSP_LED_2 | BSP_LED_3 | BSP_LED_4 );
}

float convertToTemperature( uint16_t reading ) {
    double outputVoltage = reading * TEMP_CONST;
    return ( ( outputVoltage - OFFSET_0C ) / TEMP_COEFF );
}

void displayTemperature() {
    uint16_t reading;
    float temperature;

    SOCADCSingleStart( SOCADC_TEMP_SENS );
    while( !SOCADCEndOfCOnversionGet() ) {}

    reading = SOCADCDataGet() >> SOCADC_12_BIT_RSHIFT;
    temperature = convertToTemperature( reading );
    lcdBufferPrintFloatAligned( 0, temperature, 2, eLcdAlignRight, eLcdPage1 );
}

void displayLight() {
    unsigned short lightLevel;
    
    lightLevel = alsRead();
    lcdBufferPrintIntAligned( 0, lightLevel, eLcdAlignRight, eLcdPage2 );
}

int main() {
    setup();
    
    lcdBufferClear( 0 );
    lcdBufferPrintString( 0, "Simple Sensor Monitor", 0, eLcdPage0 );
    lcdBufferPrintString( 0, "Temperature: ", 0, eLcdPage1 );
    lcdBufferPrintString( 0, "Light Level: ", 0, eLcdPage2 );

    while( 1 ) {
        displayTemperature();
        displayLight();
    }
}