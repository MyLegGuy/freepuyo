/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <goodbrew/config.h>
#include <goodbrew/text.h>
#include <goodbrew/base.h>
#include <goodbrew/images.h>
#if !defined M_PI
	#warning makeshift M_PI
	#define M_PI 3.14159265358979323846
#endif

#if !defined M_PI_2
	#warning makeshift M_PI_2
	#define M_PI_2 (M_PI/(double)2)
#endif
// Macros
#define HALFTILE (tilew/2)
#define UNSET_FLAG(_holder, _mini) _holder&=(0xFFFF ^ _mini)
// Constants
typedef enum{
	BOARD_NONE=0,
	BOARD_PUYO,
	BOARD_YOSHI,
	BOARD_MAX,
}boardType;
typedef enum{
	MODE_UNDEFINED=0,
	MODE_ENDLESS,
	MODE_BATTLE,
	MODE_GOAL,
}gameMode;
//
#define DIR_NONE	0b00000000
#define DIR_UP 	 	0b00000001
#define DIR_LEFT 	0b00000010
#define DIR_DOWN 	0b00000100
#define DIR_RIGHT 	0b00001000
//
#define AISOFTDROPS 1
// The most you could've dragged for it to still register as a tap
#define MAXTAPSCREENRATIO .1
// Divide screen height by this and that's the min of screen height you need to drag to do soft drop
#define TOUCHDROPDENOMINATOR 5
//
extern int curFontHeight;
extern crossFont regularFont;
// How much you need to touch drag to move one tile on the x axis. Updated with screen size.
extern int widthDragTile;
// Minimum amount to drag to activate softdrop. Updated with screen size.
extern int softdropMinDrag;
extern int screenWidth;
extern int screenHeight;

struct boardController;
struct gameState;
// void* _stateData, struct gameState* _curGameState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, u64 _sTime
typedef void(*boardControlFunc)(void*,struct gameState*,void*,signed char,int,int,int,u64);
struct boardController{
	boardControlFunc func;
	void* data;
};
struct gameState{
	int numBoards;
	void** boardData;
	int* boardPosX;
	int* boardPosY;
	struct boardController* controllers;
	boardType* types;
	gameMode mode;
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
double getBoardW(void* _passedBoard, boardType _passedType);
double getBoardH(void* _passedBoard, boardType _passedType);
double getMaxStateHeight(struct gameState* _passedState);
char getRelation(int _newX, int _newY, int _oldX, int _oldY);
void getRelationCoords(int _newX, int _newY, int _oldX, int _oldY, int* _retX, int* _retY);
int getStateIndexOfBoard(struct gameState* _passedState, void* _passedBoard);
double getStateWidth(struct gameState* _passedState);
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
void updateBoard(void* _passedBoard, boardType _passedType, int _drawX, int _drawY, struct gameState* _passedState, struct boardController* _passedController, u64 _sTime);
void updateGameState(struct gameState* _passedState, u64 _sTime);
void XOutFunction();
void sendGarbage(struct gameState* _passedState, void* _source, int _newGarbageSent);
void stateApplyGarbage(struct gameState* _passedState, void* _source);
char touchIn(int _touchX, int _touchY, int _boxX, int _boxY, int _boxW, int _boxH);
int getOtherScaled(int _orig, int _scaled, int _altDim);
void fitInBox(int _imgW, int _imgH, int _boxW, int _boxH, int* _retW, int* _retH);
void recalculateGameStatePos(struct gameState* _passedState);
double getBoardWMain(void* _passedBoard, boardType _passedType);
double getBoardWSub(void* _passedBoard, boardType _passedType);
int fixWithExcluded(int _passedIn, int _passedExcluded);
