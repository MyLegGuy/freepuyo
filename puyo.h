#include "goodLinkedList.h"
struct puyoSkin;
struct pieceSet;
typedef enum{
	STATUS_UNDEFINED,
	STATUS_NORMAL, // We're moving the puyo around
	STATUS_POPPING, // We're waiting for puyo to pop
	STATUS_DROPPING, // puyo are falling into place. This is the status after you place a piece and after STATUS_POPPING
	STATUS_SETTLESQUISH, // A status after STATUS_DROPPING to wait for all puyos to finish their squish animation. Needed because some puyos start squish before others. When done, checks for pops and goes to STATUS_NEXTWINDOW or STATUS_POPPING
	STATUS_NEXTWINDOW, // Waiting for next window. This is the status after STATUS_DROPPING if no puyo connect
	STATUS_DEAD,
	STATUS_DROPGARBAGE, // Can combine with STATUS_DROPPING if it ends up needed special code for the updatePuyoBoard
}boardStatus;
typedef int puyoColor;
struct puyoBoard{
	int w;
	int h;
	puyoColor** board; // ints represent colors
	char** pieceStatus;
	u64** pieceStatusTime;
	char** popCheckHelp; // 1 if already checked, 2 if already set to popping. 0 otherwise. Can also be POSSIBLEPOPBYTE
	boardStatus status;
	int numActiveSets;
	struct nList* activeSets;
	int numNextPieces;
	struct pieceSet* nextPieces;
	struct puyoSkin* usingSkin;
	u64 statusTimeEnd; // Only set for some statuses
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
};
struct gameSettings{
	int pointsPerGar;
};

// Width of next window in tiles
#define NEXTWINDOWTILEW 2

void endFrameUpdateBoard(struct puyoBoard* _passedBoard, signed char _updateRet);
void rebuildPuyoBoardDisplay(struct puyoBoard* _passedBoard, u64 _sTime);
signed char updatePuyoBoard(struct puyoBoard* _passedBoard, struct gameSettings* _passedSettings, struct gameState* _passedState, signed char _returnForIndex, u64 _sTime);
void drawPuyoBoard(struct puyoBoard* _drawThis, int _startX, int _startY, char _isPlayerBoard, u64 _sTime);
void transitionBoardNextWindow(struct puyoBoard* _passedBoard, u64 _sTime);
void initPuyo(void* _passedState);
