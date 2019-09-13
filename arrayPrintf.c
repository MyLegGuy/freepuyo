#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arrayPrintf.h"
void doneInitPrintfArray(struct printfArray* _passed){
	int _total=0;
	int i;
	for (i=0;i<_passed->count;++i){
		_total+=strlen(_passed->formats[i]);
		if (_passed->types[i]==PRINTFTYPE_NUM){
			_total+=10;
		}
	}
	_passed->res = malloc(_total+1);
}
void initPrintfArray(struct printfArray* _passed, int _count){
	_passed->count=_count;
	_passed->formats=malloc(sizeof(char*)*_count);
	_passed->types=calloc(1,sizeof(printfType)*_count);
	_passed->vals=malloc(sizeof(int*)*_count);
}
void freePrintfArray(struct printfArray* _passed){
	free(_passed->formats);
	free(_passed->types);
	free(_passed->vals);
	free(_passed->res);
}
void _lowPrintfArrayCalculate(struct printfArray* _passed, signed char _doFakeNum, int _fakedNum){
	char* _curPos=_passed->res;
	int i;
	for (i=0;i<_passed->count;++i){
		if (_passed->types[i]==PRINTFTYPE_NUM){
			_curPos+=sprintf(_curPos,_passed->formats[i],_doFakeNum ? _fakedNum : *_passed->vals[i]);
		}else{
			int _cachedStrlen=strlen(_passed->formats[i]);
			memcpy(_curPos,_passed->formats[i],_cachedStrlen);
			_curPos+=_cachedStrlen;
		}
	}
	_curPos[0]='\0';
}
void printfArrayCalculate(struct printfArray* _passed){
	_lowPrintfArrayCalculate(_passed,0,0);
}
void printfArraySoftMaxCalculate(struct printfArray* _passed){
	_lowPrintfArrayCalculate(_passed,1,99);
}
void easyStaticPrintfArray(struct printfArray* _passed, const char* _staticString){
	initPrintfArray(_passed,1);
	_passed->formats[0]=_staticString;
	_passed->res=malloc(strlen(_staticString)+1);
	free(_passed->vals);
	_passed->vals=NULL;
}
void easyNumPrintfArray(struct printfArray* _passed, int* _printThis){
	initPrintfArray(_passed,1);
	_passed->formats[0]="%d";
	_passed->types[0]=PRINTFTYPE_NUM;
	_passed->res=malloc(13);
	_passed->vals[0]=_printThis;
}
