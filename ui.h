typedef void(*uiFunc)(void*,int);
struct uiButton{
	crossTexture images[3];
	char pressStatus; // 1 - hover. 2 - just released, trigger function next frame
	uiFunc onPress;
	void* arg1;
	int arg2;
};
struct windowImg{
	crossTexture corner[4];
	crossTexture edge[4];
	crossTexture middle;
};
void drawButton(struct uiButton* _drawThis, int _x, int _y, int _w, int _h);
char checkButton(struct uiButton* _drawThis, int _x, int _y, int _w, int _h);
void drawWindow(struct windowImg* _img, int _x, int _y, int _w, int _h, int _cornerHeight);
void clickButtonDown(struct uiButton* _clickThis);
void clickButtonUp(struct uiButton* _clickThis);
