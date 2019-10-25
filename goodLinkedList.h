/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef NLISTHEADERINCLUDED
#define NLISTHEADERINCLUDED

#define ITERATENLIST(_passedStart,_passedCode)					\
	{															\
		if (_passedStart!=NULL){								\
			struct nList* _curnList=_passedStart;				\
			struct nList* _cachedNext;							\
			do{													\
				_cachedNext = _curnList->nextEntry;				\
				_passedCode;									\
			}while((_curnList=_cachedNext));					\
		}														\
	}

struct nList{
	struct nList* nextEntry;
	void* data;
};

struct nList* addnList(struct nList** _passed);
void freenList(struct nList* _freeThis, char _freeMemory);
// Makes an empty node. If you want to make a new list, set your first node to NULL
struct nList* lowNewnList();
int nListLen(struct nList* _passed);
struct nList* getnList(struct nList* _passed, int _index);
struct nList* removenList(struct nList** _removeFrom, int _removeIndex);
void appendnList(struct nList** _source, struct nList* _addThis);
struct nList* prependnList(struct nList** _passed); // fastest way to add to list

#endif
