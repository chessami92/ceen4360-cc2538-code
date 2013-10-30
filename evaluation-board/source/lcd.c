#include "bsp.h"
#include "lcd_dogm128_6.h"
#include "gptimer.h"
#include "lcd.h"

void fullLcdInit( void ) {
    bspInit( BSP_SYS_CLK_SPD );
    bspSpiInit( BSP_SPI_CLK_SPD );
    lcdInit();
}

void refreshScreen( void ) {
    TimerIntClear( GPTIMER0_BASE, GPTIMER_TIMA_TIMEOUT );
    lcdSendBuffer( 0 );
}

void createMenu( const char *header, int menuCount, const char *menu[] ) {
    int i;
    tLcdPage page = eLcdPage0;
    
    lcdBufferPrintString( 0, header, 0, page );
    page++;
    
    for( i = 0; i < menuCount; ++i ) {
        lcdBufferPrintString( 0, menu[i], 0, page );
        page++;
    }
}