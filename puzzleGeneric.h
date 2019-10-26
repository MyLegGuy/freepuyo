/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef NATHAN081519PUZZLE
#define NATHAN081519PUZZLE
//
#define COLOR_NONE 0
#define COLOR_IMPOSSIBLE 1
//
#define AUTOTILEDOWN 	0b00000001
#define AUTOTILEUP 		0b00000010
#define AUTOTILERIGHT 	0b00000100
#define AUTOTILELEFT	0b00001000
// For these movement flags, it's already been calculated that moving that way is okay
#define FLAG_MOVEDOWN 	0b00000001
//#define FLAG_MOVELEFT 	0b00000010
#define FLAG_HMOVE	 	0b00000100
#define FLAG_ROTATECW 	0b00001000 // clockwise
#define FLAG_ROTATECC 	0b00010000 // counter clock
#define FLAG_DEATHROW	0b00100000 // Death row for being a moving puyo, I mean. If this puyo has hit the puyo under it and is about to die if it's not moved
//
#define FLAG_ANY_ROTATE (FLAG_ROTATECW | FLAG_ROTATECC)
#define FLAG_DOWNORDEATH (FLAG_MOVEDOWN | FLAG_DEATHROW)
//
// bitmap
// 0 is default?
#define PIECESTATUS_POPPING 1
#define PIECESTATUS_SQUISHING 2
#define PIECESTATUS_POSTSQUISH 4
// #define					   8
//
#define FIXDISP(x) ((x)*tilew)
#define fastGetBoard(_passedBoard,_x,_y) (_passedBoard.board[_x][_y])
//
#define DEATHANIMTIME fixTime(1000)
//
#include "ui.h"
extern struct windowImg boardBorder;
//
typedef enum{
	STATUS_UNDEFINED,
	STATUS_NORMAL, // We're moving the puyo around
	STATUS_POPPING, // We're waiting for puyo to pop
	STATUS_DROPPING, // puyo are falling into place. This is the status after you place a piece and after STATUS_POPPING
	STATUS_SETTLESQUISH, // A status after STATUS_DROPPING to wait for all puyos to finish their squish animation. Needed because some puyos start squish before others. When done, checks for pops and goes to STATUS_NEXTWINDOW or STATUS_POPPING
	STATUS_NEXTWINDOW, // Waiting for next window. This is the status after STATUS_DROPPING if no puyo connect
	STATUS_DEAD,
	STATUS_WON,
	STATUS_DROPGARBAGE, // Can combine with STATUS_DROPPING if it ends up needed special code for the updatePuyoBoard
	STATUS_PREPARING,
}boardStatus;
typedef int pieceColor;
struct movingPiece{
	pieceColor color;
	int tileX;
	int tileY;
	double displayY;
	double displayX;
	char holdingDown; // flag - is this puyo being forced down
	u64 holdDownStart;

	// Variables relating to smooth transition
	unsigned char movingFlag; // sideways, down, rotate
	double transitionDeltaX;
	double transitionDeltaY;
	u64 completeFallTime; // Time when the current falling down will complete
	int diffFallTime; // How long it'll take to fall
	u64 completeRotateTime;
	int diffRotateTime;
	u64 completeHMoveTime;
	int diffHMoveTime;
};
struct genericBoard{
	int w;
	int h;
	pieceColor** board; // ints represent colors
	char** pieceStatus;
	u64** pieceStatusTime;
	boardStatus status;
	u64 statusTimeEnd; // Only set for some statuses
};
struct controlSettings{
	u64 doubleRotateTapTime;
	u64 dasTime;
};
struct controlSet{
	u64 dasChargeEnd;
	signed char dasDirection;
	u64 lastFailedRotateTime;
	u64 lastFrameTime;
	struct controlSettings settings;
	// touch
	u64 holdStartTime;
	int startTouchX;
	int startTouchY;
	char didDrag;
	char isTouchDrop;
};
struct pos{
	int x;
	int y;
};

char updatePieceDisplayY(struct movingPiece* _passedPiece, u64 _sTime, char _canUnset);
char updatePieceDisplayX(struct movingPiece* _passedPiece, u64 _sTime, char _canUnset);
void removePartialTimes(struct movingPiece* _passedPiece);
void snapPieceDisplayPossible(struct movingPiece* _passedPiece);
void lowSnapPieceTileX(struct movingPiece* _passedPiece);
void lowSnapPieceTileY(struct movingPiece* _passedPiece);
void lazyUpdatePieceDisplay(struct movingPiece* _passedPiece, u64 _sTime);
char updatePieceDisplay(struct movingPiece* _passedPiece, u64 _sTime);
char pieceCanFell(struct genericBoard* _passedBoard, struct movingPiece* _passedPiece);
pieceColor getBoard(struct genericBoard* _passedBoard, int _x, int _y);
void freeJaggedArrayu64(u64** _passed, int _w);
void freeJaggedArrayColor(pieceColor** _passed, int _w);
void freeJaggedArrayChar(char** _passed, int _w);
u64** newJaggedArrayu64(int _w, int _h);
pieceColor** newJaggedArrayColor(int _w, int _h);
char** newJaggedArrayChar(int _w, int _h);
struct genericBoard newGenericBoard(int _w, int _h);
void clearGenericBoard(pieceColor** _passed, int _w, int _h);
void clearPieceStatus(struct genericBoard* _passedBoard);
void clearBoardBoard(struct genericBoard* _passedBoard);
void setBoard(struct genericBoard* _passedBoard, int x, int y, pieceColor _color);
void updateControlDas(struct controlSet* _passedControls, u64 _sTime);
struct controlSet* newControlSet(u64 _sTime, struct controlSettings* _usableSettings);
signed char getDirectionInput(struct controlSet* _passedControls, u64 _sTime);
char pieceTryUnsetDeath(struct genericBoard* _passedBoard, struct movingPiece* _passedPiece);
void downButtonHold(struct controlSet* _passedControls, struct movingPiece* _targetPiece, double _passedMultiplier, u64 _sTime);
void controlSetFrameEnd(struct controlSet* _passedControls, u64 _sTime);
char onTouchRelease(struct controlSet* _passedControls);
signed char touchIsHDrag(struct controlSet* _passedControls);
void resetControlHDrag(struct controlSet* _passedControls);
void initialTouchDown(struct controlSet* _passedSet, u64 _sTime);
char rowIsClear(struct genericBoard* _passedBoard, int _yIndex);
void killBoard(struct genericBoard* _passedBoard, u64 _sTime);
void winBoard(struct genericBoard* _passedBoard);
void freeGenericBoard(struct genericBoard* _passedBoard);
char deathrowTimeUp(struct movingPiece* _passedPiece, u64 _sTime);
void placeSquish(struct genericBoard* _passedBoard, int _x, int _y, pieceColor _passedColor, int _squishTime, u64 _sTime);
void placeNormal(struct genericBoard* _passedBoard, int _x, int _y, pieceColor _passedColor);
int processPieceStatuses(int _retThese, struct genericBoard* _passedBoard, int _postSquishDelay, u64 _sTime);
void initControlSettings(struct controlSettings* _passedSettings);
void scaleControlSettings(struct controlSettings* _scaleThis);

#endif
