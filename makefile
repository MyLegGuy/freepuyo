src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -lgoodbrew -lSDLFontCache -lSDL2_ttf -lSDL2_image `sdl2-config --cflags --libs` -lm
CFLAGS = -g -Wall -Wno-switch -Wno-char-subscripts `sdl2-config --cflags`
OUTNAME = a.out

$(OUTNAME): $(obj)
	$(CC) -o $(OUTNAME) $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(OUTNAME)

.PHONY: depend
depend:
	makedepend -Y $(src)

# DO NOT DELETE

puzzleGeneric.o: main.h puzzleGeneric.h ui.h arrayPrintf.h
skinLoader.o: skinLoader.h
puyo.o: skinLoader.h scoreConstants.h main.h puzzleGeneric.h ui.h
puyo.o: arrayPrintf.h pieceSet.h puyo.h goodLinkedList.h
ui.o: main.h menu.h ui.h arrayPrintf.h
yoshi.o: main.h yoshi.h puzzleGeneric.h ui.h arrayPrintf.h goodLinkedList.h
menu.o: main.h menu.h puzzleGeneric.h ui.h arrayPrintf.h puyo.h
menu.o: goodLinkedList.h pieceSet.h yoshi.h heal.h skinLoader.h
main.o: main.h yoshi.h puzzleGeneric.h ui.h arrayPrintf.h heal.h pieceSet.h
main.o: puyo.h goodLinkedList.h menu.h skinLoader.h
heal.o: main.h goodLinkedList.h heal.h puzzleGeneric.h ui.h arrayPrintf.h
heal.o: pieceSet.h
arrayPrintf.o: arrayPrintf.h
pieceSet.o: main.h puzzleGeneric.h ui.h arrayPrintf.h pieceSet.h
goodLinkedList.o: goodLinkedList.h
