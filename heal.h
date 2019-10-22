#include "puzzleGeneric.h"
#include "pieceSet.h"

struct healSettings{
	int numColors;
	int fallTime;
	int rowTime;
};
struct healSkin{
};
struct healBoard{
	struct genericBoard lowBoard; // Negative versions represent targets
	struct nList* activeSets;
	struct healSettings settings;
	struct healSkin* skin;
};
void drawHealBoard(struct healBoard* _passedBoard, int _drawX, int _drawY, int tilew, u64 _sTime);
void initHealSettings(struct healSettings* _passedSettings);
void addHealBoard(struct gameState* _passedState, int i, int _w, int _h, struct healSettings* _usableSettings, struct healSkin* _passedSkin);
void updateHealBoard(struct healBoard* _passedBoard, gameMode _mode, u64 _sTime);
