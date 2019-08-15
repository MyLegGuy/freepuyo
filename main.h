#if !defined M_PI
	#warning makeshift M_PI
	#define M_PI 3.14159265358979323846
#endif

#if !defined M_PI_2
	#warning makeshift M_PI_2
	#define M_PI_2 (M_PI/(double)2)
#endif

// Macros
#define tilew tileh
#define HALFTILE (tilew/2)
#define UNSET_FLAG(_holder, _mini) _holder&=(0xFFFF ^ _mini)
// Constants
typedef enum{
	BOARD_NONE=0,
	BOARD_PUYO,
	BOARD_MAX,
}boardType;
//
#define DIR_NONE	0b00000000
#define DIR_UP 	 	0b00000001
#define DIR_LEFT 	0b00000010
#define DIR_DOWN 	0b00000100
#define DIR_RIGHT 	0b00001000
//
#define AUTOTILEDOWN 	0b00000001
#define AUTOTILEUP 		0b00000010
#define AUTOTILERIGHT 	0b00000100
#define AUTOTILELEFT	0b00001000
// For these movement flags, it's already been calculated that moving that way is okay
#define FLAG_MOVEDOWN 	0b00000001
#define FLAG_MOVELEFT 	0b00000010
#define FLAG_MOVERIGHT 	0b00000100
#define FLAG_ROTATECW 	0b00001000 // clockwise
#define FLAG_ROTATECC 	0b00010000 // counter clock
#define FLAG_DEATHROW	0b00100000 // Death row for being a moving puyo, I mean. If this puyo has hit the puyo under it and is about to die if it's not moved
//
#define FLAG_ANY_HMOVE 	(FLAG_MOVELEFT | FLAG_MOVERIGHT)
#define FLAG_ANY_ROTATE (FLAG_ROTATECW | FLAG_ROTATECC)
//
#define AISOFTDROPS 1
// The most you could've dragged for it to still register as a tap
#define MAXTAPSCREENRATIO .1
// Divide screen height by this and that's the min of screen height you need to drag to do soft drop
#define TOUCHDROPDENOMINATOR 5

extern int tileh;
extern crossFont regularFont;
// How much you need to touch drag to move one tile on the x axis. Updated with screen size.
extern int widthDragTile;
// Minimum amount to drag to activate softdrop. Updated with screen size.
extern int softdropMinDrag;
extern int screenWidth;
extern int screenHeight;

struct boardController;
struct gameState;
typedef void(*boardControlFunc)(void*,struct gameState*,void*,signed char,u64);
struct boardController{
	boardControlFunc func;
	void* data;
};
struct gameState{
	int numBoards;
	void** boardData;
	struct boardController* controllers;
	void** settings;
	boardType* types;
};

char _lowOffsetGarbage(int* _enemyGarbage, int* _myGarbage);
void boardAddIncoming(void* _passedBoard, boardType _passedType, int _amount, int _sourceIndex, boardType _sourceType);
void boardApplyGarbage(void* _passedBoard, boardType _passedType, int _applyIndex);
long cap(long _passed, long _min, long _max);
void drawBoard(void* _passedBoard, boardType _passedType, int _startX, int _startY, u64 _sTime);
void drawGameState(struct gameState* _passedState, u64 _sTime);
int easyCenter(int _smallSize, int _bigSize);
void endStateInit(struct gameState* _passedState);
int fixX(int _passedX);
int fixY(int _passedY);
short getBoardH(void* _passedBoard, boardType _passedType);
short getBoardW(void* _passedBoard, boardType _passedType);
int getMaxStateHeight(struct gameState* _passedState);
char getRelation(int _newX, int _newY, int _oldX, int _oldY);
void getRelationCoords(int _newX, int _newY, int _oldX, int _oldY, int* _retX, int* _retY);
int getStateIndexOfBoard(struct gameState* _passedState, void* _passedBoard);
int getStateWidth(struct gameState* _passedState);
u64 goodGetMilli();
void init();
int intCap(int _passed, int _min, int _max);
crossTexture loadImageEmbedded(const char* _path);
int main(int argc, char* argv[]);
long minCap(long _passed, long _min);
struct gameState newGameState(int _count);
double partMoveEmptysCapped(u64 _curTicks, u64 _destTicks, int _totalDifference, double _max);
double partMoveEmptys(u64 _curTicks, u64 _destTicks, int _totalDifference, double _max);
double partMoveFills(u64 _curTicks, u64 _destTicks, int _totalDifference, double _max);
int randInt(int _lowBound, int _highBound);
void rebuildBoard(void* _passedBoard, boardType _passedType, u64 _sTime);
void rebuildGameState(struct gameState* _passedState, u64 _sTime);
void startBoard(void* _passedBoard, boardType _passedType, u64 _sTime);
void startGameState(struct gameState* _passedState, u64 _sTime);
void updateBoard(void* _passedBoard, boardType _passedType, struct gameState* _passedState, struct boardController* _passedController, u64 _sTime);
void updateGameState(struct gameState* _passedState, u64 _sTime);
void XOutFunction();
void sendGarbage(struct gameState* _passedState, void* _source, int _newGarbageSent);
void stateApplyGarbage(struct gameState* _passedState, void* _source);
