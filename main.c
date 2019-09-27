/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
If it takes 16 milliseconds for a frame to pass and we only needed 1 millisecond to progress to the next tile, we can't just say the puyo didn't move for those other 15 milliseconds. We need to account for those 15 milliseconds for the puyo's next falling down action. The difference between the actual finish time and the expected finish time is stored back in the complete dest time variable. In this case, 15 would be stored. We'd take that 15 and account for it when setting any more down movement time values for this frame. But only if we're going to do more down moving this frame. Otherwise we throw that 15 value away at the end of the frame. Anyway, 4 is the bit that indicates that these values were set.
*/
// TODO - Hitting a stack that's already squishing wrong behavior
// TODO - Maybe a board can have a pointer to a function to get the next piece. I can map it to either network or random generator
// TODO - Draw board better. Have like a wrapper struct drawableBoard where elements can be repositioned or remove.
// TODO - Only check for potential pops on piece move?
// TODO - 2p in vetical mode.  put second board on the top left partially transparent
// TODO - Put score and garbage queue in extra space on the right?
// TODO - tap registers on release?
// TODO - touch button repeat?

#define __USE_MISC // enable MATH_PI_2
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <goodbrew/config.h>
#include <goodbrew/base.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include <goodbrew/images.h>
#include <goodbrew/text.h>
#include <goodbrew/useful.h>
#include <goodbrew/paths.h>

#include "main.h"
#include "yoshi.h"
#include "puyo.h"
#include "menu.h"

// Internal use only functions
void rebuildSizes(double _w, double _h, double _tileRatioPad);
// Internal use only constantsw
#define FPSCOUNT 1
#define SHOWFPSCOUNT 0
// Internal use variables
static u64 _globalReferenceMilli;

// Globals
int curFontHeight;
static int tilew = 45;
crossFont regularFont;
int widthDragTile;
int softdropMinDrag;
int screenWidth;
int screenHeight;
char* vitaAppId="FREEPUYOV";
char* androidPackageName = "com.mylegguy.freepuyo";
//////////////////////////////////////////////////////////
crossTexture loadImageEmbedded(const char* _path){
	char* _realPath = fixPathAlloc(_path,TYPE_EMBEDDED);
	crossTexture _ret = loadImage(_realPath);
	free(_realPath);
	return _ret;
}
// inclusive
int randInt(int _lowBound, int _highBound){
	return rand()%(_highBound-_lowBound+1)+_lowBound;
}
long minCap(long _passed, long _min){
	if (_passed<_min){
		return _min;
	}
	return _passed;
}
int intCap(int _passed, int _min, int _max){
	if (_passed<_min){
		return _min;
	}else if (_passed>_max){
		return _max;
	}
	return _passed;
}
long cap(long _passed, long _min, long _max){
	if (_passed<_min){
		return _min;
	}else if (_passed>_max){
		return _max;
	}
	return _passed;
}
int easyCenter(int _smallSize, int _bigSize){
	return (_bigSize-_smallSize)/2;
}
// get relation of < > to < >
// _newX, _newY relation to _oldX, _oldY
char getRelation(int _newX, int _newY, int _oldX, int _oldY){
	char _ret = 0;
	if (_newX!=_oldX){
		if (_newX<_oldX){ // left
			_ret|=DIR_LEFT;
		}else{ // right
			_ret|=DIR_RIGHT;
		}
	}
	if (_newY!=_oldY){
		if (_newY<_oldY){ // up
			_ret|=DIR_UP;
		}else{ // down
			_ret|=DIR_DOWN;
		}
	}
	return _ret;
}
// would return a negative number if _newX is to the left of _oldX
void getRelationCoords(int _newX, int _newY, int _oldX, int _oldY, int* _retX, int* _retY){
	*_retX=(_newX-_oldX);
	*_retY=(_newY-_oldY);
}
// For the real version, we can disable fix coords
int fixX(int _passedX){
	return _passedX;
}
int fixY(int _passedY){
	return _passedY;
}
double partMoveFills(u64 _curTicks, u64 _destTicks, int _totalDifference, double _max){
	return ((_totalDifference-(_destTicks-_curTicks))/(double)_totalDifference)*_max;
}
double partMoveEmptys(u64 _curTicks, u64 _destTicks, int _totalDifference, double _max){
	return _max-partMoveFills(_curTicks,_destTicks,_totalDifference,_max);
}
double partMoveEmptysCapped(u64 _curTicks, u64 _destTicks, int _totalDifference, double _max){
	double _ret = partMoveEmptys(_curTicks,_destTicks,_totalDifference,_max);
	if (_ret<0){
		return _max;
	}else{
		return _ret;
	}
}
u64 goodGetMilli(){
	return getMilli()-_globalReferenceMilli;
}
void XOutFunction(){
	exit(0);
}
char touchIn(int _touchX, int _touchY, int _boxX, int _boxY, int _boxW, int _boxH){
	return (_touchX>_boxX && _touchX<_boxX+_boxW && _touchY>_boxY && _touchY<_boxY+_boxH);
}
int getOtherScaled(int _orig, int _scaled, int _altDim){
	return _altDim*(_scaled/(double)_orig);
}
void fitInBox(int _imgW, int _imgH, int _boxW, int _boxH, int* _retW, int* _retH){
	if ((_boxW/(double)_imgW) < (_boxH/(double)_imgH)){
		*_retW=_boxW;
		*_retH=getOtherScaled(_imgW,_boxW,_imgH);
	}else{
		*_retW=getOtherScaled(_imgH,_boxH,_imgW);
		*_retH=_boxH;
	}
}
//////////////////////////////////////////////////
// generic bindings
//////////////////////////////////////////////////
// Visual width
double getBoardW(void* _passedBoard, boardType _passedType){
	switch(_passedType){
		case BOARD_PUYO:
			return ((struct puyoBoard*)_passedBoard)->lowBoard.w+NEXTWINDOWTILEW+1; // the border total perfectly aligns to +1 tile
			break;
		case BOARD_YOSHI:
			return ((struct yoshiBoard*)_passedBoard)->lowBoard.w*YOSHI_TILE_SCALE;
			break;
	}
	return 0;
}
// Visual height
double getBoardH(void* _passedBoard, boardType _passedType){
	switch(_passedType){
		case BOARD_PUYO:
			// plus top and bottom borders
			return ((struct puyoBoard*)_passedBoard)->lowBoard.h-((struct puyoBoard*)_passedBoard)->numGhostRows+2+PUYOBORDERSMALLSZDEC*2;
			break;
		case BOARD_YOSHI:
			return (((struct yoshiBoard*)_passedBoard)->lowBoard.h+YOSHINEXTNUM-YOSHINEXTOVERLAPH)*YOSHI_TILE_SCALE+SWAPDUDESMALLTILEH;
			break;
	}
	return 0;
}
void updateBoard(void* _passedBoard, boardType _passedType, int _drawX, int _drawY, struct gameState* _passedState, struct boardController* _passedController, u64 _sTime){
	switch (_passedType){
		case BOARD_PUYO:
			;
			signed char _updateRet = updatePuyoBoard(_passedBoard,_passedState,((struct puyoBoard*)_passedBoard)->lowBoard.status==STATUS_NORMAL ? 0 : -1,_sTime);
			_passedController->func(_passedController->data,_passedState,_passedBoard,_updateRet,_drawX,_drawY,tilew,_sTime);
			endFrameUpdateBoard(_passedBoard,_updateRet); // TODO - Move this to frame end?
			break;
		case BOARD_YOSHI:
			updateYoshiBoard(_passedBoard,_sTime);
			_passedController->func(_passedController->data,_passedState,_passedBoard,0,_drawX,_drawY,tilew,_sTime);
	}
}
void drawBoard(void* _passedBoard, boardType _passedType, int _startX, int _startY, u64 _sTime){
	switch(_passedType){
		case BOARD_PUYO:
			drawPuyoBoard(_passedBoard,_startX,_startY,1,tilew,_sTime);
			break;
		case BOARD_YOSHI:
			drawYoshiBoard(_passedBoard,_startX,_startY,tilew,_sTime);
			break;
	}
}
void startBoard(void* _passedBoard, boardType _passedType, u64 _sTime){
	switch(_passedType){
		case BOARD_PUYO:
			transitionBoardNextWindow(_passedBoard,_sTime);
			break;
		case BOARD_YOSHI:
			yoshiSpawnNext(_passedBoard,_sTime);
	}
}
void boardAddIncoming(void* _passedBoard, boardType _passedType, int _amount, int _sourceIndex, boardType _sourceType){
	switch (_passedType){
		case BOARD_PUYO:
			((struct puyoBoard*)_passedBoard)->incomingGarbage[_sourceIndex]+=_amount;
			break;
	}
}
// Applys the pending garbage from the specified index
void boardApplyGarbage(void* _passedBoard, boardType _passedType, int _applyIndex){
	switch(_passedType){
		case BOARD_PUYO:
			((struct puyoBoard*)_passedBoard)->readyGarbage+=((struct puyoBoard*)_passedBoard)->incomingGarbage[_applyIndex];
			((struct puyoBoard*)_passedBoard)->incomingGarbage[_applyIndex]=0;
			break;
	}
}
//////////////////////////////////////////////////
// gameState
//////////////////////////////////////////////////
double getMaxStateHeight(struct gameState* _passedState){
	if (_passedState->numBoards==0){
		return 0;
	}
	double _biggest=getBoardH(_passedState->boardData[0],_passedState->types[0]);
	int i;
	for (i=1;i<_passedState->numBoards;++i){
		double _curHeight = getBoardH(_passedState->boardData[i],_passedState->types[i]);
		if (_curHeight>_biggest){
			_biggest=_curHeight;
		}
	}
	return _biggest;
}
double getStateWidth(struct gameState* _passedState){
	double _ret=0;
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		_ret+=getBoardW(_passedState->boardData[i],_passedState->types[i]);
	}
	return _ret;
}
void rebuildGameState(struct gameState* _passedState, u64 _sTime){
	rebuildSizes(getStateWidth(_passedState),getMaxStateHeight(_passedState),1);
	recalculateGameStatePos(_passedState);
}
int getStateIndexOfBoard(struct gameState* _passedState, void* _passedBoard){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		if (_passedState->boardData[i]==_passedBoard){
			return i;
		}
	}
	printf("Board not found\n");;
	return -1;
}
struct gameState newGameState(int _count){
	struct gameState _ret;
	_ret.numBoards=_count;
	_ret.boardData = malloc(sizeof(void*)*_count);
	_ret.boardPosX = malloc(sizeof(int)*_count);
	_ret.boardPosY = malloc(sizeof(int)*_count);
	_ret.controllers = malloc(sizeof(struct boardController)*_count);
	_ret.types = malloc(sizeof(boardType)*_count);
	return _ret;
}
// Use after everything is set up
void endStateInit(struct gameState* _passedState){
	int i;
	// This init is done here because it may change one day to require more than just the number of other boards, like team data.
	for (i=0;i<_passedState->numBoards;++i){
		switch(_passedState->types[i]){
			case BOARD_PUYO:
				((struct puyoBoard*)_passedState->boardData[i])->incomingLength = _passedState->numBoards;
				((struct puyoBoard*)_passedState->boardData[i])->incomingGarbage = calloc(1,sizeof(int)*_passedState->numBoards);
				break;
		}
	}
}
void startGameState(struct gameState* _passedState, u64 _sTime){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		startBoard(_passedState->boardData[i],_passedState->types[i],_sTime);
	}
}
void updateGameState(struct gameState* _passedState, u64 _sTime){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		updateBoard(_passedState->boardData[i],_passedState->types[i],_passedState->boardPosX[i],_passedState->boardPosY[i],_passedState,&(_passedState->controllers[i]),_sTime);
	}
}

void drawGameState(struct gameState* _passedState, u64 _sTime){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		drawBoard(_passedState->boardData[i],_passedState->types[i],_passedState->boardPosX[i],_passedState->boardPosY[i],_sTime);
	}
}
// This board's chain is up. Apply all its garbage to its targets.
void stateApplyGarbage(struct gameState* _passedState, void* _source){
	int _applyIndex = getStateIndexOfBoard(_passedState,_source);
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		boardApplyGarbage(_passedState->boardData[i],_passedState->types[i],_applyIndex);
	}
}
void recalculateGameStatePos(struct gameState* _passedState){
	int _boardSeparation=(screenWidth-getStateWidth(_passedState)*tilew)/(_passedState->numBoards+1);
	int _curX = _boardSeparation;
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		_passedState->boardPosX[i] = _curX;
		_passedState->boardPosY[i] = screenHeight/2-(getBoardH(_passedState->boardData[i],_passedState->types[i])*tilew)/2;
		_curX+=getBoardW(_passedState->boardData[i],_passedState->types[i])*tilew+_boardSeparation;
	}
}
//////////////////////////////////////////////////
// 1 if you should quit
// Updates both variables
// Won't update _myGarbage if you're not going to keep going
char _lowOffsetGarbage(int* _enemyGarbage, int* _myGarbage){
	if (*_enemyGarbage==0){
		return 0;
	}
	if (*_myGarbage>*_enemyGarbage){
		*_myGarbage=*_myGarbage-*_enemyGarbage;
		*_enemyGarbage = 0;
		return 0;
	}else{
		*_enemyGarbage = *_enemyGarbage-*_myGarbage;
		return 1;
	}
}
// _newGarbageSent refers to only the new garbage, not the total for this chain.
// todo - maybe return type of animation.
// positibilies: garbage offset, garbage counter, garbage attack other boards
// multiple animations at once, like garbage counter and garbage attack other boards at once.
void sendGarbage(struct gameState* _passedState, void* _source, int _newGarbageSent){
	int _sourceIndex = getStateIndexOfBoard(_passedState,_source);
	// Offset if supported
	switch(_passedState->types[_sourceIndex]){
		case BOARD_PUYO:
			if (((struct puyoBoard*)_source)->readyGarbage!=0){
				if (_lowOffsetGarbage(&((struct puyoBoard*)_source)->readyGarbage,&_newGarbageSent)){
					return;
				}
			}
			int i;
			for (i=0;i<((struct puyoBoard*)_source)->incomingLength;++i){
				if (_lowOffsetGarbage(&(((struct puyoBoard*)_source)->incomingGarbage)[i],&_newGarbageSent)){
					return;
				}
			}
			break;
	}
	// Attack using leftover garbage
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		if (i!=_sourceIndex){
			boardAddIncoming(_passedState->boardData[i],_passedState->types[i],_newGarbageSent,_sourceIndex,_passedState->types[_sourceIndex]);
		}
	}
}
void rebuildSizes(double _w, double _h, double _tileRatioPad){
	screenWidth = getScreenWidth();
	screenHeight = getScreenHeight();

	int _fitWidthSize = screenWidth/(double)(_w+_tileRatioPad*2);
	int _fitHeightSize = screenHeight/(double)(_h+_tileRatioPad*2);
	tilew = _fitWidthSize<_fitHeightSize ? _fitWidthSize : _fitHeightSize;

	//todo #warning fix widthDragTile
	widthDragTile=((_w+NEXTWINDOWTILEW)*tilew)/_w;
	softdropMinDrag=screenHeight/TOUCHDROPDENOMINATOR;
}
void init(){
	srand(time(NULL));
	generalGoodInit();
	_globalReferenceMilli = goodGetMilli();
	initGraphics(480,640,WINDOWFLAG_RESIZABLE);
	screenWidth = getScreenWidth();
	screenHeight = getScreenHeight();
	initImages();
	setWindowTitle("Test happy");
	setClearColor(0,0,0);
	char* _fixedPath = fixPathAlloc("assets/liberation-sans-bitmap.sfl",TYPE_EMBEDDED);
	regularFont = loadFont(_fixedPath,-1);
	curFontHeight = textHeight(regularFont);
	free(_fixedPath);
}
int main(int argc, char* argv[]){
	init();
	struct gameState _testState;
	_testState.numBoards=0;
	titleScreen(&_testState);
	//
	rebuildGameState(&_testState,goodGetMilli());
	startGameState(&_testState,goodGetMilli());
	#if FPSCOUNT == 1
	u64 _frameCountTime = goodGetMilli();
	int _frames=0;
	#endif
	setJustPressed(BUTTON_RESIZE);
	crossTexture _curBg = loadImageEmbedded("assets/bg/Sunrise.png");
	while(1){
		u64 _sTime = goodGetMilli();
		controlsStart();
		if (isDown(BUTTON_RESIZE)){ // Impossible for BUTTON_RESIZE for two frames, so just use isDown
			rebuildGameState(&_testState,_sTime);
		}
		updateGameState(&_testState,_sTime);
		controlsEnd();
		startDrawing();
		drawTextureSized(_curBg,0,0,screenWidth,screenHeight);		
		drawGameState(&_testState,_sTime);
		endDrawing();
		#if FPSCOUNT
			++_frames;
			if (goodGetMilli()>=_frameCountTime+1000){
				_frameCountTime=goodGetMilli();
				#if SHOWFPSCOUNT
					printf("%d\n",_frames);
				#else
					if (_frames<60){
						printf("Slowdown %d\n",_frames);
					}
				#endif
				_frames=0;
			}
		#endif
	}
	generalGoodQuit();
	return 0;
}
