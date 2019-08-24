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
#define FLAG_MOVELEFT 	0b00000010
#define FLAG_MOVERIGHT 	0b00000100
#define FLAG_ROTATECW 	0b00001000 // clockwise
#define FLAG_ROTATECC 	0b00010000 // counter clock
#define FLAG_DEATHROW	0b00100000 // Death row for being a moving puyo, I mean. If this puyo has hit the puyo under it and is about to die if it's not moved
//
#define FLAG_ANY_HMOVE 	(FLAG_MOVELEFT | FLAG_MOVERIGHT)
#define FLAG_ANY_ROTATE (FLAG_ROTATECW | FLAG_ROTATECC)
//
#define FIXDISP(x) ((x)*tilew)
//
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
	u64 referenceFallTime; // Copy of completeFallTime, unadjusted for the current down hold
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

char updatePieceDisplayY(struct movingPiece* _passedPiece, u64 _sTime, char _canUnset);
char updatePieceDisplayX(struct movingPiece* _passedPiece, u64 _sTime, char _canUnset);
void removePuyoPartialTimes(struct movingPiece* _passedPiece);
void snapPieceDisplayPossible(struct movingPiece* _passedPiece);
void lowSnapPieceTileX(struct movingPiece* _passedPiece);
void lowSnapPieceTileY(struct movingPiece* _passedPiece);
void lazyUpdatePieceDisplay(struct movingPiece* _passedPiece, u64 _sTime);
char updatePieceDisplay(struct movingPiece* _passedPiece, u64 _sTime);
char pieceCanFell(struct genericBoard* _passedBoard, struct movingPiece* _passedPiece);
pieceColor getBoard(struct genericBoard* _passedBoard, int _x, int _y);
void freeColorArray(pieceColor** _passed, int _w);
u64** newJaggedArrayu64(int _w, int _h);
pieceColor** newJaggedArrayColor(int _w, int _h);
char** newJaggedArrayChar(int _w, int _h);
struct genericBoard newGenericBoard(int _w, int _h);
void clearGenericBoard(pieceColor** _passed, int _w, int _h);
void clearPieceStatus(struct genericBoard* _passedBoard);
void clearBoardBoard(struct genericBoard* _passedBoard);
void setBoard(struct genericBoard* _passedBoard, int x, int y, pieceColor _color);

#endif
