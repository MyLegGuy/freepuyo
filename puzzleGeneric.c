#include "main.h"
#include "puzzleGeneric.h"
#include <goodbrew/config.h>
#include <goodbrew/controls.h>
//////////////////////////////////////////////////
// misc
//////////////////////////////////////////////////
void freeColorArray(pieceColor** _passed, int _w){
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
	if (_passedPiece->movingFlag & FLAG_ANY_HMOVE){
		if (_sTime>=_passedPiece->completeHMoveTime){
			if (_canUnset){
				UNSET_FLAG(_passedPiece->movingFlag,FLAG_ANY_HMOVE);
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
void removePuyoPartialTimes(struct movingPiece* _passedPiece){
	if (!(_passedPiece->movingFlag & FLAG_ANY_HMOVE)){
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
		if (!(_passedPiece->movingFlag & FLAG_ANY_HMOVE)){
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
//////////////////////////////////////////////////
// controlSet
//////////////////////////////////////////////////
void updateControlDas(struct controlSet* _passedControls, u64 _sTime){
	if (wasJustReleased(BUTTON_LEFT) || wasJustReleased(BUTTON_RIGHT)){
		_passedControls->dasDirection=0; // Reset DAS
	}
	if (wasJustPressed(BUTTON_LEFT) || wasJustPressed(BUTTON_RIGHT)){
		_passedControls->dasDirection = wasJustPressed(BUTTON_RIGHT) ? 1 : -1;
		_passedControls->dasChargeEnd = _sTime+DASTIME;
	}
	if (isDown(BUTTON_LEFT) || isDown(BUTTON_RIGHT)){
		if (_passedControls->dasDirection==0){
			_passedControls->dasDirection = isDown(BUTTON_RIGHT) ? 1 : -1;
			_passedControls->dasChargeEnd = _sTime+DASTIME;
		}
	}
}
struct controlSet* newControlSet(u64 _sTime){
	struct controlSet* _ret = malloc(sizeof(struct controlSet));
	_ret->dasDirection=0;
	_ret->startHoldTime=0;
	_ret->lastFailedRotateTime=0;
	_ret->lastFrameTime=_sTime;
	_ret->holdStartTime=0;
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
