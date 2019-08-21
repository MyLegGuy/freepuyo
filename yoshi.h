#include "puzzleGeneric.h"
#define YOSHI_TILE_SCALE 4
#define YOSHINEXTNUM 3
struct yoshiBoard{
	struct genericBoard lowBoard;
	pieceColor** nextPieces;
};
void initYoshi();
void drawYoshiBoard(struct yoshiBoard* _passedBoard, int _drawX, int _drawY, int tilew, u64 _sTime);
