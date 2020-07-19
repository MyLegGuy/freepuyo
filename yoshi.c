/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
// todo - egg code right now depends on the fact that there will always be nothing over the squishing stack
// todo - maybe allow matching horizontally by swaping while pieces aligned
// todo - allow swapping the ends?
// todo - hold swap button and right to continously move the column to the right.
// todo - the pop too soon animation probably happens because it starts popping right when you press the swap button and column index changes. so swap animation still going on.
// todo -effect when you swap
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <goodbrew/config.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include "main.h"
#include "yoshi.h"
#include "puzzleGeneric.h"
#include "goodLinkedList.h"
//
#define YOSHI_STANDARD_FALL 2
//
#define YOSHINEXTDIVH 1 // this space gets stolen from tile 0
//
#define SWAPDUDECOLOR 255,0,0
//
#define CRUSHERBASEFADE fixTime(200)
//////////////////////////////////////////////////
void loadYoshiSkin(struct yoshiSkin* _ret, const char* _filename){
	_ret->img = loadImageEmbedded(_filename);
	_ret->normColors=5;
	_ret->colorW=16;
	_ret->colorH=16;
	_ret->colorX = malloc(sizeof(int)*(_ret->normColors+YOSHI_NUM_SPECIAL));
	_ret->colorY = malloc(sizeof(int)*(_ret->normColors+YOSHI_NUM_SPECIAL));

	_ret->colorX[0]=16;// top shell
	_ret->colorY[0]=16;
	_ret->colorX[1]=0; // bottom shell;
	_ret->colorY[1]=16;
	_ret->stretchBlockX=32;
	_ret->stretchBlockY=16;
	int i;
	for (i=0;i<_ret->normColors;++i){
		_ret->colorX[i+YOSHI_NUM_SPECIAL]=i*16;
		_ret->colorY[i+YOSHI_NUM_SPECIAL]=0;
	}
}
void freeYoshiSkin(struct yoshiSkin* _freeThis){
	freeTexture(_freeThis->img);
	free(_freeThis->colorX);
	free(_freeThis->colorY);
}
//////////////////////////////////////////////////
void getcolor(pieceColor _tileColor, unsigned char* r, unsigned char* g, unsigned char* b){
	switch(_tileColor){
		case YOSHI_TOPSHELL:
		case YOSHI_BOTTOMSHELL:
			*r=255;
			*g=255;
			*b=255;
			break;
		case YOSHI_NORMALSTART:
			*r=150;
			*g=150;
			*b=255;
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
void drawCrusherBar(struct yoshiSkin* _passedSkin, int _x, int _y, short tilew, short _entireH, unsigned char a){
	int _wholeTiles=_entireH/tilew;
	int i;
	for (i=0;i<_wholeTiles;++i){
		drawTexturePartSizedAlpha(_passedSkin->img,_x,_y+i*tilew,tilew,tilew,_passedSkin->stretchBlockX,_passedSkin->stretchBlockY,_passedSkin->colorW,_passedSkin->colorH,a);
	}
	int _leftoverH = _entireH%tilew;
	if (_leftoverH!=0){
		drawTexturePartSizedAlpha(_passedSkin->img,_x,_y+_wholeTiles*tilew,tilew,_leftoverH,_passedSkin->stretchBlockX,_passedSkin->stretchBlockY,_passedSkin->colorW,_passedSkin->colorH*(_leftoverH/(double)tilew),a);
	}
}
void advDrawYoshiTile(struct yoshiSkin* _passedSkin, pieceColor _tileColor, int _x, int _y, short tilew, short tileh, unsigned char a){
	int _skinIndex = _tileColor-YOSHI_SPECIALSTART;
	unsigned char r;
	unsigned char g;
	unsigned char b;
	getcolor(_tileColor,&r,&g,&b);
	drawTexturePartSizedTintAlpha(_passedSkin->img,_x,_y,tilew,tileh,_passedSkin->colorX[_skinIndex],_passedSkin->colorY[_skinIndex],_passedSkin->colorW,_passedSkin->colorH,r,g,b,a);
}
void drawNormYoshiTile(struct yoshiSkin* _passedSkin, pieceColor _tileColor, int _x, int _y, short tilew, short tileh){
	advDrawYoshiTile(_passedSkin,_tileColor,_x,_y,tilew,tileh,255);
}
void drawPoppingYoshiTile(struct yoshiSkin* _passedSkin, pieceColor _tileColor, int _x, int _y, short tilew, u64 _endTime, u64 _diffPopTime, u64 _sTime){
	int _destSize=tilew*(_endTime-_sTime)/(double)_diffPopTime;
	advDrawYoshiTile(_passedSkin,_tileColor,easyCenter(_destSize,tilew)+_x,easyCenter(_destSize,tilew)+_y,_destSize,_destSize,255);
}
void drawSwapDude(struct yoshiSkin* _passedSkin, int _x, int _y, short _smallw, short tilew){
	_x+=easyCenter(_smallw,tilew);
	drawRectangle(_x,_y,_smallw,_smallw,SWAPDUDECOLOR,255);
	drawRectangle(_x+tilew,_y,_smallw,_smallw,SWAPDUDECOLOR,255);
	drawRectangle(_x,_y+_smallw,tilew+_smallw,_smallw,SWAPDUDECOLOR,255);
}
//////////////////////////////////////////////////
void swapYoshiColumns(struct yoshiBoard* _passedBoard, short _leftIndex, u64 _sTime){
	_passedBoard->swappingIndex=_leftIndex;
	_passedBoard->swapEndTime=_sTime+_passedBoard->settings.swapTime;
	int i;
	for (i=0;i<_passedBoard->lowBoard.h;++i){
		pieceColor _holdColor = _passedBoard->lowBoard.board[_leftIndex][i];
		char _holdStatus = _passedBoard->lowBoard.pieceStatus[_leftIndex][i];
		u64 _holdTime = _passedBoard->lowBoard.pieceStatusTime[_leftIndex][i];

		_passedBoard->lowBoard.board[_leftIndex][i]=_passedBoard->lowBoard.board[_leftIndex+1][i];
		_passedBoard->lowBoard.pieceStatus[_leftIndex][i]=_passedBoard->lowBoard.pieceStatus[_leftIndex+1][i];
		_passedBoard->lowBoard.pieceStatusTime[_leftIndex][i]=_passedBoard->lowBoard.pieceStatusTime[_leftIndex+1][i];

		_passedBoard->lowBoard.board[_leftIndex+1][i]=_holdColor;
		_passedBoard->lowBoard.pieceStatus[_leftIndex+1][i]=_holdStatus;
		_passedBoard->lowBoard.pieceStatusTime[_leftIndex+1][i]=_holdTime;
	}
	// dont kill any pieces that were going to die, but now have free space under them
	ITERATENLIST(_passedBoard->activePieces,{
			struct movingPiece* _curPiece = _curnList->data;
			if (_curPiece->tileX==_leftIndex || _curPiece->tileX==_leftIndex+1){
				if (_passedBoard->lowBoard.board[_leftIndex][_curPiece->tileY]!=COLOR_NONE || _passedBoard->lowBoard.board[_leftIndex+1][_curPiece->tileY]!=COLOR_NONE){
					signed char _direction = _curPiece->tileX==_leftIndex ? 1 : -1;
					_curPiece->tileX+=_direction;
					_curPiece->movingFlag|=FLAG_HMOVE;
					_curPiece->diffHMoveTime = _passedBoard->settings.swapTime;
					_curPiece->completeHMoveTime = _passedBoard->swapEndTime;
					_curPiece->transitionDeltaX=_direction;
				}
			}
			if (pieceTryUnsetDeath(&_passedBoard->lowBoard,_curnList->data)){
				tryStartYoshiFall(_passedBoard,_curnList->data,_sTime);
			}
		});
}
void yoshiHoldDown(struct yoshiBoard* _passedBoard, struct controlSet* _passedControls, u64 _sTime){
	ITERATENLIST(_passedBoard->activePieces,{
			downButtonHold(_passedControls,_curnList->data,_passedBoard->settings.pushMultiplier,_sTime);
		});
}
void yoshiUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime){
	updateControlDas(_controlData,_sTime);
	struct yoshiBoard* _passedBoard = _passedGenericBoard;
	struct controlSet* _passedControls = _controlData;
	signed char _inputDirection = getDirectionInput(_controlData,_sTime);
	if (_passedControls->holdStartTime){
		if (!isDown(BUTTON_TOUCH)){ // On release
			onTouchRelease(_passedControls);
		}else if (!_passedControls->didDrag){ // On stable or drag
			signed char _didDrag = touchIsHDrag(_passedControls);
			if (_didDrag){
				_passedBoard->swapDudeX = (_passedControls->startTouchX-_drawX)/(tilew*YOSHI_TILE_SCALE);
				if (_didDrag==-1){
					--_passedBoard->swapDudeX;
				}
				if (_passedBoard->swapDudeX>=0 && _passedBoard->swapDudeX<_passedBoard->lowBoard.w-1){
					swapYoshiColumns(_passedGenericBoard,_passedBoard->swapDudeX,_sTime);
				}
				resetControlHDrag(_passedControls);
				_passedControls->didDrag=1;
			}
			int _touchYDiff = abs(touchY-_passedControls->startTouchY);
			if (_touchYDiff>=softdropMinDrag){
				if (!_passedControls->isTouchDrop){ // New down push
					_passedControls->isTouchDrop=1;
				}else{ // Hold down push
					yoshiHoldDown(_passedBoard,_passedControls,_sTime);
				}
			}else{
				if (_passedControls->isTouchDrop){
					_passedControls->isTouchDrop=0;
				}
			}
		}
	}else{
		if (isDown(BUTTON_TOUCH)){ // On initial touch
			initialTouchDown(_passedControls,_sTime);
		}
	}
	if (_inputDirection!=0){
		_passedBoard->swapDudeX=intCap(_passedBoard->swapDudeX+_inputDirection,0,_passedBoard->lowBoard.w-2);
	}
	if (wasJustPressed(BUTTON_A)){
		if (_passedBoard->swappingIndex==-1){
			swapYoshiColumns(_passedGenericBoard,_passedBoard->swapDudeX,_sTime);
		}
	}
	if (isDown(BUTTON_DOWN) && !wasJustPressed(BUTTON_DOWN)){
		yoshiHoldDown(_passedBoard,_passedControls,_sTime);
	}
	controlSetFrameEnd(_controlData,_sTime);
}
//////////////////////////////////////////////////
pieceColor yoshiGenColor(){
	int _result = randInt(1,YOSHI_NORM_COLORS*3+2);
	if (_result>YOSHI_NORM_COLORS*3){
		return YOSHI_SPECIALSTART+(_result-YOSHI_NORM_COLORS*3-1);
	}
	return _result%YOSHI_NORM_COLORS+YOSHI_NORMALSTART;
}
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
		_nextArray[_realIndex]=yoshiGenColor();
	}
}
char tryStartYoshiFall(struct yoshiBoard* _passedBoard, struct movingPiece* _curPiece, u64 _sTime){
	if (!(_curPiece->movingFlag & FLAG_MOVEDOWN)){
		if (pieceCanFell(&_passedBoard->lowBoard,_curPiece)){
			_curPiece->movingFlag |= FLAG_MOVEDOWN;
			_curPiece->transitionDeltaY=1;
			_curPiece->completeFallTime=_sTime+_passedBoard->settings.fallTime-_curPiece->completeFallTime;
			_curPiece->diffFallTime=_passedBoard->settings.fallTime;
			++(_curPiece->tileY);
		}else{
			_curPiece->movingFlag |= FLAG_DEATHROW;
			_curPiece->completeFallTime=_sTime+_passedBoard->settings.rowTime;
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
	_passedBoard->lowBoard.pieceStatusTime[_x][_y]=_sTime+_passedBoard->settings.popTime;
}
void updateYoshiBoard(struct yoshiBoard* _passedBoard, gameMode _mode, u64 _sTime){
	if (_passedBoard->lowBoard.status!=STATUS_NORMAL){
		return;
	}
	char _boardSettled=1;
	if (_passedBoard->swappingIndex!=-1){
		if (_sTime>=_passedBoard->swapEndTime){
			_passedBoard->swappingIndex=-1;
		}
	}
	int i=0;
	ITERATENLIST(_passedBoard->activePieces,{
			struct movingPiece* _curPiece = _curnList->data;
			char _needRepeat;
			do{
				if (updatePieceDisplay(_curPiece,_sTime)){
					tryStartYoshiFall(_passedBoard,_curPiece,_sTime);
					if (_sTime<_curPiece->completeFallTime){
						updatePieceDisplayY(_curPiece,_sTime,0);
					}else{
						_needRepeat=1;
					}
				}else{
					_needRepeat=0;
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
									_passedBoard->lowBoard.pieceStatusTime[_curPiece->tileX][j]=_sTime+_passedBoard->settings.squishPerPiece*(j-_curPiece->tileY-1)+CRUSHERBASEFADE;
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
						_needRepeat=0;
					}
				}
			}while(_needRepeat);
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
		if (!rowIsClear(&_passedBoard->lowBoard,0)){
			killBoard(&_passedBoard->lowBoard,_sTime);
		}else{
			if (_mode==MODE_GOAL && rowIsClear(&_passedBoard->lowBoard,_passedBoard->lowBoard.h-1)){
				winBoard(&_passedBoard->lowBoard);
			}else{
				yoshiSpawnNext(_passedBoard,_sTime);
			}
		}
	}else{
		ITERATENLIST(_passedBoard->activePieces,{
				removePartialTimes(_curnList->data);
			});
	}
}
void drawYoshiColumn(struct yoshiBoard* _passedBoard, short i, int _drawX, int _boardY, int tilew, u64 _sTime){
	int j;
	for (j=_passedBoard->lowBoard.h-1;j>=0;--j){
		if (_passedBoard->lowBoard.board[i][j]>COLOR_IMPOSSIBLE){
			if (_passedBoard->lowBoard.pieceStatus[i][j]==0){
				drawNormYoshiTile(_passedBoard->skin,_passedBoard->lowBoard.board[i][j],_drawX,_boardY+j*tilew,tilew,tilew);
			}else{
				switch(_passedBoard->lowBoard.pieceStatus[i][j]){
					case PIECESTATUS_POPPING:
						drawPoppingYoshiTile(_passedBoard->skin,_passedBoard->lowBoard.board[i][j],_drawX,_boardY+j*tilew,tilew,_passedBoard->lowBoard.pieceStatusTime[i][j],_passedBoard->settings.popTime,_sTime);
						break;
					case PIECESTATUS_SQUISHING:
						;
						int k;
						for (k=j-1;k>=0;--k){
							if (_passedBoard->lowBoard.board[i][k]==YOSHI_TOPSHELL){
								break;
							}
						}
						u64 _meatSquishTime = (j-k-1)*_passedBoard->settings.squishPerPiece;
						int _crushBarStart = _boardY+k*tilew;
						if (_sTime<_passedBoard->lowBoard.pieceStatusTime[i][j]-CRUSHERBASEFADE){
							int _curY=_boardY+j*tilew;
							short _meatWidth=partMoveEmptys(_sTime,_passedBoard->lowBoard.pieceStatusTime[i][j]-CRUSHERBASEFADE,_meatSquishTime,tilew);
							drawNormYoshiTile(_passedBoard->skin,YOSHI_BOTTOMSHELL,_drawX,_curY,tilew,tilew);
							for (j--;j>k;--j){
								_curY-=_meatWidth;
								drawNormYoshiTile(_passedBoard->skin,_passedBoard->lowBoard.board[i][j],_drawX,_curY,tilew,_meatWidth);
							}
							_curY-=tilew;
							drawNormYoshiTile(_passedBoard->skin,YOSHI_TOPSHELL,_drawX,_curY,tilew,tilew);
							drawCrusherBar(_passedBoard->skin,_drawX,_crushBarStart,tilew,_curY-_crushBarStart,255);
						}else{
							unsigned char _destAlpha=partMoveEmptys(_sTime,_passedBoard->lowBoard.pieceStatusTime[i][j],CRUSHERBASEFADE,255);
							int _curY = _boardY+j*tilew;
							advDrawYoshiTile(_passedBoard->skin,YOSHI_BOTTOMSHELL,_drawX,_curY,tilew,tilew,_destAlpha);
							_curY-=tilew;
							advDrawYoshiTile(_passedBoard->skin,YOSHI_TOPSHELL,_drawX,_curY,tilew,tilew,_destAlpha);
							drawCrusherBar(_passedBoard->skin,_drawX,_crushBarStart,tilew,_curY-_crushBarStart,_destAlpha);
						}
						j=-1; // Break from this column draw
						break;
					default:
						drawRectangle(_drawX,_boardY+j*tilew,tilew,tilew,0,0,0,255);
						printf("invalid status %d\n",_passedBoard->lowBoard.pieceStatus[i][j]);
						break;
				}
			}
		}
	}
}
void drawYoshiBoard(struct yoshiBoard* _passedBoard, int _drawX, int _startY, int _smallw, u64 _sTime){
	if (_passedBoard->lowBoard.status==STATUS_DEAD){
		if (_sTime<=_passedBoard->lowBoard.statusTimeEnd){
			_startY+=partMoveFills(_sTime,_passedBoard->lowBoard.statusTimeEnd,DEATHANIMTIME,screenHeight-_startY);
		}else{
			return;
		}
	}
	int tilew=_smallw*YOSHI_TILE_SCALE;
	int _boardY = _startY+(YOSHINEXTNUM-YOSHINEXTOVERLAPH)*tilew; // one tile of board and next window overlap
	// draw next window background
	drawRectangle(_drawX,_startY,tilew*_passedBoard->lowBoard.w,YOSHINEXTNUM*tilew,0,0,150,255);
	// draw main window background. does not include the overlap part
	int _boardBgHeight = (_passedBoard->lowBoard.h-YOSHINEXTOVERLAPH)*tilew-YOSHINEXTDIVH*_smallw;
	int _boardOverlapSpace = tilew*YOSHINEXTOVERLAPH+YOSHINEXTDIVH*_smallw;
	drawRectangle(_drawX,_boardY+_boardOverlapSpace,tilew*_passedBoard->lowBoard.w,_boardBgHeight,150,0,0,255);
	// Draw active pieces
	double _extraNextTileOffY=0;
	ITERATENLIST(_passedBoard->activePieces,{
			struct movingPiece* _curPiece = _curnList->data;
			drawNormYoshiTile(_passedBoard->skin,_curPiece->color,_drawX+FIXDISP(_curPiece->displayX),_boardY+FIXDISP(_curPiece->displayY),tilew,tilew);
			if (_curPiece->displayY<YOSHINEXTOVERLAPH){
				_extraNextTileOffY=(YOSHINEXTOVERLAPH-_curPiece->displayY)*-1;
			}
		});
	// Draw next window
	int i;
	for (i=0;i<YOSHINEXTNUM;++i){
		int j;
		for (j=0;j<_passedBoard->lowBoard.w;++j){
			if (_passedBoard->nextPieces[i][j]>COLOR_IMPOSSIBLE){
				drawNormYoshiTile(_passedBoard->skin,_passedBoard->nextPieces[i][j],_drawX+j*tilew,_startY+(YOSHINEXTNUM-i-1+_extraNextTileOffY)*tilew,tilew,tilew);
			}
		}
	}
	// Draw static board pieces.
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		if (i==_passedBoard->swappingIndex){
			drawYoshiColumn(_passedBoard,i,_drawX+i*tilew+partMoveEmptys(_sTime,_passedBoard->swapEndTime,_passedBoard->settings.swapTime,tilew),_boardY,tilew,_sTime);
			drawYoshiColumn(_passedBoard,i+1,_drawX+i*tilew+partMoveFills(_sTime,_passedBoard->swapEndTime,_passedBoard->settings.swapTime,tilew),_boardY,tilew,_sTime);
			++i;
			continue;
		}else{
			drawYoshiColumn(_passedBoard,i,_drawX+i*tilew,_boardY,tilew,_sTime);
		}
	}	
	// Draw divider between next window and board
	drawRectangle(_drawX,_boardY+YOSHINEXTOVERLAPH*tilew,tilew*_passedBoard->lowBoard.w,_smallw,255,255,255,255);
	//
	drawSwapDude(_passedBoard->skin,_drawX+_passedBoard->swapDudeX*tilew,_boardY+_boardOverlapSpace+_boardBgHeight,_smallw,tilew);
}
//////////////////////////////////////////////////
void yoshiInitLevelMode(void* _uncastBoard, struct gameState* _passedState, void* _level){
	struct yoshiBoard* _curBoard=_uncastBoard;
	_passedState->mode=MODE_GOAL;
	clearPieceStatus(&_curBoard->lowBoard);
	int _minY=_curBoard->lowBoard.h-1-*((int*)_level);
	int j;
	for (j=_curBoard->lowBoard.h-1;j>_minY;--j){
		int i;
		if (j==_curBoard->lowBoard.h-1){
			for (i=0;i<_curBoard->lowBoard.w;++i){
				_curBoard->lowBoard.board[i][j] = randInt(YOSHI_NORMALSTART,YOSHI_NORMALSTART+YOSHI_NORM_COLORS-1);
			}
		}else{
			for (i=0;i<_curBoard->lowBoard.w;++i){
				_curBoard->lowBoard.board[i][j] = fixWithExcluded(randInt(YOSHI_NORMALSTART,YOSHI_NORMALSTART+YOSHI_NORM_COLORS-2),_curBoard->lowBoard.board[i][j+1]);
			}
		}
	}
}
//////////////////////////////////////////////////
void initYoshiSettings(struct yoshiSettings* _passedSettings){
	// default settings
	_passedSettings->fallTime=300;
	_passedSettings->rowTime=0;
	_passedSettings->popTime=500;
	_passedSettings->squishPerPiece=300;
	_passedSettings->swapTime=100;
	_passedSettings->pushMultiplier=2;
}
void scaleYoshiSettings(struct yoshiSettings* _passedSettings){
	_passedSettings->fallTime=fixTime(_passedSettings->fallTime);
	_passedSettings->rowTime=fixTime(_passedSettings->rowTime);
	_passedSettings->popTime=fixTime(_passedSettings->popTime);
	_passedSettings->swapTime=fixTime(_passedSettings->swapTime);
	_passedSettings->squishPerPiece=fixTime(_passedSettings->squishPerPiece);
}
void resetYoshiBoard(struct yoshiBoard* _passedBoard){
	clearBoardBoard(&_passedBoard->lowBoard);
	_passedBoard->swappingIndex=-1;
	_passedBoard->lowBoard.status=STATUS_NORMAL;	
	int i;
	for (i=0;i<YOSHINEXTNUM+1;++i){
		fillYoshiNextSet(_passedBoard->nextPieces[i],_passedBoard->lowBoard.w,YOSHI_STANDARD_FALL);
	}
	freenList(_passedBoard->activePieces,1);
	_passedBoard->activePieces=NULL;
}
void freeYoshiBoard(struct yoshiBoard* _passedBoard){
	freeJaggedArrayColor(_passedBoard->nextPieces,YOSHINEXTNUM+1);
	freeGenericBoard(&_passedBoard->lowBoard);
}
struct yoshiBoard* newYoshi(int _w, int _h, struct yoshiSettings* _usableSettings, struct yoshiSkin* _passedSkin){
	struct yoshiBoard* _ret = malloc(sizeof(struct yoshiBoard));
	_ret->lowBoard = newGenericBoard(_w,_h);
	_ret->activePieces = NULL;
	_ret->skin=_passedSkin;
	_ret->swapDudeX=0;
	memcpy(&_ret->settings,_usableSettings,sizeof(struct yoshiSettings));
	scaleYoshiSettings(&_ret->settings);
	_ret->nextPieces = newJaggedArrayColor(YOSHINEXTNUM+1,_ret->lowBoard.w);
	resetYoshiBoard(_ret);
	return _ret;
}
void addYoshiPlayer(struct gameState* _passedState, int _w, int _h, struct yoshiSettings* _usableSettings, struct yoshiSkin* _passedSkin, struct controlSettings* _controlSettings){
	_passedState->types[0] = BOARD_YOSHI;
	_passedState->boardData[0] = newYoshi(_w,_h,_usableSettings,_passedSkin);
	_passedState->controllers[0].func = yoshiUpdateControlSet;
	_passedState->controllers[0].data = newControlSet(goodGetHDTime(),_controlSettings);
}
