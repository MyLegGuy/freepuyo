#include <stdio.h>
#include <goodbrew/config.h>
#include <goodbrew/images.h>

#include "skinLoader.h"

struct puyoSkin loadSkinFile(char* _passedFilename){
	struct puyoSkin _ret;
	_ret.puyoW=32;
	_ret.puyoH=32;
	_ret.img=loadImage(_passedFilename);
	_ret.numColors=5;
	_ret.colorX=malloc(sizeof(int*)*_ret.numColors);
	_ret.colorY=malloc(sizeof(int*)*_ret.numColors);
	int i;
	for (i=0;i<_ret.numColors;++i){
		_ret.colorX[i]=malloc(sizeof(int)*16);
		_ret.colorY[i]=malloc(sizeof(int)*16);
		int j;
		for (j=0;j<_ret.numColors;++j){
			_ret.colorX[i][j]=j*32;
			_ret.colorY[i][j]=i*32;
		}
	}
	return _ret;
}