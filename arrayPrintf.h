/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
typedef enum{
	PRINTFTYPE_NONE=0,
	PRINTFTYPE_NUM,
	PRINTFTYPE_DOUBLE,
}printfType;
struct printfArray{
	int count;
	const char** formats;
	printfType* types;
	void** vals;
	char* res;
};
void doneInitPrintfArray(struct printfArray* _passed);
void freePrintfArray(struct printfArray* _passed);
void initPrintfArray(struct printfArray* _passed, int _count);
void printfArrayCalculate(struct printfArray* _passed);
void easyStaticPrintfArray(struct printfArray* _passed, const char* _staticString);
void easyNumPrintfArray(struct printfArray* _passed, int* _printThis);
void easyDoublePrintfArray(struct printfArray* _passed, double* _printThis);
