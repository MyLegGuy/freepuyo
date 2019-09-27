/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
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
#include "skinLoader.h"
#include "scoreConstants.h"
#include "main.h"
#include "puzzleGeneric.h"
#include "puyo.h"
//
#define BRICKBGCOLOR 130,75,40,255
//
#define TESTFEVERPIECE 0
//
#define COLOR_GARBAGE (COLOR_REALSTART-1) // I can't spell nuisance
#define COLOR_REALSTART 3
//
// When checking for potential pops if the current piece set moves as is, this is the byte we use in popCheckHelp to identify this puyo as a verified potential pop
#define POSSIBLEPOPBYTE 5
// Control constants
#define DOUBLEROTATETAPTIME 350
// Display constants
#define POTENTIALPOPALPHA 200
#define GHOSTPIECERATIO .80
#define POPANIMRELEASERATIO .50 // The amount of the total squish time is used to come back up
#define SQUISHDOWNLIMIT .30 // The smallest of a puyo's original size it will get when squishing. Given as a decimal percent. .30 would mean puyo doesn't get less than 30% of its size
// Timing constants
#define SPLITFALLTIME(_usualTime) (_usualTime/7)
#define GARBAGEFALLTIME(_usualTime) (_usualTime/10)
#define SQUISHNEXTFALLTIME 30
#define SQUISHDELTAY (tilew)
// Game mechanic constants
#define STANDARDMINPOP 4 // used when calculating group bonus.
// bitmap
// 0 is default?
#define PIECESTATUS_POPPING 1
#define PIECESTATUS_SQUSHING 2
#define PIECESTATUS_POSTSQUISH 4
// #define					   8
#define fastGetBoard(_passedBoard,_x,_y) (_passedBoard.board[_x][_y])
struct pieceSet{
	signed char isSquare; // If this is a square set of puyos then this will be the width of that square. 0 otherwise
	struct movingPiece* rotateAround; // If this isn't a square, rotate around this puyo
	char quickLock;
	int count;
	struct movingPiece* pieces;
};
struct aiInstruction{
	int anchorDestX;
	int anchorDestY;
	int secondaryDestX;
	int secondaryDestY;
};
struct aiState;
typedef void(*aiFunction)(struct aiState*,struct pieceSet*,struct puyoBoard*,int,struct gameState*);
struct aiState{
	struct aiInstruction nextAction; // Do not manually set from ai function
	aiFunction updateFunction;
	char softdropped;
};
void drawSingleGhostColumn(int _offX, int _offY, int _tileX, struct puyoBoard* _passedBoard, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin);
int getPopNum(struct puyoBoard* _passedBoard, int _x, int _y, char _helpChar, pieceColor _shapeColor);
int getFreeColumnYPos(struct puyoBoard* _passedBoard, int _columnIndex, int _minY);
void updateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime);
char forceFallStatics(struct puyoBoard* _passedBoard);
char boardHasConnections(struct puyoBoard* _passedBoard);
unsigned char tryStartRotate(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard, char _isClockwise, char _canDoubleRotate, u64 _sTime);

struct puyoSkin currentSkin;
//////////////////////////////////////////////////////////
double scoreToGarbage(struct gameSettings* _passedSettings, long _passedPoints){
	return _passedPoints/(double)_passedSettings->pointsPerGar;
}
double getLeftoverGarbage(struct gameSettings* _passedSettings, long _passedPoints){
	double _all = scoreToGarbage(_passedSettings,_passedPoints);
	return _all-(int)_all;
}
int getSpawnCol(int _w){
	if (_w & 1){
		return _w/2; // 7 -> 3
	}else{
		return _w/2-1; //6 -> 2
	}
}
void _rotateAxisFlip(char _isClockwise, char _dirRelation, int *_outX, int* _outY){
	if (!_isClockwise){
		if (_dirRelation & (DIR_UP | DIR_DOWN)){
			*_outX=(*_outX*-1);
		}else{
			*_outY=(*_outY*-1);
		}
	}
}
// _outX and _outY with be -1 or 1
// You pass the relation you are to the anchor before rotation
void getPostRotatePos(char _isClockwise, char _dirRelation, int* _outX, int* _outY){
	// Get as if it's clockwise
	// Primary direction goes first, direction that changes with direction goes second
	switch(_dirRelation){
		case DIR_UP:
			*_outY=1;
			*_outX=1;
			break;
		case DIR_DOWN:
			*_outY=-1;
			*_outX=-1;
			break;
		case DIR_LEFT:
			*_outX=1;
			*_outY=-1;
			break;
		case DIR_RIGHT:
			*_outX=-1;
			*_outY=1;
			break;
	}
	_rotateAxisFlip(_isClockwise,_dirRelation,_outX,_outY);
}
// Apply this sign change to the end results of sin and cos
// You pass the relation you are to the anchor after rotation, we're trying to go backwards
// Remember, the trig is applied to the center of the anchor
void getRotateTrigSign(char _isClockwise, char _dirRelation, int* _retX, int* _retY){
	*_retX=1;
	*_retY=1;
	// Get signs for clockwise
	switch(_dirRelation){
		case DIR_UP:
			*_retY=-1;
			*_retX=-1;
			break;
		case DIR_DOWN:
			*_retY=1;
			*_retX=1;
			break;
		case DIR_LEFT:
			*_retX=-1;
			*_retY=1;
			break;
		case DIR_RIGHT:
			*_retX=1;
			*_retY=-1;
			break;
	}
	_rotateAxisFlip(_isClockwise,_dirRelation,_retX,_retY);
}
void placePuyo(struct puyoBoard* _passedBoard, int _x, int _y, pieceColor _passedColor, int _squishTime, u64 _sTime){
	_passedBoard->lowBoard.board[_x][_y]=_passedColor;
	if (_passedColor!=COLOR_GARBAGE){
		_passedBoard->lowBoard.pieceStatus[_x][_y]=PIECESTATUS_SQUSHING;
		_passedBoard->lowBoard.pieceStatusTime[_x][_y]=_squishTime+_sTime;
	}else{
		_passedBoard->lowBoard.pieceStatus[_x][_y]=0;
	}
}

//////////////////////////////////////////////////
// movingPiece
//////////////////////////////////////////////////
void lowDrawNormPuyo(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int tilew){
	drawTexturePartSized(_passedSkin->img,_drawX,_drawY,tilew,tilew,_passedSkin->colorX[_color-COLOR_REALSTART][_tileMask],_passedSkin->colorY[_color-COLOR_REALSTART][_tileMask],_passedSkin->puyoW,_passedSkin->puyoH);
}
void lowDrawPotentialPopPuyo(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int tilew){
	drawTexturePartSizedTintAlpha(_passedSkin->img,_drawX,_drawY,tilew,tilew,_passedSkin->colorX[_color-COLOR_REALSTART][_tileMask],_passedSkin->colorY[_color-COLOR_REALSTART][_tileMask],_passedSkin->puyoW,_passedSkin->puyoH,255,255,255,POTENTIALPOPALPHA);
}
void lowDrawPoppingPuyo(int _color, int _drawX, int _drawY, u64 _destPopTime, int _diffPopTime, struct puyoSkin* _passedSkin, int tilew, u64 _sTime){
	int _destSize=tilew*(_destPopTime-_sTime)/(double)_diffPopTime;
	drawTexturePartSized(_passedSkin->img,_drawX+(tilew-_destSize)/2,_drawY+(tilew-_destSize)/2,_destSize,_destSize,_passedSkin->colorX[_color-COLOR_REALSTART][0],_passedSkin->colorY[_color-COLOR_REALSTART][0],_passedSkin->puyoW,_passedSkin->puyoH);
}
void lowDrawGarbage(int _drawX, int _drawY, struct puyoSkin* _passedSkin, int tilew, double _alpha){
	drawTexturePartSizedAlpha(_passedSkin->img,_drawX,_drawY,tilew,tilew,_passedSkin->garbageX,_passedSkin->garbageY,_passedSkin->puyoW,_passedSkin->puyoH,_alpha);
}
void lowDrawPoppingGarbage(int _drawX, int _drawY, u64 _destPopTime, int _diffPopTime, struct puyoSkin* _passedSkin, int tilew, u64 _sTime){
	lowDrawGarbage(_drawX,_drawY,_passedSkin,tilew,partMoveEmptysCapped(_sTime,_destPopTime,_diffPopTime,255));
}
void drawPoppingPiece(int _color, int _drawX, int _drawY, u64 _destPopTime, int _diffPopTime, struct puyoSkin* _passedSkin, short tilew, u64 _sTime){
	if (_color==COLOR_GARBAGE){
		lowDrawPoppingGarbage(_drawX,_drawY,_destPopTime,_diffPopTime,_passedSkin,tilew,_sTime);
	}else{
		lowDrawPoppingPuyo(_color,_drawX,_drawY,_destPopTime,_diffPopTime,_passedSkin,tilew,_sTime);
	}
}
void drawNormPiece(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, short tilew){
	if (_color==COLOR_GARBAGE){
		lowDrawGarbage(_drawX,_drawY,_passedSkin,tilew,255);
	}else{
		lowDrawNormPuyo(_color,_drawX,_drawY,_tileMask,_passedSkin,tilew);
	}
}
void drawPotentialPopPiece(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int tilew){
	if (_color==COLOR_GARBAGE){
		lowDrawGarbage(_drawX,_drawY,_passedSkin,tilew,POTENTIALPOPALPHA);
	}else{
		lowDrawPotentialPopPuyo(_color,_drawX,_drawY,_tileMask,_passedSkin,tilew);
	}
}
double drawSquishRatioPuyo(int _color, int _drawX, int _drawY, double _ratio, struct puyoSkin* _passedSkin, int tilew){
	if (_ratio<0){ // upsquish
		_ratio=1+_ratio*-1;
	}
	double _destHeight = tilew*_ratio;
	drawTexturePartSized(_passedSkin->img,_drawX,_drawY+(1-_ratio)*tilew,tilew,_destHeight,_passedSkin->colorX[_color-COLOR_REALSTART][0],_passedSkin->colorY[_color-COLOR_REALSTART][0],_passedSkin->puyoW,_passedSkin->puyoH);
	return _destHeight;
}
double drawSquishingPuyo(int _color, int _drawX, int _drawY, int _diffPopTime, u64 _endPopTime, struct puyoSkin* _passedSkin, int tilew, u64 _sTime){
	double _partRatio;
	int _releaseTimePart = _diffPopTime*POPANIMRELEASERATIO;
	if (_endPopTime>_sTime+_releaseTimePart){
		_partRatio = 1-partMoveFills(_sTime,_endPopTime-_releaseTimePart,_diffPopTime-_releaseTimePart,1-SQUISHDOWNLIMIT);
	}else{
		_partRatio = SQUISHDOWNLIMIT+partMoveFills(_sTime,_endPopTime,_releaseTimePart,1-SQUISHDOWNLIMIT);
	}
	return drawSquishRatioPuyo(_color,_drawX,_drawY,_partRatio,_passedSkin,tilew);
}
void _lowStartPuyoFall(struct movingPiece* _passedPiece, int _destTileY, int _singleFallTime, u64 _sTime){
	_passedPiece->movingFlag|=FLAG_MOVEDOWN;
	int _tileDiff = _destTileY-_passedPiece->tileY;
	_passedPiece->tileY+=_tileDiff;
	_passedPiece->transitionDeltaY = _tileDiff;
	_passedPiece->diffFallTime=_tileDiff*_singleFallTime;
	_passedPiece->completeFallTime = _sTime+_passedPiece->diffFallTime;
}
void _forceStartPuyoGravity(struct movingPiece* _passedPiece, int _singleFallTime, u64 _sTime){
	_lowStartPuyoFall(_passedPiece,_passedPiece->tileY+1,_singleFallTime,_sTime);
}
void _forceStartPuyoAutoplaceTime(struct movingPiece* _passedPiece, int _singleFallTime, u64 _sTime){
	_passedPiece->movingFlag|=FLAG_DEATHROW;
	_passedPiece->completeFallTime = _sTime+_singleFallTime/2;
}
//////////////////////////////////////////////////
// pieceSet
//////////////////////////////////////////////////
void rotateButtonPress(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, struct controlSet* _passedControls, char _isClockwise, u64 _sTime){
	char _canDoubleRotate=_sTime<=_passedControls->lastFailedRotateTime+DOUBLEROTATETAPTIME;
	if (tryStartRotate(_passedSet,_passedBoard,_isClockwise,_canDoubleRotate,_sTime)&1){ // If double rotate tried to be used
		if (_canDoubleRotate){
			// It worked, reset it
			_passedControls->lastFailedRotateTime=0;
		}else{
			// Queue the double press time
			_passedControls->lastFailedRotateTime=_sTime;
		}
	}
}
// These two functions for temp pieces *can* be used, but don't have to be. Meaning don't add anything special here because it can't be assumed these are used.
void removeTempPieces(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard, int* _yDests){
	int i;
	for (i=0;i<_passedSet->count;++i){
		_passedBoard->lowBoard.board[_passedSet->pieces[i].tileX][_yDests[i]]=0;
	}
}
void setTempPieces(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard, int* _yDests, char _canGhostRows){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if (_canGhostRows || _yDests[i]>=_passedBoard->numGhostRows){
			_passedBoard->lowBoard.board[_passedSet->pieces[i].tileX][_yDests[i]]=_passedSet->pieces[i].color;
		}
	}
}
// Given a pieceSet, return an array. The array contains the dest y positions of the pieces if they all went down. Index in the returned array corresponds to pieceSet piece array.
// Point is to account for pieces that will land on the same column, meaning their y positions will stack on top of eachother.
int* getSetDestY(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard){
	int* _retArray = malloc(sizeof(int)*_passedSet->count);
	int _prevColumn=-1;
	while(1){
		// First, find the next smallest column we're going to use.
		int _newLowColumn=_passedBoard->lowBoard.w;
		int i;
		for (i=0;i<_passedSet->count;++i){
			if (_passedSet->pieces[i].tileX>_prevColumn && _passedSet->pieces[i].tileX<_newLowColumn){
				_newLowColumn = _passedSet->pieces[i].tileX;
			}
		}
		if (_newLowColumn==_passedBoard->lowBoard.w){
			return _retArray;
		}

		int _firstYDest = getFreeColumnYPos(_passedBoard,_newLowColumn,0);
		// Draw all the puyos in order of lowest Y to highest Y.
		// Like the same thing we did above, just with Y instead of X and bigger instead of smaller
		int _oldLowestY=_passedBoard->lowBoard.h;
		while(1){
			int _newLowYIndex=-1;
			for (i=0;i<_passedSet->count;++i){
				if (_passedSet->pieces[i].tileX==_newLowColumn && _passedSet->pieces[i].tileY<_oldLowestY && (_newLowYIndex==-1 || _passedSet->pieces[i].tileY>_passedSet->pieces[_newLowYIndex].tileY)){
					_newLowYIndex=i;
				}
			}
			if (_newLowYIndex==-1){
				break;
			}
			_retArray[_newLowYIndex]=(_firstYDest--);
			_oldLowestY=_passedSet->pieces[_newLowYIndex].tileY;
		}
		_prevColumn=_newLowColumn;
	}
}
void forceResetSetDeath(struct pieceSet* _passedSet){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if (_passedSet->pieces[i].movingFlag & FLAG_DEATHROW){
			UNSET_FLAG(_passedSet->pieces[i].movingFlag,FLAG_DEATHROW);
			_passedSet->pieces[i].completeFallTime=0;
		}
	}
}
char puyoSetCanFell(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if (!pieceCanFell(&_passedBoard->lowBoard,&(_passedSet->pieces[i]))){
			return 0;
		}
	}
	return 1;
}
void resetDyingFlagMaybe(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet){
	if (puyoSetCanFell(_passedBoard,_passedSet)){
		forceResetSetDeath(_passedSet);
	}
}
// Try to start an h shift on a set
void tryHShiftSet(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard, signed char _direction, u64 _sTime){
	if (!(_passedSet->pieces[0].movingFlag & FLAG_HMOVE)){
		char _upShiftNeeded=0;
		int i;
		for (i=0;i<_passedSet->count;++i){
			if (getBoard(&_passedBoard->lowBoard,_passedSet->pieces[i].tileX+_direction,_passedSet->pieces[i].tileY)==COLOR_NONE){
				continue;
			}else if (_passedSet->pieces[i].movingFlag & FLAG_MOVEDOWN && (_passedSet->pieces[i].completeFallTime-_sTime)>_passedSet->pieces[i].diffFallTime/2){ // Allow the puyo to jump up a tile a little bit if the timing is tight
				if (getBoard(&_passedBoard->lowBoard,_passedSet->pieces[i].tileX+_direction,_passedSet->pieces[i].tileY-1)!=COLOR_NONE){
					break;
				}
				_upShiftNeeded=1;
			}else{
				break;
			}
		}
		if (i==_passedSet->count){ // all can move
			for (i=0;i<_passedSet->count;++i){
				_passedSet->pieces[i].movingFlag|=FLAG_HMOVE;
				_passedSet->pieces[i].diffHMoveTime = _passedBoard->settings.hMoveTime;
				_passedSet->pieces[i].completeHMoveTime = _sTime+_passedSet->pieces[i].diffHMoveTime-_passedSet->pieces[i].completeHMoveTime;
				_passedSet->pieces[i].transitionDeltaX = _direction;
				_passedSet->pieces[i].tileX+=_direction;
				if (_upShiftNeeded){
					_passedSet->pieces[i].tileY-=_upShiftNeeded;
					_passedSet->pieces[i].completeFallTime=_sTime;
				}
			}
			resetDyingFlagMaybe(_passedBoard,_passedSet);
		}
	}
}
void forceSetSetX(struct pieceSet* _passedSet, int _leftX, char _isRelative){
	// Calculate amount to subtract from position
	int _minDeltaX;
	if (!_isRelative){
		_minDeltaX=99;
		int i;
		for (i=0;i<_passedSet->count;++i){
			int _deltaX = _passedSet->pieces[i].tileX-_leftX;
			if (_deltaX<_minDeltaX){
				_minDeltaX=_deltaX;
			}
		}
	}else{
		_minDeltaX=_leftX*-1;
	}
	//
	int i;
	for (i=0;i<_passedSet->count;++i){
		_passedSet->pieces[i].tileX-=_minDeltaX;
	}
}
// Returns a piece that isn't the anchor piece. Can return NULL if there's only one piece in the set and it's the anchor.
// Always returns first or second piece.
struct movingPiece* getNotAnchorPiece(struct pieceSet* _passedSet){
	if (_passedSet->pieces!=_passedSet->rotateAround){
		return _passedSet->pieces;
	}else if (_passedSet->count>1){
		return &_passedSet->pieces[1];
	}
	return NULL;
}
// Duplicate
struct pieceSet* dupPieceSet(struct pieceSet* _passedSet){
	struct pieceSet* _ret = malloc(sizeof(struct pieceSet));
	memcpy(_ret,_passedSet,sizeof(struct pieceSet));

	// Duplicate the movingPieces stored in _passedSet
	struct movingPiece* _newPieces = malloc(sizeof(struct movingPiece)*_ret->count);
	int i;
	for (i=0;i<_ret->count;++i){
		memcpy(&_newPieces[i],&_passedSet->pieces[i],sizeof(struct movingPiece));
	}
	_ret->pieces=_newPieces;

	// Find the index of the rotateAround in _passedSet
	int _rotateAroundIndex=-1;
	for (i=0;i<_passedSet->count;++i){
		if (_passedSet->rotateAround==&_passedSet->pieces[i]){
			_rotateAroundIndex=i;
			break;
		}
	}
	if (_rotateAroundIndex==-1){
		printf("Error in dupPieceSet\n");
		_rotateAroundIndex=0;
	}
	_ret->rotateAround = &_ret->pieces[_rotateAroundIndex];
	return _ret;
}
void lazyUpdateSetDisplay(struct pieceSet* _passedSet, u64 _sTime){
	int i;
	for (i=0;i<_passedSet->count;++i){
		lazyUpdatePieceDisplay(&(_passedSet->pieces[i]),_sTime);
	}
}
char setCanObeyShift(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, int _xDist, int _yDist){
	// Make sure all pieces in this set can obey the force shift
	int j;
	for (j=0;j<_passedSet->count;++j){
		if (getBoard(&_passedBoard->lowBoard,_passedSet->pieces[j].tileX+_xDist,_passedSet->pieces[j].tileY+_yDist)!=COLOR_NONE){
			return 0;
		}
	}
	return 1;
}
struct pieceSet getRandomPieceSet(struct gameSettings* _passedSettings, int _boardW){
	struct pieceSet _ret;
	int _spawnCol = getSpawnCol(_boardW);
	#if TESTFEVERPIECE
		_ret.count=3;
	#else
		_ret.count=2;
	#endif
	_ret.isSquare=0;
	_ret.quickLock=0;
	_ret.pieces = calloc(1,sizeof(struct movingPiece)*_ret.count);

	_ret.pieces[1].tileX=_spawnCol;
	_ret.pieces[1].tileY=0;
	_ret.pieces[1].displayX=0;
	_ret.pieces[1].displayY=1;
	_ret.pieces[1].movingFlag=0;
	_ret.pieces[1].color=randInt(0,_passedSettings->numColors-1)+COLOR_REALSTART;

	_ret.pieces[0].tileX=_spawnCol;
	_ret.pieces[0].tileY=1;
	_ret.pieces[0].displayX=0;
	_ret.pieces[0].displayY=0;
	_ret.pieces[0].movingFlag=0;
	_ret.pieces[0].color=randInt(0,_passedSettings->numColors-1)+COLOR_REALSTART;

	#if TESTFEVERPIECE
		_ret.pieces[2].tileX=3;
		_ret.pieces[2].tileY=1;
		_ret.pieces[2].displayX=0;
		_ret.pieces[2].displayY=0;
		_ret.pieces[2].movingFlag=0;
		_ret.pieces[2].color=randInt(0,numColors-1)+COLOR_REALSTART;
		snapPieceDisplayPossible(&(_ret.pieces[2]));
	#endif

	_ret.rotateAround = &(_ret.pieces[0]);
	snapPieceDisplayPossible(_ret.pieces);
	snapPieceDisplayPossible(&(_ret.pieces[1]));
	return _ret;
}
void freePieceSet(struct pieceSet* _freeThis){
	free(_freeThis->pieces);
	free(_freeThis);
}
void drawGhostIcon(int _color, int _x, int _y, struct puyoSkin* _passedSkin, short tilew){
	int _destSize = tilew*GHOSTPIECERATIO;
	drawTexturePartSized(_passedSkin->img,_x+easyCenter(_destSize,tilew),_y+easyCenter(_destSize,tilew),_destSize,_destSize,_passedSkin->ghostX[_color-COLOR_REALSTART],_passedSkin->ghostY[_color-COLOR_REALSTART],_passedSkin->puyoW,_passedSkin->puyoH);
}
void drawPiecesetOffset(int _offX, int _offY, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin, int tilew){
	int i;
	for (i=0;i<_myPieces->count;++i){
		drawNormPiece(_myPieces->pieces[i].color,_offX+FIXDISP(_myPieces->pieces[i].displayX),_offY+FIXDISP(_myPieces->pieces[i].displayY),0,_passedSkin,tilew);
	}
}
// Don't draw based on the position data stored in the pieces, draw the _anchorIndex puyo at _x, _y and then draw all the other pieces relative to it.
void drawPiecesetRelative(int _x, int _y, int _anchorIndex, int tilew, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin){
	int i;
	for (i=0;i<_myPieces->count;++i){
		drawNormPiece(_myPieces->pieces[i].color,_x+(_myPieces->pieces[i].tileX-_myPieces->pieces[_anchorIndex].tileX)*tilew,_y+(_myPieces->pieces[i].tileY-_myPieces->pieces[_anchorIndex].tileY)*tilew,0,_passedSkin,tilew);
	}
}
/*
return bits:
0 - nothing special
1 - piece locked
	- piece set should be freed
2 - piece moved to next tile
	- If we started moving down another tile
	- If we started to autolock
4 - Frame difference values set. They should be set to 0 at the end of the frame.
*/
signed char updatePieceSet(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, u64 _sTime){
	char _ret=0;
	int i;
	for (i=0;i<_passedSet->count;++i){
		if (updatePieceDisplay(&(_passedSet->pieces[i]),_sTime)){
			_ret|=4;
		}
	}
	// Update display for rotating pieces. Not in updatePieceDisplay because only sets can rotate
	// TODO - Square pieces from fever can't rotate yet
	if (_passedSet->isSquare==0){
		for (i=0;i<_passedSet->count;++i){
			if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE) && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
				if (_sTime>=_passedSet->pieces[i].completeRotateTime){ // displayX and displayY have already been set
					UNSET_FLAG(_passedSet->pieces[i].movingFlag,FLAG_ANY_ROTATE);
					snapPieceDisplayPossible(&(_passedSet->pieces[i]));
				}else{
					char _isClockwise = (_passedSet->pieces[i].movingFlag & FLAG_ROTATECW);
					signed char _dirRelation = getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY);
					int _trigSignX;
					int _trigSignY;
					getRotateTrigSign(_isClockwise, _dirRelation, &_trigSignX, &_trigSignY);
					double _angle = partMoveFills(_sTime,_passedSet->pieces[i].completeRotateTime,_passedBoard->settings.rotateTime,M_PI_2);
					if (_dirRelation & (DIR_LEFT | DIR_RIGHT)){
						_angle = M_PI_2 - _angle;
					}
					// completely overwrite the displayX and displayY set by updatePieceDisplay
					_passedSet->pieces[i].displayX=_passedSet->rotateAround->displayX+cos(_angle)*_trigSignX;
					_passedSet->pieces[i].displayY=_passedSet->rotateAround->displayY+sin(_angle)*_trigSignY;
				}
			}
		}
	}
	// Processing part.
	// If the piece isn't falling, maybe we can make it start falling
	if (!(_passedSet->pieces[0].movingFlag & FLAG_MOVEDOWN)){
		char _shouldLock=0;
		if (_passedSet->pieces[0].movingFlag & FLAG_DEATHROW){ // Autolock if we've sat with no space under for too long
			if (_sTime>=_passedSet->pieces[0].completeFallTime){
				_shouldLock=1;
			}
		}else{
			_ret|=2;
			char _needRepeat=1;
			while(_needRepeat){
				_needRepeat=0;
				if (puyoSetCanFell(_passedBoard,_passedSet)){
					for (i=0;i<_passedSet->count;++i){
						_forceStartPuyoGravity(&(_passedSet->pieces[i]),_passedBoard->settings.fallTime,_sTime-_passedSet->pieces[i].completeFallTime); // offset the time by the extra frames we may've missed
						if (updatePieceDisplayY(&(_passedSet->pieces[i]),_sTime,1)){ // We're using the partial time from the last frame, so we may need to jump down a little bit.
							_needRepeat=1;
						}
					}
				}else{
					if (_passedSet->quickLock){ // For autofall pieces
						_shouldLock=1;
					}else{
						for (i=0;i<_passedSet->count;++i){
							_forceStartPuyoAutoplaceTime(&(_passedSet->pieces[i]),_passedBoard->settings.fallTime,_sTime-_passedSet->pieces[i].completeFallTime); // offset the time by the extra frames we may've missed
						}
					}
				}
			}
		}
		// We may need to lock, either because we detect there's no free space with quickLock on or deathRow time has expired
		if (_shouldLock){
			// autolock all pieces
			for (i=0;i<_passedSet->count;++i){
				placePuyo(_passedBoard,_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->pieces[i].color,_passedBoard->settings.squishTime,_sTime);
			}
			_ret|=1;
		}
	}
	return _ret;
}
// _sTime only needed if _changeFlags
void forceRotateSet(struct pieceSet* _passedSet, char _isClockwise, char _changeFlags, struct gameSettings* _passedSettings, u64 _sTime){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE)==0 && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
			int _destX;
			int _destY;
			getPostRotatePos(_isClockwise,getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY),&_destX,&_destY);
			_passedSet->pieces[i].tileX+=_destX;
			_passedSet->pieces[i].tileY+=_destY;
			if (_changeFlags){ // may only use settings if here
				_passedSet->pieces[i].completeRotateTime = _sTime+_passedSettings->rotateTime;
				_passedSet->pieces[i].movingFlag|=(_isClockwise ? FLAG_ROTATECW : FLAG_ROTATECC);
			}
		}
	}
}
// If _allowForceShift>0 then the piece set may be shifted
// If _allowForceShift is 2 then also change flags when force shift
// _sTime only needed if_allowForceShift == 2
char setCanRotate(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard, char _isClockwise, char _allowForceShift, u64 _sTime){
	if (!_passedSet->isSquare){
		int i;
		for (i=0;i<_passedSet->count;++i){
			if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE)==0 && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
				int _destX;
				int _destY;
				getPostRotatePos(_isClockwise,getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY),&_destX,&_destY);
				_destX+=_passedSet->pieces[i].tileX;
				_destY+=_passedSet->pieces[i].tileY;
				if (getBoard(&_passedBoard->lowBoard,_destX,_destY)!=COLOR_NONE){
					if (_allowForceShift){
						int _xDist;
						int _yDist;
						getRelationCoords(_destX, _destY, _passedSet->rotateAround->tileX, _passedSet->rotateAround->tileY, &_xDist, &_yDist);
						_xDist*=-1;
						_yDist*=-1;
						// If they can all obey the force shift, shift them all
						if (setCanObeyShift(_passedBoard,_passedSet,_xDist,_yDist)){
							// HACK - If the other pieces rotating in this set can't rotate, these new positions set below would remain. For the piece shapes I'll have in my game, it is impossible for one piece to be able to rotate but not another.
							int j;
							for (j=0;j<_passedSet->count;++j){
								_passedSet->pieces[j].tileX+=_xDist;
								_passedSet->pieces[j].tileY+=_yDist;
								if (_allowForceShift==2){ // Optional change flags
									if (_yDist!=0){
										UNSET_FLAG(_passedSet->pieces[j].movingFlag,(FLAG_MOVEDOWN | FLAG_DEATHROW));
										_passedSet->pieces[j].completeFallTime=0;
									}
									if (_xDist!=0){
										UNSET_FLAG(_passedSet->pieces[j].movingFlag,FLAG_HMOVE);
									}
								}
							}
							if (_allowForceShift==2){ // Optional change flags
								resetDyingFlagMaybe(_passedBoard,_passedSet);
								// If there's a forced shift, give it a smooth transition by hvaing the anchor piece, which all the other pieces' positions are relative to, move smoothly.
								if (_xDist!=0){
									_passedSet->rotateAround->movingFlag|=FLAG_HMOVE;
									_passedSet->rotateAround->diffHMoveTime = _passedBoard->settings.rotateTime;
									_passedSet->rotateAround->completeHMoveTime = _sTime+_passedSet->rotateAround->diffHMoveTime;
									_passedSet->rotateAround->transitionDeltaX = _xDist;
								}
							}
						}else{
							return 0;
						}
					}else{
						return 0;
					}
				}
			}
		}
	}
	return 1;
}
// _canDoubleRotate only taken into account if no space to rotate.
// It also refers to the ability to swap top and bottom puyos while in a one wide
// First bit tells you if double rotate was used or would've been used.
unsigned char tryStartRotate(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard, char _isClockwise, char _canDoubleRotate, u64 _sTime){
	unsigned char _ret=0;
	if (_passedSet->isSquare==0){
		// if can rotate is confirmed for smash 6
		if (setCanRotate(_passedSet,_passedBoard,_isClockwise,2,_sTime)){
			forceRotateSet(_passedSet,_isClockwise,1,&_passedBoard->settings,_sTime);
			resetDyingFlagMaybe(_passedBoard,_passedSet);
		}else{ // If we can't rotate
			// If we're a regular piece with one piece above the other
			if (_passedSet->count==2 && _passedSet->pieces[0].tileX==_passedSet->pieces[1].tileX){
				if (_canDoubleRotate){ // Do the rotate
					struct movingPiece* _moveOnhis = _passedSet->rotateAround==&(_passedSet->pieces[0])?&(_passedSet->pieces[1]):&(_passedSet->pieces[0]); // of the two puyos, get the one that isn't the anchor
					int _yChange = 2*(_moveOnhis->tileY<_passedSet->rotateAround->tileY ? 1 : -1);
					char _canProceed=1;
					if (getBoard(&_passedBoard->lowBoard,_moveOnhis->tileX,_moveOnhis->tileY+_yChange)!=COLOR_NONE){
						int _forceYChange = (_yChange*-1)/2;
						if (setCanObeyShift(_passedBoard,_passedSet,0,_forceYChange)){
							int i;
							for (i=0;i<_passedSet->count;++i){
								_passedSet->pieces[i].tileY+=_forceYChange;
							}
						}else{
							_canProceed=0;
						}
					}
					if (_canProceed){
						_moveOnhis->tileY+=_yChange;
						_moveOnhis->movingFlag|=FLAG_ROTATECW;
						_moveOnhis->completeRotateTime = _sTime+_passedBoard->settings.rotateTime;
						resetDyingFlagMaybe(_passedBoard,_passedSet);
					}
				}
				_ret|=1;
			}
		}
		// update puyo h shift
		int i;
		for (i=0;i<_passedSet->count;++i){
			snapPieceDisplayPossible(&(_passedSet->pieces[i]));
		}
	}
	return _ret;
}
void pieceSetControls(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, struct controlSet* _passedControls, u64 _sTime, signed char _moveDirection){
	if (_moveDirection!=0){
		tryHShiftSet(_passedSet,_passedBoard,_moveDirection,_sTime);
	}
	if (wasJustPressed(BUTTON_A) || wasJustPressed(BUTTON_B)){
		rotateButtonPress(_passedBoard,_passedSet,_passedControls,wasJustPressed(BUTTON_A),_sTime);
	}
}
//////////////////////////////////////////////////
// puyoBoard
//////////////////////////////////////////////////
// if a regular piece is touching the x
// applies the death status for you if needed.
char applyBoardDeath(struct puyoBoard* _passedBoard, u64 _sTime){
	if (fastGetBoard(_passedBoard->lowBoard,getSpawnCol(_passedBoard->lowBoard.w),_passedBoard->numGhostRows)!=0 && _passedBoard->lowBoard.pieceStatus[getSpawnCol(_passedBoard->lowBoard.w)][_passedBoard->numGhostRows]==0){
		_passedBoard->lowBoard.statusTimeEnd=_sTime+DEATHANIMTIME;
		_passedBoard->lowBoard.status=STATUS_DEAD;
		return 1;
	}
	return 0;
}
int getFreeColumnYPos(struct puyoBoard* _passedBoard, int _columnIndex, int _minY){
	int i;
	for (i=_passedBoard->lowBoard.h-1;i>=_minY;--i){
		if (fastGetBoard(_passedBoard->lowBoard,_columnIndex,i)==COLOR_NONE){
			return i;
		}
	}
	return _minY-1;
}
void addSetToBoard(struct puyoBoard* _passedBoard, struct pieceSet* _addThis){
	if (_addThis->count==0){
		printf("Error - tried to add piece set with 0 pieces\n");
		return;
	}
	++(_passedBoard->numActiveSets);
	struct pieceSet* _pieceCopy = malloc(sizeof(struct pieceSet));
	memcpy(_pieceCopy,_addThis,sizeof(struct pieceSet));
	addnList(&_passedBoard->activeSets)->data=_pieceCopy;
}
void removeSetFromBoard(struct puyoBoard* _passedBoard, int _removeIndex){
	if (_passedBoard->numActiveSets==0){
		printf("Tried to remove set when have 0 sets.\n");
		return;
	}
	struct nList* _freeThis = removenList(&_passedBoard->activeSets,_removeIndex);
	free(((struct pieceSet*)_freeThis->data)->pieces);
	free(_freeThis->data);
	free(_freeThis);
	--_passedBoard->numActiveSets;
}
void clearBoardPopCheck(struct puyoBoard* _passedBoard){
	int i;
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		memset(_passedBoard->popCheckHelp[i],0,_passedBoard->lowBoard.h*sizeof(char));
	}
}
void resetBoard(struct puyoBoard* _passedBoard){
	_passedBoard->lowBoard.status = STATUS_NORMAL;
	clearBoardBoard(&_passedBoard->lowBoard);
	clearPieceStatus(&_passedBoard->lowBoard);	
	clearBoardPopCheck(_passedBoard);
}
void freeBoard(struct puyoBoard* _passedBoard){
	freeColorArray(_passedBoard->lowBoard.board,_passedBoard->lowBoard.w);
	printf("TODO - freeBoard\n");
}
void initPuyoSettings(struct gameSettings* _passedSettings){
	// default settings
	_passedSettings->pointsPerGar=70;
	_passedSettings->numColors=4;
	_passedSettings->minPopNum=4;
	_passedSettings->popTime=500;
	_passedSettings->nextWindowTime=200;
	_passedSettings->rotateTime=50;
	_passedSettings->hMoveTime=30;
	_passedSettings->fallTime=500;
	_passedSettings->postSquishDelay=100;
	_passedSettings->pushMultiplier=13;
	_passedSettings->maxGarbageRows=5;
	_passedSettings->squishTime=300;
}
struct puyoBoard* newBoard(int _w, int _h, int numGhostRows, int _numNextPieces, struct gameSettings* _usableSettings, struct puyoSkin* _passedSkin){
	struct puyoBoard* _retBoard = malloc(sizeof(struct puyoBoard));
	_retBoard->lowBoard = newGenericBoard(_w,_h);
	_retBoard->numGhostRows=numGhostRows;
	_retBoard->popCheckHelp = newJaggedArrayChar(_w,_h);
	_retBoard->numActiveSets=0;
	_retBoard->activeSets=NULL;
	_retBoard->score=0;
	_retBoard->leftoverGarbage=0;
	_retBoard->curChain=0;
	_retBoard->readyGarbage=0;
	_retBoard->incomingLength=0;
	_retBoard->incomingGarbage=NULL;
	resetBoard(_retBoard);
	_retBoard->numNextPieces=_numNextPieces+1;
	_retBoard->nextPieces = malloc(sizeof(struct pieceSet)*_retBoard->numNextPieces);
	memcpy(&_retBoard->settings,_usableSettings,sizeof(struct gameSettings));
	int i;
	for (i=0;i<_retBoard->numNextPieces;++i){
		_retBoard->nextPieces[i]=getRandomPieceSet(&_retBoard->settings,_w);
	}
	_retBoard->usingSkin=_passedSkin;
	return _retBoard;
}
char canTile(struct puyoBoard* _passedBoard, int _searchColor, int _x, int _y){
	return (getBoard(&_passedBoard->lowBoard,_x,_y)==_searchColor && (_passedBoard->lowBoard.pieceStatus[_x][_y]==0 || _passedBoard->lowBoard.pieceStatus[_x][_y]==PIECESTATUS_POSTSQUISH)); // Unless _searchColor is the out of bounds color, it's impossible for the first check to pass with out of bounds coordinates. Therefor no range checks needed for second check.
}
unsigned char getTilingMask(struct puyoBoard* _passedBoard, int _x, int _y){
	if (_passedBoard->lowBoard.board[_x][_y]==COLOR_GARBAGE){
		return 0;
	}
	unsigned char _ret=0;
	if (canTile(_passedBoard,_passedBoard->lowBoard.board[_x][_y],_x,_y+1)){
		_ret|=AUTOTILEDOWN;
	}
	if (_y!=_passedBoard->numGhostRows && canTile(_passedBoard,_passedBoard->lowBoard.board[_x][_y],_x,_y-1)){
		_ret|=AUTOTILEUP;
	}
	if (canTile(_passedBoard,_passedBoard->lowBoard.board[_x][_y],_x+1,_y)){
		_ret|=AUTOTILERIGHT;
	}
	if (canTile(_passedBoard,_passedBoard->lowBoard.board[_x][_y],_x-1,_y)){
		_ret|=AUTOTILELEFT;
	}
	return _ret;
}
void drawPuyoBoard(struct puyoBoard* _drawThis, int _startX, int _startY, char _isPlayerBoard, int tilew, u64 _sTime){
	int _oldStartY = _startY;
	_startX+=PUYOBORDERSMALLSZ;
	// temp draw garbage
	int _totalGarbage = _drawThis->readyGarbage;
	int i;
	for (i=0;i<_drawThis->incomingLength;++i){
		_totalGarbage+=_drawThis->incomingGarbage[i];
	}
	if (_totalGarbage!=0){
		int _symbolAmounts[_drawThis->usingSkin->numQueueImages];
		int _loopAmount = _totalGarbage;
		for (i=0;i<_drawThis->usingSkin->numQueueImages;++i){
			_symbolAmounts[i] = _loopAmount % _drawThis->lowBoard.w;
			_loopAmount/=_drawThis->lowBoard.w;
		}
		if (_loopAmount!=0){
			_symbolAmounts[_drawThis->usingSkin->numQueueImages-1]=_drawThis->lowBoard.w;
		}
		int _curSymbolX=0;
		for (i=_drawThis->usingSkin->numQueueImages-1;i>=0;--i){
			int j;
			for (j=0;j<_symbolAmounts[i];++j){
				drawTexturePartSized(_drawThis->usingSkin->img,_startX+_curSymbolX*tilew,_startY,tilew,tilew,_drawThis->usingSkin->queueIconX[i],_drawThis->usingSkin->queueIconY[i],_drawThis->usingSkin->puyoW,_drawThis->usingSkin->puyoH);
				if ((++_curSymbolX)==_drawThis->lowBoard.w){
					i=-1;
					break;
				}
			}
		}
	}
	// Find full size of the board and start clipping that area
	_startY=_startY+tilew+PUYOBORDERSMALLSZ;
	int _fullWidth=tilew*_drawThis->lowBoard.w+PUYOBORDERSMALLSZ*2;
	int _fullHeight=tilew*(_drawThis->lowBoard.h-_drawThis->numGhostRows)+PUYOBORDERSMALLSZ*2;
	enableClipping(_startX-PUYOBORDERSMALLSZ,_startY-PUYOBORDERSMALLSZ,_fullWidth,_fullHeight);
	if (_drawThis->lowBoard.status==STATUS_DEAD){
		if (_sTime<=_drawThis->lowBoard.statusTimeEnd){
			_startY+=partMoveFills(_sTime,_drawThis->lowBoard.statusTimeEnd,DEATHANIMTIME,(_drawThis->lowBoard.h-_drawThis->numGhostRows+1)*tilew);
		}else{
			disableClipping();
			return;
		}
	}
	drawRectangle(_startX,_startY,tilew*_drawThis->lowBoard.w,(_drawThis->lowBoard.h-_drawThis->numGhostRows)*tilew,150,0,0,255);
	if (_isPlayerBoard && _drawThis->lowBoard.status==STATUS_NORMAL){
		clearBoardPopCheck(_drawThis);
		struct pieceSet* _passedSet = _drawThis->activeSets->data;
		int* _yDests = getSetDestY(_passedSet,_drawThis);
		// For use when checking for pops
		setTempPieces(_passedSet,_drawThis,_yDests,0);
		// Check for potential pops, draw ghost icon, and then remove temp piece
		int i;
		for (i=0;i<_passedSet->count;++i){
			int _tileX = _passedSet->pieces[i].tileX;
			if (_yDests[i]>=_drawThis->numGhostRows){
				// If this puyo hasen't been checked yet
				if (_drawThis->popCheckHelp[_tileX][_yDests[i]]!=1 && _drawThis->popCheckHelp[_tileX][_yDests[i]]!=POSSIBLEPOPBYTE){
					// Try get puyos. It is impossible for this to overwrite ones that have been marked with POSSIBLEPOPBYTE
					if (getPopNum(_drawThis,_tileX,_yDests[i],1,_passedSet->pieces[i].color)>=_drawThis->settings.minPopNum){
						// Mark all those puyos with POSSIBLEPOPBYTE
						getPopNum(_drawThis,_tileX,_yDests[i],POSSIBLEPOPBYTE,_passedSet->pieces[i].color);
					}
				}
				// Any check that could've needed this temp puyo is now over. Remove the temp piece.
				_drawThis->lowBoard.board[_passedSet->pieces[i].tileX][_yDests[i]]=COLOR_NONE;
			}
			//
			if (_yDests[i]>=_drawThis->numGhostRows){
				drawGhostIcon(_passedSet->pieces[i].color,_startX+_tileX*tilew,_startY+(_yDests[i]-_drawThis->numGhostRows)*tilew,_drawThis->usingSkin,tilew);
			}
		}
		free(_yDests);
	}
	for (i=0;i<_drawThis->lowBoard.w;++i){
		double _squishyY;
		char _isSquishyColumn=0;
		int j;
		for (j=_drawThis->lowBoard.h-1;j>=_drawThis->numGhostRows;--j){
			if (_drawThis->lowBoard.board[i][j]!=0){
				if (_isSquishyColumn){
					if (_drawThis->lowBoard.board[i][j]!=COLOR_GARBAGE){
						_squishyY-=drawSquishingPuyo(_drawThis->lowBoard.board[i][j],tilew*i+_startX,_squishyY,_drawThis->settings.squishTime,_drawThis->lowBoard.pieceStatusTime[i][j],_drawThis->usingSkin,tilew,_sTime);
					}else{
						drawNormPiece(_drawThis->lowBoard.board[i][j],tilew*i+_startX,_squishyY,0,_drawThis->usingSkin,tilew);
						_squishyY-=tilew;
					}
				}else{
					switch (_drawThis->lowBoard.pieceStatus[i][j]){
						case PIECESTATUS_POPPING:
							drawPoppingPiece(_drawThis->lowBoard.board[i][j],tilew*i+_startX,tilew*(j-_drawThis->numGhostRows)+_startY,_drawThis->lowBoard.statusTimeEnd,_drawThis->settings.popTime,_drawThis->usingSkin,tilew,_sTime);
							break;
						case PIECESTATUS_SQUSHING:
							_isSquishyColumn=1;
							_squishyY=tilew*(j-_drawThis->numGhostRows)+_startY;
							++j; // Redo this iteration where we'll draw as squishy column
							break;
						default:
							if (_isPlayerBoard && _drawThis->popCheckHelp[i][j]==POSSIBLEPOPBYTE){
								drawPotentialPopPiece(_drawThis->lowBoard.board[i][j],tilew*i+_startX,tilew*(j-_drawThis->numGhostRows)+_startY,getTilingMask(_drawThis,i,j),_drawThis->usingSkin,tilew);
							}else{
								drawNormPiece(_drawThis->lowBoard.board[i][j],tilew*i+_startX,tilew*(j-_drawThis->numGhostRows)+_startY,getTilingMask(_drawThis,i,j),_drawThis->usingSkin,tilew);
							}
							break;
					}
				}
			}
		}
	}
	// draw falling pieces, active pieces, etc
	ITERATENLIST(_drawThis->activeSets,{
			drawPiecesetOffset(_startX,_startY+(_drawThis->numGhostRows*tilew*-1),_curnList->data,_drawThis->usingSkin,tilew);
		});
	// border
	drawPreciseWindow(&boardBorder,_startX-PUYOBORDERSMALLSZ,_startY-PUYOBORDERSMALLSZ,_fullWidth,_fullHeight,PUYOBORDERSZ,0xFF);
	// Board drawing done
	disableClipping();
	// Draw next window, animate if needed
	int _nextWindowX = _startX+PUYOBORDERSMALLSZ*3+_drawThis->lowBoard.w*tilew;
	int _nextWidth = NEXTWINDOWTILEW*tilew+PUYOBORDERSMALLSZ;
	int _nextHeight = (_drawThis->numNextPieces-1)*2*tilew;
	drawRectangle(_nextWindowX,_startY,_nextWidth,_nextHeight,BRICKBGCOLOR); // next window background
	drawPreciseWindow(&boardBorder,_nextWindowX-PUYOBORDERSMALLSZ,_startY-PUYOBORDERSMALLSZ,_nextWidth+PUYOBORDERSMALLSZ*2,_nextHeight+PUYOBORDERSMALLSZ*2,PUYOBORDERSZ,0xFF);
	enableClipping(_nextWindowX,_startY,_nextWidth,_nextHeight); // assumes pieces are two tall
	_nextWindowX+=PUYOBORDERSMALLSZ/2;
	if (_drawThis->lowBoard.status!=STATUS_NEXTWINDOW){
		for (i=0;i<_drawThis->numNextPieces-1;++i){
			drawPiecesetRelative(_nextWindowX+(i&1)*tilew,_startY+i*tilew*2+tilew,0,tilew,&(_drawThis->nextPieces[i]),_drawThis->usingSkin);
		}
	}else{
		for (i=0;i<_drawThis->numNextPieces;++i){
			int _xChange;
			if (i!=0){
				_xChange=partMoveFills(_sTime, _drawThis->lowBoard.statusTimeEnd, _drawThis->settings.nextWindowTime, tilew)*((i&1) ? -1 : 1);
			}else{
				_xChange=0;
			}
			drawPiecesetRelative(_nextWindowX+(i&1)*tilew+_xChange,_startY+i*tilew*2+tilew-partMoveFills(_sTime, _drawThis->lowBoard.statusTimeEnd, _drawThis->settings.nextWindowTime, tilew*2),0,tilew,&(_drawThis->nextPieces[i]),_drawThis->usingSkin);
		}
	}
	// end draw next window
	disableClipping();
	if (_drawThis->lowBoard.status==STATUS_DEAD && _sTime<_drawThis->lowBoard.statusTimeEnd){
		// Draw death rectangle
		drawRectangle(_startX,_oldStartY+(_drawThis->lowBoard.h-_drawThis->numGhostRows+1)*tilew,_drawThis->lowBoard.w*tilew,tilew,255,0,0,255);
	}else{
		// chain
		if (_drawThis->lowBoard.status==STATUS_POPPING){
			char* _drawString = easySprintf("%d-COMBO",_drawThis->curChain);
			int _cachedWidth = textWidth(regularFont,_drawString);
			gbDrawText(regularFont,_startX+cap(FIXDISP(_drawThis->chainNotifyX)-_cachedWidth/2,0,_drawThis->lowBoard.w*tilew-_cachedWidth),_startY+FIXDISP(_drawThis->chainNotifyY)+textHeight(regularFont)/2,_drawString,95,255,83);
			free(_drawString);
		}
		// draw score
		// half small size padding above and below score
		// 1 small size padding each on left and right
		char* _drawString = easySprintf("%08"PRIu64,_drawThis->score);
		int _scoreTextW=textWidth(regularFont,_drawString);
		int _scoreDrawX=_startX+easyCenter(_scoreTextW,_drawThis->lowBoard.w*tilew);
		int _scoreDrawY= _startY+PUYOBORDERSMALLSZ+(_drawThis->lowBoard.h-_drawThis->numGhostRows)*tilew;
		int _scoreBoxW=_scoreTextW+PUYOBORDERSMALLSZ*4;
		int _scoreBoxH=textHeight(regularFont)+PUYOBORDERSMALLSZ*2;
		int _scoreBoxX=_scoreDrawX-PUYOBORDERSMALLSZ*2;
		drawRectangle(_scoreBoxX,_scoreDrawY,_scoreBoxW,_scoreBoxH,BRICKBGCOLOR); // score background
		drawPreciseWindow(&boardBorder,_scoreBoxX,_scoreDrawY,_scoreBoxW,_scoreBoxH,PUYOBORDERSZ,0xFF^1);
		gbDrawText(regularFont,_scoreDrawX,_scoreDrawY+PUYOBORDERSMALLSZ/2,_drawString,255,255,255);
		free(_drawString);
	}
}

void removeBoardPartialTimes(struct puyoBoard* _passedBoard){
	ITERATENLIST(_passedBoard->activeSets,{
			struct pieceSet* _curSet = _curnList->data;
			int j;
			for (j=0;j<_curSet->count;++j){
				removePartialTimes(&(_curSet->pieces[j]));
			}
		});
}
int getPopNum(struct puyoBoard* _passedBoard, int _x, int _y, char _helpChar, pieceColor _shapeColor){
	if (_y>=_passedBoard->numGhostRows && getBoard(&_passedBoard->lowBoard,_x,_y)==_shapeColor){
		if (_passedBoard->popCheckHelp[_x][_y]!=_helpChar){
			_passedBoard->popCheckHelp[_x][_y]=_helpChar;
			int _addRet=0;
			_addRet+=getPopNum(_passedBoard,_x-1,_y,_helpChar,_shapeColor);
			_addRet+=getPopNum(_passedBoard,_x+1,_y,_helpChar,_shapeColor);
			_addRet+=getPopNum(_passedBoard,_x,_y-1,_helpChar,_shapeColor);
			_addRet+=getPopNum(_passedBoard,_x,_y+1,_helpChar,_shapeColor);
			return 1+_addRet;
		}
	}
	return 0;
}
// sets the pieceStatus for all the puyos in this shape to the _setStatusToThis. can be used to set the shape to popping, or maybe set it to highlighted.
// you should've already checked this shape's length with getPopNum
void setPopStatus(struct puyoBoard* _passedBoard, char _setStatusToThis, int _x, int _y, pieceColor _shapeColor, int* _retAvgX, int* _retAvgY){
	if (_y>=_passedBoard->numGhostRows){
		pieceColor _curColor = getBoard(&_passedBoard->lowBoard,_x,_y);
		if (_curColor==COLOR_GARBAGE){
			_passedBoard->popCheckHelp[_x][_y]=2;
			_passedBoard->lowBoard.pieceStatus[_x][_y]=_setStatusToThis;
		}else if (_curColor==_shapeColor && _passedBoard->popCheckHelp[_x][_y]!=2){
			_passedBoard->popCheckHelp[_x][_y]=2;
			_passedBoard->lowBoard.pieceStatus[_x][_y]=_setStatusToThis;
			setPopStatus(_passedBoard,_setStatusToThis,_x-1,_y,_shapeColor,_retAvgX,_retAvgY);
			setPopStatus(_passedBoard,_setStatusToThis,_x+1,_y,_shapeColor,_retAvgX,_retAvgY);
			setPopStatus(_passedBoard,_setStatusToThis,_x,_y-1,_shapeColor,_retAvgX,_retAvgY);
			setPopStatus(_passedBoard,_setStatusToThis,_x,_y+1,_shapeColor,_retAvgX,_retAvgY);
			*_retAvgY+=_y;
			*_retAvgX+=_x;
		}
	}
}
// return 0 if no puyo will fall
// return 1 if stuff will fall
char transitionBoardFallMode(struct puyoBoard* _passedBoard, u64 _sTime){
	char _ret=0;
	int i;
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		// Find the first empty spot in this column with a puyo above it
		int _nextFallY=-20; // Y position of the destination of the next puyo to fall.
		int j;
		for (j=_passedBoard->lowBoard.h-1;j>0;--j){ // > 0 because it's useless if top column is empty
			if (fastGetBoard(_passedBoard->lowBoard,i,j-1)!=COLOR_NONE && fastGetBoard(_passedBoard->lowBoard,i,j)==COLOR_NONE){
				int k;
				_ret=1;
				if (_nextFallY==-20){ // init this variable if needed
					_nextFallY = getFreeColumnYPos(_passedBoard,i,0);
				}
				--j; // Start at the piece we found
				// get number of pieces for this piece set
				int _numPieces=0;
				k=j;
				do{
					++_numPieces;
					--k;
				}while(k>=0 && fastGetBoard(_passedBoard->lowBoard,i,k)!=COLOR_NONE);
				struct pieceSet _newSet;
				_newSet.pieces=malloc(sizeof(struct movingPiece)*_numPieces);
				_newSet.count=_numPieces;
				_newSet.isSquare=0;
				_newSet.quickLock=1;
				for (k=0;k<_numPieces;++k){
					struct movingPiece _newPiece;
					memset(&_newPiece,0,sizeof(struct movingPiece));
					_newPiece.tileX=i;
					_newPiece.tileY=j-k; // set this is required for _lowStartPuyoFall
					_newPiece.color=_passedBoard->lowBoard.board[i][j-k];
					_passedBoard->lowBoard.board[i][j-k]=COLOR_NONE;
					snapPieceDisplayPossible(&_newPiece);
					_lowStartPuyoFall(&_newPiece,_nextFallY--,SPLITFALLTIME(_passedBoard->settings.fallTime),_sTime);
					_newSet.pieces[k] = _newPiece;
				}
				addSetToBoard(_passedBoard,&_newSet);
				j-=(_numPieces-1); // This means that if there was just one piece added, don't do anything because we already subtracted one.
			}
		}
	}
	if (_ret){
		_passedBoard->lowBoard.status=STATUS_DROPPING;
	}
	return _ret;
}
void transitionBoardNextWindow(struct puyoBoard* _passedBoard, u64 _sTime){
	_passedBoard->lowBoard.status=STATUS_NEXTWINDOW;
	_passedBoard->lowBoard.statusTimeEnd=_sTime+_passedBoard->settings.nextWindowTime;
}
// _passedState is optional
// _returnForIndex will tell it which set to return the value of
// pass -1 to get return values from all
// TODO - The _returnForIndex is a bit useless right now because the same index can refer to two piece sets. Like if you want to return for index 0, and index 0 is a removed piece set. Then index 1 will also become index 0.
signed char updatePuyoBoard(struct puyoBoard* _passedBoard, struct gameState* _passedState, signed char _returnForIndex, u64 _sTime){
	if (_passedBoard->lowBoard.status==STATUS_DEAD){
		return 0;
	}
	// If we're done dropping, try popping
	if (_passedBoard->lowBoard.status==STATUS_DROPPING && _passedBoard->numActiveSets==0){
		_passedBoard->lowBoard.status=STATUS_SETTLESQUISH;
	}else if (_passedBoard->lowBoard.status==STATUS_DROPGARBAGE && _passedBoard->numActiveSets==0){
		if (applyBoardDeath(_passedBoard,_sTime)){
			return 0;
		}else{
			transitionBoardNextWindow(_passedBoard,_sTime);
		}
	}else if (_passedBoard->lowBoard.status==STATUS_SETTLESQUISH){ // When we're done squishing, try popping
		int _x, _y;
		char _doneSquishing=1;
		for (_x=0;_x<_passedBoard->lowBoard.w;++_x){
			for (_y=_passedBoard->lowBoard.h-1;_y>=0;--_y){
				if (_passedBoard->lowBoard.pieceStatus[_x][_y] & (PIECESTATUS_SQUSHING | PIECESTATUS_POSTSQUISH)){
					_doneSquishing=0;
					_x=_passedBoard->lowBoard.w;
					break;
				}
			}
		}
		if (_doneSquishing){
			clearPieceStatus(&_passedBoard->lowBoard);
			clearBoardPopCheck(_passedBoard);
			int _numGroups=0; // Just number of unique groups that we're popping. So it's 1 or 2.
			int _numUniqueColors=0; // Optional variable. Can calculate using _whichColorsFlags
			int _totalGroupBonus=0; // Actual group bonus for score
			long _whichColorsFlags=0;
			int _x, _y, _numPopped=0;
			// Average tile position of the popped puyos. Right now, it takes all new pops into account.
			double _avgX=0;
			double _avgY=0;
			for (_x=0;_x<_passedBoard->lowBoard.w;++_x){
				for (_y=_passedBoard->numGhostRows;_y<_passedBoard->lowBoard.h;++_y){
					if (fastGetBoard(_passedBoard->lowBoard,_x,_y)>=COLOR_REALSTART && _passedBoard->popCheckHelp[_x][_y]==0){
						int _possiblePop;
						if ((_possiblePop=getPopNum(_passedBoard,_x,_y,1,_passedBoard->lowBoard.board[_x][_y]))>=_passedBoard->settings.minPopNum){
							long _flagIndex = 1L<<(_passedBoard->lowBoard.board[_x][_y]-COLOR_REALSTART);
							if (!(_whichColorsFlags & _flagIndex)){ // If this color index isn't in our flag yet, up the unique colors count
								++_numUniqueColors;
								_whichColorsFlags|=_flagIndex;
							}
							_totalGroupBonus+=groupBonus[cap(_possiblePop-(_passedBoard->settings.minPopNum>=STANDARDMINPOP ? STANDARDMINPOP : _passedBoard->settings.minPopNum),0,7)]; // bonus for number of puyo in the group.
							++_numGroups;
							_numPopped+=_possiblePop;
							int _totalX=0;
							int _totalY=0;
							setPopStatus(_passedBoard,PIECESTATUS_POPPING,_x,_y,_passedBoard->lowBoard.board[_x][_y],&_totalX,&_totalY);
							_avgX=((_totalX/(double)_possiblePop)+_avgX)/_numGroups;
							_avgY=((_totalY/(double)_possiblePop)+_avgY)/_numGroups;
						}
						
					}
				}
			}
			if (_numGroups!=0){
				// Used when updating garbage
				u64 _oldScore = _passedBoard->curChain!=0 ? _passedBoard->curChainScore : 0;
				_passedBoard->curChain++;
				_passedBoard->chainNotifyX=_avgX;
				_passedBoard->chainNotifyY=(_avgY-_passedBoard->numGhostRows);
				_passedBoard->curChainScore=(10*_numPopped)*cap(chainPowers[cap(_passedBoard->curChain-1,0,23)]+colorCountBouns[cap(_numUniqueColors-1,0,5)]+_totalGroupBonus,1,999);
				_passedBoard->lowBoard.status=STATUS_POPPING;
				_passedBoard->lowBoard.statusTimeEnd=_sTime+_passedBoard->settings.popTime;
				// Send new garbage
				if (_passedState!=NULL){
					double _oldGarbage=_passedBoard->leftoverGarbage+scoreToGarbage(&_passedBoard->settings,_oldScore);
					//printf("last time we had %f from a socre of %ld with leftover %f\n",_oldGarbage,_oldScore,(_oldGarbage-floor(_oldGarbage)));
					//printf("This score of %ld leads to %f with toal of %f new\n",_passedBoard->curChainScore,scoreToGarbage(&_passedState->settings,_passedBoard->curChainScore),scoreToGarbage(&_passedState->settings,_passedBoard->curChainScore)+(_oldGarbage-floor(_oldGarbage)));
					int _newGarbage = scoreToGarbage(&_passedBoard->settings,_passedBoard->curChainScore)+(_oldGarbage-floor(_oldGarbage));
					sendGarbage(_passedState,_passedBoard,_newGarbage);
				}
			}else{
				if (applyBoardDeath(_passedBoard,_sTime)){
					return 0;
				}
				if (_passedBoard->readyGarbage!=0){
					int _fullRows = _passedBoard->readyGarbage/_passedBoard->lowBoard.w;
					if (_fullRows>_passedBoard->settings.maxGarbageRows){
						_fullRows=_passedBoard->settings.maxGarbageRows;
					}
					struct pieceSet* _garbageColumns = malloc(sizeof(struct pieceSet)*_passedBoard->lowBoard.w);
					int i;
					for (i=0;i<_passedBoard->lowBoard.w;++i){
						_garbageColumns[i].count=_fullRows;
						_garbageColumns[i].isSquare=0;
						_garbageColumns[i].quickLock=1;
					}
					if (_fullRows!=_passedBoard->settings.maxGarbageRows && _passedBoard->readyGarbage%_passedBoard->lowBoard.w!=0){ // If we're not the max drop lines and we can put the leftovers on top.
						int _numTop = _passedBoard->readyGarbage%_passedBoard->lowBoard.w; // Garbage on top
						int i;
						for (i=0;i<_numTop;++i){
							int _nextX = randInt(0,_passedBoard->lowBoard.w-1-i);
							// Find the column to put this on
							int j;
							for (j=0;;++j){
								if (_garbageColumns[j].count==_fullRows){ // If this column has not gotten an extra one yet
									if ((_nextX--)==0){
										++(_garbageColumns[j].count);
										break;
									}
								}

							}
						}
					}
					// Spawn in garbage
					for (i=0;i<_passedBoard->lowBoard.w;++i){
						if (_garbageColumns[i].count==0){
							continue;
						}
						int _firstDestY=getFreeColumnYPos(_passedBoard,i,0);
						if (_firstDestY==-1){
							continue;
						}
						// As much as we can fit
						if (_firstDestY-(_garbageColumns[i].count)<0){
							_garbageColumns[i].count=_firstDestY+1;
						}
						_garbageColumns[i].pieces = malloc(sizeof(struct movingPiece)*_garbageColumns[i].count);
						int j;
						for (j=0;j<_garbageColumns[i].count;++j){
							struct movingPiece _newPiece;
							memset(&_newPiece,0,sizeof(struct movingPiece));
							_newPiece.tileX=i;
							_newPiece.tileY=j*-1-1; // the tile it would apparently be falling from
							_newPiece.color=COLOR_GARBAGE;
							snapPieceDisplayPossible(&_newPiece);
							_lowStartPuyoFall(&_newPiece,_firstDestY-j,GARBAGEFALLTIME(_passedBoard->settings.fallTime),_sTime);
							_garbageColumns[i].pieces[j] = _newPiece;
						}
						addSetToBoard(_passedBoard,&_garbageColumns[i]);
					}
					//
					_passedBoard->lowBoard.status=STATUS_DROPGARBAGE;
					if (_fullRows==_passedBoard->settings.maxGarbageRows){
						_passedBoard->readyGarbage-=_passedBoard->settings.maxGarbageRows*_passedBoard->lowBoard.w;
					}else{
						_passedBoard->readyGarbage=0;
					}
				}else{
					transitionBoardNextWindow(_passedBoard,_sTime);
				}
			}
		}
	}else if (_passedBoard->lowBoard.status==STATUS_NEXTWINDOW){
		_passedBoard->curChain=0;
		if (_sTime>=_passedBoard->lowBoard.statusTimeEnd){
			lazyUpdateSetDisplay(&(_passedBoard->nextPieces[0]),_sTime);
			addSetToBoard(_passedBoard,&(_passedBoard->nextPieces[0]));
			memmove(&(_passedBoard->nextPieces[0]),&(_passedBoard->nextPieces[1]),sizeof(struct pieceSet)*(_passedBoard->numNextPieces-1));
			_passedBoard->nextPieces[_passedBoard->numNextPieces-1] = getRandomPieceSet(&_passedBoard->settings,_passedBoard->lowBoard.w);
			_passedBoard->lowBoard.status=STATUS_NORMAL;
		}
	}else if (_passedBoard->lowBoard.status==STATUS_POPPING){
		if (_sTime>=_passedBoard->lowBoard.statusTimeEnd){
			int _x, _y;
			for (_x=0;_x<_passedBoard->lowBoard.w;++_x){
 				for (_y=0;_y<_passedBoard->lowBoard.h;++_y){
					if (_passedBoard->lowBoard.pieceStatus[_x][_y]==PIECESTATUS_POPPING){
						_passedBoard->lowBoard.board[_x][_y]=0;
					}
				}
			}
			// Apply garbage or calculate leftover score
			if (_passedState!=NULL){
				char _shouldApply=1;
				// Make a backup of the current board state because we're going to mess  up the other one
				pieceColor** _oldBoard = newJaggedArrayColor(_passedBoard->lowBoard.w,_passedBoard->lowBoard.h);
				int i;
				for (i=0;i<_passedBoard->lowBoard.w;++i){
					memcpy(_oldBoard[i],_passedBoard->lowBoard.board[i],sizeof(pieceColor)*_passedBoard->lowBoard.h);
				}				
				// Only check for connections if we need to
				if (forceFallStatics(_passedBoard)){					
					clearBoardPopCheck(_passedBoard);
					// If this is the last link of the chain
					if (boardHasConnections(_passedBoard)){
						_shouldApply=0;
					}
				}
				// Restore backup board
				freeColorArray(_passedBoard->lowBoard.board,_passedBoard->lowBoard.w);
				_passedBoard->lowBoard.board=_oldBoard;
				//
				if (_shouldApply){
					stateApplyGarbage(_passedState,_passedBoard);
					double _totalSent=_passedBoard->leftoverGarbage+scoreToGarbage(&_passedBoard->settings,_passedBoard->curChainScore);
					_passedBoard->leftoverGarbage=_totalSent-floor(_totalSent);
					//printf("Applied. %f leftover from score %ld.\n",_passedBoard->leftoverGarbage,_passedBoard->curChainScore);
				}
			}
			if (_passedBoard->curChain!=0){
				// add the points from the last pop
				_passedBoard->score+=_passedBoard->curChainScore;
			}
			// Assume that we did kill at least one puyo because we wouldn't be in this situation if there weren't any to kill.
			// Assume that we popped and therefor need to drop, I mean.
			transitionBoardFallMode(_passedBoard,_sTime);
			_passedBoard->lowBoard.status=STATUS_DROPPING;
		}
	}
	// Process piece statuses.
	// This would be more efficient to just add to the draw code, but it would add processing to draw loop.
	int _x, _y;
	for (_x=0;_x<_passedBoard->lowBoard.w;++_x){
		for (_y=0;_y<_passedBoard->lowBoard.h;++_y){
			switch (_passedBoard->lowBoard.pieceStatus[_x][_y]){
				case PIECESTATUS_SQUSHING:
					if (_passedBoard->lowBoard.pieceStatusTime[_x][_y]<=_sTime){
						_passedBoard->lowBoard.pieceStatus[_x][_y]=PIECESTATUS_POSTSQUISH;
					}
					break;
				case PIECESTATUS_POSTSQUISH:
					if (_passedBoard->lowBoard.pieceStatusTime[_x][_y]+_passedBoard->settings.postSquishDelay<=_sTime){ // Reuses time from PIECESTATUS_SQUSHING
						_passedBoard->lowBoard.pieceStatus[_x][_y]=0;
					}
					break;
			}
		}
	}
	// Process piece sets
	signed char _ret=0;
	char _shouldTransition=0;
	int i=0;
	ITERATENLIST(_passedBoard->activeSets,{
			signed char _got = updatePieceSet(_passedBoard,_curnList->data,_sTime);
			if (_returnForIndex==-1 || i==_returnForIndex){
				_ret|=_got;
			}
			if (_got&1){ // if locked. we need to free.
				if (i==0 && _passedBoard->lowBoard.status==STATUS_NORMAL){
					_shouldTransition=1;
				}
				removeSetFromBoard(_passedBoard,i);
				// Because of how ITERATENLIST works, even if we remove the current entry from the list we'll still move properly to the next one.
				// But don't increment index.
			}else{
				++i;
			}
		});
	if (_shouldTransition){
		transitionBoardFallMode(_passedBoard,_sTime);
		_passedBoard->lowBoard.status=STATUS_DROPPING; // even if nothing is going to fall, go to fall mode because that will check for pops and then go to next window mode anyway.
	}
	return _ret;
}
void endFrameUpdateBoard(struct puyoBoard* _passedBoard, signed char _updateRet){
	if (_updateRet&4){ // If the partial times were set this frame, remove the ones that weren't used because the frame is over.
		removeBoardPartialTimes(_passedBoard);
	}
}
// Takes all the static pieces that can fall and force them down
// Returns 1 if pieces are actually going to fall
char forceFallStatics(struct puyoBoard* _passedBoard){
	char _ret=0;
	int i;
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		int j;
		for (j=_passedBoard->lowBoard.h-1;j>0;--j){ // > 0 because it's useless if top column is empty
			// Find the first empty spot in this column with a puyo above it
			if (fastGetBoard(_passedBoard->lowBoard,i,j-1)!=COLOR_NONE && fastGetBoard(_passedBoard->lowBoard,i,j)==COLOR_NONE){
				_ret=1;
				int _nextFallY = getFreeColumnYPos(_passedBoard,i,0);
				--j; // Start at the piece we found
				int k;
				for (k=j;k>=0;--k){
					_passedBoard->lowBoard.board[i][_nextFallY--]=_passedBoard->lowBoard.board[i][k];
					_passedBoard->lowBoard.board[i][k]=0;
				}
				break;
			}
		}
	}
	return _ret;
}
char boardHasConnections(struct puyoBoard* _passedBoard){
	clearBoardPopCheck(_passedBoard);
	int _x, _y;
	for (_x=0;_x<_passedBoard->lowBoard.w;++_x){
		for (_y=_passedBoard->numGhostRows;_y<_passedBoard->lowBoard.h;++_y){
			if (fastGetBoard(_passedBoard->lowBoard,_x,_y)>=COLOR_REALSTART && _passedBoard->popCheckHelp[_x][_y]==0){
				if (getPopNum(_passedBoard,_x,_y,1,_passedBoard->lowBoard.board[_x][_y])>=_passedBoard->settings.minPopNum){
					return 1;
				}
			}
		}
	}
	return 0;
}
//////////////////////////////////////////////////
char aiRunNeeded(struct aiState* _passedState){
	return _passedState->nextAction.anchorDestX==-1;
}
/*
// only works for regular tsu pieces
// ai must modify _retModify to the desired tile positions. _passedState can hold info you may want. _passedBoards has index 0 be the ai's board and all the other boards are enemy boards.
void frogAi(struct pieceSet* _retModify, struct aiState* _passedState, int _numBoards, struct puyoBoard** _passedBoards){
	struct puyoBoard* _aiBoard = _passedBoards[0];
	// Find the column we need to shove puyos into.
	int _spawnPos = _aiBoard->lowBoard.w/2-1;
	int i;
	for (i=_spawnPos+1;i<_aiBoard->lowBoard.w;++i){
		if (fastGetBoard(_aiBoard,i,_aiBoard->numGhostRows)!=COLOR_NONE){
			break;
		}
	}
	int _columnDest=i-1;
	if (_columnDest!=_spawnPos){
		for (i=0;i<_retModify->count;++i){
			_retModify->pieces[i].tileX=_columnDest;
		}
		int* _yDests = getSetDestY(_retModify,_aiBoard);
		for (i=0;i<_retModify->count;++i){
			_retModify->pieces[i].tileY=_yDests[i];
		}
		free(_yDests);
	}else{ // If we've run out of columns to shove puyos into
		// We need to pop

		// This part is incomplete.

		clearBoardPopCheck(_aiBoard);
		// Maybe we want to pop the thing on column three
		int _downCount=-1; // Number of puyos in the top group in column three
		if (fastGetBoard(_aiBoard,_spawnPos,_aiBoard->lowBoard.h-1)==COLOR_NONE){
			for (i=_aiBoard->lowBoard.h-;i>0;--i){
				// Find the first spot with a free spot above
				if (fastGetBoard(_aiBoard,_spawnPos,i-1)==COLOR_NONE){
					_downCount;
					break;
				}
			}
		}
		// Number of puyos in the group in the frog column
		int _sideCount=-1;

	}
}
*/
#define MATCHTHREEAI_POPDENOMINATOR 2.4
#define MATCHTHREEAI_PANICDENOMINATOR 1.4
void matchThreeAi(struct aiState* _passedState, struct pieceSet* _retModify, struct puyoBoard* _aiBoard, int _numBoards, struct gameState* _passedGameState){
	/*
	  Have pop limit affect this

	  Test all possible place positions.
	  Do the loop like this:
	  for 1 to 3{
	  for (entire board x pos){
	  store how good a place here would be
	  }
	  roate
	  }

	  Phase 1, board is no very full: score = total connections puyos after we place. Score gets -4 if it would connect 4.
	  Phase 2, board is pretty full: score = total connected puyos after we place. Score gets +3 if it would connect 4.

	  favor horizontal rotation by subtract extract to score for rotate index 0 and 2
	*/
	int i;
	clearBoardPopCheck(_aiBoard);
	char _nextPopCheckHelp=1;
	// 0 to favor sets of 3
	// 1 to favor popping
	// 2 to really favor popping
	char _panicLevel;
	// Determine _panicLevel
	int _avgStackHeight=0;
	for (i=0;i<_aiBoard->lowBoard.w;++i){
		_avgStackHeight+=(_aiBoard->lowBoard.h-getFreeColumnYPos(_aiBoard,i,0)-1);
	}
	_avgStackHeight = round(_avgStackHeight/(double)_aiBoard->lowBoard.w);
	if (_avgStackHeight>round((_aiBoard->lowBoard.h-_aiBoard->numGhostRows)/(double)MATCHTHREEAI_POPDENOMINATOR)){
		if (_avgStackHeight>round((_aiBoard->lowBoard.h-_aiBoard->numGhostRows)/(double)MATCHTHREEAI_PANICDENOMINATOR)){
			_panicLevel=2;
		}else{
			_panicLevel=1;
		}
	}else{
		_panicLevel=0;
	}
	forceSetSetX(_retModify,0,0);

	// How good each potential place position is
	int _placeScores[_aiBoard->lowBoard.w*4];
	int _spawnCol = getSpawnCol(_aiBoard->lowBoard.w);
	int _bestIndex=0;
	_placeScores[_bestIndex]=-999; // Initialize to beatable score
	int _scoreIndex;
	int _rotateLoop;
	// For each possible rotation
	for (_rotateLoop=0;_rotateLoop<4;++_rotateLoop){
		_scoreIndex=_rotateLoop*_aiBoard->lowBoard.w;
		// Try all x positions until can't move anymore
		while(1){
			int i;
			int* _yDests = getSetDestY(_retModify,_aiBoard);
			// Get the average y dest pos. also check for y dest that's too high and therefor invalid
			int _avgDest=0;
			for (i=0;i<_retModify->count;++i){
				// If it's a valid position and one that won't kill us
				if (_yDests[i]>=_aiBoard->numGhostRows/2 && _yDests[i]>=0 && !(_retModify->pieces[i].tileX==_spawnCol && _yDests[i]<=_aiBoard->numGhostRows)){
					_avgDest+=_yDests[i];
				}else{
					goto increment;
				}
			}
			if (i!=_retModify->count){ // Stop if we've found an invalid position
				continue;
			}
			_placeScores[_scoreIndex]=0;
			// average dest with small numbers being low in the board
			_avgDest=_aiBoard->lowBoard.h-floor(_avgDest/(double)_retModify->count);
			// Calculate scores based on how many will connect
			setTempPieces(_retModify,_aiBoard,_yDests,1);
			char _willPop=0;
			for (i=0;i<_retModify->count;++i){
				int _numConnect = getPopNum(_aiBoard,_retModify->pieces[i].tileX,_yDests[i],(_nextPopCheckHelp++),_retModify->pieces[i].color);
				if (_numConnect!=0){
					_placeScores[_scoreIndex]+=_numConnect;
					if (_numConnect>=_aiBoard->settings.minPopNum){
						_willPop=1;
					}
				}
			}
			if (_willPop){
				if (_panicLevel==0){
					// Penalty for popping early
					_placeScores[_scoreIndex]-=4;
				}else if (_panicLevel==1){
					_placeScores[_scoreIndex]+=2;
				}else if (_panicLevel==2){
					_placeScores[_scoreIndex]+=10;
				}
			}
			// Penalty for stacking a piece vertical if it'll make things higher
			if ((_rotateLoop==0 || _rotateLoop==3) && (_avgDest>_avgStackHeight+1)){
				_placeScores[_scoreIndex]-=1;
			}
			// Small penalty for being high.
			_placeScores[_scoreIndex]-=(_avgDest)/2;
			// Cleanup
			removeTempPieces(_retModify,_aiBoard,_yDests);
			free(_yDests);
			if (_placeScores[_scoreIndex]>_placeScores[_bestIndex]){
				//AIDEBUGprintf("Best is now %d\n",_scoreIndex);
				_bestIndex=_scoreIndex;
			}
		increment:
			// Condition
			if (!setCanObeyShift(_aiBoard,_retModify,1,0)){ // Will stop at walls
				break;
			}
			// Increment
			_scoreIndex++;
			forceSetSetX(_retModify,1,1);
		}
		// Do the next rotation
		forceSetSetX(_retModify,0,0);
		if (setCanRotate(_retModify,_aiBoard,1,1,0)){ // Force shift allowed to account for third rotation
			forceRotateSet(_retModify,1,0,NULL,0); // Rotate, but don't bother with flags
		}else{
			// Rotate back to start position before we go
			for (;_rotateLoop<4;++_rotateLoop){
				forceRotateSet(_retModify,1,0,NULL,0);
			}
			break;
		}
	}
	int _numRotates = _bestIndex/_aiBoard->lowBoard.w;
	int _destX = _bestIndex%_aiBoard->lowBoard.w;
	for (i=0;i<_numRotates;++i){
		forceRotateSet(_retModify,1,0,NULL,0);
	}
	forceSetSetX(_retModify,_destX,0);
}
void updateAi(void* _stateData, struct gameState* _curGameState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime){
	struct puyoBoard* _passedBoard = _passedGenericBoard;
	struct aiState* _passedState = _stateData;
	(void)_updateRet;

	if (_passedBoard->lowBoard.status!=STATUS_NORMAL){
		// Queue another ai update once we're normal again
		_passedState->nextAction.anchorDestX=-1;
		return;
	}
	if (aiRunNeeded(_passedState)){
		// Reset the aiState first
		_passedState->softdropped=0;

		struct pieceSet* _passModify = dupPieceSet(_passedBoard->activeSets->data);
		_passedState->updateFunction(_passedState,_passModify,_passedBoard,_curGameState->numBoards,_curGameState);
		// Make instruction based on returned stuff
		_passedState->nextAction.anchorDestX = _passModify->rotateAround->tileX;
		_passedState->nextAction.anchorDestY = _passModify->rotateAround->tileY;
		struct movingPiece* _altPiece = getNotAnchorPiece(_passModify);
		if (_altPiece!=NULL){
			_passedState->nextAction.secondaryDestX = _altPiece->tileX;
			_passedState->nextAction.secondaryDestY = _altPiece->tileY;
		}else{
			_passedState->nextAction.secondaryDestX=-1;
		}
		freePieceSet(_passModify);
	}else{
		if (!_passedState->softdropped){
			char _canDrop=1;
			// Process the movement we need
			struct pieceSet* _moveThis=_passedBoard->activeSets->data;
			if (_moveThis->rotateAround->tileX!=_passedState->nextAction.anchorDestX){
				tryHShiftSet(_moveThis,_passedBoard,_moveThis->rotateAround->tileX<_passedState->nextAction.anchorDestX ? 1 : -1,_sTime);
				_canDrop=0;
			}
			if (_passedState->nextAction.secondaryDestX!=-1){
				struct movingPiece* _altPiece = getNotAnchorPiece(_moveThis);
				int _destXRelative = _passedState->nextAction.secondaryDestX-_passedState->nextAction.anchorDestX;
				int _curXRelative = _altPiece->tileX-_moveThis->rotateAround->tileX;
				int _destYRelative = _passedState->nextAction.secondaryDestY-_passedState->nextAction.anchorDestY;
				int _curYRelative = _altPiece->tileY-_moveThis->rotateAround->tileY;
				if (_curYRelative!=_destYRelative || _curXRelative!=_destXRelative){
					tryStartRotate(_moveThis,_passedBoard,1,1,_sTime);
					_canDrop=0;
				}
			}
			// Once we're positioned, do the softdrop
			if (_canDrop){
				_passedState->softdropped=1;
				#if AISOFTDROPS==1
				_moveThis->quickLock=1;
				// Figure out how much to move the entire set.
				// Find the first puyo to hit the stack the highest and stop there
				int* _destY = getSetDestY(_moveThis,_passedBoard);
				int _minY=_destY[0];
				int i;
				for (i=1;i<_moveThis->count;++i){
					if (_destY[i]<_minY){
						_minY=_destY[i];
					}
				}
				free(_destY);
				// Figure out how much to move to get to that position we found
				int _minDeltaY=99;
				for (i=0;i<_moveThis->count;++i){
					int _deltaY = _moveThis->pieces[i].tileY-_minY;
					if (_deltaY<_minDeltaY){
						_minDeltaY=_deltaY;
					}
				}
				_minDeltaY*=-1;
				// Apply
				for (i=0;i<_moveThis->count;++i){
					int _tileDiff=_minDeltaY;
					int _bonusTime;
					if (_moveThis->pieces[i].movingFlag & FLAG_MOVEDOWN){
						_bonusTime=_moveThis->pieces[i].completeFallTime-_sTime;
						--_moveThis->pieces[i].tileY; // Correction
						++_tileDiff;
					}else{
						_bonusTime=_moveThis->pieces[i].completeFallTime; // Partial time
					}
					_moveThis->pieces[i].movingFlag|=FLAG_MOVEDOWN;
					_moveThis->pieces[i].tileY=_moveThis->pieces[i].tileY+_tileDiff;
					_moveThis->pieces[i].transitionDeltaY=_tileDiff;

					_moveThis->pieces[i].diffFallTime=(_passedBoard->settings.fallTime*_tileDiff)/(_passedBoard->settings.pushMultiplier+1);
					_moveThis->pieces[i].completeFallTime = _sTime+_moveThis->pieces[i].diffFallTime-_bonusTime/(_passedBoard->settings.pushMultiplier+1);
				}
				#endif
			}
		}
	}
}
//////////////////////////////////////////////////
void puyoDownButtonHold(struct controlSet* _passedControls, struct pieceSet* _targetSet, double _pushMultiplier, u64 _sTime){
	if (_targetSet->pieces[0].movingFlag & FLAG_MOVEDOWN){ // Normal push down
		int _offsetAmount = (_sTime-_passedControls->lastFrameTime)*_pushMultiplier;
		int j;
		for (j=0;j<_targetSet->count;++j){
			if (_offsetAmount>_targetSet->pieces[j].completeFallTime){ // Keep unisnged value from going negative
				_targetSet->pieces[j].completeFallTime=0;
			}else{
				_targetSet->pieces[j].completeFallTime=_targetSet->pieces[j].completeFallTime-_offsetAmount;
			}			
		}
	}else if (_targetSet->pieces[0].movingFlag & FLAG_DEATHROW){ // lock
		int j;
		for (j=0;j<_targetSet->count;++j){
			_targetSet->pieces[j].completeFallTime = 0;
		}
	}
}
void updateTouchControls(struct puyoBoard* _passedBoard, struct controlSet* _passedControls, struct pieceSet* _passedSet, u64 _sTime){
	if (_passedControls->holdStartTime){
		if (!isDown(BUTTON_TOUCH)){ // On release
			if (onTouchRelease(_passedControls)){
				rotateButtonPress(_passedBoard,_passedSet,_passedControls,1,_sTime);
			}
		}else{ // On stable or drag
			signed char _dragDir = touchIsHDrag(_passedControls);
			if (_dragDir){
				_passedControls->didDrag=1;
				resetControlHDrag(_passedControls);
				tryHShiftSet(_passedSet,_passedBoard,_dragDir,_sTime);
			}
			int _touchYDiff = abs(touchY-_passedControls->startTouchY);
			if (_touchYDiff>=softdropMinDrag){
				if (!_passedControls->isTouchDrop){ // New down push
					_passedControls->isTouchDrop=1;
				}else{ // Hold down push
					puyoDownButtonHold(_passedControls,_passedSet,_passedBoard->settings.pushMultiplier,_sTime);
				}
			}else{
				if (_passedControls->isTouchDrop){
					_passedControls->isTouchDrop=0;
				}
			}
		}
	}else{
		if (isDown(BUTTON_TOUCH)){ // On initial touch
			initialTouchDown(_passedControls,_sTime);
		}
	}
}
void updateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime){
	struct puyoBoard* _passedBoard = _passedGenericBoard;
	struct controlSet* _passedControls = _controlData;
	if (_updateRet!=0){
		if (_passedBoard->lowBoard.status==STATUS_NORMAL){
			if (_updateRet&2){
			}
		}
	}
	updateControlDas(_controlData,_sTime);
	if (_passedBoard->lowBoard.status==STATUS_NORMAL){
		struct pieceSet* _targetSet = _passedBoard->activeSets->data;
		updateTouchControls(_passedBoard,_passedControls,_targetSet,_sTime);
		if (isDown(BUTTON_DOWN) && !wasJustPressed(BUTTON_DOWN)){
			puyoDownButtonHold(_passedControls,_targetSet,_passedBoard->settings.pushMultiplier,_sTime);
		}
		pieceSetControls(_passedBoard,_targetSet,_passedControls,_sTime,getDirectionInput(_passedControls,_sTime));
	}
	controlSetFrameEnd(_controlData,_sTime);
}
void rebuildBoardDisplay(struct puyoBoard* _passedBoard, u64 _sTime){
	ITERATENLIST(_passedBoard->activeSets,{
			lazyUpdateSetDisplay(_curnList->data,_sTime);
		});
}
//////////////////////////////////
void addPuyoBoard(struct gameState* _passedState, int i, int _passedW, int _passedH, int _passedGhost, int _passedNextNum, struct gameSettings* _passedSettings, struct puyoSkin* _passedSkin, char _isCpu){
	_passedState->types[i] = BOARD_PUYO;
	_passedState->boardData[i] = newBoard(_passedW,_passedH,_passedGhost,_passedNextNum,_passedSettings,_passedSkin);
	if (_isCpu){
		// CPU controller for board 1
		struct aiState* _newState = malloc(sizeof(struct aiState));
		memset(_newState,0,sizeof(struct aiState));
		_newState->nextAction.anchorDestX=-1;
		_newState->updateFunction=matchThreeAi;
		_passedState->controllers[i].func = updateAi;
		_passedState->controllers[i].data = _newState;
	}else{
		_passedState->controllers[i].func = updateControlSet;
		_passedState->controllers[i].data = newControlSet(goodGetMilli());
	}
}
