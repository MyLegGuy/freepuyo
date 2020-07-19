#include "puzzleGeneric.h"
#include "pieceSet.h"

struct healSettings{
	int numColors;
	int fallTime;
	int rowTime;
	double pushMultiplier;
	int popTime;
	int minPop;
	int nextWindowTime;
	int hMoveTime;
};
struct healSkin{
};
struct healBoard{
	struct genericBoard lowBoard; // Negative versions represent targets
	struct nList* activeSets;
	struct healSettings settings;
	struct healSkin* skin;
	struct nList* checkQueue;
};
void drawHealBoard(struct healBoard* _passedBoard, int _drawX, int _drawY, int tilew, u64 _sTime);
void initHealSettings(struct healSettings* _passedSettings);
void addHealBoard(struct gameState* _passedState, int i, int _w, int _h, struct healSettings* _usableSettings, struct healSkin* _passedSkin, struct controlSettings* _controlSettings);
void updateHealBoard(struct gameState* _passedState, struct healBoard* _passedBoard, gameMode _mode, u64 _sTime);
