// For each controlled puyo each frame:
//	  If puyo isn't already falling:
//	  
//    If the space under the puyo is

// Note - For rotation, we can calculate the current position using the finish time. This would overwrite displayX and displayY completely instead of beign relative to them


// TODO -  I may need to store HMOVETIME in the piece set because it changes with DAS

#include <stdlib.h>
#include <stdio.h>

#include <goodbrew/config.h>
#include <goodbrew/base.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include <goodbrew/images.h>

// For these movement flags, it's already been calculated that moving that way is okay
#define FLAG_MOVEDOWN 	0b00000001
#define FLAG_MOVELEFT 	0b00000010
#define FLAG_MOVERIGHT 	0b00000100
#define FLAG_ROTATER 	0b00001000
#define FLAG_ROTATEL 	0b00010000
#define FLAG_DEATHROW	0b00100000 // Death row for being a moving puyo, I mean. If this puyo has hit the puyo under it and is about to die if it's not moved
//
#define FLAG_ANY_HMOVE 	(FLAG_MOVELEFT | FLAG_MOVERIGHT)
#define FLAG_ANY_ROTATE (FLAG_ROTATER | FLAG_ROTATEL)

#define UNSET_FLAG(_holder, _mini) _holder&=(0xFFFF ^ _mini)

#define TILEH 45
#define TILEW TILEH

// How long it takes a puyo to fall half of one tile on the y axis
int FALLTIME = 900;
int HMOVETIME = 30;

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
	u64 completeRotateTime;
	u64 completeHMoveTime;
	struct movingPiece* rotateRef; // Rotate around this
	puyoColor color;
};

struct pieceSet{
	int count;
	struct movingPiece* pieces;
};
// getPieceSet();

int screenWidth;
int screenHeight;

u64 _globalReferenceMilli;

struct pieceSet* getPieceSet(){
	struct pieceSet* _ret = malloc(sizeof(struct pieceSet));
	_ret->count=2;
	_ret->pieces = malloc(sizeof(struct movingPiece)*2);

	_ret->pieces[1].tileX=0;
	_ret->pieces[1].tileY=1;
	_ret->pieces[1].displayX=0;
	_ret->pieces[1].displayY=TILEH;
	_ret->pieces[1].movingFlag=0;
	_ret->pieces[1].rotateRef = _ret->pieces;
	_ret->pieces[1].color=2;

	_ret->pieces[0].tileX=0;
	_ret->pieces[0].tileY=0;
	_ret->pieces[0].displayX=0;
	_ret->pieces[0].displayY=0;
	_ret->pieces[0].movingFlag=0;
	_ret->pieces[0].color=1;

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

int partMove(u64 _curTicks, u64 _destTicks, int _totalDifference, int _max){
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
	_passedPiece->completeFallTime = _sTime+FALLTIME;
	_passedPiece->tileY+=1;
}
void _forceStartPuyoAutoplaceTime(struct movingPiece* _passedPiece, u64 _sTime){
	_passedPiece->movingFlag|=FLAG_DEATHROW;
	_passedPiece->completeFallTime = _sTime+FALLTIME/2;
}
char puyoCanFell(struct puyoBoard* _passedBoard, struct movingPiece* _passedPiece){
	return (getBoard(_passedBoard,_passedPiece->tileX,_passedPiece->tileY+1)==COLOR_NONE);
}
//void startPuyoGravity(struct puyoBoard* _passedBoard, struct movingPiece* _passedPiece, u64 _sTime){
//	if (puyoCanFell(_passedBoard,_passedPiece)){ // if the spot under isn't free, autoplace
//		_forceStartPuyoAutoplaceTime(_passedPiece,_sTime);
//	}else{ // Fall down as normal
//		_forceStartPuyoGravity(_passedPiece,_sTime);
//	}
//}

/*
	// Move down
	if (_passedPiece->movingFlag & FLAG_MOVEDOWN){ // If we're moving down, update displayY until we're done, then snap and unset
		if (_sTime>=_passedPiece->completeFallTime){ // We're done
			_passedPiece->displayY=_passedPiece->tileY*TILEH;
			_passedPiece->movingFlag^=FLAG_MOVEDOWN;
		}else{
			_passedPiece->displayY = (_passedPiece->tileY-1)*TILEH+partMove(_sTime,_passedPiece->completeFallTime,FALLTIME,TILEH);
		}
	}else if (_passedPiece->movingFlag ^ FLAG_DEATHROW){ // If we're not waiting on death, meaning this puyo is normal and should fall
		if (getBoard(_passedBoard,_passedPiece->tileX,_passedPiece->tileY+1)!=COLOR_NONE){ // if the spot under isn't free, autoplace
			_passedPiece->movingFlag|=FLAG_DEATHROW;
		}else{ // Fall down as normal
			_passedPiece->movingFlag|=FLAG_MOVEDOWN;
			_passedPiece->completeFallTime = _sTime+FALLTIME;
			_passedPiece->tileY+=1;
		}
	}else{ // We're waiting to die
		if (_sTime>=_passedPiece->completeFallTime){
			// autolock
			return 1;
		}
	}
*/
char updatePieceSet(struct puyoBoard* _passedBoard, struct pieceSet* _passedSet, u64 _sTime){
	char _ret=0;
	int i;
	for (i=0;i<_passedSet->count;++i){
		updatePieceDisplay(&(_passedSet->pieces[i]),_sTime);
		//_ret|=updatePiece(_passedBoard,&(_passedSet->pieces[i]),_sTime);
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
}

int main(int argc, char const** argv){
	init();
	struct puyoBoard _testBoard = newBoard(6,10); // 6,12
	struct pieceSet* _myPieces = getPieceSet();

	int _dasChargeEnd;
	signed char _dasDirection=0;

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
			_dasChargeEnd = _sTime+100;
		}
		if (isDown(BUTTON_LEFT) || isDown(BUTTON_RIGHT)){
			if (_dasDirection==0){
				_dasDirection = isDown(BUTTON_RIGHT) ? 1 : -1;
				_dasChargeEnd = _sTime+100;
			}
		}

		pieceSetControls(&_testBoard,_myPieces,_sTime, _sTime>=_dasChargeEnd ? _dasDirection : 0);
		if (updatePieceSet(&_testBoard,_myPieces,_sTime)){
			free(_myPieces->pieces);
			free(_myPieces);
			_myPieces = getPieceSet();
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
