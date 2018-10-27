#include <stdlib.h>
#include <stdio.h>

#include <goodbrew/config.h>
#include <goodbrew/base.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include <goodbrew/images.h>

typedef enum{
	STATUS_NORMAL,
	STATUS_POPPING,
	STAUTS_DROPPING,
	STATUS_NEXTWINDOW,
	STATUS_TRASHFALL
}boardStatus;

typedef struct{
	int w;
	int h;
	int** board; // ints represent colors
	int** pieceStatus; // Interpret this value depending on status
	boardStatus status;
	int trashQueue;
}puyoBoard;


int screenWidth;
int screenHeight;

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

void resetBoard(puyoBoard* _passedBoard){
	_passedBoard->status = STATUS_NORMAL;
	_passedBoard->trashQueue=0;

	zeroJaggedArray(_passedBoard->board,_passedBoard->w,_passedBoard->h);
	zeroJaggedArray(_passedBoard->pieceStatus,_passedBoard->w,_passedBoard->h);
}

puyoBoard newBoard(int _w, int _h){
	puyoBoard _retBoard;
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
	initGraphics(640,480,&screenWidth,&screenHeight);
	initImages();
	setWindowTitle("Test happy");
}

int main(int argc, char const** argv){
	init();
	puyoBoard _testBoard = newBoard(6,12);

	while(1){
		controlsStart();
		controlsEnd();
		startDrawing();
		endDrawing();

		// temp
		wait(1);
	}

	generalGoodQuit();
	return 0;
}

// use the bitmap autotiling