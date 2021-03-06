/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "goodLinkedList.h"
#include "puzzleGeneric.h"
#include "pieceSet.h"
struct puyoSkin;
struct gameSettings{
	int pointsPerGar;
	int numColors;
	int minPopNum;
	double pushMultiplier;
	int maxGarbageRows;
	//
	u64 popTime;
	u64 nextWindowTime;
	u64 rotateTime;
	u64 hMoveTime;
	u64 fallTime;
	u64 postSquishDelay; // Time after the squish animation before next pop check
	u64 squishTime;
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

// draw
#define PUYOBORDERSZ (tilew/3)
#define PUYOBORDERSMALLSZ (tilew/6)
#define PUYOBORDERSMALLSZDEC (.16666666)

void endFrameUpdateBoard(struct puyoBoard* _passedBoard, signed char _updateRet);
signed char updatePuyoBoard(struct puyoBoard* _passedBoard, struct gameState* _passedState, signed char _returnForIndex, u64 _sTime);
void drawPuyoBoard(struct puyoBoard* _drawThis, int _startX, int _startY, char _isPlayerBoard, int tilew, u64 _sTime);
void transitionBoardNextWindow(struct puyoBoard* _passedBoard, u64 _sTime);
void addPuyoBoard(struct gameState* _passedState, int i, int _passedW, int _passedH, int _passedGhost, int _passedNextNum, struct gameSettings* _passedSettings, struct puyoSkin* _passedSkin, struct controlSettings* _controlSettings, char _isCpu);
void initPuyoSettings(struct gameSettings* _passedSettings);
struct puyoBoard* newBoard(int _w, int _h, int numGhostRows, int _numNextPieces, struct gameSettings* _usableSettings, struct puyoSkin* _passedSkin);
void resetPuyoBoard(struct puyoBoard* _passedBoard);
void puyoUpdateControlSet(void* _controlData, struct gameState* _passedState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime);
void updatePuyoAi(void* _stateData, struct gameState* _curGameState, void* _passedGenericBoard, signed char _updateRet, int _drawX, int _drawY, int tilew, u64 _sTime);
void freePuyoBoardSets(struct puyoBoard* _passedBoard);
void freePuyoBoard(struct puyoBoard* _passedBoard);
