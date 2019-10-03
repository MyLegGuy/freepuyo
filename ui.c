/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
small one:
256 w taken
500x164
w:h ratio: 3.05139024390243902439

long one:
400w taken (1.5625 longer, causing 0.730549150036954915 ratio increase.)
500x132
w:h ratio: 3.78193939393939393939

standard height is cornerHeight*2, getting smaller the longer it is.
*/
#include <goodbrew/config.h>
#include "main.h"
#include "ui.h"
#include <goodbrew/controls.h>
#include <goodbrew/text.h>

#define UILISTVPAD(_passedList) (_passedList->rowH/4)
#define UILISTHPAD(_passedList) (_passedList->rowH)
//
// how much of the image is like the ends where text cant't be
#define UNUSABLERIBBONRATIOX 0.33715220949263502455
// padding for the text
#define EXTRARIBBONRATIOX .2
#define RIBBONCORNERRATIO 2
#define UNUSABLERIBBONRATIOY 0.385
#define RIBBONINCORNER 1
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
crossTexture ribbonImg;
int stdCornerHeight;
int stdCornerWidth;
signed char curScreenIndex=-1;
void* addMenuScreen(int _numElements, int _extraMemorySize){
	++curScreenIndex;
	curMenus = realloc(curMenus,sizeof(struct menuScreen)*(curScreenIndex+1));
	curMenus[curScreenIndex].winW=0;
	curMenus[curScreenIndex].elements = malloc(sizeof(void*)*_numElements);
	curMenus[curScreenIndex].types = malloc(sizeof(uiElemType)*_numElements);
	curMenus[curScreenIndex].numElements=_numElements;
	curMenus[curScreenIndex].title=NULL;
	return (curMenus[curScreenIndex].extraData = _extraMemorySize!=0 ? malloc(_extraMemorySize) : NULL);
}
void delMenuScreen(int _elementFreeLevel){
	free(curMenus[curScreenIndex].extraData);
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
	unsigned char a=255;
	if (_passed->winW>0){
		int _destX;
		int _destY;
		int _destWidth;
		int _destHeight;
		getWindowDrawInfo(_passed,_windowRatio,&_destX,&_destY,&_destWidth,&_destHeight);
		if (_windowRatio<1){
			a=(_windowRatio/(double)1)*255;
		}
		drawWindow(&stdWindow,_destX,_destY,_destWidth,_destHeight,curFontHeight);
		if (_passed->title!=NULL){
			drawWindowRibbonLabeled(_destX,_destY,_destWidth,curFontHeight,_passed->title);
		}
	}
	int j;
	for (j=0;j<_passed->numElements;++j){
		drawUiElem(_passed->elements[j],_passed->types[j],a);
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
void drawButton(struct uiButton* _drawThis, unsigned char a){
	drawTextureSizedAlpha(_drawThis->images[_drawThis->pressStatus],_drawThis->x,_drawThis->y,_drawThis->w,_drawThis->h,a);
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
// when the ribbon is made longer, its gets less height. this function uses magic ratios found manually looking at the images.
int getRibbonH(int _w, int _standardHeight){
	double _percentChange = _w/(_standardHeight*3.05139024390243902439);
	int _destHeight = _standardHeight+_percentChange*0.4675514560236511456;
	return _destHeight;
}
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
void drawWindowRibbonLabeled(int _x, int _y, int _windowW, int _cornerHeight, const char* _labelMessage){
	int _textW = textWidth(regularFont,_labelMessage);
	int _ribbonW = _textW*(3/(double)2)*(1+EXTRARIBBONRATIOX);
	int _destH = getRibbonH(_ribbonW,_cornerHeight*RIBBONCORNERRATIO);
	int _ribbonY=_y+_cornerHeight/RIBBONINCORNER-_destH;
	int _ribbonX=_x+easyCenter(_ribbonW,_windowW);
	drawTextureSized(ribbonImg,_ribbonX,_ribbonY,_ribbonW,_destH);
	gbDrawText(regularFont,_ribbonX+easyCenter(_textW,_ribbonW),_ribbonY+easyCenter(textHeight(regularFont),_destH*(1-UNUSABLERIBBONRATIOY)), _labelMessage, 0, 0, 0);
}
// window where the corner is height of edge piece and width of top piece at the same time.
// also repeats.
void drawPreciseWindow(struct windowImg* _img, int _x, int _y, int _w, int _h, int _size, unsigned char _sideMap){
	int _smallSizeBot = ceil(_size/(double)2);
	int _smallSizeTop = _size/2;
	if (_sideMap & 1){
		drawTextureSized(_img->corner[0],_x,_y,_size,_size); // top left corner
		drawTextureSized(_img->corner[1],_x+_w-_size,_y,_size,_size); // top right corner
	}
	if (_sideMap & 2){
		drawTextureSized(_img->corner[2],_x,_y+_h-_size,_size,_size); // bottom left corner
		drawTextureSized(_img->corner[3],_x+_w-_size,_y+_h-_size,_size,_size); // bottom right corner
	}
	int i;
	int _remainingW=_w-_size*2;
	if (_remainingW>0){
		int _bottomY=_y+_h-_smallSizeBot;
		for (i=1;_remainingW>=_size;++i){
			if (_sideMap & 1){
				drawTextureSized(_img->edge[0],_x+_size*i,_y,_size,_smallSizeTop);
			}
			if (_sideMap & 8){
				drawTextureSized(_img->edge[1],_x+_size*i,_bottomY,_size,_smallSizeBot);
			}
			_remainingW-=_size;
		}
		if (_remainingW!=0){
			double _leftRatio = _remainingW/(double)_size;
			if (_sideMap & 1){
				drawTexturePartSized(_img->edge[0],_x+_size*i,_y,_remainingW,_smallSizeTop,0,0,getTextureWidth(_img->edge[0])*_leftRatio,getTextureHeight(_img->edge[0]));
			}
			if (_sideMap & 8){
				drawTexturePartSized(_img->edge[1],_x+_size*i,_bottomY,_remainingW,_smallSizeBot,0,0,getTextureWidth(_img->edge[1])*_leftRatio,getTextureHeight(_img->edge[1]));
			}
		}
	}

	int _remainingH=_h-_size*(((_sideMap & 1)!=0)+((_sideMap & 8)!=0));
	if (_remainingH>0){
		int _rightX=_x+_w-_smallSizeBot;
		for (i=_sideMap & 1;_remainingH>=_size;++i){
			if (_sideMap & 2){
				drawTextureSized(_img->edge[2],_x,_y+_size*i,_smallSizeTop,_size);
			}
			if (_sideMap & 4){
				drawTextureSized(_img->edge[3],_rightX,_y+_size*i,_smallSizeBot,_size);
			}
			_remainingH-=_size;
		}
		if (_remainingH!=0){
			double _leftRatio = _remainingH/(double)_size;
			if (_sideMap & 2){
				drawTexturePartSized(_img->edge[2],_x,_y+_size*i,_smallSizeTop,_remainingH,0,0,getTextureWidth(_img->edge[2]),getTextureHeight(_img->edge[2])*_leftRatio);
			}
			if (_sideMap & 4){
				drawTexturePartSized(_img->edge[3],_rightX,_y+_size*i,_smallSizeTop,_remainingH,0,0,getTextureWidth(_img->edge[3]),getTextureHeight(_img->edge[3])*_leftRatio);
			}
		}
	}
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
void drawUiList(struct uiList* _passed, unsigned char a){
	int i;
	for (i=0;i<_passed->cols;++i){
		int j;
		for (j=0;j<_passed->rows;++j){
			drawUiElem(_passed->elements[i][j],_passed->types[i][j],a);
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
void drawUiLabel(struct uiLabel* _passed, unsigned char a){
	printfArrayCalculate(&_passed->format);
	gbDrawTextAlpha(regularFont,_passed->x,_passed->y,_passed->format.res,_passed->r,_passed->g,_passed->b,a);
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
void drawUiElem(void* _passedElem, uiElemType _passedType, unsigned char a){
	switch(_passedType){
		case UIELEM_BUTTON:
			drawButton(_passedElem,a);
			break;
		case UIELEM_LABEL:
			drawUiLabel(_passedElem,a);
			break;
		case UIELEM_LIST:
			drawUiList(_passedElem,a);
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
