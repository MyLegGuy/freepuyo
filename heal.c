/*
	Copyright (C) 2019	MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  small explanation about what's unique for this code goes here
*/
#include <stdlib.h>
#include <stdio.h>
#include <goodbrew/config.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include "main.h"
#include "goodLinkedList.h"
#include "heal.h"

#define TESTBGCOLOR 150,0,0

// 
#define HEALTOZEROSTART(a) ((a)-COLOR_HEALSTART)
// inverse of HEALTOZEROSTART
#define HEALTOBOARDSTART(a) ((a)+COLOR_HEALSTART)
// returns the neutral color index. for input less than COLOR_HEALSTART, the result will be less than COLOR_HEALSTART
// pass the color index and the total number of colors. a is your passed color index
#define HEALTOCOMPARABLE(a,c) (HEALTOBOARDSTART((HEALTOZEROSTART(a))%(c)))

#define COLOR_HEALSTART 2


#warning todo - rewrite the processPieceStatuses code so that the status_squishing is removed. status_squishing should NOT be removed from puyo this way though! (unless we can make it removed that way.)
#warning an idea to integrate this to puyo is to make it so the processPieceStatuses function returns an int representing the number of pieces that still have any of the supplied statuses (as a bitmap).

//////////////////////////////////////////////////
void placeHealWCheck(struct healBoard* _passedBoard, int _x, int _y, pieceColor _passedColor);
void lowPlaceHeal(struct healBoard* _passedBoard, int _x, int _y, pieceColor _passedColor);
//////////////////////////////////////////////////
// pass comparable color
void getHealColor(pieceColor _tileColor, unsigned char* r, unsigned char* g, unsigned char* b){
	switch(_tileColor){
		case 0:
			*r=255;
			*g=255;
			*b=255;
			break;
		case 1:
			*r=255;
			*g=0;
			*b=0;
			break;
		case 2:
			*r=0;
			*g=255;
			*b=0;
			break;
		case 3:
			*r=0;
			*g=0;
			*b=255;
			break;
		case 4:
			*r=150;
			*g=0;
			*b=150;
			break;
		default:
			printf("invalid color %d\n",_tileColor);
			break;
	}
}
//////////////////////////////////////////////////
void lowPlaceHeal(struct healBoard* _passedBoard, int _x, int _y, pieceColor _passedColor){
	placeSquish(&_passedBoard->lowBoard,_x,_y,_passedColor,0,0);
}
void placeHealWCheck(struct healBoard* _passedBoard, int _x, int _y, pieceColor _passedColor){
	lowPlaceHeal(_passedBoard,_x,_y,_passedColor);
	struct pos* _placePos = malloc(sizeof(struct pos));
	_placePos->x=_x;
	_placePos->y=_y;
	prependnList(&_passedBoard->checkQueue)->data=_placePos;
}
void drawHealPiece(pieceColor _color, int _numColors, int _drawX, int _drawY, int tilew, struct healSkin* _passedSkin){
	// neutral, right, left
	unsigned char r,g,b;
	getHealColor(HEALTOZEROSTART(_color)%_numColors,&r,&g,&b);
	drawRectangle(_drawX,_drawY,tilew,tilew,r*.70,g*.70,b*.70,255);
	drawRectangle(_drawX+tilew*.025,_drawY+tilew*.025,tilew*.95,tilew*.95,0,0,0,255);
	int _repeatLevel = HEALTOZEROSTART(_color)/_numColors;
	int _insideX=_drawX;
	int _insideW=tilew*.80;
	if (_repeatLevel!=2){ // if it's not left
		_insideX+=tilew*.10;
	}
	if (_repeatLevel==1){ // if it's right
		_insideW=tilew-(_insideX-_drawX);
	}
	drawRectangle(_insideX,_drawY+tilew*.10,_insideW,tilew*.80,r,g,b,255);
}
//////////////////////////////////////////////////
void drawHealSetOffset(struct pieceSet* _passedSet, int _offX, int _offY, int _numColors, int tilew, struct healSkin* _passedSkin){
	int i;
	for (i=0;i<_passedSet->count;++i){
		drawHealPiece(_passedSet->pieces[i].color,_numColors,_offX+FIXDISP(_passedSet->pieces[i].displayX),_offY+FIXDISP(_passedSet->pieces[i].displayY),tilew,_passedSkin);
	}
}
void startHealDeathrow(struct movingPiece* _curPiece, struct healSettings* _passedSettings, u64 _sTime){
	_curPiece->movingFlag |= FLAG_DEATHROW;
	_curPiece->completeFallTime=_sTime+_passedSettings->rowTime-_curPiece->completeFallTime;
}
// returns 1 if flags set by updatePieceDisplayY
// it's the caller's job to make sure the piece can move and it's not already
char startHealFall(struct movingPiece* _curPiece, struct healSettings* _passedSettings, u64 _sTime){
	_curPiece->movingFlag |= FLAG_MOVEDOWN;
	_curPiece->transitionDeltaY=1;
	_curPiece->completeFallTime=_sTime+_passedSettings->fallTime-_curPiece->completeFallTime;
	_curPiece->diffFallTime=_passedSettings->fallTime;
	++(_curPiece->tileY);
	return updatePieceDisplayY(_curPiece,_sTime,1);
}
// returns 1 if set locked
char updateHealSet(struct healBoard* _passedBoard, struct pieceSet* _passedSet, u64 _sTime){
	int i;
	char _ret=0;
	signed char _partialSet=0;
	for (i=0;i<_passedSet->count;++i){
		if (updatePieceDisplay(&(_passedSet->pieces[i]),_sTime)){
			_partialSet=1;
		}
	}
	///////////
	char _shouldLock=0;
	if (_passedSet->pieces[0].movingFlag & FLAG_DEATHROW){
		if (deathrowTimeUp(&_passedSet->pieces[0],_sTime)){ // Autolock if we've sat with no space under for too long
			_shouldLock=1;
		}
	}else if (!(_passedSet->pieces[0].movingFlag & FLAG_DOWNORDEATH)){ // if it's not falling, maybe we can make it
		char _needRepeat=1;
		while(_needRepeat){
			_needRepeat=0;
			if (pieceSetCanFall(&_passedBoard->lowBoard,_passedSet)){
				for (i=0;i<_passedSet->count;++i){
					if (startHealFall(&_passedSet->pieces[i],&_passedBoard->settings,_sTime)){
						_needRepeat=1;
					}
				}
			}else{
				if (_passedSet->quickLock){ // For autofall pieces
					_shouldLock=1;
				}else{
					for (i=0;i<_passedSet->count;++i){
						startHealDeathrow(&_passedSet->pieces[i],&_passedBoard->settings,_sTime);
					}
				}
			}
		}
	}
	if (_shouldLock){
		for (i=0;i<_passedSet->count;++i){
			placeHealWCheck(_passedBoard, _passedSet->pieces[i].tileX, _passedSet->pieces[i].tileY,_passedSet->pieces[i].color);
		}
		_ret|=1;
	}
	//
	if (_partialSet){
		for (i=0;i<_passedSet->count;++i){
			removePartialTimes(&_passedSet->pieces[i]);
		}
	}
	return _ret;
}
//////////////////////////////////////////////////
void addTestSet(struct healBoard* _passedBoard){
	struct pieceSet* _testset = malloc(sizeof(struct pieceSet));
	_testset->pieces=calloc(1,sizeof(struct movingPiece)*2);
	_testset->count=2;
	_testset->isSquare=0;
	_testset->quickLock=0;

	_testset->pieces[0].color=COLOR_HEALSTART;
	_testset->pieces[0].tileX=0;
	_testset->pieces[1].color=COLOR_HEALSTART;
	_testset->pieces[1].tileX=1;

	_testset->rotateAround=_testset->pieces;

	addnList(&_passedBoard->activeSets)->data = _testset;
	snapSetPossible(_testset,0);
}
// _checkColor should be comparable. assumes that the piece at _startX and _startY is the color you're looking for
int _lowHealCheckDirection(struct healBoard* _passedBoard, pieceColor _checkColor, int _startX, int _startY, signed char _deltaX, signed char _deltaY){
	int _ret=0;
	int _x;
	int _y;
	for(_x=_startX+_deltaX,_y=_startY+_deltaY;_checkColor==HEALTOCOMPARABLE(getBoard(&_passedBoard->lowBoard,_x,_y),_passedBoard->settings.numColors) && _passedBoard->lowBoard.pieceStatus[_x][_y]==0;_x+=_deltaX,_y+=_deltaY,++_ret);
	return _ret;
}
// mark a direction as clearing
// this is a copy of _lowHealCheckDirection
void _lowHealMarkDirection(struct healBoard* _passedBoard, pieceColor _checkColor, int _startX, int _startY, signed char _deltaX, signed char _deltaY, int _popTime, u64 _sTime){
	int _x;
	int _y;
	for(_x=_startX+_deltaX,_y=_startY+_deltaY;_checkColor==HEALTOCOMPARABLE(getBoard(&_passedBoard->lowBoard,_x,_y),_passedBoard->settings.numColors);_x+=_deltaX,_y+=_deltaY){
		_passedBoard->lowBoard.pieceStatus[_x][_y]=PIECESTATUS_POPPING;
		_passedBoard->lowBoard.pieceStatusTime[_x][_y]=_sTime+_popTime;
	}
}
// returns 1 if pieces are now clearing
char healDoCheckQueue(struct healBoard* _passedBoard, int* _garbageRet, u64 _sTime){
	*_garbageRet=0;
	char _ret=0;
	ITERATENLIST(_passedBoard->checkQueue,{
			struct pos* _curPos = _curnList->data;
			if (_passedBoard->lowBoard.pieceStatus[_curPos->x][_curPos->y]!=PIECESTATUS_POPPING){
				pieceColor _targetColor = HEALTOCOMPARABLE(_passedBoard->lowBoard.board[_curPos->x][_curPos->y],_passedBoard->settings.numColors);
				int _hConnect=1;
				int _vConnect=1;
				_hConnect+=_lowHealCheckDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,-1,0);
				_hConnect+=_lowHealCheckDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,1,0);
				_vConnect+=_lowHealCheckDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,0,-1);
				_vConnect+=_lowHealCheckDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,0,1);
				
				char _didHConnect = _hConnect>=_passedBoard->settings.minPop;
				char _didVConnect = _vConnect>=_passedBoard->settings.minPop;
				if (_didHConnect){
					_lowHealMarkDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,-1,0,_passedBoard->settings.popTime,_sTime);
					_lowHealMarkDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,1,0,_passedBoard->settings.popTime,_sTime);
				}
				if (_didVConnect){
					_lowHealMarkDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,0,-1,_passedBoard->settings.popTime,_sTime);
					_lowHealMarkDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,0,1,_passedBoard->settings.popTime,_sTime);
				}
				if (_didHConnect || _didVConnect){
					_ret=1;
					printf("connect\n");
					// TODO - Set *_garbageRet depending on _hConnect + _vConnect - 1
				}
			}
		});
	freenList(_passedBoard->checkQueue,1);
	_passedBoard->checkQueue=NULL;
	return _ret;
}
void drawHealBoard(struct healBoard* _passedBoard, int _drawX, int _drawY, int tilew, u64 _sTime){
	drawRectangle(_drawX,_drawY,tilew*_passedBoard->lowBoard.w,tilew*_passedBoard->lowBoard.h,TESTBGCOLOR,255);
	int i;
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		int j;
		for (j=0;j<_passedBoard->lowBoard.h;++j){
			if (_passedBoard->lowBoard.board[i][j]!=0){
				drawHealPiece(_passedBoard->lowBoard.board[i][j],_passedBoard->settings.numColors,_drawX+i*tilew,_drawY+j*tilew,tilew,_passedBoard->skin);
			}
		}
	}
	ITERATENLIST(_passedBoard->activeSets,{
			drawHealSetOffset(_curnList->data,_drawX,_drawY,_passedBoard->settings.numColors,tilew,_passedBoard->skin);
		});
}
void transitionHealNextWindow(struct healBoard* _passedBoard, u64 _sTime){
	_passedBoard->lowBoard.status=STATUS_NEXTWINDOW;
	_passedBoard->lowBoard.statusTimeEnd=_sTime+_passedBoard->settings.nextWindowTime;
}
void updateHealBoard(struct gameState* _passedState, struct healBoard* _passedBoard, gameMode _mode, u64 _sTime){
	int i;
	char _setsJustSettled=0; // 1 if there were sets and the last one just settled
	i=0;
	ITERATENLIST(_passedBoard->activeSets,{
			if (updateHealSet(_passedBoard,_curnList->data,_sTime) & 1){
				freePieceSet(_curnList->data); // free set contents
				free(_curnList->data); // free pieceSet struct memory
				free(removenList(&_passedBoard->activeSets,i)); // remove from list
				if (_passedBoard->activeSets==NULL){
					_setsJustSettled=1;
				}
			}else{
				++i;
			}
		});
	if (_setsJustSettled){
		if (_passedBoard->lowBoard.status==STATUS_NORMAL){
			_passedBoard->lowBoard.status=STATUS_DROPPING; // this will instantly transition to checking for matches
		}
	}
	if (_passedBoard->lowBoard.status==STATUS_DROPPING){
		// Wait for all pieces to settle
		if (!_passedBoard->activeSets){
			char _squishDone=processPieceStatuses(&_passedBoard->lowBoard,0,_sTime);
			// When everything is settled, check for connections
			if (_squishDone){
				int _garbageToSend;
				if (healDoCheckQueue(_passedBoard,&_garbageToSend,_sTime)){
					sendGarbage(_passedState,_passedBoard,_garbageToSend);
				}else{
					transitionHealNextWindow(_passedBoard,_sTime);
				}
			}
		}
	}
	if (_passedBoard->lowBoard.status==STATUS_NEXTWINDOW && _sTime>=_passedBoard->lowBoard.statusTimeEnd){ // do not make this else if because STATUS_DROPPING can chain into this one
		addTestSet(_passedBoard);
		_passedBoard->lowBoard.status=STATUS_NORMAL;
	}
}
void healUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime){
	struct healBoard* _passedBoard = _passedGenericBoard;
	struct controlSet* _passedControls = _controlData;
	updateControlDas(_controlData,_sTime);
	if (_passedBoard->lowBoard.status==STATUS_NORMAL){
		struct pieceSet* _targetSet = _passedBoard->activeSets->data;
		if (isDown(BUTTON_DOWN) && !wasJustPressed(BUTTON_DOWN)){
			setDownButtonHold(_passedControls,_targetSet,_passedBoard->settings.pushMultiplier,_sTime);
		}
		if (wasJustPressed(BUTTON_A) || wasJustPressed(BUTTON_B)){
			tryStartRotate(_targetSet,&_passedBoard->lowBoard,wasJustPressed(BUTTON_A),0,0,_sTime);
		}
	}
	controlSetFrameEnd(_controlData,_sTime);
}
void resetHealBoard(struct healBoard* _passedBoard){
	clearBoardBoard(&_passedBoard->lowBoard);
	_passedBoard->lowBoard.status=STATUS_NORMAL;	
	freenList(_passedBoard->activeSets,1);
	_passedBoard->activeSets=NULL;
	
	_passedBoard->lowBoard.board[0][_passedBoard->lowBoard.h-1]=COLOR_HEALSTART;
	_passedBoard->lowBoard.board[1][_passedBoard->lowBoard.h-1]=_passedBoard->settings.numColors+COLOR_HEALSTART;
	_passedBoard->lowBoard.board[0][_passedBoard->lowBoard.h-2]=COLOR_HEALSTART+1;
	_passedBoard->lowBoard.board[1][_passedBoard->lowBoard.h-2]=_passedBoard->settings.numColors+COLOR_HEALSTART+1;

	addTestSet(_passedBoard);
}
struct healBoard* newHeal(int _w, int _h, struct healSettings* _usableSettings, struct healSkin* _passedSkin){
	struct healBoard* _ret = malloc(sizeof(struct healBoard));
	_ret->lowBoard = newGenericBoard(_w,_h);
	_ret->activeSets = NULL;
	_ret->checkQueue=NULL;
	_ret->skin=_passedSkin;
	memcpy(&_ret->settings,_usableSettings,sizeof(struct healSettings));
	resetHealBoard(_ret);
	return _ret;
}
//////////////////////////////////////////////////
void initHealSettings(struct healSettings* _passedSettings){
	_passedSettings->numColors=4;
	_passedSettings->fallTime=1000;
	_passedSettings->rowTime=200;
	_passedSettings->pushMultiplier=2;
	_passedSettings->popTime=500;
	_passedSettings->minPop=4;
	_passedSettings->nextWindowTime=0;
}
void addHealBoard(struct gameState* _passedState, int i, int _w, int _h, struct healSettings* _usableSettings, struct healSkin* _passedSkin){
	_passedState->types[i] = BOARD_HEAL;
	_passedState->boardData[i] = newHeal(_w,_h,_usableSettings,_passedSkin);
	_passedState->controllers[i].func = healUpdateControlSet;
	_passedState->controllers[i].data = newControlSet(goodGetMilli());;
}
