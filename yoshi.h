#include "puzzleGeneric.h"
#define YOSHI_TILE_SCALE 4
#define YOSHINEXTNUM 2
struct yoshiBoard{
	struct genericBoard lowBoard;
	pieceColor** nextPieces;
	struct nList* activePieces; // of struct movingPiece*
};
void initYoshi();
void drawYoshiBoard(struct yoshiBoard* _passedBoard, int _drawX, int _drawY, int tilew, u64 _sTime);
void updateYoshiBoard(struct yoshiBoard* _passedBoard, u64 _sTime);
void yoshiSpawnSet(struct yoshiBoard* _passedBoard, pieceColor* _passedSet, u64 _sTime);
void yoshiSpawnNext(struct yoshiBoard* _passedBoard, u64 _sTime);
