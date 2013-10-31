#include "bsp.h"
#include "acc_bma250.h"

#include "lcd.h"
#include "acc.h"

int16_t accX, accY, accZ;

void fullAccInit( void ) {
    accInit();
}

void readAcc() {
    accReadData( &accX, &accY, &accZ );
}

void readAccX( RetVal *retVal ) {  
    readAcc();
    
    retVal->retType = RET_TYPE_INT;
    retVal->intRet = accX;
}

void readAccY( RetVal *retVal ) {
    readAcc();
    
    retVal->retType = RET_TYPE_INT;
    retVal->intRet = accY;
}

void readAccZ( RetVal *retVal ) {
    readAcc();
    
    retVal->retType = RET_TYPE_INT;
    retVal->intRet = accZ;
}
