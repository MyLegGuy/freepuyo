/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
	int* ghostX;
	int* ghostY;
	int garbageX;
	int garbageY;
	int numQueueImages;
	int* queueIconX;
	int* queueIconY;
};

//struct puyoSkin lowLoadSkinFile(crossTexture _passedImage, int _numColors, int _xSeparation, int _ySeparation, int _singlePuyoW, int _singlePuyoH);
//struct puyoSkin loadSkinFilePuyoVs(const char* _passedFilename);
//struct puyoSkin loadSkinFileChronicle(const char* _passedFilename);
struct puyoSkin loadChampionsSkinFile(crossTexture _passedImage);

#endif
