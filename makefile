src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -lgoodbrew -lm -lpthread -lSDL2_image -lSDL2_ttf -lSDLFontCache -lSDL2_mixer `sdl2-config --cflags --libs` -Llib -ldl
CFLAGS = -g -Wall
OUTNAME = a.out

$(OUTNAME): $(obj)
	$(CC) -o $(OUTNAME) $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(OUTNAME)