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

skinLoader.o: skinLoader.h
puyo.o: skinLoader.h scoreConstants.h main.h puzzleGeneric.h puyo.h
puyo.o: goodLinkedList.h
puzzleGeneric.o: main.h puzzleGeneric.h
ui.o: main.h ui.h
yoshi.o: main.h yoshi.h puzzleGeneric.h goodLinkedList.h
main.o: main.h yoshi.h puzzleGeneric.h puyo.h goodLinkedList.h ui.h
goodLinkedList.o: goodLinkedList.h
