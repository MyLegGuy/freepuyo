/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
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

#define NUMMAINTITLEBUTTONS 4

// images used a lot
crossTexture xNorm;
crossTexture xHover;
crossTexture xClick;
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
struct uiButton* newXButton(int _x, int _y, int _height, crossTexture _normal, crossTexture _hover, crossTexture _click){
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
void addXButtonToWin(struct menuScreen* _onHere, int _index, crossTexture _normal, crossTexture _hover, crossTexture _click){
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
struct uiList* lowCreateOptionsMenu(int _numOptions, const char** _labels, void** _nums, char* _numTypes, const int* _minNums, const int* _maxNums, const double* _incAmnts, crossTexture _plusNorm, crossTexture _plusHover, crossTexture _plusClick, crossTexture _lessNorm, crossTexture _lessHover, crossTexture _lessClick, struct incrementInfo* _incrementerSpace){
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
void showOptionsMenu(int _numOptions, const char** _labels, void** _nums, char* _numTypes, const int* _minNums, const int* _maxNums, const double* _incAmnts, crossTexture _plusNorm, crossTexture _plusHover, crossTexture _plusClick, crossTexture _lessNorm, crossTexture _lessHover, crossTexture _lessClick, u64 _sTime){
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
	restartGameState(_uncastState,goodGetMilli());
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

	crossTexture _logoImg = loadImageEmbedded("assets/ui/logo.png");

	crossTexture _butNorm = loadImageEmbedded("assets/ui/but.png");
	crossTexture _butHover = loadImageEmbedded("assets/ui/butHover.png");
	crossTexture _butClick = loadImageEmbedded("assets/ui/butClick.png");

	crossTexture _optionsNorm = loadImageEmbedded("assets/ui/settings.png");
	crossTexture _optionsHover = loadImageEmbedded("assets/ui/settingsHover.png");
	crossTexture _optionsClick = loadImageEmbedded("assets/ui/settingsClick.png");

	crossTexture _plusNorm = loadImageEmbedded("assets/ui/more.png");
	crossTexture _plusHover = loadImageEmbedded("assets/ui/moreHover.png");
	crossTexture _plusClick = loadImageEmbedded("assets/ui/moreClick.png");

	crossTexture _lessNorm = loadImageEmbedded("assets/ui/less.png");
	crossTexture _lessHover = loadImageEmbedded("assets/ui/lessHover.png");
	crossTexture _lessClick = loadImageEmbedded("assets/ui/lessClick.png");

	int _lastTitleButton;
	addMenuScreen(6,0);
	int _mainIndex = curScreenIndex;
	// blob button (battle)
	struct uiButton* _titleBlobButton = newButton();
	curMenus[_mainIndex].elements[0]=_titleBlobButton;
	_titleBlobButton->images[0] = _butNorm;
	_titleBlobButton->images[1] = _butHover;
	_titleBlobButton->images[2] = _butClick;
	_titleBlobButton->onPress=buttonSetInt;
	_titleBlobButton->arg1=&_lastTitleButton;
	_titleBlobButton->arg2=1;
	// blob button (endless)
	struct uiButton* _endlessBlobButton = malloc(sizeof(struct uiButton));
	curMenus[_mainIndex].elements[1]=_endlessBlobButton;
	memcpy(_endlessBlobButton,_titleBlobButton,sizeof(struct uiButton));
	_endlessBlobButton->arg2=2;
	// sortman endless button
	struct uiButton* _sortmanButton = malloc(sizeof(struct uiButton));
	curMenus[_mainIndex].elements[2]=_sortmanButton;
	memcpy(_sortmanButton,_titleBlobButton,sizeof(struct uiButton));
	_sortmanButton->arg2=3;
	// blob settings button
	struct uiButton* _titleBlobSettings = malloc(sizeof(struct uiButton));
	memcpy(_titleBlobSettings,_titleBlobButton,sizeof(struct uiButton));
	curMenus[_mainIndex].elements[3]=_titleBlobSettings;
	_titleBlobSettings->images[0] = _optionsNorm;
	_titleBlobSettings->images[1] = _optionsHover;
	_titleBlobSettings->images[2] = _optionsClick;
	_titleBlobSettings->arg2=4;
	// sortman downstack button
	struct uiButton* _sortmanDownstack = malloc(sizeof(struct uiButton));
	curMenus[_mainIndex].elements[4]=_sortmanDownstack;
	memcpy(_sortmanDownstack,_sortmanButton,sizeof(struct uiButton));
	_sortmanDownstack->arg2=5;
	// blob settings button
	struct uiButton* _titleYoshiSettings = malloc(sizeof(struct uiButton));
	memcpy(_titleYoshiSettings,_titleBlobSettings,sizeof(struct uiButton));
	curMenus[_mainIndex].elements[5]=_titleYoshiSettings;
	_titleYoshiSettings->arg2=6;
	
	curMenus[_mainIndex].types[0]=UIELEM_BUTTON;
	curMenus[_mainIndex].types[1]=UIELEM_BUTTON;
	curMenus[_mainIndex].types[2]=UIELEM_BUTTON;
	curMenus[_mainIndex].types[3]=UIELEM_BUTTON;
	curMenus[_mainIndex].types[4]=UIELEM_BUTTON;	
	curMenus[_mainIndex].types[5]=UIELEM_BUTTON;

	int _squareButWidth;

	setClearColor(150,255,150); // cute bg
	setDown(BUTTON_RESIZE); // Queue button position fix
	while(1){
		u64 _sTime = goodGetMilli();
		controlsStart();
		if (wasIsDown(BUTTON_RESIZE)){
			screenWidth=getScreenWidth();
			screenHeight=getScreenHeight();
			int _newButH = USUALBUTTONH;
			int _newButW = getOtherScaled(getTextureHeight(_butNorm),_newButH,getTextureWidth(_butNorm));
			_squareButWidth = getOtherScaled(getTextureHeight(_optionsNorm),_newButH,getTextureWidth(_optionsNorm));

			_titleBlobButton->w = _newButW;
			_titleBlobButton->h = _newButH;
			_sortmanButton->w = _newButW;
			_sortmanButton->h = _newButH;
			_endlessBlobButton->w = _newButW;
			_endlessBlobButton->h = _newButH;
			_sortmanDownstack->w = _newButW;
			_sortmanDownstack->h = _newButH;

			// these buttons take up 66% of the screen.
			// in the middle of that 66%, stack them up with 1/5 of their high between the buttons
			int _separation = _newButH*(STDBUTTONSEPARATION);
			int _curY = screenHeight*.33+easyCenter(NUMMAINTITLEBUTTONS*_separation,screenHeight*.66);
			_titleBlobButton->x = easyCenter(_newButW,screenWidth);
			_titleBlobButton->y = _curY;
			_endlessBlobButton->x=_titleBlobButton->x;
			_endlessBlobButton->y=_curY+_separation;
			_sortmanButton->x=_titleBlobButton->x;
			_sortmanButton->y=_curY+_separation*2;
			_sortmanDownstack->x = _titleBlobButton->x;
			_sortmanDownstack->y=_curY+_separation*3;
			
			_titleBlobSettings->x=_titleBlobButton->x+_titleBlobButton->w*1.2;
			_titleBlobSettings->y=_titleBlobButton->y;
			_titleBlobSettings->w=_squareButWidth;
			_titleBlobSettings->h=_newButH;

			_titleYoshiSettings->w=_titleBlobSettings->w;
			_titleYoshiSettings->h=_titleBlobSettings->h;
			_titleYoshiSettings->x=_titleBlobSettings->x;
			_titleYoshiSettings->y=_sortmanButton->y;
		}
		_lastTitleButton=0;
		menuProcess();
		if (_lastTitleButton!=0){
			if (_lastTitleButton==1 || _lastTitleButton==2){
				*_ret = newGameState(_lastTitleButton==1 ? 2 : 1);
				loadGameSkin(BOARD_PUYO);
				addPuyoBoard(_ret,0,_puyoW,_puyoH,_puyoGhost,_puyoNext,&_curPuyoSettings,loadedSkins[BOARD_PUYO],0);
				if (_lastTitleButton==1){
					addPuyoBoard(_ret,1,_puyoW,_puyoH,_puyoGhost,_puyoNext,&_curPuyoSettings,loadedSkins[BOARD_PUYO],1);
					_ret->mode=MODE_BATTLE;
				}else{
					_ret->mode=MODE_ENDLESS;
				}
				break;
			}else if (_lastTitleButton==3 || _lastTitleButton==5){
				*_ret = newGameState(1);
				loadGameSkin(BOARD_YOSHI);
				addYoshiPlayer(_ret,5,6,&_curYoshiSettings,loadedSkins[BOARD_YOSHI]);
				if (_lastTitleButton==5){
					_ret->initializers[0]=yoshiInitLevelMode;
					_ret->initializerInfo[0] = malloc(sizeof(int));
					*((int*)_ret->initializerInfo[0])=_yoshiLevel;
				}else{
					_ret->mode=MODE_ENDLESS;
				}
				break;
			}else if (_lastTitleButton==4){
				void** _optionNums = malloc(sizeof(void*)*NUMBLOBOPTIONS);
				_optionNums[0]=&_puyoW;
				_optionNums[1]=&_puyoH;
				_optionNums[2]=&_puyoGhost;
				_optionNums[3]=&_curPuyoSettings.pointsPerGar;
				_optionNums[4]=&_curPuyoSettings.numColors;
				_optionNums[5]=&_curPuyoSettings.minPopNum;
				_optionNums[6]=&_curPuyoSettings.pushMultiplier;
				_optionNums[7]=&_curPuyoSettings.popTime;
				_optionNums[8]=&_curPuyoSettings.nextWindowTime;
				_optionNums[9]=&_curPuyoSettings.rotateTime;
				_optionNums[10]=&_curPuyoSettings.hMoveTime;
				_optionNums[11]=&_curPuyoSettings.fallTime;
				_optionNums[12]=&_curPuyoSettings.postSquishDelay;
				_optionNums[13]=&_curPuyoSettings.maxGarbageRows;
				_optionNums[14]=&_curPuyoSettings.squishTime;
				_optionNums[15]=&_puyoNext;
				char* _optionNumTypes = calloc(1,sizeof(char)*NUMBLOBOPTIONS);
				_optionNumTypes[6]=1; // fast drop speed is double
				showOptionsMenu(NUMBLOBOPTIONS,BLOBOPTIONNAMES,_optionNums,_optionNumTypes,BLOBOPTIONMINS,BLOBOPTIONMAX,BLOBOPTIONINC,_plusNorm,_plusHover,_plusClick,_lessNorm,_lessHover,_lessClick,_sTime);
				free(_optionNums);
				free(_optionNumTypes);
			}else if (_lastTitleButton==6){
				void** _optionNums = malloc(sizeof(void*)*NUMYOSHIOPTIONS);
				_optionNums[0]=&_yoshiW;
				_optionNums[1]=&_yoshiH;
				_optionNums[2]=&_yoshiLevel;
				_optionNums[3]=&_curYoshiSettings.fallTime;
				_optionNums[4]=&_curYoshiSettings.rowTime;
				_optionNums[5]=&_curYoshiSettings.popTime;
				_optionNums[6]=&_curYoshiSettings.squishPerPiece;
				_optionNums[7]=&_curYoshiSettings.swapTime;
				_optionNums[8]=&_curYoshiSettings.pushMultiplier;
				char* _optionNumTypes = calloc(1,sizeof(char)*NUMYOSHIOPTIONS);
				_optionNumTypes[8]=1; // fast drop speed is double
				showOptionsMenu(NUMYOSHIOPTIONS,YOSHIOPTIONNAMES,_optionNums,_optionNumTypes,YOSHIOPTIONMINS,YOSHIOPTIONMAX,YOSHIOPTIONINC,_plusNorm,_plusHover,_plusClick,_lessNorm,_lessHover,_lessClick,_sTime);
				free(_optionNums);
				free(_optionNumTypes);
			}
		}
		if (wasJustPressed(BUTTON_BACK)){
			lowSetButtonState(BUTTON_BACK,0,0);
			delMenuScreen(2);
		}
		/*
		if (_curSettingsList!=NULL){
			if (uiListControls(_curSettingsList)){
				easyUiListRebuild(_curSettingsList,2); // TODO - This is wrong.
				curMenus[curScreenIndex].winW = _curSettingsList->w+stdCornerWidth*2;
			}
		}
		*/
		controlsEnd();
		startDrawing();

		int _logoW;
		int _logoH;
		fitInBox(getTextureWidth(_logoImg),getTextureHeight(_logoImg),screenWidth,screenHeight*.33,&_logoW,&_logoH);
		drawTextureSized(_logoImg,easyCenter(_logoW,screenWidth),easyCenter(_logoH,screenHeight*.33),_logoW,_logoH);

		menuDrawAll(_sTime);
		endDrawing();
	}
	if (_ret->numBoards!=0){
		endStateInit(_ret);
	}
	//if (_curSettingsList!=NULL){
	//	freeUiList(_curSettingsList,2);
	//}
	// Delete title button layer
	delMenuScreen(2);
	//
	controlsEnd();
	freeTexture(_logoImg); //
	freeTexture(_butNorm); //
	freeTexture(_butHover);
	freeTexture(_butClick);
	freeTexture(_optionsNorm); //
	freeTexture(_optionsHover);
	freeTexture(_optionsClick);
	freeTexture(_plusNorm); //
	freeTexture(_plusHover);
	freeTexture(_plusClick);
	freeTexture(_lessNorm); //
	freeTexture(_lessHover);
	freeTexture(_lessClick);
	setClearColor(0,0,0);
}
