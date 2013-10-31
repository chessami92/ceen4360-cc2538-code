#include "hw_ints.h"
#include "hw_memmap.h"
#include "gpio.h"
#include "interrupt.h"
#include "ioc.h"
#include "lcd.h"
#include "keys.h"

#define LEFT_KEY  0x10
#define RIGHT_KEY 0x20
#define UP_KEY    0x40
#define DOWN_KEY  0x80

#define ALL_BUTTONS GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7

void keyInit( void ) {
    // Set C.4-7 as input with pull-up. 
    GPIOPinTypeGPIOInput( GPIO_C_BASE, ALL_BUTTONS );
    IOCPadConfigSet( GPIO_C_BASE, ALL_BUTTONS, IOC_OVERRIDE_PUE );

    GPIOIntTypeSet( GPIO_C_BASE, ALL_BUTTONS, GPIO_FALLING_EDGE );
   
    GPIOPinIntEnable( GPIO_C_BASE, ALL_BUTTONS );

    IntEnable( INT_GPIOC );
}

void debounce ( uint32_t port, uint8_t pin ) {
    while( !GPIOPinRead( port, pin ) ) {}
}

void GPIOCIntHandler( void ) {
    uint32_t bitsChanged = GPIOPinIntStatus( GPIO_C_BASE, true );

    if( bitsChanged & LEFT_KEY ) {
        parentMenu();
        debounce( GPIO_C_BASE, GPIO_PIN_4 );
    } else if( bitsChanged & RIGHT_KEY ) {
        childMenu();
        debounce( GPIO_C_BASE, GPIO_PIN_5 );
    } else if( bitsChanged & UP_KEY ) {
        upKeyPress();
        debounce( GPIO_C_BASE, GPIO_PIN_6 );
    } else if( bitsChanged & DOWN_KEY ) {
        downKeyPress();
        debounce( GPIO_C_BASE, GPIO_PIN_7 );
    }

    GPIOPinIntClear( GPIO_C_BASE, bitsChanged );
}
