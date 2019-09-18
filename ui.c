/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <goodbrew/config.h>
#include "main.h"
#include "ui.h"
#include <goodbrew/controls.h>
#include <goodbrew/text.h>

#define UILISTVPAD(_passedList) (_passedList->rowH/4)
#define UILISTHPAD(_passedList) (_passedList->rowH)
//
int accumulateArray(int* _passedArray, int _numAccumulate){
	int _ret=0;
	int i;
	for (i=0;i<_numAccumulate;++i){
		_ret+=_passedArray[i];
	}
	return _ret;
}
//
u64 windowPopupEnd=0;
struct menuScreen* curMenus=NULL;
struct windowImg stdWindow;
int stdCornerHeight;
int stdCornerWidth;
signed char curScreenIndex=-1;
void addMenuScreen(int _numElements){
	++curScreenIndex;
	curMenus = realloc(curMenus,sizeof(struct menuScreen)*(curScreenIndex+1));
	curMenus[curScreenIndex].winW=0;
	curMenus[curScreenIndex].elements = malloc(sizeof(void*)*_numElements);
	curMenus[curScreenIndex].types = malloc(sizeof(uiElemType)*_numElements);
	curMenus[curScreenIndex].numElements=_numElements;
}
void delMenuScreen(int _elementFreeLevel){
	int i;
	for (i=0;i<curMenus[curScreenIndex].numElements;++i){
		freeUiElemLevel(curMenus[curScreenIndex].elements[i],curMenus[curScreenIndex].types[i],_elementFreeLevel);
	}
	free(curMenus[curScreenIndex].elements);
	free(curMenus[curScreenIndex].types);
	curMenus = realloc(curMenus,sizeof(struct menuScreen)*(curScreenIndex)); // realloc before changing variable because variable represents index
	--curScreenIndex;
}
void getWindowDrawInfo(struct menuScreen* _passed, double _windowRatio, int* _retX, int* _retY, int* _retWidth, int* _retHeight){
	*_retWidth=stdCornerWidth*2;
	*_retHeight=stdCornerHeight*2;
	if (_windowRatio!=1){
		*_retWidth+=_passed->winW*_windowRatio;
		*_retHeight+=_passed->winH*_windowRatio;
	}else{
		*_retWidth+=_passed->winW;
		*_retHeight+=_passed->winH;
	}
	*_retX=easyCenter(*_retWidth,screenWidth);
	*_retY=easyCenter(*_retHeight,screenHeight);
}
void drawOneMenu(struct menuScreen* _passed, double _windowRatio){
	if (_passed->winW>0){
		int _destX;
		int _destY;
		int _destWidth;
		int _destHeight;
		getWindowDrawInfo(_passed,_windowRatio,&_destX,&_destY,&_destWidth,&_destHeight);
		drawWindow(&stdWindow,_destX,_destY,_destWidth,_destHeight,curFontHeight);
	}
	int j;
	for (j=0;j<_passed->numElements;++j){
		drawUiElem(_passed->elements[j],_passed->types[j]);
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
	for (j=0;j<_passed->numElements;++j){
		uiElemControls(_passed->elements[j],_passed->types[j]);
	}
}
void menuProcess(){
	checkScreenButtons(&curMenus[curScreenIndex]);
}
void menuInit(int _cornerHeight){
	stdCornerHeight = _cornerHeight;
	stdCornerWidth = getCornerWidth(&stdWindow,_cornerHeight);
}
//
void drawButton(struct uiButton* _drawThis){
	drawTextureSized(_drawThis->images[_drawThis->pressStatus],_drawThis->x,_drawThis->y,_drawThis->w,_drawThis->h);
}
char checkButton(struct uiButton* _drawThis){
	if (_drawThis->pressStatus==2){
		if (_drawThis->onPress){
			_drawThis->onPress(_drawThis->arg1,_drawThis->arg2);
		}
		_drawThis->pressStatus=0;
		return 1;
	}	
	if (isDown(BUTTON_TOUCH)){
		_drawThis->pressStatus=touchIn(touchX,touchY,_drawThis->x,_drawThis->y,_drawThis->w,_drawThis->h);
	}else if (wasJustReleased(BUTTON_TOUCH)){
		_drawThis->pressStatus = touchIn(touchX,touchY,_drawThis->x,_drawThis->y,_drawThis->w,_drawThis->h) ? 2 : 0;
	}
	return 0;
}
void clickButtonDown(struct uiButton* _clickThis){
	_clickThis->pressStatus=1;
}
void clickButtonUp(struct uiButton* _clickThis){
	_clickThis->pressStatus=2;
}
//
int getCornerWidth(struct windowImg* _img, int _cornerHeight){
	return getOtherScaled(getTextureHeight(_img->corner[0]),_cornerHeight,getTextureWidth(_img->corner[0]));
}
void drawWindow(struct windowImg* _img, int _x, int _y, int _w, int _h, int _cornerHeight){
	int _cornerWidth = getCornerWidth(_img,_cornerHeight);
	drawTextureSized(_img->corner[0],_x,_y,_cornerWidth,_cornerHeight); // top left corner
	drawTextureSized(_img->corner[1],_x+_w-_cornerWidth,_y,_cornerWidth,_cornerHeight); // top right corner
	drawTextureSized(_img->corner[2],_x,_y+_h-_cornerHeight,_cornerWidth,_cornerHeight); // bottom left corner
	drawTextureSized(_img->corner[3],_x+_w-_cornerWidth,_y+_h-_cornerHeight,_cornerWidth,_cornerHeight); // bottom right corner

	drawTextureSized(_img->edge[0],_x+_cornerWidth,_y,_w-_cornerWidth*2,_cornerHeight); // top
	drawTextureSized(_img->edge[1],_x+_cornerWidth,_y+_h-_cornerHeight,_w-_cornerWidth*2,_cornerHeight); // bottom
	drawTextureSized(_img->edge[2],_x,_y+_cornerHeight,_cornerWidth,_h-_cornerHeight*2); // left
	drawTextureSized(_img->edge[3],_x+_w-_cornerWidth,_y+_cornerHeight,_cornerWidth,_h-_cornerHeight*2); // right

	drawTextureSized(_img->middle,_x+_cornerWidth,_y+_cornerHeight,_w-_cornerWidth*2,_h-_cornerHeight*2); // middle
}
//
struct uiList* newUiList(int _rows, int _cols, int _rowHeight){
	struct uiList* _ret = malloc(sizeof(struct uiList));
	_ret->rows=_rows;
	_ret->cols=_cols;
	_ret->rowH=_rowHeight;
	_ret->cachedWidths = malloc(sizeof(int)*_cols);
	_ret->elements = malloc(sizeof(void**)*_cols);
	_ret->types = malloc(sizeof(uiElemType*)*_cols);
	int i;
	for (i=0;i<_cols;++i){
		_ret->elements[i] = malloc(sizeof(void*)*_rows);
		_ret->types[i] = malloc(sizeof(uiElemType)*_rows);
	}
	_ret->h=_rows*_rowHeight+(_rows-1)*UILISTVPAD(_ret);
	return _ret;
}
void freeUiList(struct uiList* _freeThis, char _freeContentLevel){
	int i;
	for (i=0;i<_freeThis->cols;++i){
		if (_freeContentLevel){
			int j;
			for (j=0;j<_freeThis->rows;++j){
				freeUiElemLevel(_freeThis->elements[i][j],_freeThis->types[i][j],_freeContentLevel);
			}
		}
		free(_freeThis->elements[i]);
		free(_freeThis->types[i]);
	}
	free(_freeThis->cachedWidths);
	free(_freeThis->elements);
	free(_freeThis->types);
}
void uiListCalcSizes(struct uiList* _passed, int _startCol){
	int i;
	for (i=_startCol;i<_passed->cols;++i){
		int _maxWidth=0;
		int j;
		for (j=0;j<_passed->rows;++j){
			fitUiElemHeight(_passed->elements[i][j],_passed->types[i][j],_passed->rowH);
			int _newWidth;
			switch(_passed->types[i][j]){
				case UIELEM_BUTTON:
					_newWidth=((struct uiButton*)_passed->elements[i][j])->w;
					break;
				case UIELEM_LABEL:
					printfArrayCalculate(&((struct uiLabel*)_passed->elements[i][j])->format);
					_newWidth = textWidth(regularFont,((struct uiLabel*)_passed->elements[i][j])->format.res);
					break;
				case UIELEM_NONE:
					continue;
			}
			if (_newWidth>_maxWidth){
				_maxWidth=_newWidth;
			}
		}
		_passed->cachedWidths[i]=_maxWidth;
	}
	// element full width
	_passed->w=accumulateArray(_passed->cachedWidths,_passed->cols)+UILISTHPAD(_passed)*(_passed->cols-1);
}
void drawUiList(struct uiList* _passed){
	int i;
	for (i=0;i<_passed->cols;++i){
		int j;
		for (j=0;j<_passed->rows;++j){
			drawUiElem(_passed->elements[i][j],_passed->types[i][j]);
		}
	}
}
void uiListPos(struct uiList* _passed, int _x, int _y, int _startCol){
	_passed->x=_x;
	_passed->y=_y;
	int _destX=_x+(_startCol!=0 ? (accumulateArray(_passed->cachedWidths,_startCol)+UILISTHPAD(_passed)*_startCol) : 0);
	int i;
	for (i=_startCol;i<_passed->cols;++i){
		int j;
		for (j=0;j<_passed->rows;++j){
			int _destY = _y+(j*_passed->rowH+UILISTVPAD(_passed)*j);
			setUiPos(_passed->elements[i][j],_passed->types[i][j],_destX,_destY);
		}
		_destX+=_passed->cachedWidths[i]+UILISTHPAD(_passed);
	}
}
char uiListControls(struct uiList* _passed){
	char _ret=0;
	int i;
	for (i=0;i<_passed->cols;++i){
		int j;
		for (j=0;j<_passed->rows;++j){
			_ret|=uiElemControls(_passed->elements[i][j],_passed->types[i][j]);
		}
	}
	return _ret;
}
void easyUiListRebuild(struct uiList* _passed, int _startCol){
	uiListCalcSizes(_passed,_startCol);
	uiListPos(_passed,_passed->x,_passed->y,_startCol);
}
//
void drawUiLabel(struct uiLabel* _passed){
	printfArrayCalculate(&_passed->format);
	gbDrawTextAlpha(regularFont,_passed->x,_passed->y,_passed->format.res,_passed->r,_passed->g,_passed->b,_passed->a);
}
//
void setUiPos(void* _passedElem, uiElemType _passedType, int _passedX, int _passedY){
	switch(_passedType){
		case UIELEM_BUTTON:
			((struct uiButton*)_passedElem)->x = _passedX;
			((struct uiButton*)_passedElem)->y = _passedY;
			break;
		case UIELEM_LABEL:
			((struct uiLabel*)_passedElem)->x = _passedX;
			((struct uiLabel*)_passedElem)->y = _passedY;
			break;
		case UIELEM_LIST:
			uiListPos(_passedElem,_passedX,_passedY,0);
			break;
	}
}
void drawUiElem(void* _passedElem, uiElemType _passedType){
	switch(_passedType){
		case UIELEM_BUTTON:
			drawButton(_passedElem);
			break;
		case UIELEM_LABEL:
			drawUiLabel(_passedElem);
			break;
		case UIELEM_LIST:
			drawUiList(_passedElem);
			break;
	}
}
void fitUiElemHeight(void* _passedElem, uiElemType _passedType, int _passedHeight){
	switch(_passedType){
		case UIELEM_BUTTON: // only element we can control the height of
			((struct uiButton*)_passedElem)->h=_passedHeight;
			((struct uiButton*)_passedElem)->w=_passedHeight;
			break;
	}
}
void freeUiElemLevel(void* _passedElem, uiElemType _passedType, int _passedLevel){
	if (!_passedLevel){
		return;
	}
	switch(_passedLevel){
		case 2:
			freeSafeUiData(_passedElem,_passedType);
			break;
		case 3:
			freeRiskyUiData(_passedElem,_passedType);
			break;
	}
	free(_passedElem);
}
// run this last. frees data that is almost certianly safe to free.
void freeSafeUiData(void* _passedElem, uiElemType _passedType){
	switch(_passedType){
		case UIELEM_LABEL:
			freePrintfArray(&((struct uiLabel*)_passedElem)->format);
			break;
		case UIELEM_LIST:
			freeUiList(_passedElem,2);
			break;
	}
}
// frees all ui memory. some of it could be data that's used elsewhere if you made it that way
void freeRiskyUiData(void* _passedElem, uiElemType _passedType){
	switch(_passedType){
		case UIELEM_BUTTON:
			freeTexture(((struct uiButton*)_passedElem)->images[0]);
			freeTexture(((struct uiButton*)_passedElem)->images[1]);
			freeTexture(((struct uiButton*)_passedElem)->images[2]);
			break;
		case UIELEM_LIST:
			freeUiList(_passedElem,3);
			return;
	}
	freeSafeUiData(_passedElem,_passedType);
}
char uiElemControls(void* _passedElem, uiElemType _passedType){
	char _ret=0;
	switch(_passedType){
		case UIELEM_BUTTON:
			_ret|=checkButton(_passedElem);
			break;
		case UIELEM_LIST:
			_ret|=uiListControls(_passedElem);
			break;
	}
	return _ret;
}
