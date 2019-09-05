#include <goodbrew/config.h>
#include "main.h"
#include "ui.h"
#include <goodbrew/controls.h>
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
void clickButtonDown(struct uiButton* _clickThis){
	_clickThis->pressStatus=1;
}
void clickButtonUp(struct uiButton* _clickThis){
	_clickThis->pressStatus=2;
}
