typedef struct RetVal RetVal;
typedef void ( *Function )( RetVal* );

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
void upKeyPress( void );
void downKeyPress( void );
void refreshScreen( void );
void flashScreen( void );
void createMenu( const char *header, int menuCount, const char *menu[], Function function[] );