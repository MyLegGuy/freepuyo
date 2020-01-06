/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "puzzleGeneric.h"
#include <goodbrew/config.h>
#include <goodbrew/controls.h>
/////
struct windowImg boardBorder;
//////////////////////////////////////////////////
//
//////////////////////////////////////////////////
void initControlSettings(struct controlSettings* _passedSettings){
	_passedSettings->doubleRotateTapTime=350;
	_passedSettings->dasTime=150;
}
void scaleControlSettings(struct controlSettings* _passedSettings){
	_passedSettings->doubleRotateTapTime=fixTime(_passedSettings->doubleRotateTapTime);
	_passedSettings->dasTime=fixTime(_passedSettings->dasTime);
}
//////////////////////////////////////////////////
// misc
//////////////////////////////////////////////////
void freeJaggedArrayu64(u64** _passed, int _w){
	int i;
	for (i=0;i<_w;++i){
		free(_passed[i]);
	}
	free(_passed);
}
void freeJaggedArrayColor(pieceColor** _passed, int _w){
	int i;
	for (i=0;i<_w;++i){
		free(_passed[i]);
	}
	free(_passed);
}
void freeJaggedArrayChar(char** _passed, int _w){
	int i;
	for (i=0;i<_w;++i){
		free(_passed[i]);
	}
	free(_passed);
}
u64** newJaggedArrayu64(int _w, int _h){
	u64** _retArray = malloc(sizeof(u64*)*_w);
	int i;
	for (i=0;i<_w;++i){
		_retArray[i] = malloc(sizeof(u64)*_h);
	}
	return _retArray;
}
pieceColor** newJaggedArrayColor(int _w, int _h){
	pieceColor** _retArray = malloc(sizeof(pieceColor*)*_w);
	int i;
	for (i=0;i<_w;++i){
		_retArray[i] = malloc(sizeof(pieceColor)*_h);
	}
	return _retArray;
}
char** newJaggedArrayChar(int _w, int _h){
	char** _retArray = malloc(sizeof(char*)*_w);
	int i;
	for (i=0;i<_w;++i){
		_retArray[i] = malloc(sizeof(char)*_h);
	}
	return _retArray;
}
//////////////////////////////////////////////////
// movingPiece
//////////////////////////////////////////////////
char deathrowTimeUp(struct movingPiece* _passedPiece, u64 _sTime){
	return (_passedPiece->movingFlag & FLAG_DEATHROW && _sTime>=_passedPiece->completeFallTime);
}
char updatePieceDisplayY(struct movingPiece* _passedPiece, u64 _sTime, char _canUnset){
	if (_passedPiece->movingFlag & FLAG_MOVEDOWN){ // If we're moving down, update displayY until we're done, then snap and unset
		if (_sTime>=_passedPiece->completeFallTime){ // Unset if done
			if (_canUnset){
				UNSET_FLAG(_passedPiece->movingFlag,FLAG_MOVEDOWN);
				snapPieceDisplayPossible(_passedPiece);
				_passedPiece->completeFallTime=_sTime-_passedPiece->completeFallTime;
				return 1;
			}else{
				lowSnapPieceTileY(_passedPiece);
			}
		}else{ // Partial position if not done
			_passedPiece->displayY = _passedPiece->tileY-partMoveEmptys(_sTime,_passedPiece->completeFallTime,_passedPiece->diffFallTime,_passedPiece->transitionDeltaY);
		}
	}
	return 0;
}
char updatePieceDisplayX(struct movingPiece* _passedPiece, u64 _sTime, char _canUnset){
	if (_passedPiece->movingFlag & FLAG_HMOVE){
		if (_sTime>=_passedPiece->completeHMoveTime){
			if (_canUnset){
				UNSET_FLAG(_passedPiece->movingFlag,FLAG_HMOVE);
				snapPieceDisplayPossible(_passedPiece);
				_passedPiece->completeHMoveTime=_sTime-_passedPiece->completeHMoveTime;
				return 1;
			}else{
				lowSnapPieceTileX(_passedPiece);
			}
		}else{
			_passedPiece->displayX = _passedPiece->tileX-partMoveEmptys(_sTime,_passedPiece->completeHMoveTime,_passedPiece->diffHMoveTime,_passedPiece->transitionDeltaX);
		}
	}
	return 0;
}
void removePartialTimes(struct movingPiece* _passedPiece){
	if (!(_passedPiece->movingFlag & FLAG_HMOVE)){
		_passedPiece->completeHMoveTime=0;
	}
	if (!(_passedPiece->movingFlag & (FLAG_MOVEDOWN | FLAG_DEATHROW))){
		_passedPiece->completeFallTime=0;
	}
}
void lowSnapPieceTileX(struct movingPiece* _passedPiece){
	_passedPiece->displayX = _passedPiece->tileX;
}
void lowSnapPieceTileY(struct movingPiece* _passedPiece){
	_passedPiece->displayY = _passedPiece->tileY;
}
// Will update the puyo's displayX and displayY for the axis it isn't moving on.
void snapPieceDisplayPossible(struct movingPiece* _passedPiece){
	if (!(_passedPiece->movingFlag & FLAG_ANY_ROTATE)){
		if (!(_passedPiece->movingFlag & FLAG_HMOVE)){
			lowSnapPieceTileX(_passedPiece);
		}
		if (!(_passedPiece->movingFlag & FLAG_MOVEDOWN)){
			lowSnapPieceTileY(_passedPiece);
		}
	}
}
// like updatePieceDisplay, but temporary. Will not set partial times or transition moving flags, so no need to handle those.
void lazyUpdatePieceDisplay(struct movingPiece* _passedPiece, u64 _sTime){
	snapPieceDisplayPossible(_passedPiece);
	updatePieceDisplayY(_passedPiece,_sTime,0);
	updatePieceDisplayX(_passedPiece,_sTime,0);
}
// updates piece display depending on flags
// when a flag is unset, the time variable, completeFallTime or completeHMoveTime, is set to the difference between the current time and when it should've finished
// returns 1 if unset a flag.
// if it returns 1 consider looking at the values in completeFallTime and completeHMoveTime
char updatePieceDisplay(struct movingPiece* _passedPiece, u64 _sTime){
	return updatePieceDisplayY(_passedPiece,_sTime,1) | updatePieceDisplayX(_passedPiece,_sTime,1);
}
char pieceCanFell(struct genericBoard* _passedBoard, struct movingPiece* _passedPiece){
	return (getBoard(_passedBoard,_passedPiece->tileX,_passedPiece->tileY+1)==COLOR_NONE);
}
char pieceTryUnsetDeath(struct genericBoard* _passedBoard, struct movingPiece* _passedPiece){
	if (_passedPiece->movingFlag & FLAG_DEATHROW){
		if (pieceCanFell(_passedBoard,_passedPiece)){
			UNSET_FLAG(_passedPiece->movingFlag,FLAG_DEATHROW);
			_passedPiece->completeFallTime=0;
			return 1;
		}
	}
	return 0;
}
//////////////////////////////////////////////////
// controlSet
//////////////////////////////////////////////////
void downButtonHold(struct controlSet* _passedControls, struct movingPiece* _targetPiece, double _passedMultiplier, u64 _sTime){
	if (_targetPiece->movingFlag & FLAG_MOVEDOWN){ // Normal push down
		int _offsetAmount = (_sTime-_passedControls->lastFrameTime)*_passedMultiplier;
		if (_offsetAmount>_targetPiece->completeFallTime){ // Keep unisnged value from going negative
			_targetPiece->completeFallTime=0;
		}else{
			_targetPiece->completeFallTime=_targetPiece->completeFallTime-_offsetAmount;
		}
	}else if (_targetPiece->movingFlag & FLAG_DEATHROW){ // lock
		_targetPiece->completeFallTime=0;
	}
}
void updateControlDas(struct controlSet* _passedControls, u64 _sTime){
	if (wasJustReleased(BUTTON_LEFT) || wasJustReleased(BUTTON_RIGHT)){
		_passedControls->dasDirection=0; // Reset DAS
	}
	if (wasJustPressed(BUTTON_LEFT) || wasJustPressed(BUTTON_RIGHT)){
		_passedControls->dasDirection = wasJustPressed(BUTTON_RIGHT) ? 1 : -1;
		_passedControls->dasChargeEnd = _sTime+_passedControls->settings.dasTime;
	}
	if (isDown(BUTTON_LEFT) || isDown(BUTTON_RIGHT)){
		if (_passedControls->dasDirection==0){
			_passedControls->dasDirection = isDown(BUTTON_RIGHT) ? 1 : -1;
			_passedControls->dasChargeEnd = _sTime+_passedControls->settings.dasTime;
		}
	}
}
void controlSetFrameEnd(struct controlSet* _passedControls, u64 _sTime){
	_passedControls->lastFrameTime = _sTime;
}
struct controlSet* newControlSet(u64 _sTime, struct controlSettings* _usableSettings){
	struct controlSet* _ret = malloc(sizeof(struct controlSet));
	_ret->dasDirection=0;
	_ret->lastFailedRotateTime=0;
	_ret->lastFrameTime=_sTime;
	memcpy(&_ret->settings,_usableSettings,sizeof(struct controlSettings));
	scaleControlSettings(&_ret->settings);
	return _ret;
}
signed char getDirectionInput(struct controlSet* _passedControls, u64 _sTime){
	if (_sTime>=_passedControls->dasChargeEnd){
		return _passedControls->dasDirection;
	}
	if (wasJustPressed(BUTTON_RIGHT)){
		return 1;
	}
	if (wasJustPressed(BUTTON_LEFT)){
		return -1;
	}
	return 0;
}
void initialTouchDown(struct controlSet* _passedSet, u64 _sTime){
	_passedSet->holdStartTime=_sTime;
	_passedSet->startTouchX=touchX;
	_passedSet->startTouchY=touchY;
	_passedSet->didDrag=0;
	_passedSet->isTouchDrop=0;
}
// returns 1 if it would qualift as a tap
char onTouchRelease(struct controlSet* _passedControls){
	_passedControls->holdStartTime=0;
	return (!_passedControls->didDrag && abs(touchX-_passedControls->startTouchX)<screenWidth*MAXTAPSCREENRATIO && abs(touchY-_passedControls->startTouchY)<screenHeight*MAXTAPSCREENRATIO);
}
signed char touchIsHDrag(struct controlSet* _passedControls){
	if (abs(touchX-_passedControls->startTouchX)>=widthDragTile){
		if (touchX>_passedControls->startTouchX){
			return 1;
		}else{
			return -1;
		}
	}
	return 0;
}
// does not reset drag flag
void resetControlHDrag(struct controlSet* _passedControls){
	int _touchXDiff = abs(touchX-_passedControls->startTouchX);
	signed char _direction = touchX>_passedControls->startTouchX ? 1 : -1;
	_passedControls->startTouchX=touchX-(_touchXDiff%widthDragTile)*_direction;
}
//////////////////////////////////////////////////
// genericBoard
//////////////////////////////////////////////////
void setBoard(struct genericBoard* _passedBoard, int x, int y, pieceColor _color){
	_passedBoard->board[x][y]=_color;
	_passedBoard->pieceStatus[x][y]=STATUS_UNDEFINED;
}
pieceColor getBoard(struct genericBoard* _passedBoard, int _x, int _y){
	if (_x<0 || _y<0 || _x>=_passedBoard->w || _y>=_passedBoard->h){
		return COLOR_IMPOSSIBLE;
	}
	return _passedBoard->board[_x][_y];
}
struct genericBoard newGenericBoard(int _w, int _h){
	struct genericBoard _ret;
	_ret.w = _w;
	_ret.h = _h;
	_ret.board = newJaggedArrayColor(_w,_h);
	_ret.pieceStatus = newJaggedArrayChar(_w,_h);
	_ret.pieceStatusTime = newJaggedArrayu64(_w,_h);
	_ret.status=STATUS_NORMAL;
	return _ret;
}
void freeGenericBoard(struct genericBoard* _passedBoard){
	freeJaggedArrayColor(_passedBoard->board,_passedBoard->w);
	freeJaggedArrayChar(_passedBoard->pieceStatus,_passedBoard->w);
	freeJaggedArrayu64(_passedBoard->pieceStatusTime,_passedBoard->w);
}
void clearPieceStatus(struct genericBoard* _passedBoard){
	int i;
	for (i=0;i<_passedBoard->w;++i){
		memset(_passedBoard->pieceStatus[i],0,_passedBoard->h*sizeof(char));
		memset(_passedBoard->pieceStatusTime[i],0,_passedBoard->h*sizeof(u64));
	}
}
void clearBoardBoard(struct genericBoard* _passedBoard){
	int i;
	for (i=0;i<_passedBoard->w;++i){
		memset(_passedBoard->board[i],0,sizeof(pieceColor)*_passedBoard->h); // pray COLOR_NONE is 0
	}
}
char rowIsClear(struct genericBoard* _passedBoard, int _yIndex){
	int i;
	for (i=0;i<_passedBoard->w;++i){
		if (_passedBoard->board[i][_yIndex]!=0){
			return 0;
		}
	}
	return 1;
}
void killBoard(struct genericBoard* _passedBoard, u64 _sTime){
	_passedBoard->status=STATUS_DEAD;
	_passedBoard->statusTimeEnd=_sTime+DEATHANIMTIME;
}
void winBoard(struct genericBoard* _passedBoard){
	_passedBoard->status=STATUS_WON;
}
void placeNormal(struct genericBoard* _passedBoard, int _x, int _y, pieceColor _passedColor){
	_passedBoard->board[_x][_y]=_passedColor;
	_passedBoard->pieceStatus[_x][_y]=0;
}
void placeSquish(struct genericBoard* _passedBoard, int _x, int _y, pieceColor _passedColor, int _squishTime, u64 _sTime){
	_passedBoard->board[_x][_y]=_passedColor;
	_passedBoard->pieceStatus[_x][_y]=PIECESTATUS_SQUISHING;
	_passedBoard->pieceStatusTime[_x][_y]=_squishTime+_sTime;
}
// Returns the number of pieces that still have the status specified by _retThese. does not work for status 0
int processPieceStatuses(int _retThese, struct genericBoard* _passedBoard, int _postSquishDelay, u64 _sTime){
	int _ret=0;
	int _x, _y;
	for (_x=0;_x<_passedBoard->w;++_x){
		for (_y=0;_y<_passedBoard->h;++_y){
			if (_passedBoard->board[_x][_y]>=COLOR_IMPOSSIBLE && _passedBoard->pieceStatus[_x][_y]){
				switch (_passedBoard->pieceStatus[_x][_y]){
					case PIECESTATUS_SQUISHING: // Normal ones that expire after some time
					case PIECESTATUS_POSTSQUISH:
					case PIECESTATUS_POPPING:
						if (_sTime>=_passedBoard->pieceStatusTime[_x][_y]){
							// May have a special action when time up.
							switch(_passedBoard->pieceStatus[_x][_y]){
								case PIECESTATUS_SQUISHING: // special
									_passedBoard->pieceStatus[_x][_y]=PIECESTATUS_POSTSQUISH;
									_passedBoard->pieceStatusTime[_x][_y]+=_postSquishDelay;
									break;
								case PIECESTATUS_POPPING: // remove the piece and the status
									_passedBoard->board[_x][_y]=0;
								case PIECESTATUS_POSTSQUISH: // just remove the status
									_passedBoard->pieceStatus[_x][_y]=0;
									break;
							}
						}
						break;
				}
				if (_retThese & _passedBoard->pieceStatus[_x][_y]){
					++_ret;
				}
			}
		}
	}
	return _ret;
}
