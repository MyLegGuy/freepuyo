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
// TODO - Right edge being less thick than left edge makes everything look uncentered
// TODO - Wrong chain notification position because using the left edge of the block for positions
// TODO - remove numActiveSets from puyo? check if 0 sets by checking if set list is NULL

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
#include "heal.h"
#include "puyo.h"
#include "menu.h"
#include "skinLoader.h"

// Internal use only functions
void rebuildSizes(double _w, double _h, double _tileRatioPad);
// Internal use only constantsw
#define FPSCOUNT 1
#define SHOWFPSCOUNT 0
// Internal use variables
u64 cachedTimeRes;

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
crossTexture* preparingImages;
void* loadedSkins[BOARD_MAX];

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
double partMoveFills(u64 _curTicks, u64 _destTicks, u64 _totalDifference, double _max){
	return ((_totalDifference-(_destTicks-_curTicks))/(double)_totalDifference)*_max;
}
double partMoveEmptys(u64 _curTicks, u64 _destTicks, u64 _totalDifference, double _max){
	return _max-partMoveFills(_curTicks,_destTicks,_totalDifference,_max);
}
double partMoveEmptysCapped(u64 _curTicks, u64 _destTicks, u64 _totalDifference, double _max){
	double _ret = partMoveEmptys(_curTicks,_destTicks,_totalDifference,_max);
	if (_ret<0){
		return _max;
	}else{
		return _ret;
	}
}
u64 fixTime(int _in){
	return (_in/(double)1000)*cachedTimeRes;
}
u64 goodGetHDTime(){
	return getHDTime();
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
int fixWithExcluded(int _passedIn, int _passedExcluded){
	if (_passedIn<_passedExcluded){
		return _passedIn;
	}
	return _passedIn+1;
}
//////////////////////////////////////////////////
crossTexture* loadPreparingImages(){
	crossTexture* _ret = malloc(sizeof(crossTexture)*PREPARECOUNT);
	char* _basePath = fixPathAlloc("assets/1.png",TYPE_EMBEDDED);
	int _numIndex = strlen(_basePath)-5;
	int i;
	for (i=0;i<PREPARECOUNT;++i){
		_basePath[_numIndex]='0'+i+1;
		_ret[i]=loadImage(_basePath);
	}
	free(_basePath);
	return _ret;
}
void freePreparingImages(crossTexture* _imgs){
	int i;
	for (i=0;i<PREPARECOUNT;++i){
		freeTexture(_imgs[i]);
	}
	free(_imgs);
}
void drawCountdown(void* _passedBoard, boardType _passedType, int _boardX, int _boardY, crossTexture _passedImg){
	int _destW;
	int _destH;
	int _boardW = getBoardWMain(_passedBoard,_passedType)*tilew;
	int _boardH = getBoardH(_passedBoard,_passedType)*tilew;
	fitInBox(getTextureWidth(_passedImg),getTextureHeight(_passedImg),_boardW,_boardH/3,&_destW,&_destH);
	drawTextureSized(_passedImg,_boardX+easyCenter(_destW,_boardW),_boardY+easyCenter(_destH,_boardH),_destW,_destH); //
}
void loadGameSkin(boardType _type){
	if (loadedSkins[_type]){
		return;
	}
	switch(_type){
		case BOARD_PUYO:
			loadedSkins[BOARD_PUYO]=malloc(sizeof(struct puyoSkin));
			*((struct puyoSkin*)loadedSkins[BOARD_PUYO])=loadChampionsSkinFile(loadImageEmbedded("assets/freepuyo.png"));;
			break;
		case BOARD_YOSHI:
			loadedSkins[BOARD_YOSHI]=malloc(sizeof(struct yoshiSkin));
			loadYoshiSkin(loadedSkins[BOARD_YOSHI],"assets/Crates/yoshiSheet.png");
			break;
		case BOARD_HEAL:
			loadedSkins[BOARD_HEAL]=malloc(sizeof(struct healSkin));
			printf("load heal skin goes here\n");
			break;
	}
}
void loadNeededSkins(struct gameState* _passedState){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		loadGameSkin(_passedState->types[i]);
	}
}
void freeUselessSkins(struct gameState* _passedState){
	char _isUsed[BOARD_MAX];
	memset(_isUsed,0,sizeof(char)*BOARD_MAX);
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		_isUsed[_passedState->types[i]]=1;
	}
	// manual
	if (loadedSkins[BOARD_PUYO] && !_isUsed[BOARD_PUYO]){
		freePuyoSkin(loadedSkins[BOARD_PUYO]);
		free(loadedSkins[BOARD_PUYO]);
		loadedSkins[BOARD_PUYO]=NULL;
	}
	if (loadedSkins[BOARD_YOSHI] && !_isUsed[BOARD_YOSHI]){
		freeYoshiSkin(loadedSkins[BOARD_YOSHI]);
		free(loadedSkins[BOARD_YOSHI]);
		loadedSkins[BOARD_YOSHI]=NULL;
	}
}
//////////////////////////////////////////////////
// generic bindings
//////////////////////////////////////////////////
/*
int scaleGarbage(int _sourceCount, boardType _sourceType, boardType _destType){
	if (_destType==BOARD_PUYO){
		switch(_sourceType){
			case BOARD_PUYO:
			case BOARD_HEAL:
				return _sourceCount;
		}
	}else if (_destType==BOARD_HEAL){
		switch(_sourceType){
			case BOARD_PUYO:
				return _sourceCount/6;
			case BORAD_HEAL:
				return _sourceCount/2;
		}
	}
	return 0;
}
*/
// visual width
double getBoardWMain(void* _passedBoard, boardType _passedType){
	switch(_passedType){
		case BOARD_PUYO:
			return ((struct puyoBoard*)_passedBoard)->lowBoard.w+
				PUYOBORDERSMALLSZDEC*2; // main border around board
		case BOARD_YOSHI:
			return ((struct yoshiBoard*)_passedBoard)->lowBoard.w*YOSHI_TILE_SCALE;
		case BOARD_HEAL:
			return ((struct healBoard*)_passedBoard)->lowBoard.w;
	}
	return 0;
}
double getBoardWSub(void* _passedBoard, boardType _passedType){
	switch(_passedType){
		case BOARD_PUYO:
			return NEXTWINDOWTILEW+
				PUYOBORDERSMALLSZDEC*4; // padding between board and next box + next box boarder + padding within nextbox
		case BOARD_HEAL:
		case BOARD_YOSHI:
			return 0;
	}
	return 0;
}
struct genericBoard* getLowBoard(void* _passedBoard, boardType _passedType){
	switch (_passedType){
		case BOARD_PUYO:
			return &((struct puyoBoard*)_passedBoard)->lowBoard;
		case BOARD_YOSHI:
			return &((struct yoshiBoard*)_passedBoard)->lowBoard;
		case BOARD_HEAL:
			return &((struct healBoard*)_passedBoard)->lowBoard;
	}
	return NULL;
}
boardStatus getStatus(void* _passedBoard, boardType _passedType){
	return getLowBoard(_passedBoard,_passedType)->status;
}
// Visual width
double getBoardW(void* _passedBoard, boardType _passedType){
	return getBoardWMain(_passedBoard,_passedType)+getBoardWSub(_passedBoard,_passedType);
}
// Visual height
double getBoardH(void* _passedBoard, boardType _passedType){
	switch(_passedType){
		case BOARD_PUYO:
			// plus top and bottom borders
			return ((struct puyoBoard*)_passedBoard)->lowBoard.h-((struct puyoBoard*)_passedBoard)->numGhostRows+2+PUYOBORDERSMALLSZDEC*2;
		case BOARD_YOSHI:
			return (((struct yoshiBoard*)_passedBoard)->lowBoard.h+YOSHINEXTNUM-YOSHINEXTOVERLAPH)*YOSHI_TILE_SCALE+SWAPDUDESMALLTILEH;
		case BOARD_HEAL:
			return ((struct healBoard*)_passedBoard)->lowBoard.h;
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
			updateYoshiBoard(_passedBoard,_passedState->mode,_sTime);
			_passedController->func(_passedController->data,_passedState,_passedBoard,0,_drawX,_drawY,tilew,_sTime);
			break;
		case BOARD_HEAL:
			updateHealBoard(_passedState,_passedBoard,_passedState->mode,_sTime);
			_passedController->func(_passedController->data,_passedState,_passedBoard,0,_drawX,_drawY,tilew,_sTime);
			break;
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
		case BOARD_HEAL:
			drawHealBoard(_passedBoard,_startX,_startY,tilew,_sTime);
			break;
	}
}
void startBoard(void* _passedBoard, boardType _passedType, u64 _sTime){
	switch(_passedType){
		case BOARD_PUYO:
			transitionBoardNextWindow(_passedBoard,_sTime);
			break;
		case BOARD_YOSHI:
			((struct yoshiBoard*)_passedBoard)->lowBoard.status=STATUS_NORMAL;
			yoshiSpawnNext(_passedBoard,_sTime);
			break;
		case BOARD_HEAL:
			((struct healBoard*)_passedBoard)->lowBoard.status=STATUS_NORMAL;
			printf("init heal board\n");
			break;
	}
}
void boardAddIncoming(void* _passedBoard, boardType _passedType, int _amount, int _sourceIndex, boardType _sourceType){
	switch (_passedType){
		case BOARD_PUYO:
			((struct puyoBoard*)_passedBoard)->incomingGarbage[_sourceIndex]+=_amount;
			break;
		case BOARD_HEAL:
			printf("heal got incoming garbage\n");
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
		case BOARD_HEAL:
			printf("apply garbage to heal board\n");
			break;
	}
}
void resetBoard(void* _passedBoard, boardType _passedType){
	switch(_passedType){
		case BOARD_PUYO:
			resetPuyoBoard(_passedBoard);
			break;
		case BOARD_YOSHI:
			resetYoshiBoard(_passedBoard);
			break;
		case BOARD_HEAL:
			printf("reset heal board\n");
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
	_ret.initializers = calloc(1,sizeof(boardInitializer)*_count); // assumes null by default
	_ret.initializerInfo = calloc(1,sizeof(void*)*_count); // assumes null by default
	_ret.types = malloc(sizeof(boardType)*_count);
	_ret.mode=MODE_UNDEFINED;
	_ret.status=MAJORSTATUS_POSTGAME;
	return _ret;
}
void freeGameState(struct gameState* _passedState){
	int i;
	// free initializers and board positions.
	for (i=0;i<_passedState->numBoards;++i){
		free(_passedState->initializerInfo[i]);
	}
	free(_passedState->initializerInfo);
	free(_passedState->initializers);
	free(_passedState->boardPosX);
	free(_passedState->boardPosY);
	// free board main data
	for (i=0;i<_passedState->numBoards;++i){
		switch(_passedState->types[i]){
			case BOARD_PUYO:
				freePuyoBoard(_passedState->boardData[i]);
				break;
			case BOARD_YOSHI:
				freeYoshiBoard(_passedState->boardData[i]);
				break;
		}
		free(_passedState->boardData[i]);
	}
	free(_passedState->boardData);
	free(_passedState->types);


	// free board controllers. 
	for (i=0;i<_passedState->numBoards;++i){
		/*
		//figure out what to free depending on the function it points to.
		switch(_passedState->controllers[i].func){
			case updateControlSet: // data is a controlSet
			case yoshiUpdateControlSet:
				break;
			case updatePuyoAi: // data is aiState
				break;
		}
		*/
		// as of now, no board controllers point to a data struct that requires extra freeing.
		free(_passedState->controllers[i].data);
	}
	free(_passedState->controllers);
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
	_passedState->status=MAJORSTATUS_NORMAL;
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		startBoard(_passedState->boardData[i],_passedState->types[i],_sTime);
	}
}
void updateGameState(struct gameState* _passedState, u64 _sTime){
	if (_passedState->status==MAJORSTATUS_POSTGAME){
		menuProcess();
		return;
	}
	int _numDead=0;
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		updateBoard(_passedState->boardData[i],_passedState->types[i],_passedState->boardPosX[i],_passedState->boardPosY[i],_passedState,&(_passedState->controllers[i]),_sTime);
		switch(getStatus(_passedState->boardData[i],_passedState->types[i])){
			case STATUS_DEAD:
				++_numDead;
				break;
			case STATUS_WON:
				_numDead=-1;
				i=_passedState->numBoards;
				break;
		}
	}
	// Detect win by goal or win by opponents dead.
	if (_numDead!=0){ // can't win by opponents dead if only one player
		if (_numDead==-1 || _numDead>=_passedState->numBoards-1){
			boardStatus _playerStatus = getStatus(_passedState->boardData[0],_passedState->types[0]);
			if (_playerStatus==STATUS_DEAD || (_numDead==-1 && _playerStatus!=STATUS_WON)){
				spawnLoseMenu(_passedState,_sTime);
			}else{
				spawnWinMenu(_passedState,_sTime);
			}
			_passedState->status=MAJORSTATUS_POSTGAME;
		}
	}
	if (_passedState->status==MAJORSTATUS_PREPARING){ // process countdown if needed
		if (_sTime>=_passedState->statusTime){
			startGameState(_passedState,_sTime); // also changes status for us
		}
	}
}
void setGameStatePreparing(struct gameState* _passedState, u64 _sTime){
	_passedState->statusTime=_sTime+PREPARINGTIME;
	_passedState->status=MAJORSTATUS_PREPARING;
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		switch(_passedState->types[i]){
			case BOARD_PUYO:
				((struct puyoBoard*)_passedState->boardData[i])->lowBoard.status=STATUS_PREPARING;
				break;
			case BOARD_YOSHI:
				((struct yoshiBoard*)_passedState->boardData[i])->lowBoard.status=STATUS_PREPARING;
				break;
			case BOARD_HEAL:
				((struct healBoard*)_passedState->boardData[i])->lowBoard.status=STATUS_PREPARING;
				break;
		}
	}
}
void drawGameState(struct gameState* _passedState, u64 _sTime){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		drawBoard(_passedState->boardData[i],_passedState->types[i],_passedState->boardPosX[i],_passedState->boardPosY[i],_sTime);
	}	
	if (_passedState->status==MAJORSTATUS_PREPARING){ // draw countdown if needed
		int _imgIndex=intCap(((_passedState->statusTime-_sTime)/(PREPARINGTIME/(double)PREPARECOUNT)),0,PREPARECOUNT-1);
		int i;
		for (i=0;i<_passedState->numBoards;++i){
			drawCountdown(_passedState->boardData[i],_passedState->types[i],_passedState->boardPosX[i],_passedState->boardPosY[i],preparingImages[_imgIndex]);
		}
	}else if (_passedState->status==MAJORSTATUS_POSTGAME){
		menuDrawAll(_sTime);
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
void restartGameState(struct gameState* _passedState, u64 _sTime){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		resetBoard(_passedState->boardData[i],_passedState->types[i]);
	}
	for (i=0;i<_passedState->numBoards;++i){
		if (_passedState->initializers[i]){
			_passedState->initializers[i](_passedState->boardData[i],_passedState,_passedState->initializerInfo[i]);
		}
	}
	setGameStatePreparing(_passedState,_sTime);
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
	if (_newGarbageSent==0){
		return;
	}
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
	cachedTimeRes = getHDTimeRes();
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
	preparingImages = loadPreparingImages();
	loadGlobalUI();
}
void play(struct gameState* _passedState){
	#if FPSCOUNT == 1
		u64 _frameCountTime = goodGetHDTime();
		int _frames=0;
	#endif
	//
	restartGameState(_passedState,goodGetHDTime());
	rebuildGameState(_passedState,goodGetHDTime());
	//
	crossTexture _curBg = loadImageEmbedded("assets/bg/Sunrise.png");
	setJustPressed(BUTTON_RESIZE);
	while(_passedState->status!=MAJORSTATUS_EXIT){
		u64 _sTime = goodGetHDTime();
		controlsStart();
		if (isDown(BUTTON_RESIZE)){ // Impossible for BUTTON_RESIZE for two frames, so just use isDown
			rebuildGameState(_passedState,_sTime);
		}
		updateGameState(_passedState,_sTime);
		controlsEnd();
		startDrawing();
		drawTextureSized(_curBg,0,0,getOtherScaled(getTextureHeight(_curBg),screenHeight,getTextureWidth(_curBg)),screenHeight);
		drawGameState(_passedState,_sTime);
		endDrawing();
		#if FPSCOUNT
			++_frames;
			if (goodGetHDTime()>=_frameCountTime+cachedTimeRes){
				_frameCountTime=goodGetHDTime();
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
	freeTexture(_curBg);
}
int main(int argc, char* argv[]){
	init();
	while(1){
		struct gameState _testState;
		_testState.numBoards=0;
		titleScreen(&_testState);
		freeUselessSkins(&_testState);
		play(&_testState);
		freeGameState(&_testState);
	}
	// game exit
	generalGoodQuit();
	return 0;
}
