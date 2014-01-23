/*********************************************************************
 * INCLUDES
 */
#include "adc.h"
#include "sys_ctrl.h"
#include "hw_cctest.h"
#include "hw_rfcore_xreg.h"
#include "gpio.h"
#include "hw_memmap.h"
#include "ioc.h"

#include "zcl.h"
#include <stdio.h>

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
// Constants for converting to temerature
#define TEMP_CONST 0.58134 //(VREF / 2047) = (1190 / 2047), VREF from Datasheet
#define OFFSET_DATASHEET_25C 827 // 1422*TEMP_CONST, from Datasheet 
#define TEMP_COEFF (TEMP_CONST * 4.2) // From Datasheet
#define OFFSET_0C (OFFSET_DATASHEET_25C - (25 * TEMP_COEFF))

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static volatile uint16 reading;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static float convertToTemperature( uint16_t reading );
static void temperatureReadComplete( void );
static void floatToString( uint8* buffer, float number );

void adc_Init( void ) {  
  // Enable RF Core
  SysCtrlPeripheralEnable( SYS_CTRL_PERIPH_RFC );
  
  // Connect temperature sensor to ADC
  HWREG( CCTEST_TR0 ) |= CCTEST_TR0_ADCTM;
  
  // Enable the temperature sensor
  HWREG( RFCORE_XREG_ATEST ) = 0x01;
  
  SOCADCSingleConfigure( SOCADC_12_BIT, SOCADC_REF_INTERNAL );
  SOCADCIntRegister( temperatureReadComplete );
  
  // Configure port A pin 7 to be analog input
  IOCPadConfigSet(GPIO_A_BASE, GPIO_PIN_7, IOC_OVERRIDE_ANA);
  GPIODirModeSet(GPIO_A_BASE, GPIO_PIN_7, GPIO_DIR_MODE_IN);
}

uint8* readTemperature( void ) {
  float temperature;
  static uint8 buffer[10];
  
  reading = 0;
  SOCADCSingleStart( SOCADC_TEMP_SENS );
  while( !reading );
  
  temperature = convertToTemperature( reading );
  floatToString( buffer, temperature );
  
  return buffer;
}

uint8* readBattery( void ) {
  float voltage;
  static uint8 buffer[10];
  
  reading = 0;
  SOCADCSingleStart( SOCADC_AIN7 );
  while( !reading );
  
  voltage = reading / 2047.0 * 11.0 * 2.3;
  floatToString( buffer, voltage );
  
  return buffer;
}

static void floatToString( uint8* buffer, float number ) {
  uint8 integer, fraction;
  
  integer = ( int )number;
  number = ( number - integer ) * 100;
  fraction = ( int )number;
  sprintf( ( char* )buffer, "%d.%d", integer, fraction );
}

static void temperatureReadComplete( void ) {
  reading = SOCADCDataGet() >> SOCADC_12_BIT_RSHIFT;
}

static float convertToTemperature( uint16_t reading ) {
  double outputVoltage = reading * TEMP_CONST;
  return ( ( outputVoltage - OFFSET_0C ) / TEMP_COEFF );
}