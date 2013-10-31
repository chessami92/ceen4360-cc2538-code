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
    
    const char *menus[] = { "Temperature", "Light Level", "X Accelleration", "Y Acceleration", "Z Acceleration" };
    Function functions[] = { readTemperature, readLight, readAccX, readAccY, readAccZ };
    
    createMenu( "Simple Sensor Monitor", 5, menus, functions );
    
    while( 1 ) {}
}
