/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef UIHEADERINCLUDED
#define UIHEADERINCLUDED
#include <goodbrew/config.h>
#include <goodbrew/images.h>
#include "arrayPrintf.h"

#define WINDOWPOPUPTIME fixTime(500)

typedef void(*uiFunc)(void*,double);
typedef enum{
	UIELEM_NONE,
	UIELEM_BUTTON,
	UIELEM_LABEL,
	UIELEM_LIST,
}uiElemType;
struct uiButton{ // no extra free other than images (risky) required
	crossTexture images[3];
	int x;
	int y;
	int w;
	int h;
	char pressStatus; // 1 - hover. 2 - just released, trigger function next frame
	uiFunc onPress;
	void* arg1;
	double arg2;
};
struct windowImg{
	crossTexture corner[4];
	crossTexture edge[4];
	crossTexture middle;
};
// ui elements arranged in a list
struct uiList{ // extra free (safe) required
	int rows;
	int cols;
	int rowH;
	void*** elements; // accessed in col, row order
	uiElemType** types;
	int* cachedWidths;
	int w;
	int h;
	int x;
	int y;
};
struct uiLabel{ // extra free (safe) required
	struct printfArray format;
	int x;
	int y;
	unsigned char r;
	unsigned char g;
	unsigned char b;
};
struct menuScreen{
	int winW;
	int winH;
	void** elements;
	uiElemType* types;
	int numElements;
	char* title;
	void* extraData;
};

extern struct windowImg stdWindow;
extern signed char curScreenIndex;
extern struct menuScreen* curMenus;
extern u64 windowPopupEnd;
extern int stdCornerHeight;
extern int stdCornerWidth;
extern crossTexture ribbonImg;

int accumulateArray(int* _passedArray, int _numAccumulate);
char checkButton(struct uiButton* _drawThis);
void clickButtonDown(struct uiButton* _clickThis);
void clickButtonUp(struct uiButton* _clickThis);
void drawButton(struct uiButton* _drawThis, unsigned char a);
void drawUiElem(void* _passedElem, uiElemType _passedType, unsigned char a);
void drawUiLabel(struct uiLabel* _passed, unsigned char a);
void drawUiList(struct uiList* _passed, unsigned char a);
void drawWindow(struct windowImg* _img, int _x, int _y, int _w, int _h, int _cornerHeight);
void freeUiList(struct uiList* _freeThis, char _freeContentLevel);
int getCornerWidth(struct windowImg* _img, int _cornerHeight);
struct uiList* newUiList(int _rows, int _cols, int _rowHeight);
void setUiPos(void* _passedElem, uiElemType _passedType, int _passedX, int _passedY);
void uiListCalcSizes(struct uiList* _passed, int _startCol);
void uiListPos(struct uiList* _passed, int _x, int _y, int _startCol);
void fitUiElemHeight(void* _passedElem, uiElemType _passedType, int _passedHeight);
void freeRiskyUiData(void* _passedElem, uiElemType _passedType);
void freeSafeUiData(void* _passedElem, uiElemType _passedType);
char uiListControls(struct uiList* _passed);
char uiElemControls(void* _passedElem, uiElemType _passedType);
void easyUiListRebuild(struct uiList* _passed, int _startCol);
void menuInit(int _cornerHeight);
void* addMenuScreen(int _numElements, int _extraMemorySize);
void menuProcess();
void menuDrawAll(u64 _sTime);
void freeUiElemLevel(void* _passedElem, uiElemType _passedType, int _passedLevel);
void delMenuScreen(int _elementFreeLevel);
void getWindowDrawInfo(struct menuScreen* _passed, double _windowRatio, int* _retX, int* _retY, int* _retWidth, int* _retHeight);
void drawPreciseWindow(struct windowImg* _img, int _x, int _y, int _w, int _h, int _size, unsigned char _sideMap);
void drawWindowRibbonLabeled(int _x, int _y, int _windowW, int _cornerHeight, const char* _labelMessage);
struct uiButton* newButton();
void initButton(struct uiButton* _passed);

#endif
