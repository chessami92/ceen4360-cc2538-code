typedef struct Menu Menu;
typedef struct RetVal RetVal;
typedef void ( *Function )( RetVal* );

typedef enum {
    RET_TYPE_INT,
    RET_TYPE_FLOAT,
    RET_TYPE_NONE
} RetType;

struct Menu {
    const char *header;
    const int menuCount;
    const char *menu[6];
    const Function hoverFunction[6];
    Menu *subMenu[6];
    Menu *parentMenu;
};

struct RetVal {
    int intRet;
    float floatRet;
    RetType retType;
};

void fullLcdInit( void );
void upKeyPress( void );
void downKeyPress( void );
void parentMenu( void );
void childMenu( void );
void refreshScreen( void );
void flashScreen( void );
void noOp( RetVal *retVal );
void createMenu( Menu *menu );