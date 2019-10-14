#ifndef NATHAN101319
#define NATHAN101319

void updateRotatingDisp(struct movingPiece* _passedPiece, struct movingPiece* _rotateAround, int _rotateTime, u64 _sTime);
char pieceSetCanFall(struct genericBoard* _passedBoard, struct pieceSet* _passedSet);
void tryHShiftSet(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, signed char _direction, int _hMoveTime, u64 _sTime);
void lazyUpdateSetDisplay(struct pieceSet* _passedSet, u64 _sTime);
void forceSetSetX(struct pieceSet* _passedSet, int _leftX, char _isRelative);
char setCanObeyShift(struct genericBoard* _passedBoard, struct pieceSet* _passedSet, int _xDist, int _yDist);
char setCanRotate(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, char _isClockwise, char _allowForceShift, int _rotateTime, u64 _sTime);
void forceRotateSet(struct pieceSet* _passedSet, char _isClockwise, char _changeFlags, int _rotateTime, u64 _sTime);
struct pieceSet* dupPieceSet(struct pieceSet* _passedSet);
struct movingPiece* getNotAnchorPiece(struct pieceSet* _passedSet);
void getRotateTrigSign(char _isClockwise, char _dirRelation, int* _retX, int* _retY);
unsigned char tryStartRotate(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, char _isClockwise, char _canDoubleRotate, int _rotateTime, u64 _sTime);

#endif
