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
struct puyoSkin loadSkinFile(char* _passedFilename);
#endif