/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <goodbrew/config.h>
#include <goodbrew/images.h>

#include "skinLoader.h"

struct puyoSkin lowLoadSkinFile(crossTexture _passedImage, int _numColors, int _xStart, int _yStart, int _xSeparation, int _ySeparation, int _singlePuyoW, int _singlePuyoH, int _ghostStartX, int _ghostY){
	struct puyoSkin _ret;
	if (_passedImage==NULL){
		printf("Image not found\n");
		memset(&_ret,0,sizeof(struct puyoSkin));
		return _ret;
	}
	_ret.img = _passedImage;
	_ret.puyoW=_singlePuyoW;
	_ret.puyoH=_singlePuyoH;
	_ret.numColors=_numColors;
	_ret.colorX=malloc(sizeof(int*)*_ret.numColors);
	_ret.colorY=malloc(sizeof(int*)*_ret.numColors);
	_ret.ghostX = malloc(sizeof(int)*_ret.numColors);
	_ret.ghostY = malloc(sizeof(int)*_ret.numColors);
	int i;
	for (i=0;i<_ret.numColors;++i){ // foe each color
		_ret.colorX[i]=malloc(sizeof(int)*16);
		_ret.colorY[i]=malloc(sizeof(int)*16);
		_ret.ghostX[i]=_ghostStartX+i*_xSeparation;
		_ret.ghostY[i]=_ghostY;
		int j;
		for (j=0;j<16;++j){ // assign the row
			_ret.colorX[i][j]=j*_xSeparation+_xStart;
			_ret.colorY[i][j]=i*_ySeparation+_yStart;
		}
	}
	return _ret;
}
struct puyoSkin loadChampionsSkinFile(crossTexture _passedImage){
	struct puyoSkin _ret = lowLoadSkinFile(_passedImage,5,0,0,72,72,64,64,0,11*72);
	_ret.garbageX=18*72;
	_ret.garbageY=72;
	return _ret;
}
/*
// not working right now
struct puyoSkin loadSkinFilePuyoVs(const char* _passedFilename){
	return lowLoadSkinFile(loadImage(_passedFilename),5,1,1,32,32,31,31,0,0);
}
// not working right now
struct puyoSkin loadSkinFileChronicle(const char* _passedFilename){
	return lowLoadSkinFile(loadImage(_passedFilename),5,0,0,18,18,16,16,0,0);
}
*/
// TODO - is there a way to tell vs skins and chronicle skins apart?
//struct puyoSKin loadSkin(const char* _passedFilename){
//	crossTexture _loadedImage = loadImage(_passedFilename);
//	if (getTextureWidth(_loadedImage)==512){
//
//	}
//}
