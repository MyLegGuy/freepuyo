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
//
#include "ui.h"

#define NUMBLOBOPTIONS 3
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
//////////////////////////////////////////////////
void titleScreen(struct gameState* _ret){
	stdWindow.middle = loadImageEmbedded("./assets/ui/winm.png");
	stdWindow.corner[0] = loadImageEmbedded("./assets/ui/winc1.png"); //
	stdWindow.corner[1] = loadImageEmbedded("./assets/ui/winc2.png");
	stdWindow.corner[2] = loadImageEmbedded("./assets/ui/winc3.png");
	stdWindow.corner[3] = loadImageEmbedded("./assets/ui/winc4.png");	
	stdWindow.edge[0] = loadImageEmbedded("./assets/ui/wine1.png"); //
	stdWindow.edge[1] = loadImageEmbedded("./assets/ui/wine2.png");
	stdWindow.edge[2] = loadImageEmbedded("./assets/ui/wine3.png");
	stdWindow.edge[3] = loadImageEmbedded("./assets/ui/wine4.png");
	menuInit(curFontHeight); // must init after window
	
	crossTexture _logoImg = loadImageEmbedded("./assets/ui/logo.png");

	crossTexture _butNorm = loadImageEmbedded("./assets/ui/but.png");
	crossTexture _butHover = loadImageEmbedded("./assets/ui/butHover.png");
	crossTexture _butClick = loadImageEmbedded("./assets/ui/butClick.png");

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

	u64 _popupEndTime=0;

	setClearColor(150,255,150);
	setDown(BUTTON_RESIZE); // Queue button position fix
	while(1){
		u64 _sTime = goodGetMilli();
		controlsStart();
		if (wasIsDown(BUTTON_RESIZE)){
			screenWidth=getScreenWidth();
			screenHeight=getScreenHeight();
			int _newButH = curFontHeight*1.3;
			int _newButW = getOtherScaled(getTextureHeight(_butNorm),_newButH,getTextureWidth(_butNorm));
			
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

			curMenus[_mainIndex].buttons[2].x=curMenus[_mainIndex].buttons[0].x+curMenus[_mainIndex].buttons[0].w*1.5;
			curMenus[_mainIndex].buttons[2].y=curMenus[_mainIndex].buttons[0].y;
			curMenus[_mainIndex].buttons[2].w=100;
			curMenus[_mainIndex].buttons[2].h=32;
		}
		menuProcess();
		if (curPushedButton!=0){
			if (curPushedButton==1){
				*_ret = newGameState(2);
				initPuyo(_ret);
				break;
			}else if (curPushedButton==2){
				*_ret = newGameState(1);
				initYoshi(_ret);
				break;
			}else if (curPushedButton==3){
				addMenuScreen(0);
				curMenus[curScreenIndex].winW = 1000;
				curMenus[curScreenIndex].winH = curFontHeight*NUMBLOBOPTIONS;
				windowPopupEnd=_sTime+WINDOWPOPUPTIME;
			}
		}
		controlsEnd();
		startDrawing();

		if (_popupEndTime!=0){
			/*
			if (_sTime>_popupEndTime){
				// draw depending on submenu variable
			}else{
				int _destWidth =  getCornerWidth(&_usualWindow,curFontHeight)*2+partMoveFills(_sTime, _popupEndTime, WINDOWPOPUPTIME, 1000);
				int _destHeight = curFontHeight*2+partMoveFills(_sTime, _popupEndTime, WINDOWPOPUPTIME, curFontHeight*NUMBLOBOPTIONS); // must be at least corner size
				drawWindow(&_usualWindow, easyCenter(_destWidth,screenWidth), easyCenter(_destHeight,screenHeight), _destWidth, _destHeight, curFontHeight);
			}
			*/
		}

		menuDrawAll(_sTime);

		
		int _logoW;
		int _logoH;
		fitInBox(getTextureWidth(_logoImg),getTextureHeight(_logoImg),screenWidth,screenHeight*.33,&_logoW,&_logoH);		
		drawTextureSized(_logoImg,easyCenter(_logoW,screenWidth),easyCenter(_logoH,screenHeight*.33),_logoW,_logoH);
		
		endDrawing();
	}
	if (_ret->numBoards!=0){
		endStateInit(_ret);
	}
	controlsEnd();
	freeTexture(_logoImg);
	freeTexture(_butNorm);
	freeTexture(_butHover);
	freeTexture(_butClick);
	setClearColor(0,0,0);
}
