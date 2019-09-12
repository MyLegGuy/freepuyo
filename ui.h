#include <goodbrew/config.h>
#include <goodbrew/images.h>
typedef void(*uiFunc)(void*,int);
typedef enum{
	UIELEM_NONE,
	UIELEM_BUTTON,
	UIELEM_LABEL,
	UIELEM_LIST,
}uiElemType;
struct uiButton{
	crossTexture images[3];
	int x;
	int y;
	int w;
	int h;
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
// ui elements arranged in a list
struct uiList{
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
struct uiLabel{
	char* text;
	int x;
	int y;
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

char checkButton(struct uiButton* _drawThis);
void clickButtonDown(struct uiButton* _clickThis);
void clickButtonUp(struct uiButton* _clickThis);
void drawButton(struct uiButton* _drawThis);
void drawUiElem(void* _passedElem, uiElemType _passedType);
void drawUiLabel(struct uiLabel* _passed);
void drawUiList(struct uiList* _passed);
void drawWindow(struct windowImg* _img, int _x, int _y, int _w, int _h, int _cornerHeight);
void freeUiList(struct uiList* _freeThis, char _freeContents);
int getCornerWidth(struct windowImg* _img, int _cornerHeight);
struct uiList* newUiList(int _rows, int _cols, int _rowHeight);
void setUiPos(void* _passedElem, uiElemType _passedType, int _passedX, int _passedY);
void uiListCalcSizes(struct uiList* _passed);
void uiListPos(struct uiList* _passed, int _x, int _y);
void fitUiElemHeight(void* _passedElem, uiElemType _passedType, int _passedHeight);
void freeRiskyUiData(void* _passedElem, uiElemType _passedType);
