#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "yoshi.h"
#include "puzzleGeneric.h"

#define YOSHINEXTNUM 3

void yoshiUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, u64 _sTime){
}
void drawYoshiBoard(struct yoshiBoard* _passedBoard, int _drawX, int _drawY){
}
struct yoshiBoard* newYoshi(int _w, int _h){
	struct yoshiBoard* _ret = malloc(sizeof(struct yoshiBoard));
	_ret->lowBoard = newGenericBoard(_w,_h);

	_ret.nextPieces = malloc(sizeof(pieceColor*)*YOSHINEXTNUM);
	int i;
	for (i=0;i<YOSHINEXTNUM;++i){
		_ret.nextPieces[i] = malloc(sizeof(pieceColor)*_w);
	}
	
	return _ret;
}
void initYoshi(struct gameState* _passedState){
	_passedState->types[0] = BOARD_YOSHI;
	_passedState->boardData[0] = newYoshi(5,6);

	_passedState->controllers[0].func = yoshiUpdateControlSet;
	_passedState->controllers[0].data = NULL;
}
