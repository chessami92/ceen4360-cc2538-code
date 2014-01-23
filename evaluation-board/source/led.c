#include "bsp.h"
#include "bsp_led.h"
#include "gptimer.h"
#include "lcd.h"
#include "led.h"

typedef enum {
    INCREMENT, 
    DECREMENT, 
    DISABLED
} LedFunction;

LedFunction ledFunction = INCREMENT;
char count = 0;

void clearLeds( void );
void outputAsBinary( char number );

void ledInit( void ) {
    bspLedInit();
    clearLeds();
}

void toggleLeds( void ) {
    TimerIntClear( GPTIMER0_BASE, GPTIMER_TIMB_TIMEOUT );
    
    switch( ledFunction ) {
    case INCREMENT:
        count++;
        break;
    case DECREMENT:
        count--;
        break;
    case DISABLED:
        return;
    }
    
    outputAsBinary( count );
}

void clearLeds( void ) {
    bspLedClear( BSP_LED_1 | BSP_LED_2 | BSP_LED_3 | BSP_LED_4 );
}

void outputAsBinary( char number ) {
    uint8_t ledMask = 0;
    
    if( number & 0x01 ) {
        ledMask |= BSP_LED_4;
    }
    if( number & 0x02 ) {
        ledMask |= BSP_LED_3;
    }
    if( number & 0x04 ) {
        ledMask |= BSP_LED_2;
    }
    if( number & 0x08 ) {
        ledMask |= BSP_LED_1;
    }
    
    clearLeds();
    bspLedSet( ledMask );
}

void incrementLeds( RetVal *retVal ) {
    ledFunction = INCREMENT;
    retVal->retType = RET_TYPE_NONE;
}

void decrementLeds( RetVal *retVal ) {
    ledFunction = DECREMENT;
    retVal->retType = RET_TYPE_NONE;
}

void disableLeds( RetVal *retVal ) {
    count = 0;
    outputAsBinary( count );
    ledFunction = DISABLED;
    retVal->retType = RET_TYPE_NONE;
}