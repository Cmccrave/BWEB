#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB::Stations {

    namespace {
        vector<Station> stations;
    }

    set<TilePosition> stationDefenses(TilePosition here, bool mirrorHorizontal, bool mirrorVertical)
    {
        set<TilePosition> defenses;

        const auto &offset = [&](int x, int y) {
            return here + TilePosition(x, y);
        };

        const auto &topLeft = [&]() {
            defenses.insert({
                offset(-2, -2),
                offset(2, -2),
                offset(-2, 1) });
            if (here != Map::getMainTile() && here != Map::getNaturalTile())
                defenses.insert({
                offset(4, -1),
                offset(4, 1),
                offset(4, 3),
                offset(0,3),
                offset(2, 3) });
        };

        const auto &topRight = [&]() {
            if (Broodwar->self()->getRace() == Races::Terran)
                defenses.insert({
                offset(4, -2),
                offset(0, -2) });
            else {
                defenses.insert({
                offset(4, -2),
                offset(0, -2),
                offset(4, 1) });

                if (here != Map::getMainTile() && here != Map::getNaturalTile())
                    defenses.insert({
                    offset(-2, -1),
                    offset(-2, 1),
                    offset(-2, 3),
                    offset(0, 3),
                    offset(2, 3) });
            }
        };

        const auto &bottomLeft = [&]() {
            defenses.insert({
                offset(-2, 3),
                offset(-2, 0),
                offset(2, 3) });

            if (here != Map::getMainTile() && here != Map::getNaturalTile())
                defenses.insert({
                offset(0, -2),
                offset(2, -2),
                offset(4, -2),
                offset(4, 0),
                offset(4, 2) });
        };

        const auto &bottomRight = [&]() {
            if (Broodwar->self()->getRace() == Races::Terran)
                defenses.insert({
                offset(0, 3),
                offset(4, 3) });
            else {
                defenses.insert({
                offset(4, 0),
                offset(0, 3),
                offset(4, 3) });

                if (here != Map::getMainTile() && here != Map::getNaturalTile())
                    defenses.insert({
                    offset(-2, 2),
                    offset(-2, 0),
                    offset(-2,-2),
                    offset(0, -2),
                    offset(2, -2) });
            }
        };

        if (mirrorVertical) {
            if (mirrorHorizontal)
                bottomRight();
            else
                bottomLeft();
        }
        else {
            if (mirrorHorizontal)
                topRight();
            else
                topLeft();
        }

        // Check if it's buildable (some maps don't allow for these tiles, like Destination)
        set<TilePosition>::iterator itr = defenses.begin();
        while (itr != defenses.end()) {
            auto tile = *itr;
            if (!Map::isPlaceable(UnitTypes::Protoss_Photon_Cannon, tile))
                itr = defenses.erase(itr);
            else
                ++itr;
        }

        // Temporary fix for CC Addons
        if (Broodwar->self()->getRace() == Races::Terran)
            defenses.insert(here + TilePosition(4, 1));

        // Add overlap
        for (auto &tile : defenses)
            Map::addReserve(tile, 2, 2);

        return defenses;
    }

    void findStations()
    {
        const auto addResourceOverlap = [&](Position genCenter) {
            TilePosition start(genCenter);
            for (int x = start.x - 4; x < start.x + 4; x++) {
                for (int y = start.y - 4; y < start.y + 4; y++) {
                    TilePosition t(x, y);
                    if (!t.isValid())
                        continue;
                    if (t.getDistance(start) <= 4)
                        Map::addReserve(t, 1, 1);
                }
            }
        };

        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {
                auto h = false, v = false;
                Position genCenter, sCenter;
                auto cnt = 0;

                for (auto &mineral : base.Minerals()) {
                    genCenter += mineral->Pos();
                    cnt++;
                }

                if (cnt > 0)
                    sCenter = genCenter / cnt;

                for (auto &gas : base.Geysers()) {
                    sCenter = (sCenter + gas->Pos()) / 2;
                    genCenter += gas->Pos();
                    cnt++;
                }

                if (cnt > 0)
                    genCenter = genCenter / cnt;

                h = base.Center().x < sCenter.x;
                v = base.Center().y < sCenter.y;

                for (auto &m : base.Minerals())
                    Map::addReserve(m->TopLeft(), 2, 1);

                for (auto &g : base.Geysers())
                    Map::addReserve(g->TopLeft(), 4, 2);

                Station newStation(genCenter, stationDefenses(base.Location(), h, v), &base);
                stations.push_back(newStation);
                Map::addReserve(base.Location(), 4, 3);
                addResourceOverlap(genCenter);

                if (Broodwar->self()->getRace() == Races::Zerg)
                    Map::addReserve(base.Location() - TilePosition(1,1), 6, 5);
            }
        }
    }

    void draw()
    {
        for (auto &station : Stations::getStations()) {
            for (auto &tile : station.getDefenseLocations())
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), Broodwar->self()->getColor());
            Broodwar->drawBoxMap(Position(station.getBWEMBase()->Location()), Position(station.getBWEMBase()->Location()) + Position(129, 97), Broodwar->self()->getColor());
        }
    }

    Station * getClosestStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Station* bestStation = nullptr;
        for (auto &station : stations) {
            const auto dist = here.getDistance(station.getBWEMBase()->Location());

            if (dist < distBest) {
                distBest = dist;
                bestStation = &station;
            }
        }
        return bestStation;
    }

    vector<Station>& getStations() {
        return stations;
    }
}