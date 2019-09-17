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

puzzleGeneric.o: main.h puzzleGeneric.h
skinLoader.o: skinLoader.h
puyo.o: skinLoader.h scoreConstants.h main.h puzzleGeneric.h puyo.h
puyo.o: goodLinkedList.h
ui.o: main.h ui.h arrayPrintf.h
yoshi.o: main.h yoshi.h puzzleGeneric.h goodLinkedList.h
menu.o: main.h puzzleGeneric.h puyo.h goodLinkedList.h yoshi.h skinLoader.h
menu.o: ui.h arrayPrintf.h
main.o: main.h yoshi.h puzzleGeneric.h puyo.h goodLinkedList.h menu.h
arrayPrintf.o: arrayPrintf.h
goodLinkedList.o: goodLinkedList.h
