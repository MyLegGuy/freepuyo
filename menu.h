/*
	Copyright (C) 2019  MyLegGuy
	This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#define STDBUTTONSEPARATION (1.2)
#define STDCORNERH (curFontHeight)
void titleScreen(struct gameState* _ret);
void spawnWinMenu(u64 _sTime);
void spawnLoseMenu(u64 _sTime);
void loadGlobalUI();
