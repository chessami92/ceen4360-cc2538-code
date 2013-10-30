typedef struct RetVal RetVal;

typedef enum {
    RET_TYPE_INT,
    RET_TYPE_FLOAT
} RetType;

struct RetVal {
    int intRet;
    float floatRet;
    RetType retType;
};

void fullLcdInit( void );
void refreshScreen( void );
void createMenu( const char *header, int menuCount, const char *menu[] );