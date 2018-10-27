#include <stdlib.h>
#include <stdion.h>

typedef struct{
	int w;
	int h;
	int** board; // ints represent colors
	int** pieceStatus; // If pieces are popping
}puyoBoard;

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
			_zeroThis[_w][_h]=0;
		}
	}
}

puyoBoard newBoard(int _w, int _h){
	puyoBoard _retBoard;
	_retBoard.w = _w;
	_retBoard.h = _h;
	_retBoard.board = newJaggedArray(_w,_h);
	_retBoard.pieceStatus = newJaggedArray(_w,_h);

	zeroJaggedArray(_retBoard.board,_w,_h);
	zeroJaggedArray(_retBoard.pieceStatus,_w,_h);

	return _retBoard;
}