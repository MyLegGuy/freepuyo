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
#endif
