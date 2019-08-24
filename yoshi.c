// todo egg code right now depends on the fact that there will always be nothing over the squishing stack
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
//
#define YOSHI_NORM_COLORS 4
//
#define YOSHI_STANDARD_FALL 2
//
#define YOSHIDIFFFALL 200
#define YOSHIROWTIME 100
#define POPTIME 500
#define EGGSQUISHTIME 1000
//
#define PIECESTATUS_POPPING 1
#define PIECESTATUS_SQUISHING 2

//////////////////////////////////////////////////
void getcolor(pieceColor _tileColor, unsigned char* r, unsigned char* g, unsigned char* b){
	switch(_tileColor){
		case YOSHI_TOPSHELL:
			*r=0;
			*g=200;
			*b=0;
			break;
		case YOSHI_BOTTOMSHELL:
			*r=0;
			*g=100;
			*b=0;
			break;
		case YOSHI_NORMALSTART:
			*r=255;
			*g=0;
			*b=0;
			break;
		case YOSHI_NORMALSTART+1:
			*r=138;
			*g=255;
			*b=205;
			break;
		case YOSHI_NORMALSTART+2:
			*r=0;
			*g=0;
			*b=255;
			break;
		case YOSHI_NORMALSTART+3:
			*r=150;
			*g=0;
			*b=150;
			break;
		default:
			printf("invalid color %d\n",_tileColor);
			break;
	}
}
void drawNormYoshiTile(pieceColor _tileColor, int _x, int _y, short tilew, short tileh){
	unsigned char r;
	unsigned char g;
	unsigned char b;
	getcolor(_tileColor,&r,&g,&b);
	drawRectangle(_x,_y,tilew,tileh,r,g,b,255);
}
void drawPoppingYoshiTile(pieceColor _tileColor, int _x, int _y, short tilew, u64 _endTime, u64 _sTime){
	unsigned char r;
	unsigned char g;
	unsigned char b;
	getcolor(_tileColor,&r,&g,&b);
	int _destSize=tilew*(_endTime-_sTime)/(double)POPTIME;
	drawRectangle(_x,_y,_destSize,_destSize,r,g,b,255);
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
char tryStartYoshiFall(struct yoshiBoard* _passedBoard, struct movingPiece* _curPiece, u64 _sTime){
	if (_curPiece->movingFlag ^ FLAG_MOVEDOWN){
		if (pieceCanFell(&_passedBoard->lowBoard,_curPiece)){
			_curPiece->movingFlag |= FLAG_MOVEDOWN;
			_curPiece->transitionDeltaY=1;
			_curPiece->completeFallTime=_sTime+YOSHIDIFFFALL;
			_curPiece->referenceFallTime=_curPiece->completeFallTime;
			_curPiece->diffFallTime=YOSHIDIFFFALL;
			++(_curPiece->tileY);
		}else{
			_curPiece->movingFlag |= FLAG_DEATHROW;
			_curPiece->completeFallTime=_sTime+YOSHIROWTIME;
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
			tryStartYoshiFall(_passedBoard,_newPiece,_sTime);
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
void makePopping(struct yoshiBoard* _passedBoard, int _x, int _y, u64 _sTime){
	_passedBoard->lowBoard.pieceStatus[_x][_y]=PIECESTATUS_POPPING;
	_passedBoard->lowBoard.pieceStatusTime[_x][_y]=_sTime+POPTIME;
}
void updateYoshiBoard(struct yoshiBoard* _passedBoard, u64 _sTime){
	char _boardSettled=1;
	int i=0;
	ITERATENLIST(_passedBoard->activePieces,{
			struct movingPiece* _curPiece = _curnList->data;
			if (updatePieceDisplay(_curPiece,_sTime)){
				tryStartYoshiFall(_passedBoard,_curPiece,_sTime);
			}
			if (_curPiece->movingFlag & FLAG_DEATHROW){
				if (_sTime>=_curPiece->completeFallTime){
					setBoard(&_passedBoard->lowBoard,_curPiece->tileX,_curPiece->tileY,_curPiece->color);
					if (_curPiece->color!=YOSHI_TOPSHELL){ // Check for normal matches
						if (getBoard(&_passedBoard->lowBoard,_curPiece->tileX,_curPiece->tileY+1)==_curPiece->color){
							makePopping(_passedBoard,_curPiece->tileX,_curPiece->tileY,_sTime);
							makePopping(_passedBoard,_curPiece->tileX,_curPiece->tileY+1,_sTime);							
						}
					}else{ // Check for egg completion using top shell
						int j;
						for (j=_curPiece->tileY+1;j<_passedBoard->lowBoard.h;++j){
							if (_passedBoard->lowBoard.board[_curPiece->tileX][j]==YOSHI_BOTTOMSHELL){
								_passedBoard->lowBoard.pieceStatusTime[_curPiece->tileX][j]=_sTime+EGGSQUISHTIME;
								_passedBoard->lowBoard.pieceStatus[_curPiece->tileX][j]=PIECESTATUS_SQUISHING;
								break;
							}
						}
						if (j==_passedBoard->lowBoard.h){ // If no bottom shell found
							makePopping(_passedBoard,_curPiece->tileX,_curPiece->tileY,_sTime);
						}
					}
					// remove piece from board
					struct nList* _freeThis = removenList(&_passedBoard->activePieces,i);
					free(_freeThis->data);
					free(_freeThis);
					--i;
				}
			}
			++i;
		});
	if (i!=0){ // If there are some active pieces still
		_boardSettled=0;
	}
	// Process piece status
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		int j;
		for (j=0;j<_passedBoard->lowBoard.h;++j){
			if (_passedBoard->lowBoard.board[i][j]!=COLOR_NONE && _passedBoard->lowBoard.pieceStatus[i][j]!=0){
				_boardSettled=0;
				if (_sTime>_passedBoard->lowBoard.pieceStatusTime[i][j]){
					switch(_passedBoard->lowBoard.pieceStatus[i][j]){
						case PIECESTATUS_POPPING:
							_passedBoard->lowBoard.board[i][j]=0;
							break;
						case PIECESTATUS_SQUISHING:
							for (;j>=0;--j){
								_passedBoard->lowBoard.board[i][j]=COLOR_NONE;
							}
							break;
					}
				}
			}
		}
	}
	//
	if (_boardSettled){
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
				drawNormYoshiTile(_passedBoard->nextPieces[i][j],_drawX+j*tilew,_drawY+(YOSHINEXTNUM-i-1)*tilew,tilew,tilew);
			}
		}
	}
	// draw main window background
	_drawY+=YOSHINEXTNUM*tilew+_smallw;
	drawRectangle(_drawX,_drawY,tilew*_passedBoard->lowBoard.w,_passedBoard->lowBoard.h*tilew,150,0,0,255);
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		int j;
		for (j=_passedBoard->lowBoard.h-1;j>=0;--j){
			if (_passedBoard->lowBoard.board[i][j]>COLOR_IMPOSSIBLE){
				if (_passedBoard->lowBoard.pieceStatus[i][j]==0){
					drawNormYoshiTile(_passedBoard->lowBoard.board[i][j],_drawX+i*tilew,_drawY+j*tilew,tilew,tilew);
				}else{
					switch(_passedBoard->lowBoard.pieceStatus[i][j]){
						case PIECESTATUS_POPPING:
							drawPoppingYoshiTile(_passedBoard->lowBoard.board[i][j],_drawX+i*tilew,_drawY+j*tilew,tilew,_passedBoard->lowBoard.pieceStatusTime[i][j],_sTime);
							break;
						case PIECESTATUS_SQUISHING:
							;
							int k;
							for (k=j-1;k>=0;--k){
								if (_passedBoard->lowBoard.board[i][k]==YOSHI_TOPSHELL){
									break;
								}
							}
							int _curY=_drawY+j*tilew;
							short _meatWidth=partMoveEmptys(_sTime,_passedBoard->lowBoard.pieceStatusTime[i][j],EGGSQUISHTIME,tilew);
							drawNormYoshiTile(YOSHI_BOTTOMSHELL,_drawX+i*tilew,_curY,tilew,tilew);
							for (j--;j>k;--j){
								_curY-=_meatWidth;
								drawNormYoshiTile(_passedBoard->lowBoard.board[i][j],_drawX+i*tilew,_curY,tilew,_meatWidth);
							}
							drawNormYoshiTile(YOSHI_TOPSHELL,_drawX+i*tilew,_curY-tilew,tilew,tilew);
							j=-1; // Break from this column draw
							break;
						default:
							drawRectangle(_drawX+i*tilew,_drawY+j*tilew,tilew,tilew,0,0,0,255);
							printf("invalid status %d\n",_passedBoard->lowBoard.pieceStatus[i][j]);
							break;
					}
				}
			}
		}
	}
	ITERATENLIST(_passedBoard->activePieces,{
			struct movingPiece* _curPiece = _curnList->data;
			drawNormYoshiTile(_curPiece->color,_drawX+FIXDISP(_curPiece->displayX),_drawY+FIXDISP(_curPiece->displayY),tilew,tilew);
		});
}
//////////////////////////////////////////////////
struct yoshiBoard* newYoshi(int _w, int _h){
	struct yoshiBoard* _ret = malloc(sizeof(struct yoshiBoard));
	_ret->lowBoard = newGenericBoard(_w,_h);
	clearBoardBoard(&_ret->lowBoard);
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
