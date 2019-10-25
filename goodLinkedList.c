/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>

#include "goodLinkedList.h"

struct nList* lowNewnList(){
	struct nList* _ret = malloc(sizeof(struct nList));
	_ret->nextEntry = NULL;
	_ret->data = NULL;
	return _ret;
}
struct nList* getnList(struct nList* _passed, int _index){
	int i=0;
	ITERATENLIST(_passed,{
		if ((i++)==_index){
			return _curnList;
		}
	})
	return NULL;
}
int nListLen(struct nList* _passed){
	if (_passed==NULL){
		return 0;
	}
	int _ret;
	for (_ret=1;(_passed=_passed->nextEntry)!=NULL;++_ret);
	return _ret;
}
void appendnList(struct nList** _source, struct nList* _addThis){
	if (*_source==NULL){
		*_source=_addThis;
		return;
	}
	struct nList* _temp=*_source;
	while(_temp->nextEntry!=NULL){
		_temp=_temp->nextEntry;
	}
	_temp->nextEntry=_addThis;
}
struct nList* addnList(struct nList** _passed){
	struct nList* _addThis = lowNewnList();
	appendnList(_passed,_addThis);
	return _addThis;
}
struct nList* prependnList(struct nList** _passed){
	struct nList* _addThis = lowNewnList();
	if (*_passed==NULL){
		return (*_passed=_addThis);
	}
	_addThis->nextEntry=*_passed;
	*_passed=_addThis;
	return _addThis;
}
struct nList* removenList(struct nList** _removeFrom, int _removeIndex){
	if (_removeIndex==0){
		struct nList* _tempHold = *_removeFrom;
		*_removeFrom=_tempHold->nextEntry;
		return _tempHold;
	}
	struct nList* _prev=*_removeFrom;
	int i=1;
	ITERATENLIST(_prev->nextEntry,{
			if (i==_removeIndex){
				_prev->nextEntry = _curnList->nextEntry;
				return _curnList;
			}else{
				_prev=_curnList;
				++i;
			}
		});
	return NULL;
}
void freenList(struct nList* _freeThis, char _freeMemory){
	if (!_freeThis){
		return;
	}
	ITERATENLIST(_freeThis,{
		if (_freeMemory){
			free(_curnList->data);
		}
		free(_curnList);
	})
}
