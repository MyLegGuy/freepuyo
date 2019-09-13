typedef enum{
	PRINTFTYPE_NONE=0,
	PRINTFTYPE_NUM,
}printfType;
struct printfArray{
	int count;
	const char** formats;
	printfType* types;
	int** vals;
	char* res;
};
void doneInitPrintfArray(struct printfArray* _passed);
void freePrintfArray(struct printfArray* _passed);
void initPrintfArray(struct printfArray* _passed, int _count);
void printfArrayCalculate(struct printfArray* _passed);
void printfArraySoftMaxCalculate(struct printfArray* _passed);
void _lowPrintfArrayCalculate(struct printfArray* _passed, signed char _doFakeNum, int _fakedNum);
void easyStaticPrintfArray(struct printfArray* _passed, const char* _staticString);
void easyNumPrintfArray(struct printfArray* _passed, int* _printThis);
