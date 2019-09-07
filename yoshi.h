#include "puzzleGeneric.h"
//
// how big is the next window space where the pieces spawn (in scaled tiles)
#define YOSHINEXTOVERLAPH 1
//
#define SWAPDUDESMALLTILEH 2
//
#define YOSHI_TILE_SCALE 4
#define YOSHINEXTNUM 1
struct yoshiSkin{
	crossTexture img;
	int normColors;
	int* colorX;
	int* colorY;
	int colorW;
	int colorH;
	int stretchBlockX;
	int stretchBlockY;
};
struct yoshiSettings{
	int fallTime;
	int rowTime; // stall time?
	int popTime;
	int squishPerPiece;
	int swapTime;
	double pushMultiplier;
};
struct yoshiBoard{
	struct genericBoard lowBoard;
	pieceColor** nextPieces;
	struct nList* activePieces; // of struct movingPiece*
	struct yoshiSkin skin;
	short swapDudeX;
	short swappingIndex;
	u64 swapEndTime;
	struct yoshiSettings settings;
};
void initYoshi();
void drawYoshiBoard(struct yoshiBoard* _passedBoard, int _drawX, int _drawY, int tilew, u64 _sTime);
void updateYoshiBoard(struct yoshiBoard* _passedBoard, u64 _sTime);
void yoshiSpawnSet(struct yoshiBoard* _passedBoard, pieceColor* _passedSet, u64 _sTime);
void yoshiSpawnNext(struct yoshiBoard* _passedBoard, u64 _sTime);
char tryStartYoshiFall(struct yoshiBoard* _passedBoard, struct movingPiece* _curPiece, u64 _sTime);
