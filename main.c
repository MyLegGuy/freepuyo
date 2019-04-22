// The y position is doubled. This is to allow half.
// How moving down works:

// If Puyo have the falling flag already on:
//		update displayY
//	else
//	   Is the puyo halfway down?
//		   is the space under free?
//			  Start moving the second half
//		   else
//			  autoplace
//	   elseif the puyo is fully in one unit
//			Start moving down the second hafl

// For each controlled puyo each frame:
//	  If puyo isn't already falling:
//	  
//    If the space under the puyo is

// Note - For rotation, we can calculate the current position using the finish time. This would overwrite displayX and displayY completely instead of beign relative to them


#include <stdlib.h>
#include <stdio.h>

#include <goodbrew/config.h>
#include <goodbrew/base.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include <goodbrew/images.h>

#define FLAG_MOVEDOWN 	0b1
#define FLAG_MOVELEFT 	0b01
#define FLAG_MOVERIGHT 	0b001
#define FLAG_ROTATER 	0b0001
#define FLAG_ROTATEL 	0b00001
//
#define FLAG_ANY_HMOVE	0b011
#define FLAG_ANY_ROTATE 0b00011


#define TILEH 45
#define HALFTILEH (45/(double)2)

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
	int** board; // ints represent colors
	int** pieceStatus; // Interpret this value depending on status
	boardStatus status;
	int garbageQueue;
};

struct movingPiece{
	int tileX;
	int halfTileY;
	int displayY;
	int displayX;
	unsigned char movingFlag; // sideways, down, rotate
	u64 completeFallTime; // Time when the current falling down will complete
	u64 completeRotateTime;
	u64 completeMoveTime;
	struct movingPiece* rotateRef; // Rotate around this
};

struct pieceSet{

};
// getPieceSet();

int screenWidth;
int screenHeight;

u64 _globalReferenceMilli;

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

int** newJaggedArray(int _w, int _h){
	int** _retArray = malloc(sizeof(int*)*_w);
	int i;
	for (i=0;i<_w;++i){
		_retArray[i] = malloc(sizeof(int)*_h);
	}
	return _retArray;
}
void zeroJaggedArray(int** _zeroThis, int _w, int _h){
	int i, j;
	for (i=0;i<_w;++i){
		for (j=0;j<_h;++j){
			_zeroThis[i][j]=0;
		}
	}
}

void resetBoard(struct puyoBoard* _passedBoard){
	_passedBoard->status = STATUS_NORMAL;
	_passedBoard->garbageQueue=0;

	zeroJaggedArray(_passedBoard->board,_passedBoard->w,_passedBoard->h);
	zeroJaggedArray(_passedBoard->pieceStatus,_passedBoard->w,_passedBoard->h);
}

struct puyoBoard newBoard(int _w, int _h){
	struct puyoBoard _retBoard;
	_retBoard.w = _w;
	_retBoard.h = _h;
	_retBoard.board = newJaggedArray(_w,_h);
	_retBoard.pieceStatus = newJaggedArray(_w,_h);
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

#define HALFFALLTIME 500

int main(int argc, char const** argv){
	init();
	struct puyoBoard _testBoard = newBoard(6,12);

	int numPieces=2;
	struct movingPiece* _curPieces[2];
	struct movingPiece _pieceOne = {0};
	struct movingPiece _pieceTwo = {0};
	_curPieces[0] = &_pieceOne;
	_curPieces[1] = &_pieceTwo;

	_pieceTwo.tileX=0;
	_pieceTwo.halfTileY=2;
	_pieceTwo.displayX=0;
	_pieceTwo.displayY=TILEH;
	_pieceTwo.movingFlag=0;
	_pieceTwo.rotateRef = &_pieceOne;

	_pieceOne.tileX=0;
	_pieceOne.halfTileY=0;
	_pieceOne.displayX=0;
	_pieceOne.displayY=0;
	_pieceOne.movingFlag=0;
	_pieceOne.rotateRef = &_pieceOne;

	/*
	typedef struct{
	int tileX;
	int halfTileY;
	int displayY;
	int displayX;
	unsigned char movingFlag; // sideways, down, rotate
	u64 completeFallTime; // Time when the current falling down will complete
	u64 completeRotateTime;
	u64 completeMoveTime;
	movingPiece* rotateRef; // Rotate around this
	}movingPiece;
	*/



	while(1){
		int i;
		u64 _sTime = goodGetMilli();
		controlsStart();

		for (i=0;i<numPieces;++i){
			if (_curPieces[i]->movingFlag & FLAG_MOVEDOWN){
				if (_sTime>=_curPieces[i]->completeFallTime){ // We're done
					printf("Done\n");
					_curPieces[i]->displayY=(_curPieces[i]->halfTileY)*HALFTILEH;
					_curPieces[i]->movingFlag^=FLAG_MOVEDOWN;
				}else{
					_curPieces[i]->displayY = (_curPieces[i]->halfTileY-1)*HALFTILEH+partMove(_sTime,_curPieces[i]->completeFallTime,HALFFALLTIME,HALFTILEH);
				}
			}else{
				if ((_curPieces[i]->displayY & 1) && !(1 /* space under free */)){ // If we're halfway down and the space under isn't free
					// autoplace
				}else{ // If we're in a whole tile then start going down more.
					printf("ok\n");
					_curPieces[i]->movingFlag|=FLAG_MOVEDOWN;
					_curPieces[i]->completeFallTime = _sTime+HALFFALLTIME;
					_curPieces[i]->halfTileY+=1;
				}
			}
		}


		controlsEnd();
		startDrawing();
		for (i=0;i<numPieces;++i){
			drawRectangle(_curPieces[i]->displayX,_curPieces[i]->displayY,TILEH,TILEH,i*255,0,0,255);
		}
		endDrawing();
	}

	generalGoodQuit();
	return 0;
}

// use the bitmap autotiling
