#include "puzzleGeneric.h"
struct yoshiBoard{
	struct genericBoard lowBoard;
	pieceColor** nextPieces;
};
void initYoshi();
void drawYoshiBoard(struct yoshiBoard* _passedBoard, int _drawX, int _drawY);
