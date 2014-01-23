#include "bsp.h"
#include "als_sfh5711.h"
#include "lcd.h"
#include "light.h"

void lightInit( void ) {
    alsInit();
}

void readLight( RetVal *retVal ) {
    unsigned short lightLevel;
    
    lightLevel = alsRead();
    
    retVal->retType = RET_TYPE_INT;
    retVal->intRet = lightLevel;
}
