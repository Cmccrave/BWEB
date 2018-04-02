#include "Station.h"
#include <chrono>

namespace BWEB
{
	Station::Station(const Position newResourceCenter, const set<TilePosition>& newDefenses, const BWEM::Base* newBase)
	{
		resourceCentroid = newResourceCenter;
		defenses = newDefenses;
		base = newBase;
	}

	void Map::findStations()
	{
		auto const start{ chrono::high_resolution_clock::now() };

		for (auto& area : BWEM::Map::Instance().Areas())
		{
			for (auto& base : area.Bases())
			{
				auto h = false, v = false;

				Position genCenter, sCenter;
				auto cnt = 0;
				for (auto& mineral : base.Minerals())
					genCenter += mineral->Pos(), cnt++;

				if (cnt > 0) sCenter = genCenter / cnt;

				for (auto& gas : base.Geysers())
				{
					sCenter = (sCenter + gas->Pos()) / 2;
					genCenter += gas->Pos();
					cnt++;
				}

				if (cnt > 0) genCenter = genCenter / cnt;

				if (base.Center().x < sCenter.x) h = true;
				if (base.Center().y < sCenter.y) v = true;

				auto here = base.Location();
				set<Unit> minerals, geysers;

				for (auto m : base.Minerals()) { minerals.insert(m->Unit()); }
				for (auto g : base.Geysers()) { geysers.insert(g->Unit()); }

				const Station newStation(genCenter, stationDefenses(base.Location(), h, v), &base);
				stations.push_back(newStation);
				addOverlap(base.Location(), 4, 3);
			}
		}

		const auto dur = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
		Broodwar << "Station time: " << dur << endl;
	}

	set<TilePosition>& Map::stationDefenses(const TilePosition here, const bool mirrorHorizontal, const bool mirrorVertical)
	{
		returnValues.clear();
		if (mirrorVertical)
		{
			if (mirrorHorizontal)
			{
				if (Broodwar->self()->getRace() == Races::Terran)
					returnValues.insert({ here + TilePosition(0, 3), here + TilePosition(4, 3) });
				else
					returnValues.insert({ here + TilePosition(4, 0), here + TilePosition(0, 3), here + TilePosition(4, 3) });
			}
			else returnValues.insert({ here + TilePosition(-2, 3), here + TilePosition(-2, 0), here + TilePosition(2, 3) });
		}
		else
		{
			if (mirrorHorizontal)
			{
				// Temporary fix for CC Addons
				if (Broodwar->self()->getRace() == Races::Terran)
					returnValues.insert({ here + TilePosition(4, -2), here + TilePosition(0, -2) });
				else
					returnValues.insert({ here + TilePosition(4, -2), here + TilePosition(0, -2), here + TilePosition(4, 1) });
			}
			else
				returnValues.insert({ here + TilePosition(-2, -2), here + TilePosition(2, -2), here + TilePosition(-2, 1) });
		}

		// Temporary fix for CC Addons
		if (Broodwar->self()->getRace() == Races::Terran)
			returnValues.insert(here + TilePosition(4, 1));

		for (auto& tile : returnValues)
			addOverlap(tile, 2, 2);

		return returnValues;
	}

	const Station* Map::getClosestStation(TilePosition here) const
	{
		auto distBest = DBL_MAX;
		const Station* bestStation = nullptr;
		for (auto& station : stations)
		{
			const auto dist = here.getDistance(station.BWEMBase()->Location());

			if (dist < distBest)
			{
				distBest = dist;
				bestStation = &station;
			}
		}
		return bestStation;
	}
}