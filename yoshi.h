/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "puzzleGeneric.h"
//
// how big is the next window space where the pieces spawn (in scaled tiles)
#define YOSHINEXTOVERLAPH 1
//
#define SWAPDUDESMALLTILEH 2
//
#define YOSHI_NORM_COLORS 4
#define YOSHI_SPECIALSTART 2
#define YOSHI_TOPSHELL YOSHI_SPECIALSTART
#define YOSHI_BOTTOMSHELL 3
#define YOSHI_NORMALSTART 4
//
#define YOSHI_NUM_SPECIAL 2
//
//
#define YOSHI_TILE_SCALE 4
#define YOSHINEXTNUM 2
struct yoshiSkin{
	crossTexture* img;
	int normColors;
	int* colorX;
	int* colorY;
	int colorW;
	int colorH;
	int stretchBlockX;
	int stretchBlockY;
};
struct yoshiSettings{
	u64 fallTime;
	u64 rowTime; // stall time?
	u64 popTime;
	u64 squishPerPiece;
	u64 swapTime;
	double pushMultiplier;
};
struct yoshiBoard{
	struct genericBoard lowBoard;
	pieceColor** nextPieces;
	struct nList* activePieces; // of struct movingPiece*
	struct yoshiSkin* skin;
	short swapDudeX;
	short swappingIndex;
	u64 swapEndTime;
	struct yoshiSettings settings;
};
void drawYoshiBoard(struct yoshiBoard* _passedBoard, int _drawX, int _drawY, int tilew, u64 _sTime);
void updateYoshiBoard(struct yoshiBoard* _passedBoard, gameMode _mode, u64 _sTime);
void yoshiSpawnSet(struct yoshiBoard* _passedBoard, pieceColor* _passedSet, u64 _sTime);
void yoshiSpawnNext(struct yoshiBoard* _passedBoard, u64 _sTime);
char tryStartYoshiFall(struct yoshiBoard* _passedBoard, struct movingPiece* _curPiece, u64 _sTime);
struct yoshiBoard* newYoshi(int _w, int _h, struct yoshiSettings* _usableSettings, struct yoshiSkin* _passedSkin);
void initYoshiSettings(struct yoshiSettings* _passedSettings);
void addYoshiPlayer(struct gameState* _passedState, int _w, int _h, struct yoshiSettings* _usableSettings, struct yoshiSkin* _passedSkin, struct controlSettings* _controlSettings);
void loadYoshiSkin(struct yoshiSkin* _ret, const char* _filename);
void resetYoshiBoard(struct yoshiBoard* _passedBoard);
void yoshiInitLevelMode(void* _uncastBoard, struct gameState* _passedState, void* _level);
void yoshiUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime);
void freeYoshiBoard(struct yoshiBoard* _passedBoard);
void freeYoshiSkin(struct yoshiSkin* _freeThis);
