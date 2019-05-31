#ifndef SKINLOADERHINCLUDED
#define SKINLOADERHINCLUDED

#include <goodbrew/config.h>
#include <goodbrew/images.h>

struct puyoSkin{
	crossTexture img;
	char numColors;
	int** colorX; // array of arrays with x positions of puyo images
	int** colorY;
	int puyoW;
	int puyoH;
};

//struct puyoSkin lowLoadSkinFile(crossTexture _passedImage, int _numColors, int _xSeparation, int _ySeparation, int _singlePuyoW, int _singlePuyoH);
struct puyoSkin loadSkinFilePuyoVs(const char* _passedFilename);
struct puyoSkin loadSkinFileChronicle(const char* _passedFilename);
struct puyoSkin loadChampionsSkinFile(const char* _passedFilename);

#endif