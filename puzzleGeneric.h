#ifndef NATHAN081519PUZZLE
#define NATHAN081519PUZZLE
//
#define COLOR_NONE 0
#define COLOR_IMPOSSIBLE 1
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
	double transitionDeltaX;
	double transitionDeltaY;
	unsigned char movingFlag; // sideways, down, rotate
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

#endif
