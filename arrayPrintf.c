/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
	_passed->vals=malloc(sizeof(void*)*_count);
}
void freePrintfArray(struct printfArray* _passed){
	free(_passed->formats);
	free(_passed->types);
	free(_passed->vals);
	free(_passed->res);
}
void printfArrayCalculate(struct printfArray* _passed){
	char* _curPos=_passed->res;
	int i;
	for (i=0;i<_passed->count;++i){
		if (_passed->types[i]!=0){
			switch(_passed->types[i]){
				case PRINTFTYPE_NUM:
					_curPos+=sprintf(_curPos,_passed->formats[i],*((int*)_passed->vals[i]));
					break;
				case PRINTFTYPE_DOUBLE:
					_curPos+=sprintf(_curPos,_passed->formats[i],*((double*)_passed->vals[i]));
					break;
			}
		}else{
			int _cachedStrlen=strlen(_passed->formats[i]);
			memcpy(_curPos,_passed->formats[i],_cachedStrlen);
			_curPos+=_cachedStrlen;
		}
	}
	_curPos[0]='\0';
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
void easyDoublePrintfArray(struct printfArray* _passed, double* _printThis){
	initPrintfArray(_passed,1);
	_passed->formats[0]="%.2f";
	_passed->types[0]=PRINTFTYPE_DOUBLE;
	_passed->res=malloc(13);
	_passed->vals[0]=_printThis;
}
