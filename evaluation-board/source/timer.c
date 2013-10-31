#include "gptimer.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "sys_ctrl.h"

#include "lcd.h"
#include "led.h"
#include "timer.h"

void timerInit( void ) {
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
    
    IntEnable( INT_TIMER0A );
    IntEnable( INT_TIMER0B );
}