// Note - For rotation, we can calculate the current position using the finish time. This would overwrite displayX and displayY completely instead of beign relative to them

//#error the problem is that it is using the difference in Y values to know how far to go. When I rotate, the difference between y values becomes greater. possible solution is to store the change in y values instead of the transitionOldY. That's actually genius, because its always goign to be relative to the destination. perfecdt. geius. Thats kind of like what i was doing before because change in Y va;ie was TILEH
//#warning TODO - Completley change the movement system. Allow a puyo to move as many tiles as it wants on either axis over as long as it wants. Point of this is to allow puyos to fall large distances with only being set once. This could involve removing fallSpeed from pieceSet.

// TODO - Smooth transition if a puyo is forced ot the side by rotate
// TODO - Put DAS Charge and stuff in a struct
// TODO - use the bitmap autotiling
// TODO - Maybe a board can have a pointer to a function to get the next piece. I can map it to either network or random generator

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

#if !defined M_PI
	#warning makeshift M_PI
	#define M_PI 3.14159265358979323846
#endif

#if !defined M_PI_2
	#warning makeshift M_PI_2
	#define M_PI_2 (M_PI/(double)2)
#endif

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

#define TILEH 45
#define TILEW TILEH
#define HALFTILE (TILEW/2)

#define DASTIME 150

// How long it takes a puyo to fall half of one tile on the y axis
int FALLTIME = 900;
int HMOVETIME = 30;
int ROTATETIME = 50;

typedef int puyoColor;

#define COLOR_NONE 0
#define COLOR_IMPOSSIBLE 1

#define FPSCOUNT 1
#define SHOWFPSCOUNT 0

typedef enum{
	STATUS_NORMAL, // We're moving the puyo around and stuff
	STATUS_POPPING, // We're waiting for puyo to pop
	STATUS_DROPPING, // puyo are falling into place. This is the status after you place a piece and after STATUS_POPPING
	STATUS_NEXTWINDOW, // Waiting for next window. This is the status after STATUS_DROPPING if no puyo connect
	STATUS_TRASHFALL //
}boardStatus;

struct puyoBoard{
	int w;
	int h;
	puyoColor** board; // ints represent colors
	int** pieceStatus; // Interpret this value depending on status
	boardStatus status;
	int garbageQueue;
	int numActiveSets;
	struct pieceSet* activeSets;
	int numNextPieces;
	struct pieceSet* nextPieces;
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
// getPieceSet();

int screenWidth;
int screenHeight;

u64 _globalReferenceMilli;

#define fastGetBoard(_passedBoard,_x,_y) (_passedBoard->board[_x][_y])

// set you pass is copied over, but piece array inside stays the same malloc
void addSetToBoard(struct puyoBoard* _passedBoard, struct pieceSet* _addThis){
	++(_passedBoard->numActiveSets);
	_passedBoard->activeSets = realloc(_passedBoard->activeSets,sizeof(struct pieceSet)*_passedBoard->numActiveSets);
	memcpy(&(_passedBoard->activeSets[_passedBoard->numActiveSets-1]),_addThis,sizeof(struct pieceSet));
}
void removeSetFromBoard(struct puyoBoard* _passedBoard, int _removeIndex){
	if (_passedBoard->numActiveSets==0){
		printf("Tried to remove set when have 0 sets.\n");
		return;
	}
	free(_passedBoard->activeSets[_removeIndex].pieces);
	struct pieceSet* _newArray = malloc(sizeof(struct pieceSet)*(_passedBoard->numActiveSets-1));
	memcpy(_newArray,_passedBoard->activeSets,sizeof(struct pieceSet)*_removeIndex);
	if (_removeIndex!=_passedBoard->numActiveSets-1){
		memcpy(_newArray,&(_passedBoard->activeSets[_removeIndex+1]),sizeof(struct pieceSet)*(_passedBoard->numActiveSets-_removeIndex-1));
	}
	--_passedBoard->numActiveSets;
	free(_passedBoard->activeSets);
	_passedBoard->activeSets=_newArray;
}
// Will update the puyo's displayX and displayY for the axis it isn't moving on.
void snapPuyoDisplayPossible(struct movingPiece* _passedPiece){
	if ((_passedPiece->movingFlag & FLAG_ANY_HMOVE)==0){
		_passedPiece->displayX = _passedPiece->tileX*TILEW;
	}
	if (!(_passedPiece->movingFlag & FLAG_MOVEDOWN)){
		_passedPiece->displayY = _passedPiece->tileY*TILEH;
	}
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
struct pieceSet getRandomPieceSet(){
	struct pieceSet _ret;
	_ret.count=2;
	_ret.isSquare=0;
	_ret.quickLock=0;
	_ret.singleTileVSpeed=FALLTIME;
	_ret.singleTileHSpeed=HMOVETIME;
	_ret.pieces = malloc(sizeof(struct movingPiece)*_ret.count);

	_ret.pieces[1].tileX=0;
	_ret.pieces[1].tileY=0;
	_ret.pieces[1].displayX=0;
	_ret.pieces[1].displayY=TILEH;
	_ret.pieces[1].movingFlag=0;
	_ret.pieces[1].color=rand()%4+COLOR_IMPOSSIBLE+1;
	_ret.pieces[1].holdingDown=1;

	_ret.pieces[0].tileX=0;
	_ret.pieces[0].tileY=1;
	_ret.pieces[0].displayX=0;
	_ret.pieces[0].displayY=0;
	_ret.pieces[0].movingFlag=0;
	_ret.pieces[0].color=rand()%4+COLOR_IMPOSSIBLE+1;
	_ret.pieces[0].holdingDown=0;

	_ret.rotateAround = &(_ret.pieces[0]);

	snapPuyoDisplayPossible(_ret.pieces);
	snapPuyoDisplayPossible(&(_ret.pieces[1]));
	return _ret;
}
void freePieceSet(struct pieceSet* _freeThis){
	free(_freeThis->pieces);
	free(_freeThis);
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
int** newJaggedArrayInt(int _w, int _h){
	int** _retArray = malloc(sizeof(int*)*_w);
	int i;
	for (i=0;i<_w;++i){
		_retArray[i] = malloc(sizeof(int)*_h);
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
void resetBoard(struct puyoBoard* _passedBoard){
	_passedBoard->status = STATUS_NORMAL;
	_passedBoard->garbageQueue=0;
	int i;
	for (i=0;i<_passedBoard->w;++i){
		memset(_passedBoard->board[i],0,_passedBoard->h*sizeof(puyoColor));
		memset(_passedBoard->pieceStatus[i],0,_passedBoard->h*sizeof(int));
	}
}
struct puyoBoard newBoard(int _w, int _h){
	struct puyoBoard _retBoard;
	_retBoard.w = _w;
	_retBoard.h = _h;
	_retBoard.board = newJaggedArrayColor(_w,_h);
	_retBoard.pieceStatus = newJaggedArrayInt(_w,_h);
	_retBoard.numActiveSets=0;
	_retBoard.activeSets=NULL;
	_retBoard.status=STATUS_NORMAL;
	resetBoard(&_retBoard);
	_retBoard.numNextPieces=3;
	_retBoard.nextPieces = malloc(sizeof(struct pieceSet)*_retBoard.numNextPieces);
	_retBoard.nextPieces[0] = getRandomPieceSet();
	_retBoard.nextPieces[1] = getRandomPieceSet();
	_retBoard.nextPieces[2] = getRandomPieceSet();
	return _retBoard;
}
void XOutFunction(){
	exit(0);
}
void init(){
	srand(time(NULL));
	generalGoodInit();
	_globalReferenceMilli = getMilli();
	initGraphics(640,480,&screenWidth,&screenHeight);
	initImages();
	setWindowTitle("Test happy");
	setClearColor(255,255,255);
}
void drawPuyo(int _color, int _drawX, int _drawY){
	int r=0;
	int g=0;
	int b=0;
	switch (_color){
		case 2:
			r=255;
			break;
		case 3:
			g=255;
			break;
		case 4:
			b=255;
			break;
		case 5:
			r=255;
			g=255;
			break;
	}
	drawRectangle(_drawX,_drawY,TILEH,TILEH,r,g,b,255);
}
void drawPieceset(struct pieceSet* _myPieces){
	int i;
	for (i=0;i<_myPieces->count;++i){
		drawPuyo(_myPieces->pieces[i].color,_myPieces->pieces[i].displayX,_myPieces->pieces[i].displayY);
	}
}
void drawBoard(int _startX, int _startY, struct puyoBoard* _drawThis){
	drawRectangle(0,0,TILEH*_drawThis->w,_drawThis->h*TILEH,255,180,0,255);
	int i;
	for (i=0;i<_drawThis->w;++i){
		int j;
		for (j=0;j<_drawThis->h;++j){
			if (_drawThis->board[i][j]!=0){
				drawPuyo(_drawThis->board[i][j],TILEH*i+_startX,TILEH*j+_startY);
			}
		}
	}

	for (i=0;i<_drawThis->numActiveSets;++i){
		drawPieceset(&(_drawThis->activeSets[i]));
	}
}
int getBoard(struct puyoBoard* _passedBoard, int _x, int _y){
	if (_x<0 || _y<0 || _x>=_passedBoard->w || _y>=_passedBoard->h){
		return COLOR_IMPOSSIBLE;
	}
	return _passedBoard->board[_x][_y];
}
// updates piece display depending on flags
void updatePieceDisplay(struct movingPiece* _passedPiece, u64 _sTime){
	// Move down
	if (_passedPiece->movingFlag & FLAG_MOVEDOWN){ // If we're moving down, update displayY until we're done, then snap and unset
		if (_sTime>=_passedPiece->completeFallTime){ // We're done
			UNSET_FLAG(_passedPiece->movingFlag,FLAG_MOVEDOWN);
			snapPuyoDisplayPossible(_passedPiece);
		}else{
			_passedPiece->displayY = _passedPiece->tileY*TILEH-partMoveEmptys(_sTime,_passedPiece->completeFallTime,_passedPiece->diffFallTime,_passedPiece->transitionDeltaY);
		}
	}
	if (_passedPiece->movingFlag & FLAG_ANY_HMOVE){
		if (_sTime>=_passedPiece->completeHMoveTime){ // We're done
			UNSET_FLAG(_passedPiece->movingFlag,FLAG_ANY_HMOVE);
			snapPuyoDisplayPossible(_passedPiece);
		}else{
			//signed char _shiftSign = (_passedPiece->movingFlag & FLAG_MOVERIGHT) ? -1 : 1;
			_passedPiece->displayX = _passedPiece->tileX*TILEW-partMoveEmptys(_sTime,_passedPiece->completeHMoveTime,_passedPiece->diffHMoveTime,_passedPiece->transitionDeltaX);
		}
	}
}
void _lowStartPuyoFall(struct movingPiece* _passedPiece, int _destTileY, int _singleFallTime, u64 _sTime){
	_passedPiece->movingFlag|=FLAG_MOVEDOWN;
	int _tileDiff = _destTileY-_passedPiece->tileY;
	_passedPiece->tileY+=_tileDiff;
	_passedPiece->transitionDeltaY = _tileDiff*TILEH;
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
void placePuyo(struct puyoBoard* _passedBoard, int _x, int _y, puyoColor _passedColor){
	_passedBoard->board[_x][_y]=_passedColor;
	_passedBoard->pieceStatus[_x][_y]=0;
}
char puyoCanFell(struct puyoBoard* _passedBoard, struct movingPiece* _passedPiece){
	return (getBoard(_passedBoard,_passedPiece->tileX,_passedPiece->tileY+1)==COLOR_NONE);
}
// return 0 if no puyo will fall
// return 1 if stuff will fall
char transitionBoardFallMode(struct puyoBoard* _passedBoard, u64 _sTime){
	char _ret=0;
	int i;
	for (i=0;i<_passedBoard->w;++i){
		// Find the first empty spot in this column with a puyo above it
		int j;
		for (j=_passedBoard->h-1;j>0;--j){ // > 0 because it's useless if top column is empty
			if (fastGetBoard(_passedBoard,i,j)==COLOR_NONE && fastGetBoard(_passedBoard,i,j-1)!=COLOR_NONE){
				break;
			}
		}
		// If we found a puyo with an empty space under it 
		if (j>0){
			int _fallDestBottomY;
			for (j=_passedBoard->h-1;;--j){ // > 0 because it's useless if top column is empty
				if (fastGetBoard(_passedBoard,i,j)==COLOR_NONE){
					_fallDestBottomY=j;
					break;
				}
			}
			_ret=1;
			// Find how many puyo will be falling
			int _bottomPuyoY=j; // Y position of the first puyo that'll fall in this column
			int _totalToFall=0;
			for (j=_bottomPuyoY-1;j>=0;--j){
				if (fastGetBoard(_passedBoard,i,j)!=COLOR_NONE){
					++_totalToFall;
				}
			}
			// Put those puyo as movingPiece
			struct pieceSet _newSet;
			_newSet.pieces =  malloc(sizeof(struct movingPiece)*_totalToFall);
			_newSet.count=_totalToFall;
			_newSet.isSquare=0;
			_newSet.quickLock=1;
			_newSet.singleTileVSpeed=FALLTIME;
			int _freeIndex=0;
			for (j=_bottomPuyoY-1;j>=0;--j){
				if (fastGetBoard(_passedBoard,i,j)!=COLOR_NONE){
					struct movingPiece _newPiece;
					memset(&_newPiece,0,sizeof(struct movingPiece));
					_newPiece.tileX=i;
					_newPiece.tileY=j; // set this is required for _lowStartPuyoFall
					_newPiece.color=_passedBoard->board[i][j];
					_passedBoard->board[i][j]=COLOR_NONE;
					snapPuyoDisplayPossible(&_newPiece);
					_lowStartPuyoFall(&_newPiece,_fallDestBottomY-_freeIndex,FALLTIME/5,_sTime);
					_newSet.pieces[_freeIndex++] = _newPiece;
				}
			}
			addSetToBoard(_passedBoard,&_newSet);
		}
	}
	if (_ret){
		_passedBoard->status=STATUS_DROPPING;
	}
	return _ret;
}
/*
return values:
0 - nothing special
1 - piece locked
	- piece set should be freed
2 - piece moved to next tile
	- If we started moving down another tile
	- If we started to autolock
*/
signed char updatePieceSet(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, u64 _sTime){
	char _ret=0;
	int i;
	for (i=0;i<_passedSet->count;++i){
		updatePieceDisplay(&(_passedSet->pieces[i]),_sTime);
		//_ret|=updatePiece(_passedBoard,&(_passedSet->pieces[i]),_sTime);
	}
	// Update display for rotating pieces. Not in updatePieceDisplay because only sets can rotate
	// Square pieces from fever can't rotate yet
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
					_passedSet->pieces[i].displayX = _passedSet->rotateAround->displayX+HALFTILE + (cos(_angle)*_trigSignX)*TILEW-HALFTILE;
					_passedSet->pieces[i].displayY = _passedSet->rotateAround->displayY+HALFTILE + (sin(_angle)*_trigSignY)*TILEW-HALFTILE;
				}
			}
		}
	}
	// If the piece isn't falling
	if (!(_passedSet->pieces[0].movingFlag & FLAG_MOVEDOWN)){
		char _shouldLock=0;
		if (_passedSet->pieces[0].movingFlag & FLAG_DEATHROW){ // Autolock if we've sat with no space under for too long
			if (_sTime>=_passedSet->pieces[0].completeFallTime){
				_shouldLock=1;	
			}
		}else{
			// All pieces must be able to move if we want to do this set
			char _setCanMove=1;
			for (i=0;i<_passedSet->count;++i){
				_setCanMove&=puyoCanFell(_passedBoard,&(_passedSet->pieces[i]));
			}
			_ret=2;
			if (_setCanMove){
				for (i=0;i<_passedSet->count;++i){
					_forceStartPuyoGravity(&(_passedSet->pieces[i]),_passedSet->singleTileVSpeed,_sTime);
				}
			}else{
				if (_passedSet->quickLock){
					_shouldLock=1;
				}else{
					for (i=0;i<_passedSet->count;++i){
						_forceStartPuyoAutoplaceTime(&(_passedSet->pieces[i]),_passedSet->singleTileVSpeed,_sTime);
					}
				}
			}
			
		}
		// We may need to lock, either because we detect there's no free space with quickLock on or deathRow time has expired
		if (_shouldLock){
			// autolock all pieces
			for (i=0;i<_passedSet->count;++i){
				placePuyo(_passedBoard,_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->pieces[i].color);
			}
			_ret=1;
		}
	}
	return _ret;
}
void pieceSetControls(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, u64 _sTime, signed char _dasActive){
	if (wasJustPressed(BUTTON_RIGHT) || wasJustPressed(BUTTON_LEFT) || _dasActive!=0){
		if (!(_passedSet->pieces[0].movingFlag & FLAG_ANY_HMOVE)){
			signed char _direction = _dasActive!=0 ? _dasActive : (wasJustPressed(BUTTON_RIGHT) ? 1 : -1);
			char _canMove=1;
			int i;
			for (i=0;i<_passedSet->count;++i){
				_canMove&=(getBoard(_passedBoard,_passedSet->pieces[i].tileX+_direction,_passedSet->pieces[i].tileY)==COLOR_NONE);
			}
			if (_canMove){
				int _setFlag = (_direction==1) ? FLAG_MOVERIGHT : FLAG_MOVELEFT;
				for (i=0;i<_passedSet->count;++i){
					_passedSet->pieces[i].movingFlag|=(_setFlag);
					_passedSet->pieces[i].diffHMoveTime = _passedSet->singleTileHSpeed;
					_passedSet->pieces[i].completeHMoveTime = _sTime+_passedSet->pieces[i].diffHMoveTime;
					_passedSet->pieces[i].transitionDeltaX = TILEW*_direction;
					_passedSet->pieces[i].tileX+=_direction;
				}
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
						// Make sure all pieces in this set can obey the force shift
						int j;
						for (j=0;j<_passedSet->count;++j){
							if (getBoard(_passedBoard,_passedSet->pieces[j].tileX+_xDist,_passedSet->pieces[j].tileY+_yDist)!=COLOR_NONE){
								break;
							}
						}
						// If they can all obey the force shift, shift them all
						if (j==_passedSet->count){
							// HACK - If the other pieces rotating in this set can't rotate, these new positions set below would remain. For the piece shapes I'll have in my game, it is impossible for one piece to be able to rotate but not another.
							int _resetFlags=0;
							if (_yDist!=0){
								_resetFlags|=FLAG_MOVEDOWN;
							}
							if (_xDist!=0){
								_resetFlags|=FLAG_ANY_HMOVE;
							}
							for (j=0;j<_passedSet->count;++j){
								_passedSet->pieces[j].tileX+=_xDist;
								_passedSet->pieces[j].tileY+=_yDist;
								UNSET_FLAG(_passedSet->pieces[j].movingFlag,_resetFlags);
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
			}
			// update puyo h shift
			for (i=0;i<_passedSet->count;++i){
				snapPuyoDisplayPossible(&(_passedSet->pieces[i]));
			}
		}
	}
}
// _returnForIndex will tell it which set to return the value of
// pass -1 to get return values from all
signed char updateBoard(struct puyoBoard* _passedBoard, signed char _returnForIndex, u64 _sTime){
	if (_passedBoard->status==STATUS_DROPPING && _passedBoard->numActiveSets==0){
		_passedBoard->status=STATUS_NEXTWINDOW;
	}
	if (_passedBoard->status==STATUS_NEXTWINDOW){
		addSetToBoard(_passedBoard,&(_passedBoard->nextPieces[0]));
		memmove(&(_passedBoard->nextPieces[0]),&(_passedBoard->nextPieces[1]),sizeof(struct pieceSet)*(_passedBoard->numNextPieces-1));
		_passedBoard->nextPieces[_passedBoard->numNextPieces-1] = getRandomPieceSet();
		_passedBoard->status=STATUS_NORMAL;
	}
	signed char _ret=0;
	char _shouldTransition=0;
	int i;
	for (i=0;i<_passedBoard->numActiveSets;++i){
		signed char _got = updatePieceSet(_passedBoard,&(_passedBoard->activeSets[i]),_sTime);
		if (_returnForIndex==-1 || i==_returnForIndex){
			_ret|=_got;
		}
		if (_got==1){
			if (i==0 && _passedBoard->status==STATUS_NORMAL){
				_shouldTransition=1;
			}
			removeSetFromBoard(_passedBoard,i);
			--i;
		}
	}
	if (_shouldTransition){
		if (!transitionBoardFallMode(_passedBoard,_sTime)){
			_passedBoard->status=STATUS_NEXTWINDOW;
		}
	}
	return _ret;
}
int main(int argc, char const** argv){
	init();
	struct puyoBoard _testBoard = newBoard(6,10); // 6,12

	int _dasChargeEnd=INT32_MAX; // initialize just to avoid valgrind warnings
	signed char _dasDirection=0;
	u64 _startHoldTime;

	_testBoard.activeSets = malloc(sizeof(struct pieceSet));
	_testBoard.activeSets[0] = getRandomPieceSet();
	_testBoard.numActiveSets=1;

	#if FPSCOUNT == 1
		u64 _frameCountTime = getMilli();
		int _frames=0;
	#endif
	while(1){
		u64 _sTime = goodGetMilli();
		controlsStart();

		if (wasJustReleased(BUTTON_LEFT) || wasJustReleased(BUTTON_RIGHT)){
			_dasDirection=0; // Reset DAS
		}
		if (_testBoard.status==STATUS_NORMAL){
			if (wasJustPressed(BUTTON_LEFT) || wasJustPressed(BUTTON_RIGHT)){
				_dasDirection=0; // Reset DAS
				_dasDirection = wasJustPressed(BUTTON_RIGHT) ? 1 : -1;
				_dasChargeEnd = _sTime+DASTIME;
			}
			if (isDown(BUTTON_LEFT) || isDown(BUTTON_RIGHT)){
				if (_dasDirection==0){
					_dasDirection = isDown(BUTTON_RIGHT) ? 1 : -1;
					_dasChargeEnd = _sTime+DASTIME;
				}
			}
	
			if (wasJustPressed(BUTTON_DOWN)){
				_startHoldTime=_sTime;
			}else if (isDown(BUTTON_DOWN)){
				if (_testBoard.activeSets[0].pieces[0].movingFlag & FLAG_MOVEDOWN){ // Normal push down
					int j;
					for (j=0;j<_testBoard.activeSets[0].count;++j){
						_testBoard.activeSets[0].pieces[j].completeFallTime = _testBoard.activeSets[0].pieces[j].referenceFallTime - (_sTime-_startHoldTime)*13;
					}
				}else if (_testBoard.activeSets[0].pieces[0].movingFlag & FLAG_DEATHROW){ // lock
					int j;
					for (j=0;j<_testBoard.activeSets[0].count;++j){
						_testBoard.activeSets[0].pieces[j].completeFallTime = 0;
					}
				}
			}else if (wasJustReleased(BUTTON_DOWN)){
				// Allows us to push down, wait, and push down again in one tile. Not like that's possible with how fast it goes though
				int j;
				for (j=0;j<_testBoard.activeSets[0].count;++j){
					_testBoard.activeSets[0].pieces[j].referenceFallTime = _testBoard.activeSets[0].pieces[j].completeFallTime;
				}
			}
			pieceSetControls(&_testBoard,&(_testBoard.activeSets[0]),_sTime,_sTime>=_dasChargeEnd ? _dasDirection : 0);
		}
		char _updateRet = updateBoard(&_testBoard,_testBoard.status==STATUS_NORMAL ? 0 : -1,_sTime);
		if (_updateRet!=0){
			if (_testBoard.status==STATUS_NORMAL){
				if (_updateRet==2){
					_startHoldTime=_sTime;
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
		controlsEnd();
		startDrawing();
		drawBoard(0,0,&_testBoard);
		endDrawing();

		#if FPSCOUNT
			++_frames;
			if (getMilli()>=_frameCountTime+1000){
				_frameCountTime=getMilli();
				#if SHOWFPSCOUNT
					printf("%d\n",_frames);
				#else
					if (_frames<60){
						printf("Slowdown %d\n\n",_frames);
					}
				#endif
				_frames=0;
			}
		#endif
	}
	generalGoodQuit();
	return 0;
}