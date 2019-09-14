#include <stdio.h>
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

#define NUMBLOBOPTIONS 12
char* BLOBOPTIONNAMES[NUMBLOBOPTIONS]={
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

#define NUMTITLEBUTTONS 2
#define WINDOWPOPUPTIME 500

struct windowImg stdWindow;
struct menuScreen{
	int winW;
	int winH;
	struct uiButton* buttons;
	int numButtons;
};
u64 windowPopupEnd=0;
struct menuScreen* curMenus=NULL;
int stdCornerHeight;
int stdCornerWidth;
signed char curScreenIndex=-1;
int curPushedButton=0;
void addMenuScreen(int _numButtons){
	++curScreenIndex;
	curMenus = realloc(curMenus,sizeof(struct menuScreen)*(curScreenIndex+1));
	curMenus[curScreenIndex].winW=0;
	curMenus[curScreenIndex].buttons = malloc(sizeof(struct uiButton)*_numButtons);
	curMenus[curScreenIndex].numButtons=_numButtons;
}
void drawOneMenu(struct menuScreen* _passed, double _windowRatio){
	if (_passed->winW>0){
		int _destWidth;
		int _destHeight;
		if (_windowRatio!=1){
			_destWidth = stdCornerWidth*2+_passed->winW*_windowRatio;
			_destHeight = stdCornerHeight*2+_passed->winH*_windowRatio;
		}else{
			_destWidth = _passed->winW+stdCornerWidth*2;
			_destHeight = _passed->winH+stdCornerHeight*2;
		}
		drawWindow(&stdWindow,easyCenter(_destWidth,screenWidth),easyCenter(_destHeight,screenHeight),_destWidth,_destHeight,curFontHeight);
	}
	int j;
	for (j=0;j<_passed->numButtons;++j){
		drawButton(&(_passed->buttons[j]));
	}
}
void menuDrawTo(int _max){
	int i;
	for (i=0;i<=_max;++i){
		drawOneMenu(&curMenus[i],1);
	}
}
// Draw all screens except this one
void menuDrawPre(){
	menuDrawTo(curScreenIndex-1);
}
void menuDrawAll(u64 _sTime){
	if (windowPopupEnd==0){
		menuDrawTo(curScreenIndex);
	}else{
		if (_sTime>=windowPopupEnd){
			windowPopupEnd=0;
			menuDrawAll(_sTime);
			return;
		}
		menuDrawPre();
		drawOneMenu(&curMenus[curScreenIndex],1-(windowPopupEnd-_sTime)/(double)WINDOWPOPUPTIME);
	}
}
void checkScreenButtons(struct menuScreen* _passed){	
	int j;
	for (j=0;j<_passed->numButtons;++j){
		if (checkButton(&(_passed->buttons[j]))){
			curPushedButton=j+1;
			return;
		}
	}
	curPushedButton=0;
}
void menuProcess(){
	checkScreenButtons(&curMenus[curScreenIndex]);
}
void menuInit(int _cornerHeight){
	stdCornerHeight = _cornerHeight;
	stdCornerWidth = getCornerWidth(&stdWindow,_cornerHeight);
}
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
void menuChangeInt(void* _passedInt, int _changeAmount){
	*((int*)_passedInt)=*((int*)_passedInt)+_changeAmount;
}
//////////////////////////////////////////////////
void titleScreen(struct gameState* _ret){
	struct gameSettings _curPuyoSettings;
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
	
	addMenuScreen(3);
	
	int _mainIndex = curScreenIndex;
	// blob button
	curMenus[_mainIndex].buttons[0].images[0] = _butNorm;
	curMenus[_mainIndex].buttons[0].images[1] = _butHover;
	curMenus[_mainIndex].buttons[0].images[2] = _butClick;
	curMenus[_mainIndex].buttons[0].onPress=NULL;
	curMenus[_mainIndex].buttons[0].pressStatus=0;
	// sortman button
	memcpy(&curMenus[_mainIndex].buttons[1],&curMenus[_mainIndex].buttons[0],sizeof(struct uiButton));
	// settings button
	memcpy(&curMenus[_mainIndex].buttons[2],&curMenus[_mainIndex].buttons[0],sizeof(struct uiButton));
	curMenus[_mainIndex].buttons[2].images[0] = _optionsNorm;
	curMenus[_mainIndex].buttons[2].images[1] = _optionsHover;
	curMenus[_mainIndex].buttons[2].images[2] = _optionsClick;

	int _squareButWidth;

	struct uiList* _curSettingsList=NULL;

	int testint=5;
	
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
			
			curMenus[_mainIndex].buttons[0].w = _newButW;
			curMenus[_mainIndex].buttons[0].h = _newButH;
			curMenus[_mainIndex].buttons[1].w = _newButW;
			curMenus[_mainIndex].buttons[1].h = _newButH;

			// these buttons take up 66% of the screen.
			// in the middle of that 66%, stack them up with 1/5 of their high between the buttons
			int _separation = _newButH*1.2;
			int _curY = screenHeight*.33+easyCenter(NUMTITLEBUTTONS*_separation,screenHeight*.66);
			curMenus[_mainIndex].buttons[0].x = easyCenter(_newButW,screenWidth);
			curMenus[_mainIndex].buttons[0].y = _curY;
			curMenus[_mainIndex].buttons[1].x=curMenus[_mainIndex].buttons[0].x;
			curMenus[_mainIndex].buttons[1].y=_curY+_separation;

			curMenus[_mainIndex].buttons[2].x=curMenus[_mainIndex].buttons[0].x+curMenus[_mainIndex].buttons[0].w*1.2;
			curMenus[_mainIndex].buttons[2].y=curMenus[_mainIndex].buttons[0].y;
			curMenus[_mainIndex].buttons[2].w=_squareButWidth;
			curMenus[_mainIndex].buttons[2].h=_newButH;
		}
		menuProcess();
		if (curPushedButton!=0){
			if (curPushedButton==1){
				*_ret = newGameState(2);
				struct puyoSkin* _newSkin = malloc(sizeof(struct puyoSkin));
				*_newSkin = loadChampionsSkinFile(loadImageEmbedded("freepuyo.png"));
				addPuyoBoard(_ret,0,6,12,2,&_curPuyoSettings,_newSkin,0);
				addPuyoBoard(_ret,1,6,12,2,&_curPuyoSettings,_newSkin,1);
				break;
			}else if (curPushedButton==2){
				*_ret = newGameState(1);
				struct yoshiSkin* _newYoshiSkin = malloc(sizeof(struct yoshiSkin));
				loadYoshiSkin(_newYoshiSkin,"assets/Crates/yoshiSheet.png");
				addYoshiPlayer(_ret,5,6,&_curYoshiSettings,_newYoshiSkin);
				break;
			}else if (curPushedButton==3){
				// test list make
				_curSettingsList = newUiList(NUMBLOBOPTIONS,4,curFontHeight);
				int i;
				for (i=0;i<NUMBLOBOPTIONS;++i){
					struct uiLabel* _newNameLabel = malloc(sizeof(struct uiLabel));
					_newNameLabel->r=0;
					_newNameLabel->g=0;
					_newNameLabel->b=0;
					_newNameLabel->a=255;
					easyStaticPrintfArray(&_newNameLabel->format,BLOBOPTIONNAMES[i]);
					_curSettingsList->elements[0][i] = _newNameLabel;
					_curSettingsList->types[0][i] = UIELEM_LABEL;

					struct uiButton* _newPlusButton = malloc(sizeof(struct uiButton));
					_newPlusButton->images[0] = _plusNorm;
					_newPlusButton->images[1] = _plusHover;
					_newPlusButton->images[2] = _plusClick;
					_newPlusButton->onPress=menuChangeInt;
					_newPlusButton->arg2=1;
					_newPlusButton->pressStatus=0;
					_curSettingsList->elements[1][i] = _newPlusButton;
					_curSettingsList->types[1][i] = UIELEM_BUTTON;

					struct uiLabel* _newValLabel = malloc(sizeof(struct uiLabel));
					_newValLabel->r=0;
					_newValLabel->g=0;
					_newValLabel->b=0;
					_newValLabel->a=255;
					_curSettingsList->elements[2][i] = _newValLabel;
					_curSettingsList->types[2][i] = UIELEM_LABEL;
					
					struct uiButton* _newMinusButton = malloc(sizeof(struct uiButton));
					_newMinusButton->images[0] = _lessNorm;
					_newMinusButton->images[1] = _lessHover;
					_newMinusButton->images[2] = _lessClick;
					_newMinusButton->onPress=menuChangeInt;
					_newMinusButton->arg2=-1;
					_newMinusButton->pressStatus=0;
					_curSettingsList->elements[3][i] = _newMinusButton;
					_curSettingsList->types[3][i] = UIELEM_BUTTON;


					easyNumPrintfArray(&_newValLabel->format,&testint);
					_newPlusButton->arg1=&testint;
					_newMinusButton->arg1=&testint;
				}
				uiListCalcSizes(_curSettingsList,0);
				uiListPos(_curSettingsList,easyCenter(_curSettingsList->w,screenWidth),easyCenter(_curSettingsList->h,screenHeight),0);
				// window
				addMenuScreen(0);
				curMenus[curScreenIndex].winW = _curSettingsList->w+stdCornerWidth*2;
				curMenus[curScreenIndex].winH = _curSettingsList->h+stdCornerHeight*2;
				windowPopupEnd=_sTime+WINDOWPOPUPTIME;
			}
		}
		if (_curSettingsList!=NULL){
			if (uiListControls(_curSettingsList)){
				easyUiListRebuild(_curSettingsList,2);
				curMenus[curScreenIndex].winW = _curSettingsList->w+stdCornerWidth*2;
			}
		}
		controlsEnd();
		startDrawing();

		int _logoW;
		int _logoH;
		fitInBox(getTextureWidth(_logoImg),getTextureHeight(_logoImg),screenWidth,screenHeight*.33,&_logoW,&_logoH);		
		drawTextureSized(_logoImg,easyCenter(_logoW,screenWidth),easyCenter(_logoH,screenHeight*.33),_logoW,_logoH);
		
		menuDrawAll(_sTime);
		
		if (_curSettingsList!=NULL){
			drawUiList(_curSettingsList);
		}
		
		endDrawing();
	}
	if (_ret->numBoards!=0){
		endStateInit(_ret);
	}
	if (_curSettingsList!=NULL){
		freeUiList(_curSettingsList,2);
	}
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
