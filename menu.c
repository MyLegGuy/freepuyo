#include <stdio.h>
#include <limits.h>
#include <goodbrew/config.h>
#include <goodbrew/images.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include "main.h"
// these 4 are needed for init puyo/yoshi functions
#include <goodbrew/base.h>
#include "puzzleGeneric.h"
#include "puyo.h"
#include "yoshi.h"
#include "skinLoader.h"
//
#include "ui.h"

struct intCrementInfo{
	int* target;
	int min;
	int max;
};
struct doubleIncrementInfo{
	double* target;
	double min;
	double max;
};

#define NUMBLOBOPTIONS 15
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
};
const int BLOBOPTIONMINS[NUMBLOBOPTIONS]={1,2,0,1,1,1,0,0,0,0,0,0,0,1,0};
const int BLOBOPTIONMAX[NUMBLOBOPTIONS]={SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,5,SHRT_MAX,20,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX,SHRT_MAX};
const double BLOBOPTIONINC[NUMBLOBOPTIONS]={1,1,1,10,1,1,.2,50,50,50,50,50,50,1,50};

#define NUMTITLEBUTTONS 2

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
	struct intCrementInfo* _passedInfo = _uncastInfo;
	*_passedInfo->target=*_passedInfo->target+_changeAmount;
	if (*_passedInfo->target<_passedInfo->min){
		*_passedInfo->target=_passedInfo->min;
	}else if (*_passedInfo->target>_passedInfo->max){
		*_passedInfo->target=_passedInfo->max;
	}
}
void buttonDoubleIncrement(void* _uncastInfo, double _changeAmount){
	struct doubleIncrementInfo* _passedInfo = _uncastInfo;
	*_passedInfo->target=*_passedInfo->target+_changeAmount;
	if (*_passedInfo->target<_passedInfo->min){
		*_passedInfo->target=_passedInfo->min;
	}else if (*_passedInfo->target>_passedInfo->max){
		*_passedInfo->target=_passedInfo->max;
	}
}
void buttonSetInt(void* _uncastInt, double _setVal){
	*((int*)_uncastInt)=_setVal;
}
// _numTypes is 1 for double, 0 for int
struct uiList* constructOptionsMenu(int _numOptions, const char** _labels, void** _nums, char* _numTypes, const int* _minNums, const int* _maxNums, const double* _incAmnts, crossTexture _plusNorm, crossTexture _plusHover, crossTexture _plusClick, crossTexture _lessNorm, crossTexture _lessHover, crossTexture _lessClick){
	struct uiList* _ret = newUiList(_numOptions,4,curFontHeight);
	int i;
	for (i=0;i<_numOptions;++i){
		struct uiLabel* _newNameLabel = malloc(sizeof(struct uiLabel));
		_newNameLabel->r=0;
		_newNameLabel->g=0;
		_newNameLabel->b=0;
		_newNameLabel->a=255;
		easyStaticPrintfArray(&_newNameLabel->format,_labels[i]);
		_ret->elements[0][i] = _newNameLabel;
		_ret->types[0][i] = UIELEM_LABEL;

		struct uiButton* _newPlusButton = malloc(sizeof(struct uiButton));
		_newPlusButton->images[0] = _plusNorm;
		_newPlusButton->images[1] = _plusHover;
		_newPlusButton->images[2] = _plusClick;
		_newPlusButton->pressStatus=0;
		_ret->elements[1][i] = _newPlusButton;
		_ret->types[1][i] = UIELEM_BUTTON;

		struct uiLabel* _newValLabel = malloc(sizeof(struct uiLabel));
		_newValLabel->r=0;
		_newValLabel->g=0;
		_newValLabel->b=0;
		_newValLabel->a=255;
		_ret->elements[2][i] = _newValLabel;
		_ret->types[2][i] = UIELEM_LABEL;

		struct uiButton* _newMinusButton = malloc(sizeof(struct uiButton));
		_newMinusButton->images[0] = _lessNorm;
		_newMinusButton->images[1] = _lessHover;
		_newMinusButton->images[2] = _lessClick;
		_newMinusButton->pressStatus=0;
		_ret->elements[3][i] = _newMinusButton;
		_ret->types[3][i] = UIELEM_BUTTON;

		if (_numTypes[i]==1){ // double
			easyDoublePrintfArray(&_newValLabel->format,_nums[i]); //
			struct doubleIncrementInfo* _incInfo = malloc(sizeof(struct doubleIncrementInfo)); //
			_newPlusButton->arg1 = _incInfo;
			_newMinusButton->arg1 = _incInfo;
			_incInfo->target = _nums[i];
			_incInfo->min=_minNums[i];
			_incInfo->max=_maxNums[i];
			_newPlusButton->onPress=buttonDoubleIncrement; //
			_newMinusButton->onPress=buttonDoubleIncrement;
		}else{ // int
			easyNumPrintfArray(&_newValLabel->format,_nums[i]);
			struct intCrementInfo* _incInfo = malloc(sizeof(struct intCrementInfo)); //
			_newPlusButton->arg1 = _incInfo;
			_newMinusButton->arg1 = _incInfo;
			_incInfo->target = _nums[i];
			_incInfo->min=_minNums[i];
			_incInfo->max=_maxNums[i];
			_newPlusButton->onPress=buttonIntIncrement; //
			_newMinusButton->onPress=buttonIntIncrement;
		}
		_newPlusButton->arg2=_incAmnts[i]; //
		_newMinusButton->arg2=_incAmnts[i]*-1;
	}
	return _ret;
}
//////////////////////////////////////////////////
void titleScreen(struct gameState* _ret){
	struct gameSettings _curPuyoSettings;
	int _puyoW=6;
	int _puyoH=12;
	int _puyoGhost=2;
	initPuyoSettings(&_curPuyoSettings);
	struct yoshiSettings _curYoshiSettings;
	initYoshiSettings(&_curYoshiSettings);
	
	stdWindow.middle = loadImageEmbedded("assets/ui/winm.png");
	stdWindow.corner[0] = loadImageEmbedded("assets/ui/winc1.png"); //
	stdWindow.corner[1] = loadImageEmbedded("assets/ui/winc2.png");
	stdWindow.corner[2] = loadImageEmbedded("assets/ui/winc3.png");
	stdWindow.corner[3] = loadImageEmbedded("assets/ui/winc4.png");
	stdWindow.edge[0] = loadImageEmbedded("assets/ui/wine1.png"); //
	stdWindow.edge[1] = loadImageEmbedded("assets/ui/wine2.png");
	stdWindow.edge[2] = loadImageEmbedded("assets/ui/wine3.png");
	stdWindow.edge[3] = loadImageEmbedded("assets/ui/wine4.png");
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
	addMenuScreen(3);
	int _mainIndex = curScreenIndex;
	// blob button
	struct uiButton* _titleBlobButton = malloc(sizeof(struct uiButton));
	curMenus[_mainIndex].elements[0]=_titleBlobButton;
	_titleBlobButton->images[0] = _butNorm;
	_titleBlobButton->images[1] = _butHover;
	_titleBlobButton->images[2] = _butClick;
	_titleBlobButton->onPress=buttonSetInt;
	_titleBlobButton->arg1=&_lastTitleButton;
	_titleBlobButton->arg2=1;
	_titleBlobButton->pressStatus=0;
	// sortman button
	struct uiButton* _sortmanButton = malloc(sizeof(struct uiButton));
	curMenus[_mainIndex].elements[1]=_sortmanButton;
	memcpy(_sortmanButton,_titleBlobButton,sizeof(struct uiButton));
	_sortmanButton->arg2=2;
	// settings button
	struct uiButton* _titleBlobSettings = malloc(sizeof(struct uiButton));
	memcpy(_titleBlobSettings,_titleBlobButton,sizeof(struct uiButton));
	curMenus[_mainIndex].elements[2]=_titleBlobSettings;
	_titleBlobSettings->images[0] = _optionsNorm;
	_titleBlobSettings->images[1] = _optionsHover;
	_titleBlobSettings->images[2] = _optionsClick;
	_titleBlobSettings->arg2=3;

	curMenus[_mainIndex].types[0]=UIELEM_BUTTON;
	curMenus[_mainIndex].types[1]=UIELEM_BUTTON;
	curMenus[_mainIndex].types[2]=UIELEM_BUTTON;

	int _squareButWidth;

	setClearColor(150,255,150); // cute bg
	setDown(BUTTON_RESIZE); // Queue button position fix
	while(1){
		u64 _sTime = goodGetMilli();
		controlsStart();
		if (wasIsDown(BUTTON_RESIZE)){
			screenWidth=getScreenWidth();
			screenHeight=getScreenHeight();
			int _newButH = curFontHeight*1.3;
			int _newButW = getOtherScaled(getTextureHeight(_butNorm),_newButH,getTextureWidth(_butNorm));
			_squareButWidth = getOtherScaled(getTextureHeight(_optionsNorm),_newButH,getTextureWidth(_optionsNorm));

			_titleBlobButton->w = _newButW;
			_titleBlobButton->h = _newButH;
			_sortmanButton->w = _newButW;
			_sortmanButton->h = _newButH;

			// these buttons take up 66% of the screen.
			// in the middle of that 66%, stack them up with 1/5 of their high between the buttons
			int _separation = _newButH*1.2;
			int _curY = screenHeight*.33+easyCenter(NUMTITLEBUTTONS*_separation,screenHeight*.66);
			_titleBlobButton->x = easyCenter(_newButW,screenWidth);
			_titleBlobButton->y = _curY;
			_sortmanButton->x=_titleBlobButton->x;
			_sortmanButton->y=_curY+_separation;

			_titleBlobSettings->x=_titleBlobButton->x+_titleBlobButton->w*1.2;
			_titleBlobSettings->y=_titleBlobButton->y;
			_titleBlobSettings->w=_squareButWidth;
			_titleBlobSettings->h=_newButH;
		}
		_lastTitleButton=0;
		menuProcess();
		if (_lastTitleButton!=0){
			if (_lastTitleButton==1){
				*_ret = newGameState(2);
				struct puyoSkin* _newSkin = malloc(sizeof(struct puyoSkin));
				*_newSkin = loadChampionsSkinFile(loadImageEmbedded("freepuyo.png"));
				addPuyoBoard(_ret,0,_puyoW,_puyoH,_puyoGhost,&_curPuyoSettings,_newSkin,0);
				addPuyoBoard(_ret,1,_puyoW,_puyoH,_puyoGhost,&_curPuyoSettings,_newSkin,1);
				break;
			}else if (_lastTitleButton==2){
				*_ret = newGameState(1);
				struct yoshiSkin* _newYoshiSkin = malloc(sizeof(struct yoshiSkin));
				loadYoshiSkin(_newYoshiSkin,"assets/Crates/yoshiSheet.png");
				addYoshiPlayer(_ret,5,6,&_curYoshiSettings,_newYoshiSkin);
				break;
			}else if (_lastTitleButton==3){
				// test list make
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
				char* _optionNumTypes = calloc(1,sizeof(char)*NUMBLOBOPTIONS);
				_optionNumTypes[6]=1; // fast drop speed is double
				struct uiList* _newSettingsList = constructOptionsMenu(NUMBLOBOPTIONS,BLOBOPTIONNAMES,_optionNums,_optionNumTypes,BLOBOPTIONMINS,BLOBOPTIONMAX,BLOBOPTIONINC,_plusNorm,_plusHover,_plusClick,_lessNorm,_lessHover,_lessClick);
				free(_optionNums);
				free(_optionNumTypes);
				uiListCalcSizes(_newSettingsList,0);
				uiListPos(_newSettingsList,easyCenter(_newSettingsList->w,screenWidth),easyCenter(_newSettingsList->h,screenHeight),0);
				// window
				addMenuScreen(1);
				curMenus[curScreenIndex].winW = _newSettingsList->w+stdCornerWidth*2;
				curMenus[curScreenIndex].winH = _newSettingsList->h+stdCornerHeight*2;
				curMenus[curScreenIndex].elements[0]=_newSettingsList;
				curMenus[curScreenIndex].types[0]=UIELEM_LIST;
				windowPopupEnd=_sTime+WINDOWPOPUPTIME;
			}
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
