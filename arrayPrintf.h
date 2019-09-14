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
