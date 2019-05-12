#include <stdio.h>
#include <goodbrew/config.h>
#include <goodbrew/images.h>

#include "skinLoader.h"

struct puyoSkin lowLoadSkinFile(crossTexture _passedImage, int _numColors, int _xSeparation, int _ySeparation, int _singlePuyoW, int _singlePuyoH){
	struct puyoSkin _ret;
	_ret.img = _passedImage;
	_ret.puyoW=_singlePuyoW;
	_ret.puyoH=_singlePuyoH;
	_ret.numColors=_numColors;
	_ret.colorX=malloc(sizeof(int*)*_ret.numColors);
	_ret.colorY=malloc(sizeof(int*)*_ret.numColors);
	int i;
	for (i=0;i<_ret.numColors;++i){ // foe each color
		_ret.colorX[i]=malloc(sizeof(int)*16);
		_ret.colorY[i]=malloc(sizeof(int)*16);
		int j;
		for (j=0;j<16;++j){ // assign the row
			_ret.colorX[i][j]=j*_xSeparation;
			_ret.colorY[i][j]=i*_ySeparation;
		}
	}
	return _ret;
}
struct puyoSkin loadSkinFilePuyoVs(const char* _passedFilename){
	return lowLoadSkinFile(loadImage(_passedFilename),5,32,32,31,31);
}
struct puyoSkin loadSkinFileChronicle(const char* _passedFilename){
	return lowLoadSkinFile(loadImage(_passedFilename),5,18,18,16,16);
}
// TODO - is there a way to tell vs skins and chronicle skins apart?
//struct puyoSKin loadSkin(const char* _passedFilename){
//	crossTexture _loadedImage = loadImage(_passedFilename);
//	if (getTextureWidth(_loadedImage)==512){
//
//	}
//}