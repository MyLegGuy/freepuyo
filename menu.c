/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <goodbrew/config.h>
#include <goodbrew/images.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include "main.h"
#include "menu.h"
// these 4 are needed for init puyo/yoshi functions
#include <goodbrew/base.h>
#include "puzzleGeneric.h"
#include "puyo.h"
#include "yoshi.h"
#include "heal.h"
#include "skinLoader.h"
//
#include "ui.h"

struct incrementInfo{
	void* target;
	double min;
	double max;
};

#define NUMBLOBOPTIONS 16
const char* BLOBOPTIONNAMES[NUMBLOBOPTIONS]={
	"Board Width",
	"Board Height",
	"Invisible Rows",
	"Garbage Score",
	"Colors",
	"Pop Number",
	"Fast Drop Speed",
	"Pop Time",
	"Next Time",
	"Rotate Time",
	"Shift Time",
	"Fall Speed",
	"Post-Squish Time",
	"Max Garbage Rows",
	"Squish Time",
	"Preview Pieces",
};
const int BLOBOPTIONMINS[NUMBLOBOPTIONS]={1,2,0,1,1,1,0,0,0,0,0,0,0,1,0,0};
const int BLOBOPTIONMAX[NUMBLOBOPTIONS]={SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,5,SHRT_MAX,40,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX};
const double BLOBOPTIONINC[NUMBLOBOPTIONS]={1,1,1,10,1,1,.5,50,50,50,5,50,50,1,50,1};

#define NUMYOSHIOPTIONS 9
const char* YOSHIOPTIONNAMES[NUMYOSHIOPTIONS]={
	"Board Width",
	"Board Height",
	"Level",
	"Fall Time",
	"Stall Time",
	"Clear Time",
	"Squish Time",
	"Swap Time",
	"Fast Drop Speed",
};
const int YOSHIOPTIONMINS[NUMYOSHIOPTIONS]={1,1,0,0,0,0,0,0,0};
const int YOSHIOPTIONMAX[NUMBLOBOPTIONS]={SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX};
const double YOSHIOPTIONINC[NUMBLOBOPTIONS]={1,1,1,50,50,50,50,50,.1};

#define NUMMAINTITLEBUTTONS 5

// images used a lot
crossTexture* xNorm;
crossTexture* xHover;
crossTexture* xClick;
//

int maxTextWidth(int _numStrings, ...){
	va_list _args;
	va_start(_args,_numStrings);
	int _maxLen = 0;
	int i;
	for (i=0;i<_numStrings;++i){
		int _curLen = textWidth(regularFont,va_arg(_args, char*));
		if (_curLen>_maxLen){
			_maxLen=_curLen;
		}
	}
	return _maxLen;
}
void buttonIntIncrement(void* _uncastInfo, double _changeAmount){
	struct incrementInfo* _passedInfo = _uncastInfo;
	int* _passedInt = _passedInfo->target;
	*_passedInt=*_passedInt+_changeAmount;
	if (*_passedInt<_passedInfo->min){
		*_passedInt=_passedInfo->min;
	}else if (*_passedInt>_passedInfo->max){
		*_passedInt=_passedInfo->max;
	}
}
void buttonDoubleIncrement(void* _uncastInfo, double _changeAmount){
	struct incrementInfo* _passedInfo = _uncastInfo;
	double* _passedInt = _passedInfo->target;
	*_passedInt=*_passedInt+_changeAmount;
	if (*_passedInt<_passedInfo->min){
		*_passedInt=_passedInfo->min;
	}else if (*_passedInt>_passedInfo->max){
		*_passedInt=_passedInfo->max;
	}
}
void buttonSetInt(void* _uncastInt, double _setVal){
	*((int*)_uncastInt)=_setVal;
}
void backButtonEvent(void* _ignoredParam, double _otherIgnoredParam){
	setJustPressed(BUTTON_BACK);
}
struct uiButton* newXButton(int _x, int _y, int _height, crossTexture* _normal, crossTexture* _hover, crossTexture* _click){
	struct uiButton* _ret = newButton();
	_ret->onPress=backButtonEvent;
	_ret->images[0]=_normal;
	_ret->images[1]=_hover;
	_ret->images[2]=_click;
	_ret->w=getOtherScaled(getTextureHeight(_normal),_height,getTextureWidth(_normal));
	_ret->h=_height;
	_ret->x=_x;
	_ret->y=_y;
	return _ret;
}
void addXButtonToWin(struct menuScreen* _onHere, int _index, crossTexture* _normal, crossTexture* _hover, crossTexture* _click){
	int _windowDestX;
	int _windowDestY;
	int _windowDestW;
	int _windowDestH;
	getWindowDrawInfo(&curMenus[curScreenIndex],1,&_windowDestX,&_windowDestY,&_windowDestW,&_windowDestH);
	curMenus[curScreenIndex].elements[_index]=newXButton(0,_windowDestY-stdCornerHeight/2,stdCornerHeight,_normal,_hover,_click);
	((struct uiButton*)curMenus[curScreenIndex].elements[_index])->x=_windowDestX+_windowDestW-((struct uiButton*)curMenus[curScreenIndex].elements[_index])->w/1.5;
	curMenus[curScreenIndex].types[_index]=UIELEM_BUTTON;
}
// _numTypes is 1 for double, 0 for int
struct uiList* lowCreateOptionsMenu(int _numOptions, const char** _labels, void** _nums, char* _numTypes, const int* _minNums, const int* _maxNums, const double* _incAmnts, crossTexture* _plusNorm, crossTexture* _plusHover, crossTexture* _plusClick, crossTexture* _lessNorm, crossTexture* _lessHover, crossTexture* _lessClick, struct incrementInfo* _incrementerSpace){
	struct uiList* _ret = newUiList(_numOptions,4,curFontHeight);
	int i;
	for (i=0;i<_numOptions;++i){
		struct uiLabel* _newNameLabel = malloc(sizeof(struct uiLabel));
		_newNameLabel->r=0;
		_newNameLabel->g=0;
		_newNameLabel->b=0;
		easyStaticPrintfArray(&_newNameLabel->format,_labels[i]);
		_ret->elements[0][i] = _newNameLabel;
		_ret->types[0][i] = UIELEM_LABEL;

		struct uiButton* _newMinusButton = newButton();;
		_newMinusButton->images[0] = _lessNorm;
		_newMinusButton->images[1] = _lessHover;
		_newMinusButton->images[2] = _lessClick;
		_ret->elements[1][i] = _newMinusButton;
		_ret->types[1][i] = UIELEM_BUTTON;

		struct uiLabel* _newValLabel = malloc(sizeof(struct uiLabel));
		_newValLabel->r=0;
		_newValLabel->g=0;
		_newValLabel->b=0;
		_ret->elements[2][i] = _newValLabel;
		_ret->types[2][i] = UIELEM_LABEL;

		struct uiButton* _newPlusButton = newButton();;
		_newPlusButton->images[0] = _plusNorm;
		_newPlusButton->images[1] = _plusHover;
		_newPlusButton->images[2] = _plusClick;
		_ret->elements[3][i] = _newPlusButton;
		_ret->types[3][i] = UIELEM_BUTTON;


		// Set up button incrementers
		_incrementerSpace[i].target=_nums[i];
		_incrementerSpace[i].min=_minNums[i];
		_incrementerSpace[i].max=_maxNums[i];
		_newPlusButton->arg1 = &_incrementerSpace[i];
		_newMinusButton->arg1 = &_incrementerSpace[i];		
		if (_numTypes[i]==1){ // double
			easyDoublePrintfArray(&_newValLabel->format,_nums[i]);
			_newPlusButton->onPress=buttonDoubleIncrement;
			_newMinusButton->onPress=buttonDoubleIncrement;
		}else{ // int
			easyNumPrintfArray(&_newValLabel->format,_nums[i]);
			_newPlusButton->onPress=buttonIntIncrement;
			_newMinusButton->onPress=buttonIntIncrement;
		}
		_newPlusButton->arg2=_incAmnts[i]; //
		_newMinusButton->arg2=_incAmnts[i]*-1;
	}
	return _ret;
}
void showOptionsMenu(int _numOptions, const char** _labels, void** _nums, char* _numTypes, const int* _minNums, const int* _maxNums, const double* _incAmnts, crossTexture* _plusNorm, crossTexture* _plusHover, crossTexture* _plusClick, crossTexture* _lessNorm, crossTexture* _lessHover, crossTexture* _lessClick, u64 _sTime){
	// screen creation early to get data buffer
	struct incrementInfo* _menuExtraData = addMenuScreen(2,sizeof(struct incrementInfo)*_numOptions);
	struct uiList* _newSettingsList = lowCreateOptionsMenu(_numOptions,_labels,_nums,_numTypes,_minNums,_maxNums,_incAmnts,_plusNorm,_plusHover,_plusClick,_lessNorm,_lessHover,_lessClick,_menuExtraData);
	uiListCalcSizes(_newSettingsList,0);
	uiListPos(_newSettingsList,easyCenter(_newSettingsList->w,screenWidth),easyCenter(_newSettingsList->h,screenHeight),0);
	// complete window setup
	curMenus[curScreenIndex].winW = _newSettingsList->w;
	curMenus[curScreenIndex].winH = _newSettingsList->h;
	curMenus[curScreenIndex].elements[0]=_newSettingsList;
	curMenus[curScreenIndex].types[0]=UIELEM_LIST;
	addXButtonToWin(&curMenus[curScreenIndex],1,xNorm,xHover,xClick);
	windowPopupEnd=_sTime+WINDOWPOPUPTIME;
}
//////////////////////////////////////////////////
void wrapRestartGameState(void* _uncastState, double _ignored){	
	delMenuScreen(3);
	restartGameState(_uncastState,goodGetHDTime());
}
void wrapSetGameStateExit(void* _uncastState, double _ignored){
	delMenuScreen(3);
	((struct gameState*)_uncastState)->status=MAJORSTATUS_EXIT;
}
void spawnWinLoseShared(struct gameState* _passedState, u64 _sTime){
	addMenuScreen(2,0);	
	windowPopupEnd=_sTime+WINDOWPOPUPTIME;
	
	struct uiButton* _retryButton = newButton();
	_retryButton->images[0]=loadImageEmbedded("assets/ui/retry.png");
	_retryButton->images[1]=loadImageEmbedded("assets/ui/retryHover.png");
	_retryButton->images[2]=loadImageEmbedded("assets/ui/retryClick.png");
	_retryButton->h=USUALBUTTONH;
	_retryButton->w=getOtherScaled(getTextureHeight(_retryButton->images[0]),_retryButton->h,getTextureWidth(_retryButton->images[0]));
	_retryButton->arg1=_passedState;

	struct uiButton* _homeButton = malloc(sizeof(struct uiButton));
	memcpy(_homeButton,_retryButton,sizeof(struct uiButton));
	_homeButton->images[0] = loadImageEmbedded("assets/ui/home.png");
	_homeButton->images[1] = loadImageEmbedded("assets/ui/homeHover.png");
	_homeButton->images[2] = loadImageEmbedded("assets/ui/homeClick.png");

	curMenus[curScreenIndex].winW=_retryButton->w*STDBUTTONSEPARATION+_retryButton->w;
	curMenus[curScreenIndex].winH=_retryButton->h;
	
	_retryButton->x=easyCenter(curMenus[curScreenIndex].winW,screenWidth);
	_retryButton->y=easyCenter(curMenus[curScreenIndex].winH,screenHeight);

	_homeButton->y=_retryButton->y;
	_homeButton->x=_retryButton->x+_retryButton->w*STDBUTTONSEPARATION;

	_retryButton->onPress=wrapRestartGameState;
	_homeButton->onPress=wrapSetGameStateExit;
	
	curMenus[curScreenIndex].elements[0]=_retryButton;
	curMenus[curScreenIndex].elements[1]=_homeButton;
	curMenus[curScreenIndex].types[0]=UIELEM_BUTTON;
	curMenus[curScreenIndex].types[1]=UIELEM_BUTTON;
}
void spawnWinMenu(struct gameState* _passedState, u64 _sTime){
	spawnWinLoseShared(_passedState,_sTime);
	curMenus[curScreenIndex].title="You won!";
}
void spawnLoseMenu(struct gameState* _passedState, u64 _sTime){
	spawnWinLoseShared(_passedState,_sTime);
}
void delWinLoseMenu(){
	delMenuScreen(3);
}
//////////////////////////////////////////////////
void loadGlobalUI(){
	xNorm = loadImageEmbedded("assets/ui/x.png");
	xHover = loadImageEmbedded("assets/ui/xHover.png");
	xClick = loadImageEmbedded("assets/ui/xClick.png");
}
void titleScreen(struct gameState* _ret){
	struct gameSettings _curPuyoSettings;
	int _puyoW=6;
	int _puyoH=12;
	int _puyoGhost=2;
	int _puyoNext=2;
	//
	int _yoshiW=5;
	int _yoshiH=6;
	int _yoshiLevel=1;
	//
	struct healSettings _curHealSettings;
	initHealSettings(&_curHealSettings);
	//
	struct controlSettings _curControlSettings;
	initControlSettings(&_curControlSettings);
	//
	initPuyoSettings(&_curPuyoSettings);
	struct yoshiSettings _curYoshiSettings;
	initYoshiSettings(&_curYoshiSettings);

	ribbonImg = loadImageEmbedded("assets/ui/ribbon.png");
	
	stdWindow.middle = loadImageEmbedded("assets/ui/winm.png");
	stdWindow.corner[0] = loadImageEmbedded("assets/ui/winc1.png"); //
	stdWindow.corner[1] = loadImageEmbedded("assets/ui/winc2.png");
	stdWindow.corner[2] = loadImageEmbedded("assets/ui/winc3.png");
	stdWindow.corner[3] = loadImageEmbedded("assets/ui/winc4.png");
	stdWindow.edge[0] = loadImageEmbedded("assets/ui/wine1.png"); //
	stdWindow.edge[1] = loadImageEmbedded("assets/ui/wine2.png");
	stdWindow.edge[2] = loadImageEmbedded("assets/ui/wine3.png");
	stdWindow.edge[3] = loadImageEmbedded("assets/ui/wine4.png");

	boardBorder.corner[0] = loadImageEmbedded("assets/ui/bordc1.png"); //
	boardBorder.corner[1] = loadImageEmbedded("assets/ui/bordc2.png");
	boardBorder.corner[2] = loadImageEmbedded("assets/ui/bordc3.png");
	boardBorder.corner[3] = loadImageEmbedded("assets/ui/bordc4.png");
	boardBorder.edge[0] = loadImageEmbedded("assets/ui/borde1.png"); //
	boardBorder.edge[1] = loadImageEmbedded("assets/ui/borde2.png");
	boardBorder.edge[2] = loadImageEmbedded("assets/ui/borde3.png");
	boardBorder.edge[3] = loadImageEmbedded("assets/ui/borde4.png");
	menuInit(curFontHeight); // must init after window

	setClearColor(150,255,150); // cute bg
	int _curSelection=0;
	while(1){
		u64 _sTime = goodGetHDTime();
		controlsStart();

		if (wasJustPressed(BUTTON_DOWN)){
			_curSelection++;
		}else if (wasJustPressed(BUTTON_UP)){
			_curSelection--;
		}
		if (wasJustPressed(BUTTON_A)){
			if (_curSelection==0 || _curSelection==1){
				*_ret = newGameState(_curSelection==0 ? 2 : 1);
				loadGameSkin(BOARD_PUYO);
				addPuyoBoard(_ret,0,_puyoW,_puyoH,_puyoGhost,_puyoNext,&_curPuyoSettings,loadedSkins[BOARD_PUYO],&_curControlSettings,0);
				if (_curSelection==0){
					addPuyoBoard(_ret,1,_puyoW,_puyoH,_puyoGhost,_puyoNext,&_curPuyoSettings,loadedSkins[BOARD_PUYO],NULL,1);
					_ret->mode=MODE_BATTLE;
				}else{
					_ret->mode=MODE_ENDLESS;
				}
				break;
			}else if (_curSelection==2 || _curSelection==3){
				*_ret = newGameState(1);
				loadGameSkin(BOARD_YOSHI);
				addYoshiPlayer(_ret,_yoshiW,_yoshiH,&_curYoshiSettings,loadedSkins[BOARD_YOSHI],&_curControlSettings);
				if (_curSelection==3){
					_ret->initializers[0]=yoshiInitLevelMode;
					_ret->initializerInfo[0] = malloc(sizeof(int));
					*((int*)_ret->initializerInfo[0])=3;
				}else{
					_ret->mode=MODE_ENDLESS;
				}
				break;
			}else if (_curSelection==4){
				*_ret = newGameState(1);
				loadGameSkin(BOARD_HEAL);
				_ret->mode=MODE_ENDLESS;
				addHealBoard(_ret,0,10,10,&_curHealSettings,loadedSkins[BOARD_HEAL],&_curControlSettings);
				break;
			}
		}
	
		controlsEnd();
		startDrawing();

		drawRectangle(0,curFontHeight*_curSelection,screenWidth,curFontHeight,255,255,255,255);
		gbDrawText(regularFont,0,curFontHeight*0,"VS Blobs",0,0,0);
		gbDrawText(regularFont,0,curFontHeight*1,"1P Blobs",0,0,0);
		gbDrawText(regularFont,0,curFontHeight*2,"Crates Endless",0,0,0);
		gbDrawText(regularFont,0,curFontHeight*3,"Crates Clear Mode",0,0,0);
		gbDrawText(regularFont,0,curFontHeight*4,"Circuits 1P",0,0,0);

		
		//gbDrawText(regularFont,0,_curSelection*curFontHeight,">",0,0,0);
		endDrawing();
	}
	if (_ret->numBoards!=0){
		endStateInit(_ret);
	}
	//if (_curSettingsList!=NULL){
	//	freeUiList(_curSettingsList,2);
	//}
	// Delete title button layer
	//
	controlsEnd();

	setClearColor(0,0,0);
}
