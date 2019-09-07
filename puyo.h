#include "goodLinkedList.h"
#include "puzzleGeneric.h"
struct puyoSkin;
struct pieceSet;
struct gameSettings{
	int pointsPerGar;
	int numColors;
	int minPopNum;
	double pushMultiplier;
	//
	int popTime;
	int nextWindowTime;
	int rotateTime;
	int hMoveTime;
	int fallTime;
	int postSquishDelay; // Time after the squish animation before next pop check
	int maxGarbageRows;
	int squishTime;
};
struct puyoBoard{
	struct genericBoard lowBoard;
	char** popCheckHelp; // 1 if already checked, 2 if already set to popping. 0 otherwise. Can also be POSSIBLEPOPBYTE
	int numActiveSets;
	struct nList* activeSets;
	int numNextPieces;
	struct pieceSet* nextPieces;
	struct puyoSkin* usingSkin;
	int numGhostRows;
	u64 score;
	u64 curChainScore; // The score we're going to add after the puyo finish popping
	int curChain;
	int chainNotifyX;
	int chainNotifyY;
	double leftoverGarbage;
	int readyGarbage;
	int incomingLength;
	int* incomingGarbage;
	struct gameSettings settings;
};

// Width of next window in tiles
#define NEXTWINDOWTILEW 2

void endFrameUpdateBoard(struct puyoBoard* _passedBoard, signed char _updateRet);
signed char updatePuyoBoard(struct puyoBoard* _passedBoard, struct gameState* _passedState, signed char _returnForIndex, u64 _sTime);
void drawPuyoBoard(struct puyoBoard* _drawThis, int _startX, int _startY, char _isPlayerBoard, int tilew, u64 _sTime);
void transitionBoardNextWindow(struct puyoBoard* _passedBoard, u64 _sTime);
void initPuyo(void* _passedState);
