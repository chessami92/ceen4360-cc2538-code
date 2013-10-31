typedef struct Menu Menu;
typedef struct RetVal RetVal;
typedef void ( *Function )( RetVal* );

typedef enum {
    RET_TYPE_INT,
    RET_TYPE_FLOAT
} RetType;

struct Menu {
    const char *header;
    const int menuCount;
    const char *menu[6];
    const Function hoverFunction[6];
    const Menu *subMenu[6];
    const Menu *parentMenu;
};

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
void createMenu( Menu *menu );