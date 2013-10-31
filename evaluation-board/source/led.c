#include "bsp.h"
#include "bsp_led.h"
#include "gptimer.h"
#include "led.h"

void ledInit( void ) {
    bspLedInit();
    bspLedSet( BSP_LED_1 | BSP_LED_3 );
}

void toggleLeds( void ) {
    TimerIntClear( GPTIMER0_BASE, GPTIMER_TIMB_TIMEOUT );
    bspLedToggle( BSP_LED_1 | BSP_LED_2 | BSP_LED_3 | BSP_LED_4 );
}