/*
If it takes 16 milliseconds for a frame to pass and we only needed 1 millisecond to progress to the next tile, we can't just say the puyo didn't move for those other 15 milliseconds. We need to account for those 15 milliseconds for the puyo's next falling down action. The difference between the actual finish time and the expected finish time is stored back in the complete dest time variable. In this case, 15 would be stored. We'd take that 15 and account for it when setting any more down movement time values for this frame. But only if we're going to do more down moving this frame. Otherwise we throw that 15 value away at the end of the frame. Anyway, 4 is the bit that indicates that these values were set.
*/
// TODO - Ai sometimes crashes when high up. See //AIDEBUG
// TODO - Hitting a stack that's already squishing wrong behavior
// TODO - Maybe a board can have a pointer to a function to get the next piece. I can map it to either network or random generator
// TODO - Draw board better. Have like a wrapper struct drawableBoard where elements can be repositioned or remove.
// TODO - Only check for potential pops on piece move?
// TODO - Why is the set single tile fall time stored in the pieceSet instead of the board? Is it used anywhere?

#define TESTFEVERPIECE 0

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

#define AISOFTDROPS 1

#define MAXGARBAGEROWS 5

// Time after the squish animation before next pop check
#define POSTSQUISHDELAY 100

// When checking for potential pops if the current piece set moves as is, this is the byte we use in popCheckHelp to identify this puyo as a verified potential pop
#define POSSIBLEPOPBYTE 5

#define PUSHDOWNTIMEMULTIPLIER 13
#define PUSHDOWNTIMEMULTIPLIERCPU 13

int tileh = 45;
#define tilew tileh
#define HALFTILE (tilew/2)
#define POTENTIALPOPALPHA 200

// Width of next window in tiles
#define NEXTWINDOWTILEW 2

#define DASTIME 150
#define DOUBLEROTATETAPTIME 350
#define USUALSQUISHTIME 300
#define POPANIMRELEASERATIO .50 // The amount of the total squish time is used to come back up
#define SQUISHDOWNLIMIT .30 // The smallest of a puyo's original size it will get when squishing. Given as a decimal percent. .30 would mean puyo doesn't get less than 30% of its size
#define SPLITFALLTIME (FALLTIME/7)
#define GARBAGEFALLTIME (FALLTIME/10)

#define STANDARDMINPOP 4 // used when calculating group bonus.

#define GHOSTPIECERATIO .80

#define DEATHANIMTIME 1000

crossFont regularFont;

// How long it takes a puyo to fall half of one tile on the y axis
int FALLTIME = 500;
int HMOVETIME = 30;
int ROTATETIME = 50;
int NEXTWINDOWTIME = 200;
int popTime = 500;
int minPopNum = 4;
int numColors = 4;

typedef int puyoColor;

#define COLOR_NONE 0
#define COLOR_IMPOSSIBLE 1
#define COLOR_GARBAGE (COLOR_REALSTART-1) // I can't spell nuisance
#define COLOR_REALSTART 3

// bitmap
// 0 is default?
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

// main.h
typedef enum{
	STATUS_UNDEFINED,
	STATUS_NORMAL, // We're moving the puyo around
	STATUS_POPPING, // We're waiting for puyo to pop
	STATUS_DROPPING, // puyo are falling into place. This is the status after you place a piece and after STATUS_POPPING
	STATUS_SETTLESQUISH, // A status after STATUS_DROPPING to wait for all puyos to finish their squish animation. Needed because some puyos start squish before others. When done, checks for pops and goes to STATUS_NEXTWINDOW or STATUS_POPPING
	STATUS_NEXTWINDOW, // Waiting for next window. This is the status after STATUS_DROPPING if no puyo connect
	STATUS_DEAD,
	STATUS_DROPGARBAGE, // Can combine with STATUS_DROPPING if it ends up needed special code for the updateBoard
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
	struct puyoSkin* usingSkin;
	u64 statusTimeEnd; // Only set for some statuses
	int numGhostRows;
	u64 score;
	u64 curChainScore; // The score we're going to add after the puyo finish popping
	int curChain;
	int chainNotifyX;
	int chainNotifyY;
	double leftoverGarbage;
	int readyGarbage;
	int incomingLength;
	int* incomingGarbage;
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
	u64 lastFrameTime;
};
struct boardController;
struct gameState;
typedef void(*boardControlFunc)(void*,struct gameState*,struct puyoBoard*,signed char,u64);
struct boardController{
	boardControlFunc func;
	void* data;
};
struct gameSettings{
	int pointsPerGar;
};
struct gameState{
	int numBoards;
	struct puyoBoard* boards;
	struct boardController* controllers;
	struct gameSettings settings;
};
struct aiInstruction{
	int anchorDestX;
	int anchorDestY;
	int secondaryDestX;
	int secondaryDestY;
};
struct aiState;
typedef void(*aiFunction)(struct aiState*,struct pieceSet*,struct puyoBoard*,int,struct puyoBoard*);
struct aiState{
	struct aiInstruction nextAction; // Do not manually set from ai function
	aiFunction updateFunction;
	char softdropped;
};
void drawSingleGhostColumn(int _offX, int _offY, int _tileX, struct puyoBoard* _passedBoard, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin);
int getPopNum(struct puyoBoard* _passedBoard, int _x, int _y, char _helpChar, puyoColor _shapeColor);
int getFreeColumnYPos(struct puyoBoard* _passedBoard, int _columnIndex, int _minY);
void updateControlSet(void* _controlData, struct gameState* _passedState, struct puyoBoard* _passedBoard, signed char _updateRet, u64 _sTime);
double scoreToGarbage(struct gameSettings* _passedSettings, long _passedPoints);
void sendGarbage(struct gameState* _passedState, struct puyoBoard* _source, int _newGarbageSent);
void applyGarbage(struct gameState* _passedState, struct puyoBoard* _source);
char forceFallStatics(struct puyoBoard* _passedBoard);
char boardHasConnections(struct puyoBoard* _passedBoard);
char needApplyGarbage(struct gameState* _passedState, struct puyoBoard* _source);

int screenWidth;
int screenHeight;
char* vitaAppId="FREEPUYOV";

u64 _globalReferenceMilli;

struct puyoSkin currentSkin;
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
int getSpawnCol(int _w){
	if (_w & 1){
		return _w/2; // 7 -> 3
	}else{
		return _w/2-1; //6 -> 2
	}
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
struct controlSet newControlSet(u64 _sTime){
	struct controlSet _ret;
	_ret.dasDirection=0;
	_ret.startHoldTime=0;
	_ret.lastFailedRotateTime=0;
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
void placePuyo(struct puyoBoard* _passedBoard, int _x, int _y, puyoColor _passedColor, int _squishTime, u64 _sTime){
	_passedBoard->board[_x][_y]=_passedColor;
	if (_passedColor!=COLOR_GARBAGE){
		_passedBoard->pieceStatus[_x][_y]=PIECESTATUS_SQUSHING;
		_passedBoard->pieceStatusTime[_x][_y]=_squishTime+_sTime;
	}else{
		_passedBoard->pieceStatus[_x][_y]=0;
	}
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
void lowDrawNormPuyo(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int _size){
	drawTexturePartSized(_passedSkin->img,_drawX,_drawY,_size,_size,_passedSkin->colorX[_color-COLOR_REALSTART][_tileMask],_passedSkin->colorY[_color-COLOR_REALSTART][_tileMask],_passedSkin->puyoW,_passedSkin->puyoH);
}
void lowDrawPotentialPopPuyo(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int _size){
	drawTexturePartSizedTintAlpha(_passedSkin->img,_drawX,_drawY,_size,_size,_passedSkin->colorX[_color-COLOR_REALSTART][_tileMask],_passedSkin->colorY[_color-COLOR_REALSTART][_tileMask],_passedSkin->puyoW,_passedSkin->puyoH,255,255,255,POTENTIALPOPALPHA);
}
void lowDrawPoppingPuyo(int _color, int _drawX, int _drawY, u64 _destPopTime, struct puyoSkin* _passedSkin, int _size, u64 _sTime){
	int _destSize=_size*(_destPopTime-_sTime)/(double)popTime;
	drawTexturePartSized(_passedSkin->img,_drawX+(_size-_destSize)/2,_drawY+(_size-_destSize)/2,_destSize,_destSize,_passedSkin->colorX[_color-COLOR_REALSTART][0],_passedSkin->colorY[_color-COLOR_REALSTART][0],_passedSkin->puyoW,_passedSkin->puyoH);
}
void lowDrawGarbage(int _drawX, int _drawY, struct puyoSkin* _passedSkin, int _size, double _alpha){
	drawTexturePartSizedAlpha(_passedSkin->img,_drawX,_drawY,_size,_size,_passedSkin->garbageX,_passedSkin->garbageY,_passedSkin->puyoW,_passedSkin->puyoH,_alpha);
}
void lowDrawPoppingGarbage(int _drawX, int _drawY, u64 _destPopTime, struct puyoSkin* _passedSkin, int _size, u64 _sTime){
	lowDrawGarbage(_drawX,_drawY,_passedSkin,_size,partMoveEmptysCapped(_sTime,_destPopTime,popTime,255));
}
void drawPoppingPiece(int _color, int _drawX, int _drawY, u64 _destPopTime, struct puyoSkin* _passedSkin, u64 _sTime){
	if (_color==COLOR_GARBAGE){
		lowDrawPoppingGarbage(_drawX,_drawY,_destPopTime,_passedSkin,tilew,_sTime);
	}else{
		lowDrawPoppingPuyo(_color,_drawX,_drawY,_destPopTime,_passedSkin,tilew,_sTime);
	}
}
void drawNormPiece(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int _size){
	if (_color==COLOR_GARBAGE){
		lowDrawGarbage(_drawX,_drawY,_passedSkin,_size,255);
	}else{
		lowDrawNormPuyo(_color,_drawX,_drawY,_tileMask,_passedSkin,_size);
	}
}
void drawPotentialPopPiece(int _color, int _drawX, int _drawY, unsigned char _tileMask, struct puyoSkin* _passedSkin, int _size){
	if (_color==COLOR_GARBAGE){
		lowDrawGarbage(_drawX,_drawY,_passedSkin,_size,POTENTIALPOPALPHA);
	}else{
		lowDrawPotentialPopPuyo(_color,_drawX,_drawY,_tileMask,_passedSkin,_size);
	}
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
// These two functions for temp pieces *can* be used, but don't have to be. Meaning don't add anything special here because it can't be assumed these are used.
void removeTempPieces(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard, int* _yDests){
	int i;
	for (i=0;i<_passedSet->count;++i){
		_passedBoard->board[_passedSet->pieces[i].tileX][_yDests[i]]=0;
	}
}
void setTempPieces(struct pieceSet* _passedSet, struct puyoBoard* _passedBoard, int* _yDests, char _canGhostRows){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if (_canGhostRows || _yDests[i]>=_passedBoard->numGhostRows){
			_passedBoard->board[_passedSet->pieces[i].tileX][_yDests[i]]=_passedSet->pieces[i].color;
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
		int _newLowColumn=_passedBoard->w;
		int i;
		for (i=0;i<_passedSet->count;++i){
			if (_passedSet->pieces[i].tileX>_prevColumn && _passedSet->pieces[i].tileX<_newLowColumn){
				_newLowColumn = _passedSet->pieces[i].tileX;
			}
		}
		if (_newLowColumn==_passedBoard->w){
			return _retArray;
		}

		int _firstYDest = getFreeColumnYPos(_passedBoard,_newLowColumn,0);
		// Draw all the puyos in order of lowest Y to highest Y.
		// Like the same thing we did above, just with Y instead of X and bigger instead of smaller
		int _oldLowestY=_passedBoard->h;
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
		if (!puyoCanFell(_passedBoard,&(_passedSet->pieces[i]))){
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
	if (!(_passedSet->pieces[0].movingFlag & FLAG_ANY_HMOVE)){
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
		if (getBoard(_passedBoard,_passedSet->pieces[j].tileX+_xDist,_passedSet->pieces[j].tileY+_yDist)!=COLOR_NONE){
			return 0;
		}
	}
	return 1;
}
struct pieceSet getRandomPieceSet(){
	struct pieceSet _ret;
	int _spawnCol = getSpawnCol(6);
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

	_ret.pieces[1].tileX=_spawnCol;
	_ret.pieces[1].tileY=0;
	_ret.pieces[1].displayX=0;
	_ret.pieces[1].displayY=tileh;
	_ret.pieces[1].movingFlag=0;
	_ret.pieces[1].color=randInt(0,numColors-1)+COLOR_REALSTART;
	_ret.pieces[1].holdingDown=1;

	_ret.pieces[0].tileX=_spawnCol;
	_ret.pieces[0].tileY=1;
	_ret.pieces[0].displayX=0;
	_ret.pieces[0].displayY=0;
	_ret.pieces[0].movingFlag=0;
	_ret.pieces[0].color=randInt(0,numColors-1)+COLOR_REALSTART;
	_ret.pieces[0].holdingDown=0;

	#if TESTFEVERPIECE
		_ret.pieces[2].tileX=3;
		_ret.pieces[2].tileY=1;
		_ret.pieces[2].displayX=0;
		_ret.pieces[2].displayY=0;
		_ret.pieces[2].movingFlag=0;
		_ret.pieces[2].color=randInt(0,numColors-1)+COLOR_REALSTART;
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
void drawPiecesetOffset(int _offX, int _offY, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin){
	int i;
	for (i=0;i<_myPieces->count;++i){
		drawNormPiece(_myPieces->pieces[i].color,_offX+_myPieces->pieces[i].displayX,_offY+_myPieces->pieces[i].displayY,0,_passedSkin,tileh);
	}
}
// Don't draw based on the position data stored in the pieces, draw the _anchorIndex puyo at _x, _y and then draw all the other pieces relative to it.
void drawPiecesetRelative(int _x, int _y, int _anchorIndex, int _size, struct pieceSet* _myPieces, struct puyoSkin* _passedSkin){
	int i;
	for (i=0;i<_myPieces->count;++i){
		drawNormPiece(_myPieces->pieces[i].color,_x+(_myPieces->pieces[i].tileX-_myPieces->pieces[_anchorIndex].tileX)*_size,_y+(_myPieces->pieces[i].tileY-_myPieces->pieces[_anchorIndex].tileY)*_size,0,_passedSkin,tileh);
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
// _sTime only needed if _changeFlags
void forceRotateSet(struct pieceSet* _passedSet, char _isClockwise, char _changeFlags, u64 _sTime){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE)==0 && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
			int _destX;
			int _destY;
			getPostRotatePos(_isClockwise,getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY),&_destX,&_destY);
			_passedSet->pieces[i].tileX+=_destX;
			_passedSet->pieces[i].tileY+=_destY;
			if (_changeFlags){
				_passedSet->pieces[i].completeRotateTime = _sTime+ROTATETIME;
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
				if (getBoard(_passedBoard,_destX,_destY)!=COLOR_NONE){
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
										UNSET_FLAG(_passedSet->pieces[j].movingFlag,FLAG_ANY_HMOVE);
									}
								}
							}
							if (_allowForceShift==2){ // Optional change flags
								resetDyingFlagMaybe(_passedBoard,_passedSet);
								// If there's a forced shift, give it a smooth transition by hvaing the anchor piece, which all the other pieces' positions are relative to, move smoothly.
								if (_xDist!=0){
									_passedSet->rotateAround->movingFlag|=(_xDist<0 ? FLAG_MOVELEFT : FLAG_MOVERIGHT);
									_passedSet->rotateAround->diffHMoveTime = ROTATETIME;
									_passedSet->rotateAround->completeHMoveTime = _sTime+_passedSet->rotateAround->diffHMoveTime;
									_passedSet->rotateAround->transitionDeltaX = tilew*_xDist;
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
			forceRotateSet(_passedSet,_isClockwise,1,_sTime);
			resetDyingFlagMaybe(_passedBoard,_passedSet);
		}else{ // If we can't rotate
			// If we're a regular piece with one piece above the other
			if (_passedSet->count==2 && _passedSet->pieces[0].tileX==_passedSet->pieces[1].tileX){
				if (_canDoubleRotate){ // Do the rotate
					struct movingPiece* _moveOnhis = _passedSet->rotateAround==&(_passedSet->pieces[0])?&(_passedSet->pieces[1]):&(_passedSet->pieces[0]); // of the two puyos, get the one that isn't the anchor
					int _yChange = 2*(_moveOnhis->tileY<_passedSet->rotateAround->tileY ? 1 : -1);
					char _canProceed=1;
					if (getBoard(_passedBoard,_moveOnhis->tileX,_moveOnhis->tileY+_yChange)!=COLOR_NONE){
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
						_moveOnhis->completeRotateTime = _sTime+ROTATETIME;
						resetDyingFlagMaybe(_passedBoard,_passedSet);
					}
				}
				_ret|=1;
			}
		}
		// update puyo h shift
		int i;
		for (i=0;i<_passedSet->count;++i){
			snapPuyoDisplayPossible(&(_passedSet->pieces[i]));
		}
	}
	return _ret;
}
void pieceSetControls(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, struct controlSet* _passedControls, u64 _sTime, signed char _dasActive){
	if (wasJustPressed(BUTTON_RIGHT) || wasJustPressed(BUTTON_LEFT) || _dasActive!=0){
		tryHShiftSet(_passedSet,_passedBoard,_dasActive!=0 ? _dasActive : (wasJustPressed(BUTTON_RIGHT) ? 1 : -1),_sTime);
	}
	if (wasJustPressed(BUTTON_A) || wasJustPressed(BUTTON_B)){
		char _canDoubleRotate=_sTime<=_passedControls->lastFailedRotateTime+DOUBLEROTATETAPTIME;
		if (tryStartRotate(_passedSet,_passedBoard,wasJustPressed(BUTTON_A),_canDoubleRotate,_sTime)&1){ // If double rotate tried to be used
			if (_canDoubleRotate){
				// It worked, reset it
				_passedControls->lastFailedRotateTime=0;
			}else{
				// Queue the double press time
				_passedControls->lastFailedRotateTime=_sTime;
			}
		}

	}
}
//////////////////////////////////////////////////
// puyoBoard
//////////////////////////////////////////////////
// if a regular piece is touching the x
// applies the death status for you if needed.
char applyBoardDeath(struct puyoBoard* _passedBoard, u64 _sTime){
	if (fastGetBoard(_passedBoard,getSpawnCol(_passedBoard->w),_passedBoard->numGhostRows)!=0 && _passedBoard->pieceStatus[getSpawnCol(_passedBoard->w)][_passedBoard->numGhostRows]==0){
		_passedBoard->statusTimeEnd=_sTime+DEATHANIMTIME;
		_passedBoard->status=STATUS_DEAD;
		return 1;
	}
	return 0;
}
int getFreeColumnYPos(struct puyoBoard* _passedBoard, int _columnIndex, int _minY){
	int i;
	for (i=_passedBoard->h-1;i>=_minY;--i){
		if (fastGetBoard(_passedBoard,_columnIndex,i)==COLOR_NONE){
			return i;
		}
	}
	return _minY-1;
}
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
void freeColorArray(puyoColor** _passed, int _w){
	int i;
	for (i=0;i<_w;++i){
		free(_passed[i]);
	}
}
void freeBoard(struct puyoBoard* _passedBoard){
	freeColorArray(_passedBoard->board,_passedBoard->w);
	printf("TODO - freeBoard\n");
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
	_retBoard.leftoverGarbage=0;
	_retBoard.curChain=0;
	_retBoard.readyGarbage=0;
	_retBoard.incomingLength=0;
	_retBoard.incomingGarbage=NULL;
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
	if (_passedBoard->board[_x][_y]==COLOR_GARBAGE){
		return 0;
	}
	unsigned char _ret=0;
	if (canTile(_passedBoard,_passedBoard->board[_x][_y],_x,_y+1)){
		_ret|=AUTOTILEDOWN;
	}
	if (_y!=_passedBoard->numGhostRows && canTile(_passedBoard,_passedBoard->board[_x][_y],_x,_y-1)){
		_ret|=AUTOTILEUP;
	}
	if (canTile(_passedBoard,_passedBoard->board[_x][_y],_x+1,_y)){
		_ret|=AUTOTILERIGHT;
	}
	if (canTile(_passedBoard,_passedBoard->board[_x][_y],_x-1,_y)){
		_ret|=AUTOTILELEFT;
	}
	return _ret;
}
void drawBoard(struct puyoBoard* _drawThis, int _startX, int _startY, char _isPlayerBoard, u64 _sTime){
	int _oldStartY = _startY;
	enableClipping(_startX,_startY-tileh,tilew*(_drawThis->w+NEXTWINDOWTILEW),tileh*(_drawThis->h-_drawThis->numGhostRows+2));
	if (_drawThis->status==STATUS_DEAD){
		if (_sTime<=_drawThis->statusTimeEnd){
			_startY+=partMoveFills(_sTime,_drawThis->statusTimeEnd,DEATHANIMTIME,(_drawThis->h-_drawThis->numGhostRows+1)*tileh);
		}else{
			return;
		}
	}
	drawRectangle(_startX,_startY,tileh*_drawThis->w,(_drawThis->h-_drawThis->numGhostRows)*tileh,150,0,0,255);
	int i;
	if (_isPlayerBoard && _drawThis->status==STATUS_NORMAL){
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
					if (getPopNum(_drawThis,_tileX,_yDests[i],1,_passedSet->pieces[i].color)>=minPopNum){
						// Mark all those puyos with POSSIBLEPOPBYTE
						getPopNum(_drawThis,_tileX,_yDests[i],POSSIBLEPOPBYTE,_passedSet->pieces[i].color);
					}
				}
				// Any check that could've needed this temp puyo is now over. Remove the temp piece.
				_drawThis->board[_passedSet->pieces[i].tileX][_yDests[i]]=COLOR_NONE;
			}
			//
			if (_yDests[i]>=_drawThis->numGhostRows){
				drawGhostIcon(_passedSet->pieces[i].color,_startX+_tileX*tilew,_startY+(_yDests[i]-_drawThis->numGhostRows)*tileh,_drawThis->usingSkin);
			}
		}
		free(_yDests);
	}
	// Draw next window, animate if needed
	if (_drawThis->status!=STATUS_NEXTWINDOW){
		for (i=0;i<_drawThis->numNextPieces-1;++i){
			drawPiecesetRelative(_startX+_drawThis->w*tilew+(i&1)*tilew,_startY+i*tileh*2+tileh,0,tileh,&(_drawThis->nextPieces[i]),_drawThis->usingSkin);
		}
	}else{
		// latest piece moves up
		drawPiecesetRelative(_startX+_drawThis->w*tilew,_startY+tileh-partMoveFills(_sTime, _drawThis->statusTimeEnd, NEXTWINDOWTIME, tileh*2),0,tileh,&(_drawThis->nextPieces[0]),_drawThis->usingSkin);
		// all others move diagonal`
		for (i=0;i<_drawThis->numNextPieces;++i){
			int _xChange;
			if (i!=0){
				_xChange=partMoveFills(_sTime, _drawThis->statusTimeEnd, NEXTWINDOWTIME, tileh)*((i&1) ? -1 : 1);
			}else{
				_xChange=0;
			}
			drawPiecesetRelative(_startX+_drawThis->w*tilew+(i&1)*tilew+_xChange,_startY+i*tileh*2+tileh-partMoveFills(_sTime, _drawThis->statusTimeEnd, NEXTWINDOWTIME, tileh*2),0,tileh,&(_drawThis->nextPieces[i]),_drawThis->usingSkin);
		}
	}
	for (i=0;i<_drawThis->w;++i){
		double _squishyY;
		char _isSquishyColumn=0;
		int j;
		for (j=_drawThis->h-1;j>=_drawThis->numGhostRows;--j){
			if (_drawThis->board[i][j]!=0){
				if (_isSquishyColumn){
					if (_drawThis->board[i][j]!=COLOR_GARBAGE){
						_squishyY-=drawSquishingPuyo(_drawThis->board[i][j],tileh*i+_startX,_squishyY,USUALSQUISHTIME,_drawThis->pieceStatusTime[i][j],_drawThis->usingSkin,_sTime);
					}else{
						drawNormPiece(_drawThis->board[i][j],tileh*i+_startX,_squishyY,0,_drawThis->usingSkin,tileh);
						_squishyY-=tileh;
					}
				}else{
					switch (_drawThis->pieceStatus[i][j]){
						case PIECESTATUS_POPPING:
							drawPoppingPiece(_drawThis->board[i][j],tileh*i+_startX,tileh*(j-_drawThis->numGhostRows)+_startY,_drawThis->statusTimeEnd,_drawThis->usingSkin,_sTime);
							break;
						case PIECESTATUS_SQUSHING:
							_isSquishyColumn=1;
							_squishyY=tileh*(j-_drawThis->numGhostRows)+_startY;
							++j; // Redo this iteration where we'll draw as squishy column
							break;
						default:
							if (_isPlayerBoard && _drawThis->popCheckHelp[i][j]==POSSIBLEPOPBYTE){
								drawPotentialPopPiece(_drawThis->board[i][j],tileh*i+_startX,tileh*(j-_drawThis->numGhostRows)+_startY,getTilingMask(_drawThis,i,j),_drawThis->usingSkin,tileh);
							}else{
								drawNormPiece(_drawThis->board[i][j],tileh*i+_startX,tileh*(j-_drawThis->numGhostRows)+_startY,getTilingMask(_drawThis,i,j),_drawThis->usingSkin,tileh);
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
	// temp draw garbage
	int _totalGarbage = _drawThis->readyGarbage;
	for (i=0;i<_drawThis->incomingLength;++i){
		_totalGarbage+=_drawThis->incomingGarbage[i];
	}
	gbDrawTextf(regularFont,_startX,_startY-tileh,255,255,255,255,"%d",_totalGarbage);
	if (_drawThis->status==STATUS_DEAD && _sTime<_drawThis->statusTimeEnd){
		// Draw death rectangle
		drawRectangle(_startX,_oldStartY+(_drawThis->h-_drawThis->numGhostRows)*tileh,_drawThis->w*tilew,tileh,255,0,0,255);
	}else{
		// chain
		if (_drawThis->status==STATUS_POPPING){
			char* _drawString = easySprintf("%d-TETRIS",_drawThis->curChain);
			int _cachedWidth = textWidth(regularFont,_drawString);
			gbDrawText(regularFont,_startX+cap(_drawThis->chainNotifyX-_cachedWidth/2,0,_drawThis->w*tilew-_cachedWidth),_startY+_drawThis->chainNotifyY+textHeight(regularFont)/2,_drawString,95,255,83);
			free(_drawString);
		}
		// draw score
		char* _drawString = easySprintf("%08"PRIu64,_drawThis->score);
		gbDrawText(regularFont, _startX+easyCenter(textWidth(regularFont,_drawString),_drawThis->w*tilew), _startY+(_drawThis->h-_drawThis->numGhostRows)*tileh, _drawString, 255, 255, 255);
		free(_drawString);
	}
	disableClipping();
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
	if (_y>=_passedBoard->numGhostRows){
		puyoColor _curColor = getBoard(_passedBoard,_x,_y);
		if (_curColor==COLOR_GARBAGE){
			_passedBoard->popCheckHelp[_x][_y]=2;
			_passedBoard->pieceStatus[_x][_y]=_setStatusToThis;
		}else if (_curColor==_shapeColor && _passedBoard->popCheckHelp[_x][_y]!=2){
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
					_nextFallY = getFreeColumnYPos(_passedBoard,i,0);
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
					_lowStartPuyoFall(&_newPiece,_nextFallY--,SPLITFALLTIME,_sTime);
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
	_passedBoard->statusTimeEnd=_sTime+NEXTWINDOWTIME;
}
// _passedState is optional
// _returnForIndex will tell it which set to return the value of
// pass -1 to get return values from all
// TODO - The _returnForIndex is a bit useless right now because the same index can refer to two piece sets. Like if you want to return for index 0, and index 0 is a removed piece set. Then index 1 will also become index 0.
signed char updateBoard(struct puyoBoard* _passedBoard, struct gameState* _passedState, signed char _returnForIndex, u64 _sTime){
	if (_passedBoard->status==STATUS_DEAD){
		return 0;
	}
	// If we're done dropping, try popping
	if (_passedBoard->status==STATUS_DROPPING && _passedBoard->numActiveSets==0){
		_passedBoard->status=STATUS_SETTLESQUISH;
	}else if (_passedBoard->status==STATUS_DROPGARBAGE && _passedBoard->numActiveSets==0){
		if (applyBoardDeath(_passedBoard,_sTime)){
			return 0;
		}else{
			transitionBoradNextWindow(_passedBoard,_sTime);
		}
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
					if (fastGetBoard(_passedBoard,_x,_y)>=COLOR_REALSTART && _passedBoard->popCheckHelp[_x][_y]==0){
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
			if (applyBoardDeath(_passedBoard,_sTime)){
				return 0;
			}
			if (_numGroups!=0){
				// Used when updating garbage
				u64 _oldScore = _passedBoard->curChain!=0 ? _passedBoard->curChainScore : 0;
				_passedBoard->curChain++;
				_passedBoard->chainNotifyX=_avgX*tilew;
				_passedBoard->chainNotifyY=(_avgY-_passedBoard->numGhostRows)*tileh;
				_passedBoard->curChainScore=(10*_numPopped)*cap(chainPowers[cap(_passedBoard->curChain-1,0,23)]+colorCountBouns[cap(_numUniqueColors-1,0,5)]+_totalGroupBonus,1,999);
				_passedBoard->status=STATUS_POPPING;
				_passedBoard->statusTimeEnd=_sTime+popTime;
				// Send new garbage
				if (_passedState!=NULL){
					double _oldGarbage=_passedBoard->leftoverGarbage+scoreToGarbage(&_passedState->settings,_oldScore);
					//printf("last time we had %f from a socre of %ld with leftover %f\n",_oldGarbage,_oldScore,(_oldGarbage-floor(_oldGarbage)));
					//printf("This score of %ld leads to %f with toal of %f new\n",_passedBoard->curChainScore,scoreToGarbage(&_passedState->settings,_passedBoard->curChainScore),scoreToGarbage(&_passedState->settings,_passedBoard->curChainScore)+(_oldGarbage-floor(_oldGarbage)));
					int _newGarbage = scoreToGarbage(&_passedState->settings,_passedBoard->curChainScore)+(_oldGarbage-floor(_oldGarbage));
					sendGarbage(_passedState,_passedBoard,_newGarbage);
				}
			}else{
				if (_passedBoard->readyGarbage!=0){
					int _fullRows = _passedBoard->readyGarbage/_passedBoard->w;
					if (_fullRows>MAXGARBAGEROWS){
						_fullRows=MAXGARBAGEROWS;
					}
					struct pieceSet* _garbageColumns = malloc(sizeof(struct pieceSet)*_passedBoard->w);
					int i;
					for (i=0;i<_passedBoard->w;++i){
						_garbageColumns[i].count=_fullRows;
						_garbageColumns[i].isSquare=0;
						_garbageColumns[i].quickLock=1;
					}
					if (_fullRows!=MAXGARBAGEROWS && _passedBoard->readyGarbage%_passedBoard->w!=0){ // If we're not the max drop lines and we can put the leftovers on top.
						int _numTop = _passedBoard->readyGarbage%_passedBoard->w; // Garbage on top
						int i;
						for (i=0;i<_numTop;++i){
							int _nextX = randInt(0,_passedBoard->w-1-i);
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
					for (i=0;i<_passedBoard->w;++i){
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
							snapPuyoDisplayPossible(&_newPiece);
							_lowStartPuyoFall(&_newPiece,_firstDestY-j,GARBAGEFALLTIME,_sTime);
							_garbageColumns[i].pieces[j] = _newPiece;
						}
						addSetToBoard(_passedBoard,&_garbageColumns[i]);
					}
					//
					_passedBoard->status=STATUS_DROPGARBAGE;
					if (_fullRows==MAXGARBAGEROWS){
						_passedBoard->readyGarbage-=MAXGARBAGEROWS*_passedBoard->w;
					}else{
						_passedBoard->readyGarbage=0;
					}
				}else{
					transitionBoradNextWindow(_passedBoard,_sTime);
				}
			}
		}
	}else if (_passedBoard->status==STATUS_NEXTWINDOW){
		_passedBoard->curChain=0;
		if (_sTime>=_passedBoard->statusTimeEnd){
			lazyUpdateSetDisplay(&(_passedBoard->nextPieces[0]),_sTime);
			addSetToBoard(_passedBoard,&(_passedBoard->nextPieces[0]));
			memmove(&(_passedBoard->nextPieces[0]),&(_passedBoard->nextPieces[1]),sizeof(struct pieceSet)*(_passedBoard->numNextPieces-1));
			_passedBoard->nextPieces[_passedBoard->numNextPieces-1] = getRandomPieceSet();
			_passedBoard->status=STATUS_NORMAL;
		}
	}else if (_passedBoard->status==STATUS_POPPING){
		if (_sTime>=_passedBoard->statusTimeEnd){
			int _x, _y;
			for (_x=0;_x<_passedBoard->w;++_x){
 				for (_y=0;_y<_passedBoard->h;++_y){
					if (_passedBoard->pieceStatus[_x][_y]==PIECESTATUS_POPPING){
						_passedBoard->board[_x][_y]=0;
					}
				}
			}
			// Apply garbage or calculate leftover score
			if (_passedState!=NULL){
				char _shouldApply=1;
				// Make a backup of the current board state because we're going to mess  up the other one
				puyoColor** _oldBoard = newJaggedArrayColor(_passedBoard->w,_passedBoard->h);
				int i;
				for (i=0;i<_passedBoard->w;++i){
					memcpy(_oldBoard[i],_passedBoard->board[i],sizeof(puyoColor)*_passedBoard->h);
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
				freeColorArray(_passedBoard->board,_passedBoard->w);
				_passedBoard->board=_oldBoard;
				//
				if (_shouldApply){
					applyGarbage(_passedState,_passedBoard);
					double _totalSent=_passedBoard->leftoverGarbage+scoreToGarbage(&_passedState->settings,_passedBoard->curChainScore);
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
	for (i=0;i<_passedBoard->w;++i){
		int j;
		for (j=_passedBoard->h-1;j>0;--j){ // > 0 because it's useless if top column is empty
			// Find the first empty spot in this column with a puyo above it
			if (fastGetBoard(_passedBoard,i,j-1)!=COLOR_NONE && fastGetBoard(_passedBoard,i,j)==COLOR_NONE){
				_ret=1;
				int _nextFallY = getFreeColumnYPos(_passedBoard,i,0);
				--j; // Start at the piece we found
				int k;
				for (k=j;k>=0;--k){
					_passedBoard->board[i][_nextFallY--]=_passedBoard->board[i][k];
					_passedBoard->board[i][k]=0;
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
	for (_x=0;_x<_passedBoard->w;++_x){
		for (_y=_passedBoard->numGhostRows;_y<_passedBoard->h;++_y){
			if (fastGetBoard(_passedBoard,_x,_y)>=COLOR_REALSTART && _passedBoard->popCheckHelp[_x][_y]==0){
				if (getPopNum(_passedBoard,_x,_y,1,_passedBoard->board[_x][_y])>=minPopNum){
					return 1;
				}
			}
		}
	}
	return 0;
}
//////////////////////////////////////////////////
// gameState
//////////////////////////////////////////////////
double scoreToGarbage(struct gameSettings* _passedSettings, long _passedPoints){
	return _passedPoints/(double)_passedSettings->pointsPerGar;
}
double getLeftoverGarbage(struct gameSettings* _passedSettings, long _passedPoints){
	double _all = scoreToGarbage(_passedSettings,_passedPoints);
	return _all-(int)_all;
}
int getStateIndexOfBoard(struct gameState* _passedState, struct puyoBoard* _passedBoard){
	return (_passedBoard-_passedState->boards);
}
struct gameState newGameState(int _count){
	struct gameState _ret;
	_ret.numBoards=_count;
	_ret.boards = malloc(sizeof(struct puyoBoard)*_count);
	_ret.controllers = malloc(sizeof(struct boardController)*_count);
	// Default settings
	_ret.settings.pointsPerGar=70;
	return _ret;
}
// Use after everything is set up
void endStateInit(struct gameState* _passedState){
	int i;
	// This init is done here because it may change one day to require more than just the number of other boards, like team data.
	for (i=0;i<_passedState->numBoards;++i){
		_passedState->boards[i].incomingLength = _passedState->numBoards;
		_passedState->boards[i].incomingGarbage = calloc(1,sizeof(int)*_passedState->numBoards);
	}
}
void startGameState(struct gameState* _passedState, u64 _sTime){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		transitionBoradNextWindow(&(_passedState->boards[i]),_sTime);
	}
}
void updateGameState(struct gameState* _passedState, u64 _sTime){
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		signed char _updateRet = updateBoard(&(_passedState->boards[i]),_passedState,_passedState->boards[i].status==STATUS_NORMAL ? 0 : -1,_sTime);
		_passedState->controllers[i].func(_passedState->controllers[i].data,_passedState,&(_passedState->boards[i]),_updateRet,_sTime);
		endFrameUpdateBoard(&(_passedState->boards[i]),_updateRet); // TODO - Move this to frame end?
	}
}
void drawGameState(struct gameState* _passedState, u64 _sTime){
	int _widthPerBoard = screenWidth/_passedState->numBoards;

	int i;
	for (i=0;i<_passedState->numBoards;++i){
		drawBoard(&(_passedState->boards[i]),_widthPerBoard*i+easyCenter((_passedState->boards[i].w+NEXTWINDOWTILEW)*tilew,_widthPerBoard),easyCenter((_passedState->boards[i].h-_passedState->boards[i].numGhostRows)*tileh,screenHeight),_passedState->controllers[i].func==updateControlSet,_sTime);
	}
}
// This board's chain is up. Apply all its garbage to its targets.
void applyGarbage(struct gameState* _passedState, struct puyoBoard* _source){
	int _applyIndex = getStateIndexOfBoard(_passedState,_source);
	int i;
	for (i=0;i<_passedState->numBoards;++i){
		_passedState->boards[i].readyGarbage+=_passedState->boards[i].incomingGarbage[_applyIndex];
		_passedState->boards[i].incomingGarbage[_applyIndex]=0;
	}
}
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
void sendGarbage(struct gameState* _passedState, struct puyoBoard* _source, int _newGarbageSent){
	// Offsetting
	if (_source->readyGarbage!=0){
		if (_lowOffsetGarbage(&_source->readyGarbage,&_newGarbageSent)){
			return;
		}
	}
	int i;
	for (i=0;i<_source->incomingLength;++i){
		if (_lowOffsetGarbage(&(_source->incomingGarbage)[i],&_newGarbageSent)){
			return;
		}
	}
	int _thisBoardIndex = getStateIndexOfBoard(_passedState,_source);
	// Attack using leftover garbage
	for (i=0;i<_passedState->numBoards;++i){
		if (i!=_thisBoardIndex){
			_passedState->boards[i].incomingGarbage[_thisBoardIndex]+=_newGarbageSent;
		}
	}
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
	int _spawnPos = _aiBoard->w/2-1;
	int i;
	for (i=_spawnPos+1;i<_aiBoard->w;++i){
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
		if (fastGetBoard(_aiBoard,_spawnPos,_aiBoard->h-1)==COLOR_NONE){
			for (i=_aiBoard->h-;i>0;--i){
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
void matchThreeAi(struct aiState* _passedState, struct pieceSet* _retModify, struct puyoBoard* _aiBoard, int _numBoards, struct puyoBoard* _passedBoards){
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
	for (i=0;i<_aiBoard->w;++i){
		_avgStackHeight+=(_aiBoard->h-getFreeColumnYPos(_aiBoard,i,0)-1);
	}
	_avgStackHeight = round(_avgStackHeight/(double)_aiBoard->w);
	if (_avgStackHeight>round((_aiBoard->h-_aiBoard->numGhostRows)/(double)MATCHTHREEAI_POPDENOMINATOR)){
		if (_avgStackHeight>round((_aiBoard->h-_aiBoard->numGhostRows)/(double)MATCHTHREEAI_PANICDENOMINATOR)){
			_panicLevel=2;
		}else{
			_panicLevel=1;
		}
	}else{
		_panicLevel=0;
	}
	forceSetSetX(_retModify,0,0);

	// How good each potential place position is
	int _placeScores[_aiBoard->w*4];
	int _spawnCol = getSpawnCol(_aiBoard->w);
	int _bestIndex=0;
	_placeScores[_bestIndex]=-999; // Initialize to beatable score
	int _scoreIndex;
	int _rotateLoop;
	// For each possible rotation
	for (_rotateLoop=0;_rotateLoop<4;++_rotateLoop){
		//AIDEBUGprintf("rotate iterate\n");
		_scoreIndex=_rotateLoop*_aiBoard->w;
		// Try all x positions until can't move anymore
		for (;;_scoreIndex++,forceSetSetX(_retModify,1,1)){
			int i;
			int* _yDests = getSetDestY(_retModify,_aiBoard);
			// Get the average y dest pos. also check for y dest that's too high and therefor invalid
			int _avgDest=0;
			for (i=0;i<_retModify->count;++i){
				// If it's a valid position and one that won't kill us
				if (_yDests[i]>=_aiBoard->numGhostRows/2 && _yDests[i]>=0 && !(_retModify->pieces[i].tileX==_spawnCol && _yDests[i]<=_aiBoard->numGhostRows)){
					_avgDest+=_yDests[i];
				}else{
					//AIDEBUGprintf("%d is invalid %d\n",_scoreIndex,_yDests[i]);
					break;
				}
			}
			if (i!=_retModify->count){ // Stop if we've found an invalid position
				continue;
			}
			_placeScores[_scoreIndex]=0;
			// average dest with small numbers being low in the board
			_avgDest=_aiBoard->h-floor(_avgDest/(double)_retModify->count);
			// Calculate scores based on how many will connect
			setTempPieces(_retModify,_aiBoard,_yDests,1);
			char _willPop=0;
			for (i=0;i<_retModify->count;++i){
				int _numConnect = getPopNum(_aiBoard,_retModify->pieces[i].tileX,_yDests[i],(_nextPopCheckHelp++),_retModify->pieces[i].color);
				if (_numConnect!=0){
					_placeScores[_scoreIndex]+=_numConnect;
					if (_numConnect>=minPopNum){
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
			}else{
				//AIDEBUGprintf("%d not god enough vs %d of %d\n",_placeScores[_scoreIndex],_placeScores[_bestIndex],_bestIndex);
			}
			if (!setCanObeyShift(_aiBoard,_retModify,1,0)){ // Will stop at walls
				break;
			}
		}
		// Do the next rotation
		forceSetSetX(_retModify,0,0);
		if (setCanRotate(_retModify,_aiBoard,1,1,0)){ // Force shift allowed to account for third rotation
			forceRotateSet(_retModify,1,0,0); // Rotate, but don't bother with flags
		}else{
			// Rotate back to start position before we go
			for (;_rotateLoop<4;++_rotateLoop){
				forceRotateSet(_retModify,1,0,0);
			}
			break;
		}
	}
	int _numRotates = _bestIndex/_aiBoard->w;
	int _destX = _bestIndex%_aiBoard->w;
	for (i=0;i<_numRotates;++i){
		forceRotateSet(_retModify,1,0,0);
	}
	forceSetSetX(_retModify,_destX,0);
}
void updateAi(void* _stateData, struct gameState* _curGameState, struct puyoBoard* _passedBoard, signed char _updateRet, u64 _sTime){
	struct aiState* _passedState = _stateData;
	(void)_updateRet;

	if (_passedBoard->status!=STATUS_NORMAL){
		// Queue another ai update once we're normal again
		_passedState->nextAction.anchorDestX=-1;
		return;
	}
	if (aiRunNeeded(_passedState)){
		// Reset the aiState first
		_passedState->softdropped=0;

		struct pieceSet* _passModify = dupPieceSet(_passedBoard->activeSets->data);
		_passedState->updateFunction(_passedState,_passModify,_passedBoard,_curGameState->numBoards,_curGameState->boards);
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
		//AIDEBUGprintf("Trying to move to %d;%d\n",_passedState->nextAction.anchorDestX,_passedState->nextAction.anchorDestY);
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
					_moveThis->pieces[i].transitionDeltaY=_tileDiff*tileh;

					_moveThis->pieces[i].diffFallTime=(_moveThis->singleTileVSpeed*_tileDiff)/(PUSHDOWNTIMEMULTIPLIERCPU+1);
					_moveThis->pieces[i].completeFallTime = _sTime+_moveThis->pieces[i].diffFallTime-_bonusTime/(PUSHDOWNTIMEMULTIPLIERCPU+1);
				}
				#endif
			}
		}
	}
}
//////////////////////////////////////////////////
void updateControlSet(void* _controlData, struct gameState* _passedState, struct puyoBoard* _passedBoard, signed char _updateRet, u64 _sTime){
	struct controlSet* _passedControls = _controlData;
	if (_updateRet!=0){
		if (_passedBoard->status==STATUS_NORMAL){
			if (_updateRet&2){
				// Here's the scenario:
				// Holding down. The next millisecond, the puyo will go to the next tile.
				// It's the next frame. The puyo goes down to the next tile. 16 milliseconds have passed bwteeen the last frame and now.
				// We were holding down for those 16 milliseconds in between, so they need to be accounted for in the push down time for the next tile.
				// Note that if they weren't holding down between the frames, it won't matter because this startHoldTime variable isn't looked at in that case.
				_passedControls->startHoldTime=_sTime-(_sTime-_passedControls->lastFrameTime);
			}
		}
	}
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
					int _offsetAmount = (_sTime-_passedControls->startHoldTime)*PUSHDOWNTIMEMULTIPLIER;
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
void rebuildSizes(int _w, int _h, int _numBoards, double _tileRatioPad){
	screenWidth = getScreenWidth();
	screenHeight = getScreenHeight();

	int _fitWidthSize = screenWidth/(double)((_w+NEXTWINDOWTILEW)*_numBoards+(_numBoards-1)+_tileRatioPad*2);
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

	char* _fixedPath = fixPathAlloc("liberation-sans-bitmap.sfl",TYPE_EMBEDDED);
	regularFont = loadFont(_fixedPath,-1);
	free(_fixedPath);

	currentSkin = loadChampionsSkinFile(loadImageEmbedded("aqua.png"));
}
int main(int argc, char const** argv){
	init();
	// Boards
	struct gameState _testState = newGameState(2);
	_testState.boards[0] = newBoard(6,14,2);
	_testState.boards[1] = newBoard(6,14,2);
	// Player controller for board 0
	_testState.controllers[0].func = updateControlSet;
	_testState.controllers[0].data = malloc(sizeof(struct controlSet));
	*((struct controlSet*)_testState.controllers[0].data) = newControlSet(goodGetMilli());
	// CPU controller for board 1
	struct aiState* _newState = malloc(sizeof(struct aiState));
	memset(_newState,0,sizeof(struct aiState));
	_newState->nextAction.anchorDestX=-1;
	_newState->updateFunction=matchThreeAi;
	_testState.controllers[1].func = updateAi;
	_testState.controllers[1].data = _newState;

	endStateInit(&_testState);

	rebuildSizes(_testState.boards[0].w,_testState.boards[0].h,_testState.numBoards,1);

	startGameState(&_testState,goodGetMilli());
	#if FPSCOUNT == 1
	u64 _frameCountTime = goodGetMilli();
	int _frames=0;
	#endif

	/*
	  struct squishyBois{
	  int numPuyos;
	  // 0 indicates it's at the top
	  int* yPos;
	  int destY; // bottom of the dest
	  u64 startSquishTime;
	  u64 startTime;
	  };
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
			rebuildSizes(_testState.boards[0].w,_testState.boards[0].h,_testState.numBoards,1);
			int i;
			for (i=0;i<_testState.numBoards;++i){
				rebuildBoardDisplay(&(_testState.boards[i]),_sTime);
			}
		}
		if (wasJustPressed(BUTTON_L)){
			printf("Input in <>;<> format starting at %d:\n",COLOR_REALSTART);
			struct pieceSet* _firstSet = _testState.boards[0].activeSets->data;
			scanf("%d;%d", &(_firstSet->pieces[1].color),&(_firstSet->pieces[0].color));
		}
		if (wasJustPressed(BUTTON_R)){
			_testState.boards[1].readyGarbage+=_testState.boards[1].w;
		}
		if (wasJustPressed(BUTTON_X)){
			printf("%d\n",STATUS_UNDEFINED);
			int i;
			for (i=0;i<_testState.numBoards;++i){
				printf("id: %d; state: %d; activesets: %d\n",i,_testState.boards[i].status,_testState.boards[i].numActiveSets);
			}
		}
		updateGameState(&_testState,_sTime);
		controlsEnd();

		startDrawing();
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
