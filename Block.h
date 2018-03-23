#pragma once
#include "BWEB.h"

namespace BWEB
{

	using namespace BWAPI;
	using namespace std;

	class Block
	{
		int w, h;
		TilePosition t;
		set <TilePosition> small, medium, large;
	public:
		Block() { };
		Block(int width, int height, TilePosition tile) { w = width, h = height, t = tile; }
		int width() const { return w; }
		int height() const { return h; }

		// Returns the top left tile position of this block
		TilePosition Location() const { return t; }

		// Returns the const set of tilepositions that belong to 2x2 (small) buildings
		set<TilePosition> SmallTiles() const { return small; }

		// Returns the const set of tilepositions that belong to 3x2 (medium) buildings
		set<TilePosition> MediumTiles() const { return medium; }

		// Returns the const set of tilepositions that belong to 4x3 (large) buildings
		set<TilePosition> LargeTiles() const { return large; }
		
		void insertSmall(TilePosition here) { small.insert(here); }
		void insertMedium(TilePosition here) { medium.insert(here); }
		void insertLarge(TilePosition here) { large.insert(here); }		
	};
}
