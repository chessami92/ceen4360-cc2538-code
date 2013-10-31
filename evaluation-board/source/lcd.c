#include "bsp.h"
#include "gptimer.h"
#include "lcd_dogm128_6.h"
#include "sys_ctrl.h"

#include "lcd.h"

struct LcdState {
    int menuHover;
    tLcdPage page;
};

struct LcdState lcdState = { 0 };
Menu *currentMenu;

void invertHovered( void );
void invertScreen( void );

void fullLcdInit( void ) {
    lcdInit();
}

void invertHovered( void ) {
    tLcdPage menuHover = eLcdPage1 + lcdState.menuHover;
    lcdBufferInvertPage( 0, 0, 127, menuHover );
    lcdSendBufferPart( 0, 0, 127, menuHover, menuHover );
}

void invertScreen( void ) {
    lcdBufferInvert( 0, 0, 0, 127, 63 );
    lcdSendBuffer( 0 );
}

void upKeyPress( void ) {
    if( !currentMenu ) {
        return;
    }
    
    if( lcdState.menuHover > 0 ) {
        invertHovered();
        lcdState.menuHover--;
        invertHovered();
    } else {
        flashScreen();
    }
}

void downKeyPress( void ) {
    if( !currentMenu ) {
        return;
    }
    
    if( lcdState.menuHover < currentMenu->menuCount - 1 ) {
        invertHovered();
        lcdState.menuHover++;
        invertHovered();
    } else {
        flashScreen();
    }
}

void refreshScreen( void ) {
    RetVal retVal;
    
    TimerIntClear( GPTIMER0_BASE, GPTIMER_TIMA_TIMEOUT );
    
    if( !currentMenu ) {
        return;
    }
    
    if( currentMenu->menuCount > 0 ) {
        lcdBufferClearPage( 0, lcdState.page );
        
        ( *currentMenu->hoverFunction[lcdState.menuHover] )( &retVal );
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

void flashScreen( void ) {
    int i;
    
    for( i = 0; i < 4; ++i ) {
        invertScreen();
        SysCtrlDelay( 500000 ); // Abount 50ms
    }
}

void createMenu( Menu *menu ) {
    int i;
    currentMenu = menu;
    
    lcdState.currentMenuHover = 0;
    lcdState.page = eLcdPage0;
    
    lcdBufferPrintString( 0, currentMenu->header, 0, lcdState.page++ );
    
    for( i = 0; i < currentMenu->currentMenuCount; ++i ) {
        lcdBufferPrintString( 0, currentMenu->currentMenu[i], 0, lcdState.page++ );
    }
    
    lcdBufferInvertPage( 0, 0, 127, eLcdPage1 );
    
    lcdSendBuffer( 0 );
}
