#include <stdlib.h>
#include <stdio.h>
#include <goodbrew/config.h>
#include <goodbrew/graphics.h>
#include "main.h"
#include "yoshi.h"
#include "puzzleGeneric.h"
//
#define YOSHI_TOPSHELL 2
#define YOSHI_BOTTOMSHELL 3
#define YOSHI_NORMALSTART 4
#define YOSHI_NORM_COLORS 4
//
#define YOSHI_STANDARD_FALL 2
//
void drawNormYoshiTile(pieceColor _tileColor, int _x, int _y, short tilew){
	unsigned char r;
	unsigned char g;
	unsigned char b;
	switch(_tileColor){
		case YOSHI_TOPSHELL:
			r=0;
			g=200;
			b=0;
			break;
		case YOSHI_BOTTOMSHELL:
			r=0;
			g=100;
			b=0;
			break;
		case YOSHI_NORMALSTART:
			r=255;
			g=0;
			b=0;
			break;
		case YOSHI_NORMALSTART+1:
			r=138;
			g=255;
			b=205;
			break;
		case YOSHI_NORMALSTART+2:
			r=0;
			g=0;
			b=255;
			break;
		case YOSHI_NORMALSTART+3:
			r=150;
			g=0;
			b=150;
			break;
	}
	drawRectangle(_x,_y,tilew,tilew,r,g,b,255);
}
void yoshiUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, u64 _sTime){
}
void drawYoshiBoard(struct yoshiBoard* _passedBoard, int _drawX, int _drawY, int _smallw, u64 _sTime){
	int tilew=_smallw*YOSHI_TILE_SCALE;
	// draw next window background
	drawRectangle(_drawX,_drawY,tilew*_passedBoard->lowBoard.w,YOSHINEXTNUM*tilew,0,0,150,255);
	int i;
	for (i=0;i<YOSHINEXTNUM;++i){
		int j;
		for (j=0;j<_passedBoard->lowBoard.w;++j){
			if (_passedBoard->nextPieces[i][j]>COLOR_IMPOSSIBLE){
				drawNormYoshiTile(_passedBoard->nextPieces[i][j],_drawX+j*tilew,_drawY+i*tilew,tilew);
			}
		}
	}
	// draw main window background
	_drawY+=YOSHINEXTNUM*tilew+_smallw;
	drawRectangle(_drawX,_drawY,tilew*_passedBoard->lowBoard.w,_passedBoard->lowBoard.h*tilew,150,0,0,255);
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		int j;
		for (j=0;j<_passedBoard->lowBoard.h;++j){
			if (_passedBoard->lowBoard.board[i][j]>COLOR_IMPOSSIBLE){
				drawNormYoshiTile(_passedBoard->lowBoard.board[i][j],_drawX+j*tilew,_drawY+i*tilew,tilew);
			}
		}
	}
}
void fillYoshiPieceSet(pieceColor* _nextArray, int _w, int _numFill){
	memset(_nextArray,COLOR_NONE,sizeof(pieceColor)*_w);
	int i;
	for (i=0;i<_numFill;++i){
		signed short _fakeIndex=randInt(0,_w-1-i);
		signed short _realIndex;
		for (_realIndex=0;;++_realIndex){
			if (_nextArray[_realIndex]==0){
				if ((--_fakeIndex)==-1){
					break;
				}
			}
		}
		_nextArray[_realIndex]=randInt(YOSHI_TOPSHELL,YOSHI_NORMALSTART+YOSHI_NORM_COLORS-1);		
	}
}
struct yoshiBoard* newYoshi(int _w, int _h){
	struct yoshiBoard* _ret = malloc(sizeof(struct yoshiBoard));
	_ret->lowBoard = newGenericBoard(_w,_h);
	clearGenericBoard(_ret->lowBoard.board,_w,_h);
	_ret->nextPieces = malloc(sizeof(pieceColor*)*YOSHINEXTNUM);
	int i;
	for (i=0;i<YOSHINEXTNUM;++i){
		_ret->nextPieces[i] = malloc(sizeof(pieceColor)*_w);
		fillYoshiPieceSet(_ret->nextPieces[i],_w,YOSHI_STANDARD_FALL);
	}
	return _ret;
}
void initYoshi(struct gameState* _passedState){
	_passedState->types[0] = BOARD_YOSHI;
	_passedState->boardData[0] = newYoshi(5,6);

	_passedState->controllers[0].func = yoshiUpdateControlSet;
	_passedState->controllers[0].data = NULL;
}
