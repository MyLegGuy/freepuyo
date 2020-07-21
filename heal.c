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
#include <string.h>
#include <goodbrew/config.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include "main.h"
#include "goodLinkedList.h"
#include "heal.h"

#define TESTBGCOLOR 150,0,0

#define HEALRELATIVE_NEUTRAL 0
#define HEALRELATIVE_UP 1
#define HEALRELATIVE_DOWN 2
#define HEALRELATIVE_LEFT 3
#define HEALRELATIVE_RIGHT 4
#define HEALRELATIVE_CORE 5

// 
#define HEALTOZEROSTART(a) ((a)-COLOR_HEALSTART)
// inverse of HEALTOZEROSTART
#define HEALTOBOARDSTART(a) ((a)+COLOR_HEALSTART)
#define ZEROGETDIR(a) ((a%6))
#define HEALGETDIR(a) (ZEROGETDIR(HEALTOZEROSTART(a)))

#define COLOR_HEALSTART 2

//////////////////////////////////////////////////
void placeHealWCheck(struct healBoard* _passedBoard, int _x, int _y, pieceColor _passedColor);
void lowPlaceHeal(struct healBoard* _passedBoard, int _x, int _y, pieceColor _passedColor);
//////////////////////////////////////////////////
void loadHealSkin(struct healSkin* _dest, const char* _filename){
	_dest->img=loadImageEmbedded(_filename);
	_dest->numColors=5;
	_dest->pieceW=32;
	_dest->colorX=malloc(sizeof(int*)*_dest->numColors);
	_dest->colorY=malloc(sizeof(int*)*_dest->numColors);
	for (int i=0;i<_dest->numColors;++i){
		_dest->colorX[i] = malloc(sizeof(int)*6);
		_dest->colorY[i] = malloc(sizeof(int)*6);
		for (int j=0;j<6;++j){
			_dest->colorX[i][j]=j*32;
			_dest->colorY[i][j]=i*32;
		}
	}
}
// offsets the values relative. init yourself.
void healDirOff(char _dir, int* _retX, int* _retY){
	switch(_dir){
		case HEALRELATIVE_UP:
			(*_retY)-=1;
			break;
		case HEALRELATIVE_DOWN:
			(*_retY)+=1;
			break;
		case HEALRELATIVE_LEFT:
			(*_retX)-=1;
			break;
		case HEALRELATIVE_RIGHT:
			(*_retX)+=1;
			break;
	}
}
// use this if you know it's a piece and not a blank spot
pieceColor unsafeHealGetNeutral(pieceColor _inColor){
	return HEALTOBOARDSTART((HEALTOZEROSTART(_inColor)/6)*6);
}
pieceColor healGetNeutral(pieceColor _inColor){
	if (_inColor>=COLOR_HEALSTART){
		return unsafeHealGetNeutral(_inColor);
	}
	return 0;
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
	_color-=COLOR_HEALSTART;
	int _colorIndex = _color/6;
	int _dirIndex = _color%6;
	drawTexturePartSized(_passedSkin->img,_drawX,_drawY,tilew,tilew,_passedSkin->colorX[_colorIndex][_dirIndex],_passedSkin->colorY[_colorIndex][_dirIndex],_passedSkin->pieceW,_passedSkin->pieceW);
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

	_testset->pieces[0].color=COLOR_HEALSTART+4+randInt(0,_passedBoard->settings.numColors-1)*6;
	_testset->pieces[0].tileX=_passedBoard->lowBoard.w/2-1;
	_testset->pieces[1].color=COLOR_HEALSTART+3+randInt(0,_passedBoard->settings.numColors-1)*6;
	_testset->pieces[1].tileX=_testset->pieces[0].tileX+1;

	_testset->rotateAround=_testset->pieces;

	addnList(&_passedBoard->activeSets)->data = _testset;
	snapSetPossible(_testset,0);
}
// _checkColor should be comparable. assumes that the piece at _startX and _startY is the color you're looking for
int _lowHealCheckDirection(struct healBoard* _passedBoard, pieceColor _checkColor, int _startX, int _startY, signed char _deltaX, signed char _deltaY){
	int _ret=0;
	int _x;
	int _y;
	for(_x=_startX+_deltaX,_y=_startY+_deltaY;_checkColor==healGetNeutral(getBoard(&_passedBoard->lowBoard,_x,_y)) && _passedBoard->lowBoard.pieceStatus[_x][_y]==0;_x+=_deltaX,_y+=_deltaY,++_ret);
	return _ret;
}
// mark a direction as clearing
// this is a copy of _lowHealCheckDirection
void _lowHealPopLine(struct healBoard* _passedBoard, pieceColor _checkColor, int _startX, int _startY, signed char _deltaX, signed char _deltaY, u64 _popTime, u64 _sTime){
	int _x;
	int _y;
	for(_x=_startX,_y=_startY;_checkColor==healGetNeutral(getBoard(&_passedBoard->lowBoard,_x,_y));_x+=_deltaX,_y+=_deltaY){
		_passedBoard->lowBoard.pieceStatus[_x][_y]=PIECESTATUS_POPPING;
		_passedBoard->lowBoard.pieceStatusTime[_x][_y]=_sTime+_popTime;
		// disconnect its partner piece
		pieceColor _thisColor=fastGetBoard(_passedBoard->lowBoard,_x,_y);
		if (_thisColor!=unsafeHealGetNeutral(_thisColor)){
			signed char _dir=HEALGETDIR(_thisColor);
			if (_dir!=HEALRELATIVE_CORE){
				int _x2=_x;
				int _y2=_y;
				healDirOff(_dir,&_x2,&_y2);
				fastGetBoard(_passedBoard->lowBoard,_x2,_y2)=healGetNeutral(fastGetBoard(_passedBoard->lowBoard,_x2,_y2));
			}
		}
	}
}
// returns 1 if pieces are now clearing
char healDoCheckQueue(struct healBoard* _passedBoard, int* _garbageRet, u64 _sTime){
	*_garbageRet=0;
	char _ret=0;
	ITERATENLIST(_passedBoard->checkQueue,{
			struct pos* _curPos = _curnList->data;
			if (_passedBoard->lowBoard.pieceStatus[_curPos->x][_curPos->y]!=PIECESTATUS_POPPING){
				pieceColor _targetColor = healGetNeutral(_passedBoard->lowBoard.board[_curPos->x][_curPos->y]);
				int _hConnect=1;
				int _vConnect=1;
				_hConnect+=_lowHealCheckDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,-1,0);
				_hConnect+=_lowHealCheckDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,1,0);
				_vConnect+=_lowHealCheckDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,0,-1);
				_vConnect+=_lowHealCheckDirection(_passedBoard,_targetColor,_curPos->x,_curPos->y,0,1);
				
				char _didHConnect = _hConnect>=_passedBoard->settings.minPop;
				char _didVConnect = _vConnect>=_passedBoard->settings.minPop;
				if (_didHConnect){
					_lowHealPopLine(_passedBoard,_targetColor,_curPos->x,_curPos->y,-1,0,_passedBoard->settings.popTime,_sTime);
					_lowHealPopLine(_passedBoard,_targetColor,_curPos->x,_curPos->y,1,0,_passedBoard->settings.popTime,_sTime);
				}
				if (_didVConnect){
					_lowHealPopLine(_passedBoard,_targetColor,_curPos->x,_curPos->y,0,-1,_passedBoard->settings.popTime,_sTime);
					_lowHealPopLine(_passedBoard,_targetColor,_curPos->x,_curPos->y,0,1,_passedBoard->settings.popTime,_sTime);
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
				if (_passedBoard->lowBoard.pieceStatus[i][j] & PIECESTATUS_POPPING){
					int _destW = partMoveEmptys(_sTime,_passedBoard->lowBoard.pieceStatusTime[i][j],_passedBoard->settings.popTime,tilew);
					int _off = easyCenter(_destW,tilew);
					drawHealPiece(_passedBoard->lowBoard.board[i][j],_passedBoard->settings.numColors,_drawX+i*tilew+_off,_drawY+j*tilew+_off,_destW,_passedBoard->skin);
				}else{
					drawHealPiece(_passedBoard->lowBoard.board[i][j],_passedBoard->settings.numColors,_drawX+i*tilew,_drawY+j*tilew,tilew,_passedBoard->skin);
				}
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
			_passedBoard->lowBoard.status=STATUS_SETTLEALL; // this will instantly transition to checking for matches
		}
	}
	// process board statuses. inside here, we'll also process piece statuses
	if (_passedBoard->lowBoard.status==STATUS_SETTLEALL){ // wait for everything to be settled, check for connections, and loop if connections or give player next piece
		if (!_passedBoard->activeSets){ // active sets are settled
			// When everything is settled, check for connections
			if (processPieceStatuses(PIECESTATUS_POPPING | PIECESTATUS_SQUISHING | PIECESTATUS_POSTSQUISH,&_passedBoard->lowBoard,0,_sTime)==0){
				int _garbageToSend;
				if (healDoCheckQueue(_passedBoard,&_garbageToSend,_sTime)){
					// remain in settle state
					// TODO - actually, maybe dont send until combo is done.
					sendGarbage(_passedState,_passedBoard,_garbageToSend);
				}else{
					// make pieces fall
					int _cachedLastSolid[_passedBoard->lowBoard.w];
					{
						int i;
						for (i=0;i<_passedBoard->lowBoard.w;++i){
							_cachedLastSolid[i]=(fastGetBoard(_passedBoard->lowBoard,i,_passedBoard->lowBoard.h-1)!=COLOR_NONE ? _passedBoard->lowBoard.h-1 : _passedBoard->lowBoard.h);
						}
					}
					int _x, _y;
					for (_y=_passedBoard->lowBoard.h-2;_y>=0;--_y){
						for (_x=0;_x<_passedBoard->lowBoard.w;++_x){
							if (fastGetBoard(_passedBoard->lowBoard,_x,_y)!=COLOR_NONE){
								pieceColor _dir = HEALGETDIR(fastGetBoard(_passedBoard->lowBoard,_x,_y));
								if (fastGetBoard(_passedBoard->lowBoard,_x,_y+1)==COLOR_NONE && _cachedLastSolid[_x]!=_y+1 && _dir!=HEALRELATIVE_CORE){
									// make it fall!
									// all but right connected can fall. if it's right connected, we'll grab it on the left connect.
									if (_dir!=HEALRELATIVE_RIGHT){
										if (_dir==HEALRELATIVE_LEFT){
											if (_cachedLastSolid[_x-1]<=_y+1){ // the left pill can't fall. so we can't fall either.
												_cachedLastSolid[_x-1]=_y; // and the left pill is to our left.
												goto actuallyitssolidilied;
											}
											if (_cachedLastSolid[_x-1]<_cachedLastSolid[_x]){ // limit yourself if the other one can't fall as much
												_cachedLastSolid[_x]=_cachedLastSolid[_x-1];
											}
										}
										struct pieceSet* _newSet = malloc(sizeof(struct pieceSet));
										_newSet->count=1+(_dir==HEALRELATIVE_LEFT);
										_newSet->pieces=calloc(1,sizeof(struct movingPiece)*_newSet->count);
										_newSet->isSquare=0;
										_newSet->quickLock=1;
										int _destY=_cachedLastSolid[_x]-1;
										{
											_newSet->pieces[0].color=fastGetBoard(_passedBoard->lowBoard,_x,_y);
											fastGetBoard(_passedBoard->lowBoard,_x,_y)=COLOR_NONE;
											_newSet->pieces[0].tileX=_x;
											_newSet->pieces[0].tileY=_y;
											lowStartPieceFall(&(_newSet->pieces[0]),_destY,_passedBoard->settings.sideEffectFallTime,_sTime);
											if (_dir==HEALRELATIVE_LEFT){
												_newSet->pieces[1].color=fastGetBoard(_passedBoard->lowBoard,_x-1,_y);
												fastGetBoard(_passedBoard->lowBoard,_x-1,_y)=COLOR_NONE;
												_newSet->pieces[1].tileX=_x-1;
												_newSet->pieces[1].tileY=_y;
												lowStartPieceFall(&(_newSet->pieces[1]),_destY,_passedBoard->settings.sideEffectFallTime,_sTime);
												_cachedLastSolid[_x-1]=_destY;
											}
										}
										lazyUpdateSetDisplay(_newSet,_sTime);
										addnList(&_passedBoard->activeSets)->data = _newSet;
										_cachedLastSolid[_x]=_newSet->pieces[0].tileY;
									} // note: _cachedLastSolid not set if it's a left piece. will be set when we do the right piece.
								}else{
								actuallyitssolidilied:
									_cachedLastSolid[_x]=_y;
								}
							}
						}
					}
					if (!_passedBoard->activeSets){ // if none falling
						// TEMP
						int _x, _y;
						for (_y=_passedBoard->lowBoard.h-1;_y>=0;--_y){
							for (_x=0;_x<_passedBoard->lowBoard.w;++_x){
								if (HEALGETDIR(fastGetBoard(_passedBoard->lowBoard,_x,_y))==HEALRELATIVE_CORE){
									goto next;
								}
							}
						}
						winBoard(&_passedBoard->lowBoard);
						goto notnext;
					next:
						transitionHealNextWindow(_passedBoard,_sTime);
					notnext:
						;
					}
				}
			}
		}
	}
	if (_passedBoard->lowBoard.status==STATUS_NEXTWINDOW && _sTime>=_passedBoard->lowBoard.statusTimeEnd){ // do not make this else if because STATUS_DROPPING can chain into this one
		addTestSet(_passedBoard);
		_passedBoard->lowBoard.status=STATUS_NORMAL;
	}
}
void healSetInternalConnectionsUpdate(struct pieceSet* _targetSet){
	int i;
	for (i=1;i<_targetSet->count;i+=2){
		_targetSet->pieces[i].color=unsafeHealGetNeutral(_targetSet->pieces[i].color);
		_targetSet->pieces[i-1].color=unsafeHealGetNeutral(_targetSet->pieces[i-1].color);
		if (_targetSet->pieces[i].tileY!=_targetSet->pieces[i-1].tileY){
			if (_targetSet->pieces[i].tileY<_targetSet->pieces[i-1].tileY){
				_targetSet->pieces[i].color+=HEALRELATIVE_DOWN;
				_targetSet->pieces[i-1].color+=HEALRELATIVE_UP;
			}else{
				_targetSet->pieces[i].color+=HEALRELATIVE_UP;
				_targetSet->pieces[i-1].color+=HEALRELATIVE_DOWN;
			}
		}else{
			if (_targetSet->pieces[i].tileX<_targetSet->pieces[i-1].tileX){
				_targetSet->pieces[i].color+=HEALRELATIVE_RIGHT;
				_targetSet->pieces[i-1].color+=HEALRELATIVE_LEFT;
			}else{
				_targetSet->pieces[i].color+=HEALRELATIVE_LEFT;
				_targetSet->pieces[i-1].color+=HEALRELATIVE_RIGHT;
			}
		}
	}
	
}
void healUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime){
	struct healBoard* _passedBoard = _passedGenericBoard;
	struct controlSet* _passedControls = _controlData;
	updateControlDas(_controlData,_sTime);
	if (_passedBoard->lowBoard.status==STATUS_NORMAL && _passedBoard->activeSets){
		struct pieceSet* _targetSet = _passedBoard->activeSets->data;
		if (isDown(BUTTON_DOWN) && !wasJustPressed(BUTTON_DOWN)){
			setDownButtonHold(_passedControls,_targetSet,_passedBoard->settings.pushMultiplier,_sTime);
		}
		if (wasJustPressed(BUTTON_A) || wasJustPressed(BUTTON_B)){
			tryStartRotate(_targetSet,&_passedBoard->lowBoard,wasJustPressed(BUTTON_A),0,0,_sTime);
			lazyUpdateSetDisplay(_targetSet,_sTime); // update the snapped new x or y pos
			healSetInternalConnectionsUpdate(_targetSet);
		}
		signed char _dir;
		if ((_dir = getDirectionInput(_passedControls,_sTime))){
			tryHShiftSet(_targetSet,&_passedBoard->lowBoard,_dir,_passedBoard->settings.hMoveTime,_sTime);
		}
	}
	controlSetFrameEnd(_controlData,_sTime);
}
void resetHealBoard(struct healBoard* _passedBoard){
	clearBoardBoard(&_passedBoard->lowBoard);
	_passedBoard->lowBoard.status=STATUS_NORMAL;	
	freenList(_passedBoard->activeSets,1);
	_passedBoard->activeSets=NULL;
	_passedBoard->checkQueue=NULL;
	for (int _y=2;_y<_passedBoard->lowBoard.h;++_y){
		for (int _x=0;_x<_passedBoard->lowBoard.w;++_x){
			if (randInt(0,20)==3){
				_passedBoard->lowBoard.board[_x][_y]=COLOR_HEALSTART+randInt(0,_passedBoard->settings.numColors-1)*6+HEALRELATIVE_CORE;
				_passedBoard->lowBoard.pieceStatus[_x][_y]=0;
			}
		}
	}
}
//////////////////////////////////////////////////
void initHealSettings(struct healSettings* _passedSettings){
	_passedSettings->numColors=3;
	_passedSettings->fallTime=1000;
	_passedSettings->rowTime=200;
	_passedSettings->pushMultiplier=13;
	_passedSettings->popTime=500;
	_passedSettings->minPop=4;
	_passedSettings->nextWindowTime=0;
	_passedSettings->hMoveTime=50;
	_passedSettings->sideEffectFallTime=100;
}
void scaleHealSettings(struct healSettings* _passedSettings){
	_passedSettings->fallTime=fixTime(_passedSettings->fallTime);
	_passedSettings->rowTime=fixTime(_passedSettings->rowTime);
	_passedSettings->popTime=fixTime(_passedSettings->popTime);
	_passedSettings->nextWindowTime=fixTime(_passedSettings->nextWindowTime);
	_passedSettings->hMoveTime=fixTime(_passedSettings->hMoveTime);
	_passedSettings->sideEffectFallTime=fixTime(_passedSettings->sideEffectFallTime);
}
struct healBoard* newHeal(int _w, int _h, struct healSettings* _usableSettings, struct healSkin* _passedSkin){
	struct healBoard* _ret = malloc(sizeof(struct healBoard));
	_ret->lowBoard = newGenericBoard(_w,_h);
	_ret->activeSets = NULL;
	_ret->skin=_passedSkin;
	memcpy(&_ret->settings,_usableSettings,sizeof(struct healSettings));
	scaleHealSettings(&_ret->settings);
	resetHealBoard(_ret);
	return _ret;
}
void addHealBoard(struct gameState* _passedState, int i, int _w, int _h, struct healSettings* _usableSettings, struct healSkin* _passedSkin, struct controlSettings* _controlSettings){
	_passedState->types[i] = BOARD_HEAL;
	_passedState->boardData[i] = newHeal(_w,_h,_usableSettings,_passedSkin);
	_passedState->controllers[i].func = healUpdateControlSet;
	_passedState->controllers[i].data = newControlSet(goodGetHDTime(),_controlSettings);
}
