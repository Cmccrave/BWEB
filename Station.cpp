#include "Station.h"

namespace BWEB
{
	Station::Station(TilePosition newResourceCenter, set<TilePosition> newDefenses, const BWEM::Base* newBase)
	{
		resourceCenter = newResourceCenter;
		defenses = newDefenses;
		base = newBase;
	}

	void Map::findStations()
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

				TilePosition here = base.Location();
				set<Unit> minerals, geysers;

				for (auto m : base.Minerals()) { minerals.insert(m->Unit()); }
				for (auto g : base.Geysers()) { geysers.insert(g->Unit()); }

				Station newStation(genCenter, stationDefenses(base.Location(), h, v), &base);
				stations.push_back(newStation);
			}
		}
	}

	set<TilePosition>& Map::stationDefenses(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
	{
		returnValues.clear();
		if (mirrorVertical)
		{
			if (mirrorHorizontal) returnValues.insert({ here + TilePosition(4, 0), here + TilePosition(0, 3), here + TilePosition(4, 3) });
			else returnValues.insert({ here + TilePosition(-2, 3), here + TilePosition(-2, 0), here + TilePosition(2, 3) });
		}
		else
		{
			if (mirrorHorizontal) returnValues.insert({ here + TilePosition(4, -2), here + TilePosition(0, -2), here + TilePosition(4, 1) });
			else returnValues.insert({ here + TilePosition(-2, -2), here + TilePosition(2, -2), here + TilePosition(-2, 1) });
		}
		return returnValues;
	}

	Station Map::getClosestStation(TilePosition here)
	{
		//double distBest = DBL_MAX;
		//for (auto &station : stations)
		//{
		//	double dist = here.getDistance(station.BWEMBase()->Location());
		//	if (dist < distBest)
		//		distBest = dist;
		//	return station;
		//}
	}
}