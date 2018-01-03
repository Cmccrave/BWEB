#include "Base.h"

namespace BWEB
{
	Base::Base(TilePosition newTile, TilePosition newResourceCenter, set<TilePosition> newDefenses, const BWEM::Base* newBase)
	{
		location = newTile;
		resourceCenter = newResourceCenter;		
		defenses = newDefenses;
		base = newBase;
	}

	void Map::findBases()
	{
		for (auto &area : BWEM::Map::Instance().Areas())
		{
			for (auto &base : area.Bases())
			{
				bool h = false, v = false;
				if (base.Center().x > BWEM::Map::Instance().Center().y) h = true;
				TilePosition genCenter; Position gasCenter;
				int cnt = 0;
				for (auto &mineral : base.Minerals())
					genCenter += mineral->TopLeft(), cnt++;

				for (auto &gas : base.Geysers())
				{
					genCenter += gas->TopLeft();
					cnt++;
					gasCenter = gas->Pos();
				}

				if (gasCenter.y > base.Center().y) v = true;
				if (cnt > 0) genCenter = genCenter / cnt;
				resourceCenter.insert(genCenter);

				TilePosition here = base.Location();
				set<Unit> minerals, geysers;

				for (auto m : base.Minerals()) { minerals.insert(m->Unit()); }
				for (auto g : base.Geysers()) { geysers.insert(g->Unit()); }

				Base newBase(here, genCenter, baseDefenses(base.Location(), h, v), &base);
				bases.insert(newBase);
			}
		}
	}

	set<TilePosition>& Map::baseDefenses(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		set<TilePosition> returnValues;
		if (mirrorVertical)
		{
			if (mirrorHorizontal)
			{
				returnValues.insert(here + TilePosition(0, 3));
				returnValues.insert(here + TilePosition(4, 3));
				returnValues.insert(here + TilePosition(4, 0));
			}
			else
			{
				returnValues.insert(here + TilePosition(-2, 0));
				returnValues.insert(here + TilePosition(-2, 3));
				returnValues.insert(here + TilePosition(2, 3));
			}
		}
		else
		{
			if (mirrorHorizontal)
			{
				returnValues.insert(here + TilePosition(0, -2));
				returnValues.insert(here + TilePosition(4, -2));
				returnValues.insert(here + TilePosition(4, 1));
			}
			else
			{
				returnValues.insert(here + TilePosition(2, -2));
				returnValues.insert(here + TilePosition(-2, -2));
				returnValues.insert(here + TilePosition(-2, 1));
			}
		}
		return returnValues;
	}	
}