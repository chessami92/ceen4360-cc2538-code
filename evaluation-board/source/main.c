#include "bsp.h"
#include "sys_ctrl.h"
#include "interrupt.h"

#include "lcd.h"
#include "acc.h"
#include "keys.h"
#include "led.h"
#include "light.h"
#include "temp.h"
#include "timer.h"
#include <stdint.h>

Menu sensorMenu = { "Simple Sensor Monitor", 5, 
    { "Temperature", "Light Level", "X Accelleration", "Y Acceleration", "Z Acceleration" },
    { readTemperature, readLight, readAccX, readAccY, readAccZ }
};

Menu lightsMenu = { "Lights Menu", 3,
    { "Count Up", "Count Down", "Disable" },
    { incrementLeds, decrementLeds, disableLeds },
};

Menu mainMenu = { "Main Menu", 2, 
    { "Sensor Menu", "Lights Menu" },
    { noOp, noOp },
    { &sensorMenu, &lightsMenu }
};

void setup() {
    // Set the clocking to run directly from the external crystal/oscillator.
    // (no ext 32k osc, no internal osc)
    SysCtrlClockSet( false, false, SYS_CTRL_SYSDIV_32MHZ );
    // Set IO clock to the same as system clock
    SysCtrlIOClockSet( SYS_CTRL_SYSDIV_32MHZ );
    
    bspInit( BSP_SYS_CLK_SPD );
    bspSpiInit( BSP_SPI_CLK_SPD );
    
    fullLcdInit();
    fullAccInit();
    lightInit();
    ledInit();
    keyInit();
    tempInit();
    timerInit();
    
    IntMasterEnable();
}

int main() {
    setup();
    
    createMenu( &mainMenu );
    
    while( 1 ) {}
}
