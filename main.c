/*
If it takes 16 milliseconds for a frame to pass and we only needed 1 millisecond to progress to the next tile, we can't just say the puyo didn't move for those other 15 milliseconds. We need to account for those 15 milliseconds for the puyo's next falling down action. The difference between the actual finish time and the expected finish time is stored back in the complete dest time variable. In this case, 15 would be stored. We'd take that 15 and account for it when setting any more down movement time values for this frame. But only if we're going to do more down moving this frame. Otherwise we throw that 15 value away at the end of the frame. Anyway, 4 is the bit that indicates that these values were set.
*/
// TODO - Maybe a board can have a pointer to a function to get the next piece. I can map it to either network or random generator
// TODO - battle
// TODO - cpu board
// TODO - Draw board better. Have like a wrapper struct drawableBoard where elements can be repositioned or remove.
// TODO - Only check for potential pops on piece move?

#define TESTFEVERPIECE 0

#define __USE_MISC // enable MATH_PI_2
#include <stdlib.h>
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
#include "goodLinkedList.h"
#include "skinLoader.h"
#include "scoreConstants.h"

#if !defined M_PI
	#warning makeshift M_PI
	#define M_PI 3.14159265358979323846
#endif

#if !defined M_PI_2
	#warning makeshift M_PI_2
	#define M_PI_2 (M_PI/(double)2)
#endif

#define AUTOTILEDOWN 	0b00000001
#define AUTOTILEUP 		0b00000010
#define AUTOTILERIGHT 	0b00000100
#define AUTOTILELEFT	0b00001000

#define DIR_NONE	0b00000000
#define DIR_UP 	 	0b00000001
#define DIR_LEFT 	0b00000010
#define DIR_DOWN 	0b00000100
#define DIR_RIGHT 	0b00001000

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

#define UNSET_FLAG(_holder, _mini) _holder&=(0xFFFF ^ _mini)

// Time after the squish animation before next pop check
#define POSTSQUISHDELAY 100

// When checking for potential pops if the current piece set moves as is, this is the byte we use in popCheckHelp to identify this puyo as a verified potential pop
#define POSSIBLEPOPBYTE 5

int tileh = 45;
#define tilew tileh
#define HALFTILE (tilew/2)

#define DASTIME 150
#define DOUBLEROTATETAPTIME 350
#define USUALSQUISHTIME 300
#define POPANIMRELEASERATIO .50 // The amount of the total squish time is used to come back up
#define SQUISHDOWNLIMIT .30 // The smallest of a puyo's original size it will get when squishing. Given as a decimal percent. .30 would mean puyo doesn't get less than 30% of its size

#define STANDARDMINPOP 4 // used when calculating group bonus.

#define GHOSTPIECERATIO .80

crossFont regularFont;

// How long it takes a puyo to fall half of one tile on the y axis
int FALLTIME = 500;
int HMOVETIME = 30;
int ROTATETIME = 50;
int NEXTWINDOWTIME = 200;
int popTime = 500;
int minPopNum = 4;
int numColors=4;

typedef int puyoColor;

#define COLOR_NONE 0
#define COLOR_IMPOSSIBLE 1
#define COLOR_REALSTART 2

// bitmap
#define PIECESTATUS_POPPING 1
#define PIECESTATUS_SQUSHING 2
#define PIECESTATUS_POSTSQUISH 4
// #define					   8

#define FPSCOUNT 1
#define SHOWFPSCOUNT 0
#define PARTICLESENABLED 0

#define SQUISHNEXTFALLTIME 30
#define SQUISHDELTAY (tileh)

#define fastGetBoard(_passedBoard,_x,_y) (_passedBoard->board[_x][_y])

typedef enum{
	STATUS_UNDEFINED,
	STATUS_NORMAL, // We're moving the puyo around
	STATUS_POPPING, // We're waiting for puyo to pop
	STATUS_DROPPING, // puyo are falling into place. This is the status after you place a piece and after STATUS_POPPING
	STATUS_SETTLESQUISH, // A status after STATUS_DROPPING to wait for all puyos to finish their squish animation. Needed because some puyos start squish before others. When done, checks for pops and goes to STATUS_NEXTWINDOW or STATUS_POPPING
	STATUS_NEXTWINDOW, // Waiting for next window. This is the status after STATUS_DROPPING if no puyo connect
}boardStatus;
struct puyoBoard{
	int w;
	int h;
	puyoColor** board; // ints represent colors
	char** pieceStatus;
	u64** pieceStatusTime;
	char** popCheckHelp; // 1 if already checked, 2 if already set to popping. 0 otherwise. Can also be POSSIBLEPOPBYTE
	boardStatus status;
	int numActiveSets;
	struct nList* activeSets;
	int numNextPieces;
	struct pieceSet* nextPieces;
	u64 popFinishTime;
	struct puyoSkin* usingSkin;
	u64 nextWindowTime;
	int numGhostRows;
	u64 score;
	u64 nextScoreAdd; // The score we're going to add after the puyo finish popping
	int curChain;
	int chainNotifyX;
	int chainNotifyY;
};
struct movingPiece{
	puyoColor color;
	int tileX;
	int tileY;
	int displayY;
	int displayX;
	char holdingDown; // flag - is this puyo being forced down
	u64 holdDownStart;

	// Variables relating to smooth transition
	int transitionDeltaX;
	int transitionDeltaY;
	unsigned char movingFlag; // sideways, down, rotate
	u64 completeFallTime; // Time when the current falling down will complete
	u64 referenceFallTime; // Copy of completeFallTime, unadjusted for the current down hold
	int diffFallTime; // How long it'll take to fall
	u64 completeRotateTime;
	int diffRotateTime;
	u64 completeHMoveTime;
	int diffHMoveTime;
};
struct pieceSet{
	signed char isSquare; // If this is a square set of puyos then this will be the width of that square. 0 otherwise
	struct movingPiece* rotateAround; // If this isn't a square, rotate around this puyo
	int singleTileVSpeed;
	int singleTileHSpeed;
	char quickLock;
	int count;
	struct movingPiece* pieces;
};
struct controlSet{
	int dasChargeEnd;
	signed char dasDirection;
	u64 startHoldTime;
	u64 lastFailedRotateTime;
	struct puyoBoard* target;
	u64 lastFrameTime;
};
struct squishyBois{
	int numPuyos;
	// 0 indicates it's at the top
	int* yPos;
	int destY; // bottom of the dest
	u64 startSquishTime;
	u64 startTime;
};
// main.h
void drawSingleGhostColumn(int _offX, int _offY, int _tileX, struct puyoBoard* _passedBoard, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin);
int getPopNum(struct puyoBoard* _passedBoard, int _x, int _y, char _helpChar, puyoColor _shapeColor);

int screenWidth;
int screenHeight;

u64 _globalReferenceMilli;

struct puyoSkin currentSkin;
//////////////////////////////////////////////////////////
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
struct controlSet newControlSet(struct puyoBoard* _passed, u64 _sTime){
	struct controlSet _ret;
	_ret.dasDirection=0;
	_ret.startHoldTime=0;
	_ret.lastFailedRotateTime=0;
	_ret.target=_passed;
	_ret.lastFrameTime=_sTime;
	return _ret;
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
/*
// _outX and _outY with be -1 or 1
void getPreRotatePos(char _isClockwise, char _dirRelation, int* _outX, int* _outY){
	if (!_isClockwise){ // If you look at how the direction constants are lined up, this will work to map counter clockwiese to their equivilant clockwise transformations
		if (_dirRelation & (DIR_UP | DIR_DOWN)){
			_dirRelation=_dirRelation>>1;
		}else{
			_dirRelation=_dirRelation<<1;
		}
	}
	*_outX=0;
	*_outY=0;
	switch(_dirRelation){
		case DIR_UP:
			--*_outX;
			++*_outY;
			break;
		case DIR_DOWN:
			++*_outX;
			--*_outY;
			break;
		case DIR_LEFT:
			++*_outX;
			++*_outY;
			break;
		case DIR_RIGHT:
			--*_outY;
			--*_outX;
			break;
	}
}
*/
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
u64 goodGetMilli(){
	return getMilli()-_globalReferenceMilli;
}
void XOutFunction(){
	exit(0);
}
void placePuyo(struct puyoBoard* _passedBoard, int _x, int _y, puyoColor _passedColor, int _squishTime, u64 _sTime){
	_passedBoard->board[_x][_y]=_passedColor;
	_passedBoard->pieceStatus[_x][_y]=PIECESTATUS_SQUSHING;
	_passedBoard->pieceStatusTime[_x][_y]=_squishTime+_sTime;
}
int getBoard(struct puyoBoard* _passedBoard, int _x, int _y){
	if (_x<0 || _y<0 || _x>=_passedBoard->w || _y>=_passedBoard->h){
		return COLOR_IMPOSSIBLE;
	}
	return _passedBoard->board[_x][_y];
}
//////////////////////////////////////////////////
// movingPiece
//////////////////////////////////////////////////
void lowSnapPieceTileX(struct movingPiece* _passedPiece){
	_passedPiece->displayX = _passedPiece->tileX*tilew;
}
void lowSnapPieceTileY(struct movingPiece* _passedPiece){
	_passedPiece->displayY = _passedPiece->tileY*tileh;
}
// Will update the puyo's displayX and displayY for the axis it isn't moving on.
void snapPuyoDisplayPossible(struct movingPiece* _passedPiece){
	if (!(_passedPiece->movingFlag & FLAG_ANY_ROTATE)){
		if (!(_passedPiece->movingFlag & FLAG_ANY_HMOVE)){
			lowSnapPieceTileX(_passedPiece);
		}
		if (!(_passedPiece->movingFlag & FLAG_MOVEDOWN)){
			lowSnapPieceTileY(_passedPiece);
		}
	}
}
void drawPotentialPopPuyo(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int _size){
	drawTexturePartSizedTintAlpha(_passedSkin->img,_drawX,_drawY,_size,_size,_passedSkin->colorX[_color-COLOR_REALSTART][_tileMask],_passedSkin->colorY[_color-COLOR_REALSTART][_tileMask],_passedSkin->puyoW,_passedSkin->puyoH,255,255,255,200);
}
void drawNormPuyo(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int _size){
	drawTexturePartSized(_passedSkin->img,_drawX,_drawY,_size,_size,_passedSkin->colorX[_color-COLOR_REALSTART][_tileMask],_passedSkin->colorY[_color-COLOR_REALSTART][_tileMask],_passedSkin->puyoW,_passedSkin->puyoH);
}
void drawPoppingPuyo(int _color, int _drawX, int _drawY, u64 _destPopTime, struct puyoSkin* _passedSkin, u64 _sTime){
	int _destSize=tilew*(_destPopTime-_sTime)/(double)popTime;
	drawTexturePartSized(_passedSkin->img,_drawX+(tilew-_destSize)/2,_drawY+(tileh-_destSize)/2,_destSize,_destSize,_passedSkin->colorX[_color-COLOR_REALSTART][0],_passedSkin->colorY[_color-COLOR_REALSTART][0],_passedSkin->puyoW,_passedSkin->puyoH);
}
double drawSquishRatioPuyo(int _color, int _drawX, int _drawY, double _ratio, struct puyoSkin* _passedSkin){
	if (_ratio<0){ // upsquish
		_ratio=1+_ratio*-1;
	}
	double _destHeight = tileh*_ratio;
	drawTexturePartSized(_passedSkin->img,_drawX,_drawY+(1-_ratio)*tileh,tilew,_destHeight,_passedSkin->colorX[_color-COLOR_REALSTART][0],_passedSkin->colorY[_color-COLOR_REALSTART][0],_passedSkin->puyoW,_passedSkin->puyoH);
	return _destHeight;
}
double drawSquishingPuyo(int _color, int _drawX, int _drawY, int _diffPopTime, u64 _endPopTime, struct puyoSkin* _passedSkin, u64 _sTime){
	double _partRatio;
	int _releaseTimePart = _diffPopTime*POPANIMRELEASERATIO;
	if (_endPopTime>_sTime+_releaseTimePart){
		_partRatio = 1-partMoveFills(_sTime,_endPopTime-_releaseTimePart,_diffPopTime-_releaseTimePart,1-SQUISHDOWNLIMIT);
	}else{
		_partRatio = SQUISHDOWNLIMIT+partMoveFills(_sTime,_endPopTime,_releaseTimePart,1-SQUISHDOWNLIMIT);
	}
	return drawSquishRatioPuyo(_color,_drawX,_drawY,_partRatio,_passedSkin);
}
char updatePieceDisplayY(struct movingPiece* _passedPiece, u64 _sTime, char _canUnset){
	if (_passedPiece->movingFlag & FLAG_MOVEDOWN){ // If we're moving down, update displayY until we're done, then snap and unset
		if (_sTime>=_passedPiece->completeFallTime){ // Unset if done
			if (_canUnset){
				UNSET_FLAG(_passedPiece->movingFlag,FLAG_MOVEDOWN);
				snapPuyoDisplayPossible(_passedPiece);
				_passedPiece->completeFallTime=_sTime-_passedPiece->completeFallTime;
				return 1;
			}else{
				lowSnapPieceTileY(_passedPiece);
			}
		}else{ // Partial position if not done
			_passedPiece->displayY = _passedPiece->tileY*tileh-partMoveEmptys(_sTime,_passedPiece->completeFallTime,_passedPiece->diffFallTime,_passedPiece->transitionDeltaY);
		}
	}
	return 0;
}
char updatePieceDisplayX(struct movingPiece* _passedPiece, u64 _sTime, char _canUnset){
	if (_passedPiece->movingFlag & FLAG_ANY_HMOVE){
		if (_sTime>=_passedPiece->completeHMoveTime){
			if (_canUnset){
				UNSET_FLAG(_passedPiece->movingFlag,FLAG_ANY_HMOVE);
				snapPuyoDisplayPossible(_passedPiece);
				_passedPiece->completeHMoveTime=_sTime-_passedPiece->completeHMoveTime;
				return 1;
			}else{
				lowSnapPieceTileX(_passedPiece);
			}
		}else{
			_passedPiece->displayX = _passedPiece->tileX*tilew-partMoveEmptys(_sTime,_passedPiece->completeHMoveTime,_passedPiece->diffHMoveTime,_passedPiece->transitionDeltaX);
		}
	}
	return 0;
}
// like updatePieceDisplay, but temporary. Will not set partial times or transition moving flags, so no need to handle those.
void lazyUpdatePieceDisplay(struct movingPiece* _passedPiece, u64 _sTime){
	snapPuyoDisplayPossible(_passedPiece);
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
void _lowStartPuyoFall(struct movingPiece* _passedPiece, int _destTileY, int _singleFallTime, u64 _sTime){
	_passedPiece->movingFlag|=FLAG_MOVEDOWN;
	int _tileDiff = _destTileY-_passedPiece->tileY;
	_passedPiece->tileY+=_tileDiff;
	_passedPiece->transitionDeltaY = _tileDiff*tileh;
	_passedPiece->diffFallTime=_tileDiff*_singleFallTime;
	_passedPiece->completeFallTime = _sTime+_passedPiece->diffFallTime;
	_passedPiece->referenceFallTime = _passedPiece->completeFallTime;
}
void _forceStartPuyoGravity(struct movingPiece* _passedPiece, int _singleFallTime, u64 _sTime){
	_lowStartPuyoFall(_passedPiece,_passedPiece->tileY+1,_singleFallTime,_sTime);
}
void _forceStartPuyoAutoplaceTime(struct movingPiece* _passedPiece, int _singleFallTime, u64 _sTime){
	_passedPiece->movingFlag|=FLAG_DEATHROW;
	_passedPiece->completeFallTime = _sTime+_singleFallTime/2;
}
char puyoCanFell(struct puyoBoard* _passedBoard, struct movingPiece* _passedPiece){
	return (getBoard(_passedBoard,_passedPiece->tileX,_passedPiece->tileY+1)==COLOR_NONE);
}
//////////////////////////////////////////////////
// pieceSet
//////////////////////////////////////////////////
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
		if (getBoard(_passedBoard,_passedSet->pieces[j].tileX+_xDist,_passedSet->pieces[j].tileY+_yDist)!=COLOR_NONE){
			return 0;
		}
	}
	return 1;
}
struct pieceSet getRandomPieceSet(){
	struct pieceSet _ret;
	#if TESTFEVERPIECE
		_ret.count=3;
	#else
		_ret.count=2;
	#endif
	_ret.isSquare=0;
	_ret.quickLock=0;
	_ret.singleTileVSpeed=FALLTIME;
	_ret.singleTileHSpeed=HMOVETIME;
	_ret.pieces = calloc(1,sizeof(struct movingPiece)*_ret.count);

	_ret.pieces[1].tileX=2;
	_ret.pieces[1].tileY=0;
	_ret.pieces[1].displayX=0;
	_ret.pieces[1].displayY=tileh;
	_ret.pieces[1].movingFlag=0;
	_ret.pieces[1].color=rand()%numColors+COLOR_REALSTART;
	_ret.pieces[1].holdingDown=1;

	_ret.pieces[0].tileX=2;
	_ret.pieces[0].tileY=1;
	_ret.pieces[0].displayX=0;
	_ret.pieces[0].displayY=0;
	_ret.pieces[0].movingFlag=0;
	_ret.pieces[0].color=rand()%numColors+COLOR_REALSTART;
	_ret.pieces[0].holdingDown=0;

	#if TESTFEVERPIECE
		_ret.pieces[2].tileX=3;
		_ret.pieces[2].tileY=1;
		_ret.pieces[2].displayX=0;
		_ret.pieces[2].displayY=0;
		_ret.pieces[2].movingFlag=0;
		_ret.pieces[2].color=rand()%numColors+COLOR_REALSTART;
		_ret.pieces[2].holdingDown=0;
		snapPuyoDisplayPossible(&(_ret.pieces[2]));
	#endif

	_ret.rotateAround = &(_ret.pieces[0]);
	snapPuyoDisplayPossible(_ret.pieces);
	snapPuyoDisplayPossible(&(_ret.pieces[1]));
	return _ret;
}
void freePieceSet(struct pieceSet* _freeThis){
	free(_freeThis->pieces);
	free(_freeThis);
}
void drawGhostIcon(int _color, int _x, int _y, struct puyoSkin* _passedSkin){
	int _destSize = tilew*GHOSTPIECERATIO;
	drawTexturePartSized(_passedSkin->img,_x+easyCenter(_destSize,tilew),_y+easyCenter(_destSize,tileh),_destSize,_destSize,_passedSkin->ghostX[_color-COLOR_REALSTART],_passedSkin->ghostY[_color-COLOR_REALSTART],_passedSkin->puyoW,_passedSkin->puyoH);
}
// We draw columns in order to prevent redrawing
// Find the next column and draw it
void drawNextGhostColumn(int _prevColumn, int _offX, int _offY, struct puyoBoard* _passedBoard, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin){
	int _newLowest=_passedBoard->w;
	int i;
	for (i=0;i<_myPieces->count;++i){
		if (_myPieces->pieces[i].tileX>_prevColumn && _myPieces->pieces[i].tileX<_newLowest){
			_newLowest=_myPieces->pieces[i].tileX;
		}
	}
	if (_newLowest==_passedBoard->w){
		return;
	}
	drawSingleGhostColumn(_offX,_offY,_newLowest,_passedBoard,_myPieces,_passedSkin);
}
void drawSingleGhostColumn(int _offX, int _offY, int _tileX, struct puyoBoard* _passedBoard, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin){
	int i;
	for (i=_passedBoard->h-1;i>=_passedBoard->numGhostRows;--i){
		if (fastGetBoard(_passedBoard,_tileX,i)==COLOR_NONE){
			break;
		}
	}
	if (i>=_passedBoard->numGhostRows){
		int _startDest=i;
		int _nextDest=_startDest;
		int _oldLowestY=_passedBoard->h;
		while(1){
			int _newLowest=-1;
			int _newLowestIndex;
			for (i=0;i<_myPieces->count;++i){
				if (_myPieces->pieces[i].tileX==_tileX && _myPieces->pieces[i].tileY<_oldLowestY && _myPieces->pieces[i].tileY>_newLowest){
					_newLowest=_myPieces->pieces[i].tileY;
					_newLowestIndex=i;
				}
			}
			if (_newLowest==-1){
				break;
			}
			// Temporarily put this puyo on the board, we'll use it for checking for potential pops later.
			_passedBoard->board[_tileX][_nextDest]=_myPieces->pieces[_newLowestIndex].color;
			// Draw
			drawGhostIcon(_myPieces->pieces[_newLowestIndex].color,_offX+_tileX*tilew,_offY+(_nextDest - _passedBoard->numGhostRows)*tileh,_passedSkin);
			--_nextDest;
			_oldLowestY=_myPieces->pieces[_newLowestIndex].tileY;
		}
		// Draw the next column. We do this recursively so that by the end we'll have all the temporary pieces set.
		drawNextGhostColumn(_tileX,_offX,_offY,_passedBoard,_myPieces,_passedSkin);
		// Check for potential pops using the temporarily set puyos.
		// The most important check is the last one. During that one, all the pieces will be placed.
		// It may seem like a problem that, after unwinding this recursive stupidity, the first call to this function will check for pops with only its puyos, but it's actually not a problem. If it relied on puyos placed by other columns, those other columns would've already deteced the potential pop. Therefor we're good.
		for (i=_nextDest-1;i<=_startDest;++i){
			if (_passedBoard->popCheckHelp[_tileX][i]!=1 && _passedBoard->popCheckHelp[_tileX][i]!=POSSIBLEPOPBYTE){ // If this puyo hasen't been checked yet
				if (getPopNum(_passedBoard,_tileX,i,1,fastGetBoard(_passedBoard,_tileX,i))>=minPopNum){ // Try get puyos. It is impossible for this to overwrite ones that have been marked with POSSIBLEPOPBYTE
					getPopNum(_passedBoard,_tileX,i,POSSIBLEPOPBYTE,fastGetBoard(_passedBoard,_tileX,i)); // Mark all those puyos with POSSIBLEPOPBYTE
				}
			}
		}
		// Remove the temporarily placed puyos.
		for (i=_nextDest-1;i<=_startDest;++i){
			_passedBoard->board[_tileX][i]=COLOR_NONE;
		}
	}
}
void drawPiecesetOffset(int _offX, int _offY, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin){
	int i;
	for (i=0;i<_myPieces->count;++i){
		drawNormPuyo(_myPieces->pieces[i].color,_offX+_myPieces->pieces[i].displayX,_offY+_myPieces->pieces[i].displayY,0,_passedSkin,tileh);
	}
}
// Don't draw based on the position data stored in the pieces, draw the _anchorIndex puyo at _x, _y and then draw all the other pieces relative to it.
void drawPiecesetRelative(int _x, int _y, int _anchorIndex, int _size, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin){
	int i;
	for (i=0;i<_myPieces->count;++i){
		drawNormPuyo(_myPieces->pieces[i].color,_x+(_myPieces->pieces[i].tileX-_myPieces->pieces[_anchorIndex].tileX)*_size,_y+(_myPieces->pieces[i].tileY-_myPieces->pieces[_anchorIndex].tileY)*_size,0,_passedSkin,tileh);
	}
}
char puyoSetCanFell(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if (!puyoCanFell(_passedBoard,&(_passedSet->pieces[i]))){
			return 0;
		}
	}
	return 1;
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
					snapPuyoDisplayPossible(&(_passedSet->pieces[i]));
				}else{
					char _isClockwise = (_passedSet->pieces[i].movingFlag & FLAG_ROTATECW);
					signed char _dirRelation = getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY);
					int _trigSignX;
					int _trigSignY;
					getRotateTrigSign(_isClockwise, _dirRelation, &_trigSignX, &_trigSignY);
					double _angle = partMoveFills(_sTime,_passedSet->pieces[i].completeRotateTime,ROTATETIME,M_PI_2);
					if (_dirRelation & (DIR_LEFT | DIR_RIGHT)){
						_angle = M_PI_2 - _angle;
					}
					// completely overwrite the displayX and displayY set by updatePieceDisplay
					_passedSet->pieces[i].displayX = _passedSet->rotateAround->displayX+HALFTILE + (cos(_angle)*_trigSignX)*tilew-HALFTILE;
					_passedSet->pieces[i].displayY = _passedSet->rotateAround->displayY+HALFTILE + (sin(_angle)*_trigSignY)*tilew-HALFTILE;
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
						_forceStartPuyoGravity(&(_passedSet->pieces[i]),_passedSet->singleTileVSpeed,_sTime-_passedSet->pieces[i].completeFallTime); // offset the time by the extra frames we may've missed
						if (updatePieceDisplayY(&(_passedSet->pieces[i]),_sTime,1)){ // We're using the partial time from the last frame, so we may need to jump down a little bit.
							_needRepeat=1;
						}
					}
				}else{
					if (_passedSet->quickLock){ // For autofall pieces
						_shouldLock=1;
					}else{
						for (i=0;i<_passedSet->count;++i){
							_forceStartPuyoAutoplaceTime(&(_passedSet->pieces[i]),_passedSet->singleTileVSpeed,_sTime-_passedSet->pieces[i].completeFallTime); // offset the time by the extra frames we may've missed
						}
					}
				}
			}
		}
		// We may need to lock, either because we detect there's no free space with quickLock on or deathRow time has expired
		if (_shouldLock){
			// autolock all pieces
			for (i=0;i<_passedSet->count;++i){
				placePuyo(_passedBoard,_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->pieces[i].color,USUALSQUISHTIME,_sTime);
			}
			_ret|=1;
		}
	}
	return _ret;
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
void resetDyingFlagMaybe(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet){
	if (puyoSetCanFell(_passedBoard,_passedSet)){
		forceResetSetDeath(_passedSet);
	}
}
void pieceSetControls(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, struct controlSet* _passedControls, u64 _sTime, signed char _dasActive){
	if (wasJustPressed(BUTTON_RIGHT) || wasJustPressed(BUTTON_LEFT) || _dasActive!=0){
		if (!(_passedSet->pieces[0].movingFlag & FLAG_ANY_HMOVE)){
			signed char _direction = _dasActive!=0 ? _dasActive : (wasJustPressed(BUTTON_RIGHT) ? 1 : -1);
			char _upShiftNeeded=0;
			int i;
			for (i=0;i<_passedSet->count;++i){
				if (getBoard(_passedBoard,_passedSet->pieces[i].tileX+_direction,_passedSet->pieces[i].tileY)==COLOR_NONE){
					continue;
				}else if (_passedSet->pieces[i].movingFlag & FLAG_MOVEDOWN && (_passedSet->pieces[i].completeFallTime-_sTime)>_passedSet->pieces[i].diffFallTime/2){ // Allow the puyo to jump up a tile a little bit if the timing is tight
					if (getBoard(_passedBoard,_passedSet->pieces[i].tileX+_direction,_passedSet->pieces[i].tileY-1)!=COLOR_NONE){
						break;
					}
					_upShiftNeeded=1;
				}else{
					break;
				}
			}
			if (i==_passedSet->count){ // all can move
				int _setFlag = (_direction==1) ? FLAG_MOVERIGHT : FLAG_MOVELEFT;
				for (i=0;i<_passedSet->count;++i){
					_passedSet->pieces[i].movingFlag|=(_setFlag);
					_passedSet->pieces[i].diffHMoveTime = _passedSet->singleTileHSpeed;
					_passedSet->pieces[i].completeHMoveTime = _sTime+_passedSet->pieces[i].diffHMoveTime-_passedSet->pieces[i].completeHMoveTime;
					_passedSet->pieces[i].transitionDeltaX = tilew*_direction;
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
	if (wasJustPressed(BUTTON_A) || wasJustPressed(BUTTON_B)){
		// todo - find a way to check if any pieces are currently rotating
		char _isClockwise = wasJustPressed(BUTTON_A);
		if (_passedSet->isSquare==0){
			int i;
			// First, make sure all pieces have space to rotate
			for (i=0;i<_passedSet->count;++i){
				if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE)==0 && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
					int _destX;
					int _destY;
					getPostRotatePos(_isClockwise,getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY),&_destX,&_destY);
					_destX+=_passedSet->pieces[i].tileX;
					_destY+=_passedSet->pieces[i].tileY;
					if (getBoard(_passedBoard,_destX,_destY)!=COLOR_NONE){
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
								if (_yDist!=0){
									UNSET_FLAG(_passedSet->pieces[j].movingFlag,(FLAG_MOVEDOWN | FLAG_DEATHROW));
									_passedSet->pieces[j].completeFallTime=0;
								}
								if (_xDist!=0){
									UNSET_FLAG(_passedSet->pieces[j].movingFlag,FLAG_ANY_HMOVE);
								}
							}
							resetDyingFlagMaybe(_passedBoard,_passedSet);
							// If there's a forced shift, give it a smooth transition by hvaing the anchor piece, which all the other pieces' positions are relative to, move smoothly.
							if (_xDist!=0){
								_passedSet->rotateAround->movingFlag|=(_xDist<0 ? FLAG_MOVELEFT : FLAG_MOVERIGHT);
								_passedSet->rotateAround->diffHMoveTime = ROTATETIME;
								_passedSet->rotateAround->completeHMoveTime = _sTime+_passedSet->rotateAround->diffHMoveTime;
								_passedSet->rotateAround->transitionDeltaX = tilew*_xDist;
							}
						}else{
							// can't rotate, break
							break;
						}
					}
				}
			}
			// if can rotate is confirmed for smash 6
			if (i==_passedSet->count){
				for (i=0;i<_passedSet->count;++i){
					if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE)==0 && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
						int _destX;
						int _destY;
						getPostRotatePos(_isClockwise,getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY),&_destX,&_destY);
						_passedSet->pieces[i].tileX+=_destX;
						_passedSet->pieces[i].tileY+=_destY;
						_passedSet->pieces[i].completeRotateTime = _sTime+ROTATETIME;
						_passedSet->pieces[i].movingFlag|=(_isClockwise ? FLAG_ROTATECW : FLAG_ROTATECC);
					}
				}
				resetDyingFlagMaybe(_passedBoard,_passedSet);
			}else{ // If we can't rotate
				// If we're a regular piece with one piece above the other
				if (_passedSet->count==2 && _passedSet->pieces[0].tileX==_passedSet->pieces[1].tileX){
					if (_sTime<=_passedControls->lastFailedRotateTime+DOUBLEROTATETAPTIME){ // Do the rotate
						struct movingPiece* _moveOnhis = _passedSet->rotateAround==&(_passedSet->pieces[0])?&(_passedSet->pieces[1]):&(_passedSet->pieces[0]); // of the two puyos, get the one that isn't the anchor
						int _yChange = 2*(_moveOnhis->tileY<_passedSet->rotateAround->tileY ? 1 : -1);
						char _canProceed=1;
						if (getBoard(_passedBoard,_moveOnhis->tileX,_moveOnhis->tileY+_yChange)!=COLOR_NONE){
							int _forceYChange = (_yChange*-1)/2;
							if (setCanObeyShift(_passedBoard,_passedSet,0,_forceYChange)){
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
							_moveOnhis->completeRotateTime = _sTime+ROTATETIME;
							_passedControls->lastFailedRotateTime=0;
							resetDyingFlagMaybe(_passedBoard,_passedSet);
						}else{
							_passedControls->lastFailedRotateTime=_sTime;
						}
					}else{ // Queue the double press time
						_passedControls->lastFailedRotateTime=_sTime;
					}
				}
			}
			// update puyo h shift
			for (i=0;i<_passedSet->count;++i){
				snapPuyoDisplayPossible(&(_passedSet->pieces[i]));
			}
		}
	}
}
//////////////////////////////////////////////////
// puyoBoard
//////////////////////////////////////////////////
u64** newJaggedArrayu64(int _w, int _h){
	u64** _retArray = malloc(sizeof(u64*)*_w);
	int i;
	for (i=0;i<_w;++i){
		_retArray[i] = malloc(sizeof(u64)*_h);
	}
	return _retArray;
}
puyoColor** newJaggedArrayColor(int _w, int _h){
	puyoColor** _retArray = malloc(sizeof(puyoColor*)*_w);
	int i;
	for (i=0;i<_w;++i){
		_retArray[i] = malloc(sizeof(puyoColor)*_h);
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
void addSetToBoard(struct puyoBoard* _passedBoard, struct pieceSet* _addThis){
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
void clearBoardPieceStatus(struct puyoBoard* _passedBoard){
	int i;
	for (i=0;i<_passedBoard->w;++i){
		memset(_passedBoard->pieceStatus[i],0,_passedBoard->h*sizeof(char));
		memset(_passedBoard->pieceStatusTime[i],0,_passedBoard->h*sizeof(u64));
	}
}
void clearBoardPopCheck(struct puyoBoard* _passedBoard){
	int i;
	for (i=0;i<_passedBoard->w;++i){
		memset(_passedBoard->popCheckHelp[i],0,_passedBoard->h*sizeof(char));
	}
}
void resetBoard(struct puyoBoard* _passedBoard){
	_passedBoard->status = STATUS_NORMAL;
	int i;
	for (i=0;i<_passedBoard->w;++i){
		memset(_passedBoard->board[i],0,_passedBoard->h*sizeof(puyoColor));
	}
	clearBoardPieceStatus(_passedBoard);
	clearBoardPopCheck(_passedBoard);
}
struct puyoBoard newBoard(int _w, int _h, int numGhostRows){
	struct puyoBoard _retBoard;
	_retBoard.w = _w;
	_retBoard.h = _h;
	_retBoard.numGhostRows=numGhostRows;
	_retBoard.board = newJaggedArrayColor(_w,_h);
	_retBoard.pieceStatus = newJaggedArrayChar(_w,_h);
	_retBoard.pieceStatusTime = newJaggedArrayu64(_w,_h);
	_retBoard.popCheckHelp = newJaggedArrayChar(_w,_h);
	_retBoard.numActiveSets=0;
	_retBoard.activeSets=NULL;
	_retBoard.status=STATUS_NORMAL;
	_retBoard.score=0;
	_retBoard.curChain=0;
	resetBoard(&_retBoard);
	_retBoard.numNextPieces=3;
	_retBoard.nextPieces = malloc(sizeof(struct pieceSet)*_retBoard.numNextPieces);
	int i;
	for (i=0;i<_retBoard.numNextPieces;++i){
		_retBoard.nextPieces[i]=getRandomPieceSet();
	}
	_retBoard.usingSkin=&currentSkin;
	return _retBoard;
}
char canTile(struct puyoBoard* _passedBoard, int _searchColor, int _x, int _y){
	return (getBoard(_passedBoard,_x,_y)==_searchColor && (_passedBoard->pieceStatus[_x][_y]==0 || _passedBoard->pieceStatus[_x][_y]==PIECESTATUS_POSTSQUISH)); // Unless _searchColor is the out of bounds color, it's impossible for the first check to pass with out of bounds coordinates. Therefor no range checks needed for second check.
}
unsigned char getTilingMask(struct puyoBoard* _passedBoard, int _x, int _y){
	unsigned char _ret=0;
	_ret|=(AUTOTILEDOWN*canTile(_passedBoard,_passedBoard->board[_x][_y],_x,_y+1));
	if (_y!=_passedBoard->numGhostRows){
		_ret|=(AUTOTILEUP*canTile(_passedBoard,_passedBoard->board[_x][_y],_x,_y-1));
	}
	_ret|=(AUTOTILERIGHT*canTile(_passedBoard,_passedBoard->board[_x][_y],_x+1,_y));
	_ret|=(AUTOTILELEFT*canTile(_passedBoard,_passedBoard->board[_x][_y],_x-1,_y));
	return _ret;
}
void drawBoard(struct puyoBoard* _drawThis, int _startX, int _startY, char _isPlayerBoard, u64 _sTime){
	drawRectangle(_startX,_startY,tileh*_drawThis->w,(_drawThis->h-_drawThis->numGhostRows)*tileh,150,0,0,255);
	int i;
	if (_isPlayerBoard && _drawThis->status==STATUS_NORMAL){
		clearBoardPopCheck(_drawThis);
		// Also sets up the pop check array to mark puyos that will get popped if things continue as they are
		drawNextGhostColumn(-1,_startX,_startY,_drawThis,_drawThis->activeSets->data,_drawThis->usingSkin);
	}
	// Draw next window, animate if needed
	if (_drawThis->status!=STATUS_NEXTWINDOW){
		for (i=0;i<_drawThis->numNextPieces-1;++i){
			drawPiecesetRelative(_startX+_drawThis->w*tilew+(i&1)*tilew,_startY+i*tileh*2+tileh,0,tileh,&(_drawThis->nextPieces[i]),_drawThis->usingSkin);
		}
	}else{
		// latest piece moves up
		drawPiecesetRelative(_startX+_drawThis->w*tilew,_startY+tileh-partMoveFills(_sTime, _drawThis->nextWindowTime, NEXTWINDOWTIME, tileh*2),0,tileh,&(_drawThis->nextPieces[0]),_drawThis->usingSkin);
		// all others move diagonal`
		for (i=0;i<_drawThis->numNextPieces;++i){
			int _xChange;
			if (i!=0){
				_xChange=partMoveFills(_sTime, _drawThis->nextWindowTime, NEXTWINDOWTIME, tileh)*((i&1) ? -1 : 1);
			}else{
				_xChange=0;
			}
			drawPiecesetRelative(_startX+_drawThis->w*tilew+(i&1)*tilew+_xChange,_startY+i*tileh*2+tileh-partMoveFills(_sTime, _drawThis->nextWindowTime, NEXTWINDOWTIME, tileh*2),0,tileh,&(_drawThis->nextPieces[i]),_drawThis->usingSkin);
		}
	}
	for (i=0;i<_drawThis->w;++i){
		double _squishyY;
		char _isSquishyColumn=0;
		int j;
		for (j=_drawThis->h-1;j>=_drawThis->numGhostRows;--j){
			if (_drawThis->board[i][j]!=0){
				if (_isSquishyColumn){
					_squishyY-=drawSquishingPuyo(_drawThis->board[i][j],tileh*i+_startX,_squishyY,USUALSQUISHTIME,_drawThis->pieceStatusTime[i][j],_drawThis->usingSkin,_sTime);
				}else{
					switch (_drawThis->pieceStatus[i][j]){
						case PIECESTATUS_POPPING:
							drawPoppingPuyo(_drawThis->board[i][j],tileh*i+_startX,tileh*(j-_drawThis->numGhostRows)+_startY,_drawThis->popFinishTime,_drawThis->usingSkin,_sTime);
							break;
						case PIECESTATUS_SQUSHING:
							_isSquishyColumn=1;
							_squishyY=tileh*(j-_drawThis->numGhostRows)+_startY;
							++j; // Redo this iteration where we'll draw as squishy column
							break;
						default:
							if (_isPlayerBoard && _drawThis->popCheckHelp[i][j]==POSSIBLEPOPBYTE){
								drawPotentialPopPuyo(_drawThis->board[i][j],tileh*i+_startX,tileh*(j-_drawThis->numGhostRows)+_startY,getTilingMask(_drawThis,i,j),_drawThis->usingSkin,tileh);
							}else{
								drawNormPuyo(_drawThis->board[i][j],tileh*i+_startX,tileh*(j-_drawThis->numGhostRows)+_startY,getTilingMask(_drawThis,i,j),_drawThis->usingSkin,tileh);
							}
							break;
					}
				}
			}
		}
	}
	
	// draw falling pieces, active pieces, etc
	ITERATENLIST(_drawThis->activeSets,{
			drawPiecesetOffset(_startX,_startY+(_drawThis->numGhostRows*tileh*-1),_curnList->data,_drawThis->usingSkin);
		});
	// draw border. right now, it just covers ghost rows
	drawRectangle(_startX,_startY-_drawThis->numGhostRows*tileh,screenWidth,_drawThis->numGhostRows*tileh,0,0,0,255);
	// chain
	if (_drawThis->status==STATUS_POPPING){
		char* _drawString = easySprintf("%d-TETRIS",_drawThis->curChain);
		int _cachedWidth = textWidth(regularFont,_drawString);
		gbDrawText(regularFont,_startX+cap(_drawThis->chainNotifyX-_cachedWidth/2,0,_drawThis->w*tilew-_cachedWidth),_startY+_drawThis->chainNotifyY+textHeight(regularFont)/2,_drawString,95,255,83);
		free(_drawString);
	}
	// draw score
	char* _drawString = easySprintf("%08d",_drawThis->score);
	gbDrawText(regularFont, _startX+easyCenter(textWidth(regularFont,_drawString),_drawThis->w*tilew), _startY+(_drawThis->h-_drawThis->numGhostRows)*tileh, _drawString, 255, 255, 255);
	free(_drawString);
}
void removePuyoPartialTimes(struct movingPiece* _passedPiece){
	if (!(_passedPiece->movingFlag & FLAG_ANY_HMOVE)){
		_passedPiece->completeHMoveTime=0;
	}
	if (!(_passedPiece->movingFlag & (FLAG_MOVEDOWN | FLAG_DEATHROW))){
		_passedPiece->completeFallTime=0;
	}
}
void removeBoardPartialTimes(struct puyoBoard* _passedBoard){
	ITERATENLIST(_passedBoard->activeSets,{
			struct pieceSet* _curSet = _curnList->data;
			int j;
			for (j=0;j<_curSet->count;++j){
				removePuyoPartialTimes(&(_curSet->pieces[j]));
			}
		});
}
int getPopNum(struct puyoBoard* _passedBoard, int _x, int _y, char _helpChar, puyoColor _shapeColor){
	if (_y>=_passedBoard->numGhostRows && getBoard(_passedBoard,_x,_y)==_shapeColor){
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
void setPopStatus(struct puyoBoard* _passedBoard, char _setStatusToThis, int _x, int _y, puyoColor _shapeColor, int* _retAvgX, int* _retAvgY){
	if (_y>=_passedBoard->numGhostRows && getBoard(_passedBoard,_x,_y)==_shapeColor){
		if (_passedBoard->popCheckHelp[_x][_y]!=2){
			_passedBoard->popCheckHelp[_x][_y]=2;
			_passedBoard->pieceStatus[_x][_y]=_setStatusToThis;
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
	for (i=0;i<_passedBoard->w;++i){
		// Find the first empty spot in this column with a puyo above it
		int _nextFallY=-20; // Y position of the destination of the next puyo to fall.
		int j;
		for (j=_passedBoard->h-1;j>0;--j){ // > 0 because it's useless if top column is empty
			if (fastGetBoard(_passedBoard,i,j-1)!=COLOR_NONE && fastGetBoard(_passedBoard,i,j)==COLOR_NONE){
				int k;
				_ret=1;
				if (_nextFallY==-20){ // init this variable if needed
					for (k=_passedBoard->h-1;k>=0;--k){
						if (fastGetBoard(_passedBoard,i,k)==COLOR_NONE){
							_nextFallY=k;
							break;
						}
					}
				}
				--j; // Start at the piece we found
				// get number of pieces for this piece set
				int _numPieces=0;
				k=j;
				do{
					++_numPieces;
					--k;
				}while(k>=0 && fastGetBoard(_passedBoard,i,k)!=COLOR_NONE);
				struct pieceSet _newSet;
				_newSet.pieces=malloc(sizeof(struct movingPiece)*_numPieces);
				_newSet.count=_numPieces;
				_newSet.isSquare=0;
				_newSet.quickLock=1;
				//_newSet.singleTileVSpeed=FALLTIME; // shouldn't be needed
				for (k=0;k<_numPieces;++k){
					struct movingPiece _newPiece;
					memset(&_newPiece,0,sizeof(struct movingPiece));
					_newPiece.tileX=i;
					_newPiece.tileY=j-k; // set this is required for _lowStartPuyoFall
					_newPiece.color=_passedBoard->board[i][j-k];
					_passedBoard->board[i][j-k]=COLOR_NONE;
					snapPuyoDisplayPossible(&_newPiece);
					_lowStartPuyoFall(&_newPiece,_nextFallY--,FALLTIME/7,_sTime);
					_newSet.pieces[k] = _newPiece;
				}
				addSetToBoard(_passedBoard,&_newSet);
				j-=(_numPieces-1); // This means that if there was just one piece added, don't do anything because we already subtracted one.
			}
		}
	}
	if (_ret){
		_passedBoard->status=STATUS_DROPPING;
	}
	return _ret;
}
void transitionBoradNextWindow(struct puyoBoard* _passedBoard, u64 _sTime){
	_passedBoard->status=STATUS_NEXTWINDOW;
	_passedBoard->nextWindowTime=_sTime+NEXTWINDOWTIME;
}
// _returnForIndex will tell it which set to return the value of
// pass -1 to get return values from all
// TODO - The _returnForIndex is a bit useless right now because the same index can refer to two piece sets. Like if you want to return for index 0, and index 0 is a removed piece set. Then index 1 will also become index 0.
signed char updateBoard(struct puyoBoard* _passedBoard, signed char _returnForIndex, u64 _sTime){
	// If we're done dropping, try popping
	if (_passedBoard->status==STATUS_DROPPING && _passedBoard->numActiveSets==0){
		_passedBoard->status=STATUS_SETTLESQUISH;
	}else if (_passedBoard->status==STATUS_SETTLESQUISH){ // When we're done squishing, try popping
		int _x, _y;
		char _doneSquishing=1;
		for (_x=0;_x<_passedBoard->w;++_x){
			for (_y=_passedBoard->h-1;_y>=0;--_y){
				if (_passedBoard->pieceStatus[_x][_y] & (PIECESTATUS_SQUSHING | PIECESTATUS_POSTSQUISH)){
					_doneSquishing=0;
					_x=_passedBoard->w;
					break;
				}
			}
		}
		if (_doneSquishing){
			clearBoardPieceStatus(_passedBoard);
			clearBoardPopCheck(_passedBoard);
			int _numGroups=0; // Just number of unique groups that we're popping. So it's 1 or 2.
			int _numUniqueColors=0; // Optional variable. Can calculate using _whichColorsFlags
			int _totalGroupBonus=0; // Actual group bonus for score
			long _whichColorsFlags=0;
			int _x, _y, _numPopped=0;
			// Average tile position of the popped puyos. Right now, it takes all new pops into account.
			double _avgX=0;
			double _avgY=0;
			for (_x=0;_x<_passedBoard->w;++_x){
				for (_y=_passedBoard->numGhostRows;_y<_passedBoard->h;++_y){
					if (fastGetBoard(_passedBoard,_x,_y)!=COLOR_NONE){
						if (_passedBoard->popCheckHelp[_x][_y]==0){
							int _possiblePop;
							if ((_possiblePop=getPopNum(_passedBoard,_x,_y,1,_passedBoard->board[_x][_y]))>=minPopNum){
								long _flagIndex = 1L<<(_passedBoard->board[_x][_y]-COLOR_REALSTART);
								if (!(_whichColorsFlags & _flagIndex)){ // If this color index isn't in our flag yet, up the unique colors count
									++_numUniqueColors;
									_whichColorsFlags|=_flagIndex;
								}
								_totalGroupBonus+=groupBonus[cap(_possiblePop-(minPopNum>=STANDARDMINPOP ? STANDARDMINPOP : minPopNum),0,7)]; // bonus for number of puyo in the group. 
								++_numGroups;
								_numPopped+=_possiblePop;
								int _totalX=0;
								int _totalY=0;
								setPopStatus(_passedBoard,PIECESTATUS_POPPING,_x,_y,_passedBoard->board[_x][_y],&_totalX,&_totalY);
								_avgX=((_totalX/(double)_possiblePop)+_avgX)/_numGroups;
								_avgY=((_totalY/(double)_possiblePop)+_avgY)/_numGroups;
							}
						}
					}
				}
			}
			if (_numGroups!=0){
				_passedBoard->curChain++;
				_passedBoard->chainNotifyX=_avgX*tilew;
				_passedBoard->chainNotifyY=(_avgY-_passedBoard->numGhostRows)*tileh;
				_passedBoard->nextScoreAdd=(10*_numPopped)*cap(chainPowers[cap(_passedBoard->curChain-1,0,23)]+colorCountBouns[cap(_numUniqueColors-1,0,5)]+_totalGroupBonus,1,999);
				_passedBoard->status=STATUS_POPPING;
				_passedBoard->popFinishTime=_sTime+popTime;
			}else{
				transitionBoradNextWindow(_passedBoard,_sTime);
			}
		}
	}else if (_passedBoard->status==STATUS_NEXTWINDOW){
		_passedBoard->curChain=0;
		if (_sTime>=_passedBoard->nextWindowTime){
			lazyUpdateSetDisplay(&(_passedBoard->nextPieces[0]),_sTime);
			addSetToBoard(_passedBoard,&(_passedBoard->nextPieces[0]));
			memmove(&(_passedBoard->nextPieces[0]),&(_passedBoard->nextPieces[1]),sizeof(struct pieceSet)*(_passedBoard->numNextPieces-1));
			_passedBoard->nextPieces[_passedBoard->numNextPieces-1] = getRandomPieceSet();
			_passedBoard->status=STATUS_NORMAL;
		}
	}else if (_passedBoard->status==STATUS_POPPING){
		if (_sTime>=_passedBoard->popFinishTime){
			int _x, _y;
			for (_x=0;_x<_passedBoard->w;++_x){
 				for (_y=0;_y<_passedBoard->h;++_y){
					if (_passedBoard->pieceStatus[_x][_y]==PIECESTATUS_POPPING){
						_passedBoard->board[_x][_y]=0;
					}
				}
			}
			// add the points from the last pop
			_passedBoard->score+=_passedBoard->nextScoreAdd;
			_passedBoard->nextScoreAdd=0;
			// Assume that we did kill at least one puyo because we wouldn't be in this situation if there weren't any to kill.
			// Assume that we popped and therefor need to drop, I mean.
			transitionBoardFallMode(_passedBoard,_sTime);
			_passedBoard->status=STATUS_DROPPING;
		}
	}
	// Process piece statuses.
	// This would be more efficient to just add to the draw code, but it would add processing to draw loop.
	int _x, _y;
	for (_x=0;_x<_passedBoard->w;++_x){
		for (_y=0;_y<_passedBoard->h;++_y){
			switch (_passedBoard->pieceStatus[_x][_y]){
				case PIECESTATUS_SQUSHING:
					if (_passedBoard->pieceStatusTime[_x][_y]<=_sTime){
						_passedBoard->pieceStatus[_x][_y]=PIECESTATUS_POSTSQUISH;
					}
					break;
				case PIECESTATUS_POSTSQUISH:
					if (_passedBoard->pieceStatusTime[_x][_y]+POSTSQUISHDELAY<=_sTime){ // Reuses time from PIECESTATUS_SQUSHING
						_passedBoard->pieceStatus[_x][_y]=0;
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
				if (i==0 && _passedBoard->status==STATUS_NORMAL){
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
		_passedBoard->status=STATUS_DROPPING; // even if nothing is going to fall, go to fall mode because that will check for pops and then go to next window mode anyway.
	}
	return _ret;
}
//////////////////////////////////////////////////
void updateControlSet(struct controlSet* _passedControls, u64 _sTime){
	struct puyoBoard* _passedBoard = _passedControls->target;
	if (wasJustReleased(BUTTON_LEFT) || wasJustReleased(BUTTON_RIGHT)){
		_passedControls->dasDirection=0; // Reset DAS
	}
	if (wasJustPressed(BUTTON_LEFT) || wasJustPressed(BUTTON_RIGHT)){
		_passedControls->dasDirection=0; // Reset DAS
		_passedControls->dasDirection = wasJustPressed(BUTTON_RIGHT) ? 1 : -1;
		_passedControls->dasChargeEnd = _sTime+DASTIME;
	}
	if (isDown(BUTTON_LEFT) || isDown(BUTTON_RIGHT)){
		if (_passedControls->dasDirection==0){
			_passedControls->dasDirection = isDown(BUTTON_RIGHT) ? 1 : -1;
			_passedControls->dasChargeEnd = _sTime+DASTIME;
		}
	}
	if (_passedBoard->status==STATUS_NORMAL){
		struct pieceSet* _targetSet = _passedBoard->activeSets->data;
		if (wasJustPressed(BUTTON_DOWN)){
			_passedControls->startHoldTime=_sTime;
		}else if (isDown(BUTTON_DOWN)){
			if (_targetSet->pieces[0].movingFlag & FLAG_MOVEDOWN){ // Normal push down
				int j;
				for (j=0;j<_targetSet->count;++j){
					int _offsetAmount = (_sTime-_passedControls->startHoldTime)*13;
					if (_offsetAmount>_targetSet->pieces[j].referenceFallTime){ // Keep unisnged value from going negative
						_targetSet->pieces[j].completeFallTime=0;
					}else{
						_targetSet->pieces[j].completeFallTime=_targetSet->pieces[j].referenceFallTime-_offsetAmount;
					}
				}
			}else if (_targetSet->pieces[0].movingFlag & FLAG_DEATHROW){ // lock
				int j;
				for (j=0;j<_targetSet->count;++j){
					_targetSet->pieces[j].completeFallTime = 0;
				}
			}
		}else if (wasJustReleased(BUTTON_DOWN)){
			// Allows us to push down, wait, and push down again in one tile. Not like that's possible with how fast it goes though
			int j;
			for (j=0;j<_targetSet->count;++j){
				_targetSet->pieces[j].referenceFallTime = _targetSet->pieces[j].completeFallTime;
			}
		}
		pieceSetControls(_passedBoard,_targetSet,_passedControls,_sTime,_sTime>=_passedControls->dasChargeEnd ? _passedControls->dasDirection : 0);
	}
	_passedControls->lastFrameTime=_sTime;
}
void rebuildBoardDisplay(struct puyoBoard* _passedBoard, u64 _sTime){
	ITERATENLIST(_passedBoard->activeSets,{
			lazyUpdateSetDisplay(_curnList->data,_sTime);
		});
}
void rebuildSizes(int _w, int _h,double _tileRatioPad){
	screenWidth = getScreenWidth();
	screenHeight = getScreenHeight();

	int _fitWidthSize = screenWidth/(double)(_w+_tileRatioPad*2);
	int _fitHeightSize = screenHeight/(double)(_h+_tileRatioPad*2);
	tileh = _fitWidthSize<_fitHeightSize ? _fitWidthSize : _fitHeightSize;
}
void init(){
	srand(time(NULL));
	generalGoodInit();
	_globalReferenceMilli = goodGetMilli();
	initGraphics(tilew*9,649,WINDOWFLAG_RESIZABLE);
	initImages();
	setWindowTitle("Test happy");
	setClearColor(0,0,0);

	regularFont = loadFont("./liberation-sans-bitmap.sfl",-1);

	currentSkin = loadChampionsSkinFile("./aqua.png");
}
int main(int argc, char const** argv){
	init();
	struct puyoBoard _testBoard = newBoard(6,14,2); // 6,12
	rebuildSizes(_testBoard.w,_testBoard.h,1);
	//struct puyoBoard _enemyBoard = newBoard(6,14,2);

	struct controlSet playerControls = newControlSet(&_testBoard,goodGetMilli());

	transitionBoradNextWindow(&_testBoard,goodGetMilli());
	//transitionBoradNextWindow(&_enemyBoard,goodGetMilli());
	#if FPSCOUNT == 1
	u64 _frameCountTime = goodGetMilli();
	int _frames=0;
	#endif


	/*
	struct squishyBois testStack;
	testStack.numPuyos=8;
	testStack.yPos = malloc(sizeof(int)*testStack.numPuyos);
	int k;
	for (k=0;k<testStack.numPuyos;++k){
		testStack.yPos[k]=(testStack.numPuyos-k)*tileh;
	}
	testStack.startTime=goodGetMilli();
	testStack.destY=screenHeight-tileh;
	testStack.startSquishTime = testStack.startTime+(((testStack.destY)-testStack.yPos[0])/SQUISHDELTAY)*SQUISHNEXTFALLTIME;


	#define HALFSQUISHTIME 200
	
	#define SQUISHFLOATMAX 255
	//#define USUALSQUISHTIME 300
	#define UNSQUISHTIME 8
	#define UNSQUISHAMOUNT 20
	#define SQUISHONEWEIGHT 100
	
	while(1){
		controlsStart();
		controlsEnd();
		startDrawing();
		u64 _sTime = goodGetMilli();
		int i=0;
		if (_sTime>testStack.startSquishTime){
			int _totalUpSquish = ((_sTime-testStack.startSquishTime)/(double)HALFSQUISHTIME)*SQUISHFLOATMAX;
			// SQUISHDOWNLIMIT
			// On 0 - 255 scale, with 0 being SQUISHDOWNLIMIT
			//SQUISHDOWNLIMIT + (1-SQUSIHDOWNLIMIT)*(_currentSquishLevel/255);
			//int _currentSquish=((_sTime-testStack.startSquishTime)/(double)UNSQUISHTIME)*UNSQUISHAMOUNT*-1+SQUISHONEWEIGHT;
			//int _currentSquish;
			double _currentSquishRatio;			
			for (i=1;i<testStack.numPuyos;++i){
				// Amount the stack has been pushed down so far by the weight of puyos hitting it
				int _totalDownSquish = i*(SQUISHFLOATMAX/3);
				// Current total
				int _totalSquish = intCap(SQUISHFLOATMAX-(_totalDownSquish-_totalUpSquish),0,SQUISHFLOATMAX);
				// ratio of image height
				_currentSquishRatio = SQUISHDOWNLIMIT+(_totalSquish/(double)SQUISHFLOATMAX)*(1-SQUISHDOWNLIMIT);
				printf("Down: %d; Up: %d; total; %d; ratio %f\n",_totalDownSquish,_totalUpSquish,_totalSquish,_currentSquishRatio);
				// Our height is the number of things in the stack at the current squish ratio
				int _stackHeight = i*tileh*_currentSquishRatio;
				// Get the position of our puyo that's falling right now as if it's not on the stack
				int _curY = testStack.yPos[i];
				if (_sTime-testStack.startTime>=i*SQUISHNEXTFALLTIME){
					_curY+=partMoveFills((s64)_sTime-i*SQUISHNEXTFALLTIME,testStack.startTime+SQUISHNEXTFALLTIME,SQUISHNEXTFALLTIME,SQUISHDELTAY);
				}
				// If it is on the stack, keep going and find all the ones that are on it
				if (_curY>testStack.destY-_stackHeight){
					continue;
				}else{
					break;
				}
			}
			int _numBeingSquished=i;
			int _curY=testStack.destY;
			for (i=0;i<_numBeingSquished;++i){
				_curY-=drawSquishRatioPuyo(COLOR_REALSTART+i%5,screenWidth/2-tilew/2,_curY,_currentSquishRatio,&currentSkin);
			}
			// Draw the rest normally below starting here
			i=_numBeingSquished;
		}
		for (;i<testStack.numPuyos;++i){
			int yPos = testStack.yPos[i];
			if (_sTime-testStack.startTime>=i*SQUISHNEXTFALLTIME){
				yPos+=partMoveFills((s64)_sTime-i*SQUISHNEXTFALLTIME,testStack.startTime+SQUISHNEXTFALLTIME,SQUISHNEXTFALLTIME,SQUISHDELTAY);
			}

			drawNormPuyo(COLOR_REALSTART+i%5,screenWidth/2-tilew/2,yPos,0,&currentSkin,tilew);
		}
		endDrawing();
	}
	*/

	while(1){
		u64 _sTime = goodGetMilli();
		controlsStart();
		if (isDown(BUTTON_RESIZE)){ // Impossible for BUTTON_RESIZE for two frames, so just use isDown
			rebuildSizes(_testBoard.w,_testBoard.h,1);
			rebuildBoardDisplay(&_testBoard,_sTime);
		}
		//updateBoard(&_enemyBoard,-2,_sTime);
		char _updateRet = updateBoard(&_testBoard,_testBoard.status==STATUS_NORMAL ? 0 : -1,_sTime);
		if (_updateRet!=0){
			if (_testBoard.status==STATUS_NORMAL){
				if (_updateRet&2){
					// Here's the scenario:
					// Holding down. The next millisecond, the puyo will go to the next tile.
					// It's the next frame. The puyo goes down to the next tile. 16 milliseconds have passed bwteeen the last frame and now.
					// We were holding down for those 16 frames in between, so they need to be accounted for in the push down time for the next tile.
					// Note that if they weren't holding down between the frames, it won't matter because this startHoldTime variable isn't looked at in that case.
					playerControls.startHoldTime=_sTime-(_sTime-playerControls.lastFrameTime);
				}
				/*
				switch(_updateRet){
					case 1:
						if (!transitionBoardFallMode(&_testBoard,_sTime)){
							free(_testBoard.activeSets[0].pieces);
							_testBoard.activeSets[0] = getPieceSet();
						}
					case 2:
						_startHoldTime=_sTime;
						break;
					default:
						printf("Unknown _updateRet %d\n",_updateRet);
						break;
				}
				*/
			}
		}
		updateControlSet(&playerControls,_sTime);
		if (wasJustPressed(BUTTON_L)){
			printf("Input in <>;<> format starting at %d:\n",COLOR_REALSTART);
			struct pieceSet* _firstSet = _testBoard.activeSets->data;
			scanf("%d;%d", &(_firstSet->pieces[1].color),&(_firstSet->pieces[0].color));
		}
		if (_updateRet&4){ // If the partial times were set this frame, remove the ones that weren't used because the frame is over.
			removeBoardPartialTimes(&_testBoard);
		}
		controlsEnd();
		startDrawing();
		drawBoard(&_testBoard,easyCenter((_testBoard.w+2)*tilew,screenWidth),easyCenter((_testBoard.h-_testBoard.numGhostRows)*tileh,screenHeight),1,_sTime);
		//drawBoard(&_enemyBoard,_testBoard.w*tilew+tilew*3,easyCenter((_testBoard.h-_testBoard.numGhostRows)*tileh,screenHeight),_sTime);
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
