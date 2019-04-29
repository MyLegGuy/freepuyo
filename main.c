// Note - For rotation, we can calculate the current position using the finish time. This would overwrite displayX and displayY completely instead of beign relative to them

#define __USE_MISC // enable MATH_PI_2
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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
int ROTATETIME = 100;

typedef int puyoColor;

#define COLOR_NONE 0
#define COLOR_IMPOSSIBLE 1

#define FPSCOUNT 1

typedef enum{
	STATUS_NORMAL, // We're moving the puyo around and stuff
	STATUS_POPPING, // We're waiting for puyo to pop
	STAUTS_DROPPING, // puyo are falling into place. This is the status after you place a piece and after STATUS_POPPING
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
};

struct movingPiece{
	int tileX;
	int tileY;
	int displayY;
	int displayX;
	unsigned char movingFlag; // sideways, down, rotate
	u64 completeFallTime; // Time when the current falling down will complete
	u64 referenceFallTime; // Copy of completeFallTime, unadjusted for the current down hold
	u64 completeRotateTime;
	u64 completeHMoveTime;
	puyoColor color;
	char holdingDown;
	u64 holdDownStart;
};

struct pieceSet{
	signed char isSquare; // If this is a square set of puyos then this will be the width of that square. 0 otherwise
	struct movingPiece* rotateAround; // If this isn't a square, rotate around this puyo
	int count;
	struct movingPiece* pieces;
};
// getPieceSet();

int screenWidth;
int screenHeight;

u64 _globalReferenceMilli;

// Will update the puyo's displayX and displayY for the axis it isn't moving on.
void lockPuyoDisplayPossible(struct movingPiece* _passedPiece){
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
struct pieceSet* getPieceSet(){
	struct pieceSet* _ret = malloc(sizeof(struct pieceSet));
	_ret->count=2;
	_ret->isSquare=0;
	_ret->pieces = malloc(sizeof(struct movingPiece)*2);

	_ret->pieces[1].tileX=0;
	_ret->pieces[1].tileY=0;
	_ret->pieces[1].displayX=0;
	_ret->pieces[1].displayY=TILEH;
	_ret->pieces[1].movingFlag=0;
	_ret->pieces[1].color=2;
	_ret->pieces[1].holdingDown=1;

	_ret->pieces[0].tileX=0;
	_ret->pieces[0].tileY=1;
	_ret->pieces[0].displayX=0;
	_ret->pieces[0].displayY=0;
	_ret->pieces[0].movingFlag=0;
	_ret->pieces[0].color=1;
	_ret->pieces[0].holdingDown=0;

	_ret->rotateAround = &(_ret->pieces[0]);

	lockPuyoDisplayPossible(_ret->pieces);
	lockPuyoDisplayPossible(&(_ret->pieces[1]));
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

double partMove(u64 _curTicks, u64 _destTicks, int _totalDifference, double _max){
	return ((_totalDifference-(_destTicks-_curTicks))/(double)_totalDifference)*_max;
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
	resetBoard(&_retBoard);
	return _retBoard;
}

void XOutFunction(){
	exit(0);
}

void init(){
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
		case 1:
			r=255;
			break;
		case 2:
			g=255;
			break;
		case 3:
			b=255;
			break;
		case 4:
			r=255;
			g=255;
			break;
	}
	drawRectangle(_drawX,_drawY,TILEH,TILEH,r,g,b,255);
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
}

void drawPieceset(struct pieceSet* _myPieces){
	int i;
	for (i=0;i<_myPieces->count;++i){
		drawPuyo(_myPieces->pieces[i].color,_myPieces->pieces[i].displayX,_myPieces->pieces[i].displayY);
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
			_passedPiece->displayY=_passedPiece->tileY*TILEH;
			UNSET_FLAG(_passedPiece->movingFlag,FLAG_MOVEDOWN);
		}else{
			_passedPiece->displayY = (_passedPiece->tileY-1)*TILEH+partMove(_sTime,_passedPiece->completeFallTime,FALLTIME,TILEH);
		}
	}
	if (_passedPiece->movingFlag & FLAG_ANY_HMOVE){
		if (_sTime>=_passedPiece->completeHMoveTime){ // We're done
			_passedPiece->displayX=_passedPiece->tileX*TILEW;
			UNSET_FLAG(_passedPiece->movingFlag,FLAG_ANY_HMOVE);
		}else{
			signed char _shiftSign = (_passedPiece->movingFlag & FLAG_MOVERIGHT) ? -1 : 1;
			_passedPiece->displayX = (_passedPiece->tileX+_shiftSign)*TILEW+partMove(_sTime,_passedPiece->completeHMoveTime,HMOVETIME,TILEH*_shiftSign*(-1));
		}
	}
}
void _forceStartPuyoGravity(struct movingPiece* _passedPiece, u64 _sTime){
	_passedPiece->movingFlag|=FLAG_MOVEDOWN;
	_passedPiece->referenceFallTime = _sTime+FALLTIME;
	_passedPiece->completeFallTime = _passedPiece->referenceFallTime;
	_passedPiece->tileY+=1;
}
void _forceStartPuyoAutoplaceTime(struct movingPiece* _passedPiece, u64 _sTime){
	_passedPiece->movingFlag|=FLAG_DEATHROW;
	_passedPiece->completeFallTime = _sTime+FALLTIME/2;
}
char puyoCanFell(struct puyoBoard* _passedBoard, struct movingPiece* _passedPiece){
	return (getBoard(_passedBoard,_passedPiece->tileX,_passedPiece->tileY+1)==COLOR_NONE);
}
/*
return values:
0 - nothing special
1 - piece locked
2 - piece moved to next tile
*/
char updatePieceSet(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, u64 _sTime){
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
					lockPuyoDisplayPossible(&(_passedSet->pieces[i]));
				}else{
					char _isClockwise = (_passedSet->pieces[i].movingFlag & FLAG_ROTATECW);
					signed char _dirRelation = getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY);
					/*int _preRotateX;
					int _preRotateY;
					getPreRotatePos(_isClockwise, _dirRelation, &_preRotateX, &_preRotateX);
					_preRotateX+=_passedSet->pieces[i].tileX;
					_preRotateY+=_passedSet->pieces[i].tileY;*/
					int _trigSignX;
					int _trigSignY;
					getRotateTrigSign(_isClockwise, _dirRelation, &_trigSignX, &_trigSignY);
					double _angle = partMove(_sTime,_passedSet->pieces[i].completeRotateTime,ROTATETIME,M_PI_2);
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
		if (_passedSet->pieces[0].movingFlag & FLAG_DEATHROW){
			if (_sTime>=_passedSet->pieces[0].completeFallTime){
				// autolock all pieces
				for (i=0;i<_passedSet->count;++i){
					_passedBoard->board[_passedSet->pieces[i].tileX][_passedSet->pieces[i].tileY]=_passedSet->pieces[i].color;
					_passedBoard->pieceStatus[_passedSet->pieces[i].tileX][_passedSet->pieces[i].tileY]=0;
				}
				_ret=1;
			}
		}else{
			// All pieces must be able to move if we want to do this set
			char _setCanMove=1;
			for (i=0;i<_passedSet->count;++i){
				_setCanMove&=puyoCanFell(_passedBoard,&(_passedSet->pieces[i]));
			}
			if (_setCanMove){
				for (i=0;i<_passedSet->count;++i){
					_forceStartPuyoGravity(&(_passedSet->pieces[i]),_sTime);
				}
			}else{
				for (i=0;i<_passedSet->count;++i){
					_forceStartPuyoAutoplaceTime(&(_passedSet->pieces[i]),_sTime);
				}
			}
			_ret=2;
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
					_passedSet->pieces[i].completeHMoveTime = _sTime+HMOVETIME;
					_passedSet->pieces[i].tileX+=_direction;
				}
			}
		}
	}
	if (wasJustPressed(BUTTON_A) || wasJustPressed(BUTTON_B)){
		if (_passedSet->isSquare==0){
			int i;
			for (i=0;i<_passedSet->count;++i){
				if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE)==0 && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
					char _isClockwise = wasJustPressed(BUTTON_A);
					_passedSet->pieces[i].movingFlag|=(_isClockwise ? FLAG_ROTATECW : FLAG_ROTATECC);
					signed char _dirRelation = getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY);
					int _destX;
					int _destY;
					getPostRotatePos(_isClockwise,_dirRelation,&_destX,&_destY);
					_passedSet->pieces[i].tileX = _passedSet->pieces[i].tileX+_destX;
					_passedSet->pieces[i].tileY = _passedSet->pieces[i].tileY+_destY;
					_passedSet->pieces[i].completeRotateTime = _sTime+ROTATETIME;
				}
			}
		}
	}
}

int main(int argc, char const** argv){
	init();
	struct puyoBoard _testBoard = newBoard(6,10); // 6,12
	struct pieceSet* _myPieces = getPieceSet();

	int _dasChargeEnd;
	signed char _dasDirection=0;
	u64 _startHoldTime;

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
			if (_myPieces->pieces[0].movingFlag & FLAG_MOVEDOWN){ // Normal push down
				int j;
				for (j=0;j<_myPieces->count;++j){
					_myPieces->pieces[j].completeFallTime = _myPieces->pieces[j].referenceFallTime - (_sTime-_startHoldTime)*13;
				}
			}else if (_myPieces->pieces[0].movingFlag & FLAG_DEATHROW){ // lock
				int j;
				for (j=0;j<_myPieces->count;++j){
					_myPieces->pieces[j].completeFallTime = 0;
				}
			}
		}else if (wasJustReleased(BUTTON_DOWN)){
			// Allows us to push down, wait, and push down again in one tile. Not like that's possible with how fast it goes though
			int j;
			for (j=0;j<_myPieces->count;++j){
				_myPieces->pieces[j].referenceFallTime = _myPieces->pieces[j].completeFallTime;
			}
		}

		pieceSetControls(&_testBoard,_myPieces,_sTime, _sTime>=_dasChargeEnd ? _dasDirection : 0);
		char _updateRet = updatePieceSet(&_testBoard,_myPieces,_sTime);
		if (_updateRet!=0){
			if (_updateRet==1){
				free(_myPieces->pieces);
				free(_myPieces);
				_myPieces = getPieceSet();
				if (isDown(BUTTON_DOWN)){
					_startHoldTime = _sTime;
				}
			}else if (_updateRet==2){
				_startHoldTime=_sTime;
			}else{
				printf("Unknown _updateRet %d\n",_updateRet);
			}
		}
		controlsEnd();
		startDrawing();
		drawBoard(0,0,&_testBoard);
		drawPieceset(_myPieces);
		endDrawing();

		#if FPSCOUNT
			++_frames;
			if (getMilli()>=_frameCountTime+1000){
				_frameCountTime=getMilli();
				printf("%d\n",_frames);
				_frames=0;
			}
		#endif
	}
	generalGoodQuit();
	return 0;
}

// use the bitmap autotiling
