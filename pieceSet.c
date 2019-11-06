/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "main.h"
#include "puzzleGeneric.h"
#include "pieceSet.h"
void _rotateAxisFlip(char _isClockwise, char _dirRelation, int *_outX, int* _outY){
	if (!_isClockwise){
		if (_dirRelation & (DIR_UP | DIR_DOWN)){
			*_outX=(*_outX*-1);
		}else{
			*_outY=(*_outY*-1);
		}
	}
}
// _outX and _outY with be -1 or 1
// You pass the relation you are to the anchor before rotation
void getPostRotatePos(char _isClockwise, char _dirRelation, int* _outX, int* _outY){
	// Get as if it's clockwise
	// Primary direction goes first, direction that changes with direction goes second
	switch(_dirRelation){
		case DIR_UP:
			*_outY=1;
			*_outX=1;
			break;
		case DIR_DOWN:
			*_outY=-1;
			*_outX=-1;
			break;
		case DIR_LEFT:
			*_outX=1;
			*_outY=-1;
			break;
		case DIR_RIGHT:
			*_outX=-1;
			*_outY=1;
			break;
	}
	_rotateAxisFlip(_isClockwise,_dirRelation,_outX,_outY);
}
// Apply this sign change to the end results of sin and cos
// You pass the relation you are to the anchor after rotation, we're trying to go backwards
// Remember, the trig is applied to the center of the anchor
void getRotateTrigSign(char _isClockwise, char _dirRelation, int* _retX, int* _retY){
	*_retX=1;
	*_retY=1;
	// Get signs for clockwise
	switch(_dirRelation){
		case DIR_UP:
			*_retY=-1;
			*_retX=-1;
			break;
		case DIR_DOWN:
			*_retY=1;
			*_retX=1;
			break;
		case DIR_LEFT:
			*_retX=-1;
			*_retY=1;
			break;
		case DIR_RIGHT:
			*_retX=1;
			*_retY=-1;
			break;
	}
	_rotateAxisFlip(_isClockwise,_dirRelation,_retX,_retY);
}
//////////////////////////////////////////////////
void forceResetSetDeath(struct pieceSet* _passedSet){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if (_passedSet->pieces[i].movingFlag & FLAG_DEATHROW){
			UNSET_FLAG(_passedSet->pieces[i].movingFlag,FLAG_DEATHROW);
			_passedSet->pieces[i].completeFallTime=0;
		}
	}
}
char pieceSetCanFall(struct genericBoard* _passedBoard, struct pieceSet* _passedSet){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if (!pieceCanFell(_passedBoard,&(_passedSet->pieces[i]))){
			return 0;
		}
	}
	return 1;
}
void resetDyingFlagMaybe(struct genericBoard* _passedBoard, struct pieceSet* _passedSet){
	if (pieceSetCanFall(_passedBoard,_passedSet)){
		forceResetSetDeath(_passedSet);
	}
}
char setCanObeyShift(struct genericBoard* _passedBoard, struct pieceSet* _passedSet, int _xDist, int _yDist){
	// Make sure all pieces in this set can obey the force shift
	int j;
	for (j=0;j<_passedSet->count;++j){
		if (getBoard(_passedBoard,_passedSet->pieces[j].tileX+_xDist,_passedSet->pieces[j].tileY+_yDist)!=COLOR_NONE){
			return 0;
		}
	}
	return 1;
}
// _sTime and _rotateTime only needed if _changeFlags
void forceRotateSet(struct pieceSet* _passedSet, char _isClockwise, char _changeFlags, u64 _rotateTime, u64 _sTime){
	int i;
	for (i=0;i<_passedSet->count;++i){
		if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE)==0 && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
			int _destX;
			int _destY;
			getPostRotatePos(_isClockwise,getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY),&_destX,&_destY);
			_passedSet->pieces[i].tileX+=_destX;
			_passedSet->pieces[i].tileY+=_destY;
			if (_changeFlags && _rotateTime!=0){ // may only use settings if here
				_passedSet->pieces[i].completeRotateTime = _sTime+_rotateTime;
				_passedSet->pieces[i].movingFlag|=(_isClockwise ? FLAG_ROTATECW : FLAG_ROTATECC);
			}
		}
	}
}
// If _allowForceShift>0 then the piece set may be shifted
// If _allowForceShift is 2 then also change flags when force shift
// _sTime and _rotateTime only needed if_allowForceShift == 2
char setCanRotate(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, char _isClockwise, char _allowForceShift, u64 _rotateTime, u64 _sTime){
	if (!_passedSet->isSquare){
		int i;
		for (i=0;i<_passedSet->count;++i){
			if ((_passedSet->pieces[i].movingFlag & FLAG_ANY_ROTATE)==0 && &(_passedSet->pieces[i])!=_passedSet->rotateAround){
				int _destX;
				int _destY;
				getPostRotatePos(_isClockwise,getRelation(_passedSet->pieces[i].tileX,_passedSet->pieces[i].tileY,_passedSet->rotateAround->tileX,_passedSet->rotateAround->tileY),&_destX,&_destY);
				_destX+=_passedSet->pieces[i].tileX;
				_destY+=_passedSet->pieces[i].tileY;
				if (getBoard(_passedBoard,_destX,_destY)!=COLOR_NONE){
					if (_allowForceShift){
						int _xDist;
						int _yDist;
						getRelationCoords(_destX, _destY, _passedSet->rotateAround->tileX, _passedSet->rotateAround->tileY, &_xDist, &_yDist);
						_xDist*=-1;
						_yDist*=-1;
						// If they can all obey the force shift, shift them all
						if (setCanObeyShift(_passedBoard,_passedSet,_xDist,_yDist)){
							// HACK - If the other pieces rotating in this set can't rotate, these new positions set below would remain. For the piece shapes I'll have in my game, it is impossible for one piece to be able to rotate but not another.
							int j;
							for (j=0;j<_passedSet->count;++j){
								_passedSet->pieces[j].tileX+=_xDist;
								_passedSet->pieces[j].tileY+=_yDist;
								if (_allowForceShift==2){ // Optional change flags
									if (_yDist!=0){
										UNSET_FLAG(_passedSet->pieces[j].movingFlag,(FLAG_MOVEDOWN | FLAG_DEATHROW));
										_passedSet->pieces[j].completeFallTime=0;
									}
									if (_xDist!=0){
										UNSET_FLAG(_passedSet->pieces[j].movingFlag,FLAG_HMOVE);
									}
								}
							}
							if (_allowForceShift==2){ // Optional change flags
								resetDyingFlagMaybe(_passedBoard,_passedSet);
								// If there's a forced shift, give it a smooth transition by hvaing the anchor piece, which all the other pieces' positions are relative to, move smoothly.
								if (_xDist!=0){
									_passedSet->rotateAround->movingFlag|=FLAG_HMOVE;
									_passedSet->rotateAround->diffHMoveTime = _rotateTime;
									_passedSet->rotateAround->completeHMoveTime = _sTime+_passedSet->rotateAround->diffHMoveTime;
									_passedSet->rotateAround->transitionDeltaX = _xDist;
								}
							}
						}else{
							return 0;
						}
					}else{
						return 0;
					}
				}
			}
		}
	}
	return 1;
}
// _canDoubleRotate only taken into account if no space to rotate.
// It also refers to the ability to swap top and bottom puyos while in a one wide
// First bit tells you if double rotate was used or would've been used.
unsigned char tryStartRotate(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, char _isClockwise, char _canDoubleRotate, u64 _rotateTime, u64 _sTime){
	unsigned char _ret=0;
	if (_passedSet->isSquare==0){
		// if can rotate is confirmed for smash 6
		if (setCanRotate(_passedSet,_passedBoard,_isClockwise,2,_rotateTime,_sTime)){
			forceRotateSet(_passedSet,_isClockwise,1,_rotateTime,_sTime);
			resetDyingFlagMaybe(_passedBoard,_passedSet);
		}else{ // If we can't rotate
			// If we're a regular piece with one piece above the other
			if (_passedSet->count==2 && _passedSet->pieces[0].tileX==_passedSet->pieces[1].tileX){
				if (_canDoubleRotate){ // Do the rotate
					struct movingPiece* _moveOnhis = _passedSet->rotateAround==&(_passedSet->pieces[0])?&(_passedSet->pieces[1]):&(_passedSet->pieces[0]); // of the two puyos, get the one that isn't the anchor
					int _yChange = 2*(_moveOnhis->tileY<_passedSet->rotateAround->tileY ? 1 : -1);
					char _canProceed=1;
					if (getBoard(_passedBoard,_moveOnhis->tileX,_moveOnhis->tileY+_yChange)!=COLOR_NONE){
						int _forceYChange = (_yChange*-1)/2;
						if (setCanObeyShift(_passedBoard,_passedSet,0,_forceYChange)){
							int i;
							for (i=0;i<_passedSet->count;++i){
								_passedSet->pieces[i].tileY+=_forceYChange;
							}
						}else{
							_canProceed=0;
						}
					}
					if (_canProceed){
						_moveOnhis->tileY+=_yChange;
						if (_rotateTime!=0){
							_moveOnhis->movingFlag|=FLAG_ROTATECW;
							_moveOnhis->completeRotateTime = _sTime+_rotateTime;
						}
						resetDyingFlagMaybe(_passedBoard,_passedSet);
					}
				}
				_ret|=1;
			}
		}
	}
	return _ret;
}
// Try to start an h shift on a set
void tryHShiftSet(struct pieceSet* _passedSet, struct genericBoard* _passedBoard, signed char _direction, u64 _hMoveTime, u64 _sTime){
	if (!(_passedSet->pieces[0].movingFlag & FLAG_HMOVE)){
		char _upShiftNeeded=0;
		int i;
		for (i=0;i<_passedSet->count;++i){
			if (getBoard(_passedBoard,_passedSet->pieces[i].tileX+_direction,_passedSet->pieces[i].tileY)==COLOR_NONE){
				continue;
			}else if (_passedSet->pieces[i].movingFlag & FLAG_MOVEDOWN && (_passedSet->pieces[i].completeFallTime-_sTime)>_passedSet->pieces[i].diffFallTime/2){ // Allow the puyo to jump up a tile a little bit if the timing is tight
				if (getBoard(_passedBoard,_passedSet->pieces[i].tileX+_direction,_passedSet->pieces[i].tileY-1)!=COLOR_NONE){
					break;
				}
				_upShiftNeeded=1;
			}else{
				break;
			}
		}
		if (i==_passedSet->count){ // all can move
			for (i=0;i<_passedSet->count;++i){
				_passedSet->pieces[i].movingFlag|=FLAG_HMOVE;
				_passedSet->pieces[i].diffHMoveTime = _hMoveTime;
				_passedSet->pieces[i].completeHMoveTime = _sTime+_passedSet->pieces[i].diffHMoveTime-_passedSet->pieces[i].completeHMoveTime;
				_passedSet->pieces[i].transitionDeltaX = _direction;
				_passedSet->pieces[i].tileX+=_direction;
				if (_upShiftNeeded){
					_passedSet->pieces[i].tileY-=_upShiftNeeded;
					_passedSet->pieces[i].completeFallTime=_sTime;
				}
			}
			resetDyingFlagMaybe(_passedBoard,_passedSet);
		}
	}
}
void forceSetSetX(struct pieceSet* _passedSet, int _leftX, char _isRelative){
	// Calculate amount to subtract from position
	int _minDeltaX;
	if (!_isRelative){
		_minDeltaX=99;
		int i;
		for (i=0;i<_passedSet->count;++i){
			int _deltaX = _passedSet->pieces[i].tileX-_leftX;
			if (_deltaX<_minDeltaX){
				_minDeltaX=_deltaX;
			}
		}
	}else{
		_minDeltaX=_leftX*-1;
	}
	//
	int i;
	for (i=0;i<_passedSet->count;++i){
		_passedSet->pieces[i].tileX-=_minDeltaX;
	}
}
// Returns a piece that isn't the anchor piece. Can return NULL if there's only one piece in the set and it's the anchor.
// Always returns first or second piece.
struct movingPiece* getNotAnchorPiece(struct pieceSet* _passedSet){
	if (_passedSet->pieces!=_passedSet->rotateAround){
		return _passedSet->pieces;
	}else if (_passedSet->count>1){
		return &_passedSet->pieces[1];
	}
	return NULL;
}
// Duplicate
struct pieceSet* dupPieceSet(struct pieceSet* _passedSet){
	struct pieceSet* _ret = malloc(sizeof(struct pieceSet));
	memcpy(_ret,_passedSet,sizeof(struct pieceSet));

	// Duplicate the movingPieces stored in _passedSet
	struct movingPiece* _newPieces = malloc(sizeof(struct movingPiece)*_ret->count);
	int i;
	for (i=0;i<_ret->count;++i){
		memcpy(&_newPieces[i],&_passedSet->pieces[i],sizeof(struct movingPiece));
	}
	_ret->pieces=_newPieces;

	// Find the index of the rotateAround in _passedSet
	int _rotateAroundIndex=-1;
	for (i=0;i<_passedSet->count;++i){
		if (_passedSet->rotateAround==&_passedSet->pieces[i]){
			_rotateAroundIndex=i;
			break;
		}
	}
	if (_rotateAroundIndex==-1){
		printf("Error in dupPieceSet\n");
		_rotateAroundIndex=0;
	}
	_ret->rotateAround = &_ret->pieces[_rotateAroundIndex];
	return _ret;
}
void lazyUpdateSetDisplay(struct pieceSet* _passedSet, u64 _sTime){
	int i;
	for (i=0;i<_passedSet->count;++i){
		lazyUpdatePieceDisplay(&(_passedSet->pieces[i]),_sTime);
	}
}
void snapSetPossible(struct pieceSet* _passedSet, u64 _sTime){
	int i;
	for (i=0;i<_passedSet->count;++i){
		snapPieceDisplayPossible(&(_passedSet->pieces[i]));
	}
}
void updateRotatingDisp(struct movingPiece* _passedPiece, struct movingPiece* _rotateAround, u64 _rotateTime, u64 _sTime){
	if ((_passedPiece->movingFlag & FLAG_ANY_ROTATE) && _passedPiece!=_rotateAround){
		if (_sTime>=_passedPiece->completeRotateTime){ // displayX and displayY have already been set
			UNSET_FLAG(_passedPiece->movingFlag,FLAG_ANY_ROTATE);
			snapPieceDisplayPossible(_passedPiece);
		}else{
			char _isClockwise = (_passedPiece->movingFlag & FLAG_ROTATECW);
			signed char _dirRelation = getRelation(_passedPiece->tileX,_passedPiece->tileY,_rotateAround->tileX,_rotateAround->tileY);
			int _trigSignX;
			int _trigSignY;
			getRotateTrigSign(_isClockwise, _dirRelation, &_trigSignX, &_trigSignY);
			double _angle = partMoveFills(_sTime,_passedPiece->completeRotateTime,_rotateTime,M_PI_2);
			if (_dirRelation & (DIR_LEFT | DIR_RIGHT)){
				_angle = M_PI_2 - _angle;
			}
			// completely overwrite the displayX and displayY set by updatePieceDisplay
			_passedPiece->displayX=_rotateAround->displayX+cos(_angle)*_trigSignX;
			_passedPiece->displayY=_rotateAround->displayY+sin(_angle)*_trigSignY;
		}
	}
}
void freePieceSet(struct pieceSet* _freeThis){
	free(_freeThis->pieces);
}
void setDownButtonHold(struct controlSet* _passedControls, struct pieceSet* _targetSet, double _pushMultiplier, u64 _sTime){
	if (_targetSet->pieces[0].movingFlag & FLAG_MOVEDOWN){ // Normal push down
		int _offsetAmount = (_sTime-_passedControls->lastFrameTime)*_pushMultiplier;
		int j;
		for (j=0;j<_targetSet->count;++j){
			if (_offsetAmount>_targetSet->pieces[j].completeFallTime){ // Keep unisnged value from going negative
				_targetSet->pieces[j].completeFallTime=0;
			}else{
				_targetSet->pieces[j].completeFallTime=_targetSet->pieces[j].completeFallTime-_offsetAmount;
			}			
		}
	}else if (_targetSet->pieces[0].movingFlag & FLAG_DEATHROW){ // lock
		int j;
		for (j=0;j<_targetSet->count;++j){
			_targetSet->pieces[j].completeFallTime = 0;
		}
	}
}
