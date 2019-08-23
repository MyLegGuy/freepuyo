#include <stdlib.h>
#include <stdio.h>
#include <goodbrew/config.h>
#include <goodbrew/graphics.h>
#include "main.h"
#include "yoshi.h"
#include "puzzleGeneric.h"
#include "goodLinkedList.h"
//
#define YOSHI_TOPSHELL 2
#define YOSHI_BOTTOMSHELL 3
#define YOSHI_NORMALSTART 4
#define YOSHI_NORM_COLORS 4
//
#define YOSHI_STANDARD_FALL 2
//
#define YOSHIDIFFFALL 200
//////////////////////////////////////////////////
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
//////////////////////////////////////////////////
void yoshiUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, u64 _sTime){
}
//////////////////////////////////////////////////
void fillYoshiNextSet(pieceColor* _nextArray, int _w, int _numFill){
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
char tryStartYoshiFall(struct yoshiBoard* _passedBoard, struct movingPiece* _curPiece, int _pieceIndex, u64 _sTime){
	if (_curPiece->movingFlag ^ FLAG_MOVEDOWN){
		if (pieceCanFell(&_passedBoard->lowBoard,_curPiece)){
			_curPiece->movingFlag |= FLAG_MOVEDOWN;
			_curPiece->transitionDeltaY=1;
			_curPiece->completeFallTime=_sTime+YOSHIDIFFFALL;
			_curPiece->referenceFallTime=_curPiece->completeFallTime;
			_curPiece->diffFallTime=YOSHIDIFFFALL;
			++(_curPiece->tileY);
		}else{
			//_curPiece |= FLAG_DEATHROW;
			_passedBoard->lowBoard.board[_curPiece->tileX][_curPiece->tileY]=_curPiece->color;
			struct nList* _freeThis = removenList(&_passedBoard->activePieces,_pieceIndex);
			free(_freeThis->data);
			free(_freeThis);
			return 1;
		}
	}
	return 0;
}
void yoshiSpawnSet(struct yoshiBoard* _passedBoard, pieceColor* _passedSet, u64 _sTime){
	int i;
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		if (_passedSet[i]!=COLOR_NONE){
			struct movingPiece* _newPiece = malloc(sizeof(struct movingPiece));
			memset(_newPiece,0,sizeof(struct movingPiece));
			_newPiece->color=_passedSet[i];
			_newPiece->tileX=i;
			_newPiece->tileY=0;
			snapPieceDisplayPossible(_newPiece);
			addnList(&_passedBoard->activePieces)->data=_newPiece;
			tryStartYoshiFall(_passedBoard,_newPiece,-1,_sTime);
		}
	}
}
void yoshiSpawnNext(struct yoshiBoard* _passedBoard, u64 _sTime){
	yoshiSpawnSet(_passedBoard,_passedBoard->nextPieces[0],_sTime);
	int i;
	for (i=1;i<YOSHINEXTNUM+1;++i){
		memcpy(_passedBoard->nextPieces[i-1],_passedBoard->nextPieces[i],sizeof(pieceColor)*_passedBoard->lowBoard.w);
	}
	fillYoshiNextSet(_passedBoard->nextPieces[YOSHINEXTNUM],_passedBoard->lowBoard.w,YOSHI_STANDARD_FALL);
}
void updateYoshiBoard(struct yoshiBoard* _passedBoard, u64 _sTime){
	int i=0;
	ITERATENLIST(_passedBoard->activePieces,{
			struct movingPiece* _curPiece = _curnList->data;
			if (updatePieceDisplay(_curPiece,_sTime)){
				if (tryStartYoshiFall(_passedBoard,_curPiece,i,_sTime)){
					--i;
				}
			}
			++i;
		});
	if (i==0){ // If there are no active pieces left
		yoshiSpawnNext(_passedBoard,_sTime);
	}
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
				drawNormYoshiTile(_passedBoard->nextPieces[i][j],_drawX+j*tilew,_drawY+(YOSHINEXTNUM-i-1)*tilew,tilew);
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
				drawNormYoshiTile(_passedBoard->lowBoard.board[i][j],_drawX+i*tilew,_drawY+j*tilew,tilew);
			}
		}
	}
	ITERATENLIST(_passedBoard->activePieces,{
			struct movingPiece* _curPiece = _curnList->data;
			drawNormYoshiTile(_curPiece->color,_drawX+FIXDISP(_curPiece->displayX),_drawY+FIXDISP(_curPiece->displayY),tilew);
		});
}
//////////////////////////////////////////////////
struct yoshiBoard* newYoshi(int _w, int _h){
	struct yoshiBoard* _ret = malloc(sizeof(struct yoshiBoard));
	_ret->lowBoard = newGenericBoard(_w,_h);
	clearGenericBoard(_ret->lowBoard.board,_w,_h);
	_ret->nextPieces = malloc(sizeof(pieceColor*)*(YOSHINEXTNUM+1));
	_ret->activePieces = NULL;
	int i;
	for (i=0;i<YOSHINEXTNUM+1;++i){
		_ret->nextPieces[i] = malloc(sizeof(pieceColor)*_w);
		fillYoshiNextSet(_ret->nextPieces[i],_w,YOSHI_STANDARD_FALL);
	}
	return _ret;
}
void initYoshi(struct gameState* _passedState){
	_passedState->types[0] = BOARD_YOSHI;
	_passedState->boardData[0] = newYoshi(5,6);

	_passedState->controllers[0].func = yoshiUpdateControlSet;
	_passedState->controllers[0].data = NULL;
}
