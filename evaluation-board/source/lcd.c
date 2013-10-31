#include "bsp.h"
#include "lcd_dogm128_6.h"
#include "gptimer.h"
#include "lcd.h"

struct LcdState {
    int menuCount;
    int menuHover;
    tLcdPage page;
    Function hoverFunction[6];
};

struct LcdState lcdState = { 0 };

void invertHovered( void );

void fullLcdInit( void ) {
    lcdInit();
}

void invertHovered( void ) {
    lcdBufferInvertPage( 0, 0, 127, eLcdPage1 + lcdState.menuHover );
}

void upKeyPress( void ) {
    if( lcdState.menuHover > 0 ) {
        invertHovered();
        lcdState.menuHover--;
        invertHovered();
        lcdSendBufferPart( 0, 0, 127, eLcdPage1 + lcdState.menuHover, eLcdPage2 + lcdState.page );
    }
}

void downKeyPress( void ) {
    if( lcdState.menuHover < lcdState.menuCount - 1 ) {
        invertHovered();
        lcdState.menuHover++;
        invertHovered();
        lcdSendBufferPart( 0, 0, 127, eLcdPage0 + lcdState.menuHover, eLcdPage1 + lcdState.page );
    }
}

void refreshScreen( void ) {
    RetVal retVal;
    
    TimerIntClear( GPTIMER0_BASE, GPTIMER_TIMA_TIMEOUT );
    
    if( lcdState.menuCount > 0 ) {
        lcdBufferClearPage( 0, lcdState.page );
        
        ( *lcdState.hoverFunction[lcdState.menuHover] )( &retVal );
        switch( retVal.retType ) {
        case RET_TYPE_INT: 
            lcdBufferPrintInt( 0, retVal.intRet, 0, lcdState.page );
            break;
        case RET_TYPE_FLOAT:
            lcdBufferPrintFloat( 0, retVal.floatRet, 2, 0, lcdState.page );
            break;
        }
    }
    
    lcdSendBufferPart( 0, 0, 127, lcdState.page, lcdState.page );
}

void createMenu( const char *header, int menuCount, const char *menu[], Function hoverFunction[] ) {
    int i;
    
    lcdState.menuCount = menuCount;
    lcdState.menuHover = 0;
    lcdState.page = eLcdPage0;
    for( i = 0; i < menuCount; ++i ) {
        lcdState.hoverFunction[i] = hoverFunction[i];
    }
    
    lcdBufferPrintString( 0, header, 0, lcdState.page++ );
    
    for( i = 0; i < menuCount; ++i ) {
        lcdBufferPrintString( 0, menu[i], 0, lcdState.page++ );
    }
    
    lcdBufferInvertPage( 0, 0, 127, eLcdPage1 );
    
    lcdSendBuffer( 0 );
}
