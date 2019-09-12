#include <goodbrew/config.h>
#include "main.h"
#include "ui.h"
#include <goodbrew/controls.h>
#include <goodbrew/text.h>

#define UILISTVPAD(_passedList) (_passedList->rowH/4)
#define UILISTHPAD(_passedList) (_passedList->rowH)
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
void freeUiList(struct uiList* _freeThis, char _freeContents){
	int i;
	for (i=0;i<_freeThis->cols;++i){
		if (_freeContents){
			int j;
			for (j=0;j<_freeThis->rows;++j){
				free(_freeThis->elements[i][j]);
			}
		}
		free(_freeThis->elements[i]);
		free(_freeThis->types[i]);
	}
	free(_freeThis->cachedWidths);
	free(_freeThis->elements);
	free(_freeThis->types);
	free(_freeThis);
}
void uiListCalcSizes(struct uiList* _passed){
	_passed->w=0;
	int i;
	for (i=0;i<_passed->cols;++i){
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
					_newWidth = textWidth(regularFont,((struct uiLabel*)_passed->elements[i][j])->text);
					break;
				case UIELEM_NONE:
					continue;
			}
			if (_newWidth>_maxWidth){
				_maxWidth=_newWidth;
			}
		}
		_passed->w+=_maxWidth;
		_passed->cachedWidths[i]=_maxWidth;
	}
	// padding between columns
	_passed->w+=UILISTHPAD(_passed)*(_passed->cols-1);
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
void uiListPos(struct uiList* _passed, int _x, int _y){
	_passed->x=_x;
	_passed->y=_y;
	int i;
	for (i=0;i<_passed->cols;++i){
		int _destX = _x+(i==0 ? 0 : (_passed->cachedWidths[i-1]+UILISTHPAD(_passed)*i));
		int j;
		for (j=0;j<_passed->rows;++j){
			int _destY = _y+(j*_passed->rowH+UILISTVPAD(_passed)*j);
			setUiPos(_passed->elements[i][j],_passed->types[i][j],_destX,_destY);
		}
	}
}
//
void drawUiLabel(struct uiLabel* _passed){
	gbDrawTextAlpha(regularFont,_passed->x,_passed->y,_passed->text,_passed->r,_passed->g,_passed->b,_passed->a);
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
			uiListPos(_passedElem,_passedX,_passedY);
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
// free some underlying data that may or may not be used elsewhere
void freeRiskyUiData(void* _passedElem, uiElemType _passedType){
	switch(_passedType){
		case UIELEM_BUTTON:
			freeTexture(((struct uiButton*)_passedElem)->images[0]);
			freeTexture(((struct uiButton*)_passedElem)->images[1]);
			freeTexture(((struct uiButton*)_passedElem)->images[2]);
			break;
		case UIELEM_LABEL:
			free(((struct uiLabel*)_passedElem)->text);
			break;
	}
}
