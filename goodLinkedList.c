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
void freenList(struct nList* _freeThis, char _freeMemory){
	ITERATENLIST(_freeThis,{
		if (_freeMemory){
			free(_curnList->data);
		}
		free(_curnList);
	})
}