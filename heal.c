/*
	Copyright (C) 2019	MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <stdio.h>
#include <goodbrew/config.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include "main.h"
#include "goodLinkedList.h"
#include "heal.h"

#define TESTBGCOLOR 150,0,0

void getHealColor(pieceColor _tileColor, unsigned char* r, unsigned char* g, unsigned char* b){
	switch(_tileColor){
		case 1:
			*r=255;
			*g=255;
			*b=255;
			break;
		case 2:
			*r=255;
			*g=0;
			*b=0;
			break;
		case 3:
			*r=0;
			*g=255;
			*b=0;
			break;
		case 4:
			*r=0;
			*g=0;
			*b=255;
			break;
		case 5:
			*r=150;
			*g=0;
			*b=150;
			break;
		default:
			printf("invalid color %d\n",_tileColor);
			break;
	}
}
//////////////////////////////////////////////////
void drawHealPiece(pieceColor _color, int _numColors, int _drawX, int _drawY, int tilew, struct healSkin* _passedSkin){
	_color=_color-1;
	// neutral, right, left
	unsigned char r,g,b;
	getHealColor(_color%_numColors+1,&r,&g,&b);
	drawRectangle(_drawX,_drawY,tilew,tilew,r*.70,g*.70,b*.70,255);
	drawRectangle(_drawX+tilew*.025,_drawY+tilew*.025,tilew*.95,tilew*.95,0,0,0,255);
	int _repeatLevel = _color/_numColors;
	int _insideX=_drawX;
	int _insideW=tilew*.80;
	if (_repeatLevel!=2){ // if it's not left
		_insideX+=tilew*.10;
	}
	if (_repeatLevel==1){ // if it's right
		_insideW=tilew-(_insideX-_drawX);
	}
	drawRectangle(_insideX,_drawY+tilew*.10,_insideW,tilew*.80,r,g,b,255);
}
//////////////////////////////////////////////////
void drawHealSet(struct pieceSet* _passedSet, int _numColors, int tilew, struct healSkin* _passedSkin){
	int i;
	for (i=0;i<_passedSet->count;++i){
		drawHealPiece(_passedSet->pieces[i].color,_numColors,FIXDISP(_passedSet->pieces[i].displayX),FIXDISP(_passedSet->pieces[i].displayY),tilew,_passedSkin);
	}
}
//////////////////////////////////////////////////
void drawHealBoard(struct healBoard* _passedBoard, int _drawX, int _drawY, int tilew, u64 _sTime){
	drawRectangle(_drawX,_drawY,tilew*_passedBoard->lowBoard.w,tilew*_passedBoard->lowBoard.h,TESTBGCOLOR,255);
	int i;
	for (i=0;i<_passedBoard->lowBoard.w;++i){
		int j;
		for (j=0;j<_passedBoard->lowBoard.h;++j){
			if (_passedBoard->lowBoard.board[i][j]!=0){
				drawHealPiece(_passedBoard->lowBoard.board[i][j],_passedBoard->settings.numColors,_drawX+i*tilew,_drawY+j*tilew,tilew,_passedBoard->skin);
			}
		}
	}

	ITERATENLIST(_passedBoard->activeSets,{
			drawHealSet(_curnList->data,_passedBoard->settings.numColors,tilew,_passedBoard->skin);
		});
}
void updateHealBoard(struct healBoard* _passedBoard, gameMode _mode, u64 _sTime){
}
void healUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime){
}
void resetHealBoard(struct healBoard* _passedBoard){
	clearBoardBoard(&_passedBoard->lowBoard);
	_passedBoard->lowBoard.status=STATUS_NORMAL;	
	if (_passedBoard->activeSets!=NULL){
		freenList(_passedBoard->activeSets,1);
	}
	
	_passedBoard->lowBoard.board[0][_passedBoard->lowBoard.h-1]=1;
	_passedBoard->lowBoard.board[1][_passedBoard->lowBoard.h-1]=_passedBoard->settings.numColors+1;
	_passedBoard->lowBoard.board[0][_passedBoard->lowBoard.h-2]=2;
	_passedBoard->lowBoard.board[1][_passedBoard->lowBoard.h-2]=_passedBoard->settings.numColors+2;

	struct pieceSet* _testset = malloc(sizeof(struct pieceSet));
	_testset->pieces=calloc(1,sizeof(struct movingPiece)*2);
	_testset->count=2;
	_testset->isSquare=0;
	_testset->quickLock=1;

	_testset->pieces[0].color=1;
	_testset->pieces[0].tileX=0;
	_testset->pieces[1].color=1;
	_testset->pieces[1].tileX=1;

	addnList(&_passedBoard->activeSets)->data = _testset;
}
struct healBoard* newHeal(int _w, int _h, struct healSettings* _usableSettings, struct healSkin* _passedSkin){
	struct healBoard* _ret = malloc(sizeof(struct healBoard));
	_ret->lowBoard = newGenericBoard(_w,_h);
	_ret->activeSets = NULL;
	_ret->skin=_passedSkin;
	memcpy(&_ret->settings,_usableSettings,sizeof(struct healSettings));
	resetHealBoard(_ret);
	return _ret;
}
//////////////////////////////////////////////////
void initHealSettings(struct healSettings* _passedSettings){
	_passedSettings->numColors=4;
}
void addHealBoard(struct gameState* _passedState, int i, int _w, int _h, struct healSettings* _usableSettings, struct healSkin* _passedSkin){
	_passedState->types[i] = BOARD_HEAL;
	_passedState->boardData[i] = newHeal(_w,_h,_usableSettings,_passedSkin);
	_passedState->controllers[i].func = healUpdateControlSet;
	_passedState->controllers[i].data = NULL;
}
