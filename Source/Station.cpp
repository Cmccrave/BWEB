#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB {

    int Station::getGroundDefenseCount()
    {
        int count = 0;
        for (auto &defense : defenses) {
            auto type = Map::isUsed(defense);
            if (type == UnitTypes::Protoss_Photon_Cannon
                || type == UnitTypes::Zerg_Sunken_Colony
                || type == UnitTypes::Terran_Bunker)
                count++;
        }
        return count;
    }

    int Station::getAirDefenseCount()
    {
        int count = 0;
        for (auto &defense : defenses) {
            auto type = Map::isUsed(defense);
            if (type == UnitTypes::Protoss_Photon_Cannon
                || type == UnitTypes::Zerg_Spore_Colony
                || type == UnitTypes::Terran_Missile_Turret)
                count++;
        }
        return count;
    }

    void Station::draw()
    {
        int color = Broodwar->self()->getColor();
        int textColor = color == 185 ? textColor = Text::DarkGreen : Broodwar->self()->getTextColor();

        // Draw boxes around each feature
        for (auto &tile : defenses) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
            Broodwar->drawTextMap(Position(tile) + Position(4, 52), "%cS", textColor);
        }
        Broodwar->drawBoxMap(Position(base->Location()), Position(base->Location()) + Position(129, 97), color);
    }
}

namespace BWEB::Stations {

    namespace {
        vector<Station> stations;
        vector<Station> mains;
        vector<Station> naturals;
    }

    set<TilePosition> stationDefenses(const BWEM::Base& base, bool placeRight, bool placeBelow, bool isMain, bool isNatural)
    {
        set<TilePosition> defenses;
        set<TilePosition> basePlacements;
        set<TilePosition> geyserPlacements;
        auto here = base.Location();

        const auto &topLeft = [&]() {
            if (!isMain)
                basePlacements ={ {-2, -2}, {1, -2}, {-2, 1},                       // Close
                                  {4, -1}, {4, 1}, {4, 3}, {0, 3}, {2, 3} };        // Far
            else
                basePlacements ={ {-2, -2}, {1, -2}, {-2, 1} };                     // Close

            if (!isNatural) {
                geyserPlacements ={ {0, -2}, {2, -2}, // Above
                                    {-2, 0}, {4, 0},  // Sides
                                    {2, 2}            // Below
                };
            }
        };

        const auto &topRight = [&]() {
            if (!isMain)
                basePlacements ={ {4, -2}, {1, -2}, {4, 1},                         // Close
                                  {-2, -1}, {-2, 1}, {-2, 3}, {0, 3}, {2, 3} };     // Far
            else
                basePlacements ={ {4, -2}, {1, -2}, {4, 1} };                       // Close

            if (!isNatural) {
                geyserPlacements ={ {0, -2}, {2, -2}, // Above
                                    {-2, 0}, {4, 0},  // Sides
                                    {0, 2}            // Below
                };
            }
        };

        const auto &bottomLeft = [&]() {
            if (!isMain)
                basePlacements ={ {-2, 3}, {-2, 0}, {1, 3},                         // Close
                                  {0, -2}, {2, -2}, {4, -2}, {4, 0}, {4, 2} };      // Far
            else
                basePlacements ={ {-2, 3}, {-2, 0}, {1, 3} };                       // Close

            if (!isNatural) {
                geyserPlacements ={ {2, -2},          // Above
                                    {-2, 0}, {4, 0},  // Sides
                                    {0, 2}, {2, 2}    // Below
                };
            }
        };

        const auto &bottomRight = [&]() {
            if (!isMain)
                basePlacements ={ {4, 0}, {1, 3}, {4, 3},                           // Close
                                  {-2, 2}, {-2, 0}, {-2, -2}, {0, -2}, {2, -2} };   // Far
            else
                basePlacements ={ {4, 0}, {1, 3}, {4, 3} };                         // Close

            if (!isNatural) {
                geyserPlacements ={ {0, -2},          // Above
                                    {-2, 0}, {4, 0},  // Sides
                                    {0, 2}, {2, 2}    // Below
                };
            }
        };

        // Insert defenses
        placeBelow ? (placeRight ? bottomRight() : bottomLeft()) : (placeRight ? topRight() : topLeft());
        auto defenseType = UnitTypes::None;
        if (Broodwar->self()->getRace() == Races::Protoss)
            defenseType = UnitTypes::Protoss_Photon_Cannon;
        if (Broodwar->self()->getRace() == Races::Terran)
            defenseType = UnitTypes::Terran_Missile_Turret;
        if (Broodwar->self()->getRace() == Races::Zerg)
            defenseType = UnitTypes::Zerg_Creep_Colony;

        // Add scanner addon for Terran
        if (Broodwar->self()->getRace() == Races::Terran) {
            auto scannerTile = here + TilePosition(4, 1);
            defenses.insert(scannerTile);
            Map::addReserve(scannerTile, 2, 2);
            Map::addUsed(scannerTile, defenseType);
        }

        // Add a defense near each base placement if possible
        for (auto &placement : basePlacements) {
            auto tile = base.Location() + placement;
            if (Map::isPlaceable(defenseType, tile)) {
                defenses.insert(tile);
                Map::addReserve(tile, 2, 2);
                Map::addUsed(tile, defenseType);
            }
        }

        // Add a defense near the geysers of this base if possible
        for (auto &geyser : base.Geysers()) {
            for (auto &placement : geyserPlacements) {
                auto tile = geyser->TopLeft() + placement;
                if (Map::isPlaceable(defenseType, tile)) {
                    defenses.insert(tile);
                    Map::addReserve(tile, 2, 2);
                    Map::addUsed(tile, defenseType);
                }
            }
        }

        // Remove used
        for (auto &tile : defenses)
            Map::removeUsed(tile, 2, 2);

        return defenses;
    }

    void findStations()
    {
        const auto addResourceOverlap = [&](Position resourceCenter, Position startCenter, Position stationCenter) {
            TilePosition start(startCenter);
            TilePosition test = start;

            while (test != TilePosition(stationCenter)) {
                auto distBest = DBL_MAX;
                start = test;
                for (int x = start.x - 1; x <= start.x + 1; x++) {
                    for (int y = start.y - 1; y <= start.y + 1; y++) {
                        TilePosition t(x, y);
                        Position p = Position(t) + Position(16, 16);

                        if (!t.isValid())
                            continue;

                        auto dist = Map::isReserved(t) ? p.getDistance(stationCenter) + 16 : p.getDistance(stationCenter);
                        if (dist <= distBest) {
                            test = t;
                            distBest = dist;
                        }
                    }
                }

                if (test.isValid())
                    Map::addReserve(test, 1, 1);
            }
        };

        // Find all main bases
        vector<const BWEM::Base *> mainBases;
        vector<const BWEM::Base *> natBases;
        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {
                if (base.Starting())
                    mainBases.push_back(&base);
            }
        }

        // Find all natural bases        
        for (auto &main : mainBases) {

            const BWEM::Base * baseBest = nullptr;
            auto distBest = DBL_MAX;
            for (auto &area : Map::mapBWEM.Areas()) {
                for (auto &base : area.Bases()) {

                    // Must have gas, be accesible and at least 5 mineral patches
                    if (base.Starting()
                        || base.Geysers().empty()
                        || base.GetArea()->AccessibleNeighbours().empty()
                        || base.Minerals().size() < 5)
                        continue;

                    const auto dist = Map::getGroundDistance(base.Center(), main->Center());
                    if (dist < distBest) {
                        distBest = dist;
                        baseBest = &base;
                    }
                }
            }

            // Store any natural we found
            if (baseBest)
                natBases.push_back(baseBest);
        }

        for (auto &area : Map::mapBWEM.Areas()) {
            for (auto &base : area.Bases()) {
                auto resourceCentroid = Position(0, 0);
                auto defenseCentroid = Position(0, 0);
                auto cnt = 0;

                // Resource and defense centroids
                for (auto &mineral : base.Minerals()) {
                    resourceCentroid += mineral->Pos();
                    cnt++;
                }

                if (cnt > 0)
                    defenseCentroid = resourceCentroid / cnt;

                for (auto &gas : base.Geysers()) {
                    defenseCentroid = (defenseCentroid + gas->Pos()) / 2;
                    resourceCentroid += gas->Pos();
                    cnt++;
                }

                if (cnt > 0)
                    resourceCentroid = resourceCentroid / cnt;

                // Add reserved tiles
                for (auto &m : base.Minerals()) {
                    Map::addReserve(m->TopLeft(), 2, 1);
                    addResourceOverlap(resourceCentroid, m->Pos(), base.Center());
                }
                for (auto &g : base.Geysers()) {
                    Map::addReserve(g->TopLeft(), 4, 2);
                    addResourceOverlap(resourceCentroid, g->Pos(), base.Center());
                }
                Map::addReserve(base.Location(), 4, 3);

                // Station info
                auto isMain = find(mainBases.begin(), mainBases.end(), &base) != mainBases.end();
                auto isNatural = find(natBases.begin(), natBases.end(), &base) != natBases.end();
                auto defenses = stationDefenses(base, base.Center().x < defenseCentroid.x, base.Center().y < defenseCentroid.y, isMain, isNatural);

                // Add to our station lists
                Station newStation(resourceCentroid, defenses, &base, isMain, isNatural);
                stations.push_back(newStation);

                if (isMain)
                    mains.push_back(newStation);
                if (isNatural)
                    naturals.push_back(newStation);
            }
        }
    }

    void draw()
    {
        for (auto &station : Stations::getStations())
            station.draw();
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

    Station * getClosestMainStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Station* bestStation = nullptr;
        for (auto &station : mains) {
            const auto dist = here.getDistance(station.getBWEMBase()->Location());

            if (dist < distBest) {
                distBest = dist;
                bestStation = &station;
            }
        }
        return bestStation;
    }

    Station * getClosestNaturalStation(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Station* bestStation = nullptr;
        for (auto &station : naturals) {
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

    vector<Station>& getMainStations() {
        return mains;
    }

    vector<Station>& getNaturalStations() {
        return naturals;
    }
}