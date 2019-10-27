#ifndef NATHAN101319
#define NATHAN101319

#include "puzzleGeneric.h"

struct pieceSet{
	signed char isSquare; // If this is a square set of puyos then this will be the width of that square. 0 otherwise
	struct movingPiece* rotateAround; // If this isn't a square, rotate around this puyo
	char quickLock;
	int count;
	struct movingPiece* pieces;
};

void updateRotatingDisp(struct movingPiece* _passedPiece, struct movingPiece* _rotateAround, u64 _rotateTime, u64 _sTime);
char pieceSetCanFall(struct genericBoard* _passedBoard, struct pieceSet* _passedSet);
void tryHShiftSet(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, signed char _direction, u64 _hMoveTime, u64 _sTime);
void lazyUpdateSetDisplay(struct pieceSet* _passedSet, u64 _sTime);
void forceSetSetX(struct pieceSet* _passedSet, int _leftX, char _isRelative);
char setCanObeyShift(struct genericBoard* _passedBoard, struct pieceSet* _passedSet, int _xDist, int _yDist);
char setCanRotate(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, char _isClockwise, char _allowForceShift, u64 _rotateTime, u64 _sTime);
void forceRotateSet(struct pieceSet* _passedSet, char _isClockwise, char _changeFlags, u64 _rotateTime, u64 _sTime);
struct pieceSet* dupPieceSet(struct pieceSet* _passedSet);
struct movingPiece* getNotAnchorPiece(struct pieceSet* _passedSet);
void getRotateTrigSign(char _isClockwise, char _dirRelation, int* _retX, int* _retY);
unsigned char tryStartRotate(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, char _isClockwise, char _canDoubleRotate, u64 _rotateTime, u64 _sTime);
void snapSetPossible(struct pieceSet* _passedSet, u64 _sTime);
void freePieceSet(struct pieceSet* _freeThis);
void setDownButtonHold(struct controlSet* _passedControls, struct pieceSet* _targetSet, double _pushMultiplier, u64 _sTime);

#endif
