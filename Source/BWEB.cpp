#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB::Map
{
    namespace {
        Position mainPosition = Positions::Invalid;
        Position naturalPosition = Positions::Invalid;
        TilePosition mainTile = TilePositions::Invalid;
        TilePosition naturalTile = TilePositions::Invalid;
        const BWEM::Area * naturalArea = nullptr;
        const BWEM::Area * mainArea = nullptr;
        const BWEM::ChokePoint * naturalChoke = nullptr;
        const BWEM::ChokePoint * mainChoke = nullptr;

        int testGrid[256][256] ={};
        int reserveGrid[256][256] ={};
        int overlapGrid[256][256] ={};
        UnitType usedGrid[256][256] ={};
        bool walkGrid[256][256] ={};

        void findMain()
        {
            mainTile = Broodwar->self()->getStartLocation();
            mainPosition = Position(mainTile) + Position(64, 48);
            mainArea = mapBWEM.GetArea(mainTile);
        }

        void findNatural()
        {
            auto distBest = DBL_MAX;
            for (auto &area : mapBWEM.Areas()) {
                for (auto &base : area.Bases()) {

                    // Must have gas, be accesible and at least 5 mineral patches
                    if (base.Starting()
                        || base.Geysers().empty()
                        || area.AccessibleNeighbours().empty()
                        || base.Minerals().size() < 5)
                        continue;

                    const auto dist = getGroundDistance(base.Center(), mainPosition);
                    if (dist < distBest) {
                        distBest = dist;
                        naturalArea = base.GetArea();
                        naturalTile = base.Location();
                        naturalPosition = static_cast<Position>(naturalTile) + Position(64, 48);
                    }
                }
            }
        }

        void findMainChoke()
        {
            // Add all main chokes to a set
            set<BWEM::ChokePoint const *> mainChokes;
            set<BWEM::ChokePoint const *> naturalChokes;
            for (auto &choke : mainArea->ChokePoints())
                mainChokes.insert(choke);
            for (auto &choke : naturalArea->ChokePoints())
                naturalChokes.insert(choke);

            // If the natural area has only one chokepoint, then our main choke leads out of our base, find a choke that doesn't belong to the natural as well
            if (naturalArea && naturalArea->ChokePoints().size() == 1) {
                auto distBest = DBL_MAX;
                for (auto &choke : mainArea->ChokePoints()) {
                    const auto dist = getGroundDistance(Position(choke->Center()), mainPosition);
                    if (dist < distBest && naturalChokes.find(choke) == naturalChokes.end()) {
                        mainChoke = choke;
                        distBest = dist;
                    }
                }
            }

            // Find a chokepoint that belongs to main and natural areas            
            else if (naturalArea) {
                auto distBest = DBL_MAX;
                for (auto &choke : naturalArea->ChokePoints()) {
                    const auto dist = getGroundDistance(Position(choke->Center()), mainPosition);
                    if (dist < distBest && mainChokes.find(choke) != mainChokes.end()) {
                        mainChoke = choke;
                        distBest = dist;
                    }
                }
            }

            // If we didn't find a main choke that belongs to main and natural, check if a path exists between both positions
            if (!mainChoke && mainPosition.isValid() && naturalPosition.isValid()) {
                auto distBest = DBL_MAX;
                for (auto &choke : mapBWEM.GetPath(mainPosition, naturalPosition)) {
                    const auto width = choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2));
                    if (width < distBest) {
                        mainChoke = choke;
                        distBest = width;
                    }
                }
            }

            // If we still don't have a main choke, grab the closest chokepoint to our start
            if (!mainChoke) {
                auto distBest = DBL_MAX;
                for (auto &choke : mainArea->ChokePoints()) {
                    const auto dist = Position(choke->Center()).getDistance(mainPosition);
                    if (dist < distBest) {
                        mainChoke = choke;
                        distBest = dist;
                    }
                }
            }
        }

        void findNaturalChoke()
        {
            set<BWEM::ChokePoint const *> nonChokes;
            for (auto &choke : mapBWEM.GetPath(mainPosition, naturalPosition))
                nonChokes.insert(choke);

            // If the natural area has only one chokepoint, then choose as that
            if (naturalArea && naturalArea->ChokePoints().size() == 1)
                naturalChoke = naturalArea->ChokePoints().front();            

            // Find area that shares the choke we need to defend
            else {
                auto distBest = DBL_MAX;
                const BWEM::Area* second = nullptr;
                if (naturalArea) {
                    for (auto &area : naturalArea->AccessibleNeighbours()) {
                        auto center = area->Top();
                        const auto dist = Position(center).getDistance(mapBWEM.Center());

                        bool wrongArea = false;
                        for (auto &choke : area->ChokePoints()) {
                            if ((!choke->Blocked() && choke->Pos(choke->end1).getDistance(choke->Pos(choke->end2)) <= 2) || nonChokes.find(choke) != nonChokes.end()) {
                                wrongArea = true;
                            }
                        }
                        if (wrongArea)
                            continue;

                        if (center.isValid() && dist < distBest)
                            second = area, distBest = dist;
                    }

                    // Find second choke based on the connected area
                    distBest = DBL_MAX;
                    for (auto &choke : naturalArea->ChokePoints()) {
                        if (choke->Center() == mainChoke->Center()
                            || choke->Blocked()
                            || choke->Geometry().size() <= 3
                            || (choke->GetAreas().first != second && choke->GetAreas().second != second))
                            continue;

                        const auto dist = Position(choke->Center()).getDistance(Position(Broodwar->self()->getStartLocation()));
                        if (dist < distBest)
                            naturalChoke = choke, distBest = dist;
                    }
                }
            }
        }

        void findNeutrals()
        {
            // Add overlap for neutrals
            for (auto &unit : Broodwar->neutral()->getUnits()) {
                if (unit && unit->exists() && unit->getType().topSpeed() == 0.0)
                    addOverlap(unit->getTilePosition(), unit->getType().tileWidth(), unit->getType().tileHeight());
            }
        }
    }

    void onStart()
    {
        findNeutrals();
        findMain();
        findNatural();
        findMainChoke();
        findNaturalChoke();

        // Test
        for (int x = 0; x < Broodwar->mapWidth(); x++) {
            for (int y = 0; y < Broodwar->mapHeight(); y++) {

                usedGrid[x][y] = UnitTypes::None;

                auto walkable = true;
                for (int dx = x * 4; dx < (x * 4) + 4; dx++) {
                    for (int dy = y * 4; dy < (y * 4) + 4; dy++) {
                        if (WalkPosition(dx, dy).isValid() && !Broodwar->isWalkable(WalkPosition(dx, dy)))
                            walkable = false;
                    }
                }

                walkGrid[x][y] = walkable;
            }
        }
    }

    void onUnitDiscover(const Unit unit)
    {
        if (!unit
            || !unit->getType().isBuilding()
            || unit->isFlying()
            || unit->getType() == UnitTypes::Resource_Vespene_Geyser)
            return;

        const auto tile = unit->getTilePosition();
        const auto type = unit->getType();

        // Add used tiles
        for (auto x = tile.x; x < tile.x + type.tileWidth(); x++) {
            for (auto y = tile.y; y < tile.y + type.tileHeight(); y++) {
                TilePosition t(x, y);
                if (!t.isValid())
                    continue;
                usedGrid[x][y] = type;
            }
        }

        // Add defense count to stations
        if (type == UnitTypes::Protoss_Photon_Cannon
            || type == UnitTypes::Zerg_Sunken_Colony
            || type == UnitTypes::Zerg_Spore_Colony
            || type == UnitTypes::Terran_Missile_Turret) {

            for (auto &station : Stations::getStations()) {
                int defCnt = station.getDefenseCount();
                for (auto &defense : station.getDefenseLocations()) {
                    if (unit->getTilePosition() == defense) {
                        station.setDefenseCount(defCnt + 1);
                        return;
                    }
                }
            }
        }
    }

    void onUnitMorph(const Unit unit)
    {
        onUnitDiscover(unit);
    }

    void onUnitDestroy(const Unit unit)
    {
        if (!unit
            || !unit->getType().isBuilding()
            || unit->isFlying())
            return;

        const auto tile = unit->getTilePosition();
        const auto type = unit->getType();

        // Remove any used tiles
        for (auto x = tile.x; x < tile.x + type.tileWidth(); x++) {
            for (auto y = tile.y; y < tile.y + type.tileHeight(); y++) {
                TilePosition t(x, y);
                if (!t.isValid())
                    continue;
                usedGrid[x][y] = UnitTypes::None;
            }
        }

        // Remove defense count from stations
        if (type == UnitTypes::Protoss_Photon_Cannon
            || type == UnitTypes::Zerg_Sunken_Colony
            || type == UnitTypes::Zerg_Spore_Colony
            || type == UnitTypes::Terran_Missile_Turret) {

            for (auto &station : Stations::getStations()) {
                int defCnt = station.getDefenseCount();
                for (auto &defense : station.getDefenseLocations()) {
                    if (unit->getTilePosition() == defense) {
                        station.setDefenseCount(defCnt - 1);
                        return;
                    }
                }
            }
        }
    }

    void draw()
    {
        // Debugging stuff
        Broodwar->drawCircleMap((Position)mainChoke->Center(), 4, Colors::Red, true);
        Broodwar->drawCircleMap((Position)naturalChoke->Center(), 4, Colors::Green, true);

        // Draw Reserve Path and some grids
        for (int x = 0; x < Broodwar->mapWidth(); x++) {
            for (int y = 0; y < Broodwar->mapHeight(); y++) {
                TilePosition t(x, y);
                if (reserveGrid[x][y] >= 1)
                    Broodwar->drawBoxMap(Position(t), Position(t) + Position(33, 33), Colors::Black, false);
                if (overlapGrid[x][y] >= 1)
                    Broodwar->drawBoxMap(Position(t), Position(t) + Position(33, 33), Colors::Grey, false);
            }
        }

        Walls::draw();
        Blocks::draw();
        Stations::draw();
    }

    template <class T>
    double getGroundDistance(T s, T e)
    {
        Position start(s), end(e);
        auto dist = 0.0;
        if (!start.isValid()
            || !end.isValid()
            || !mapBWEM.GetArea(WalkPosition(start))
            || !mapBWEM.GetArea(WalkPosition(end))
            || !mapBWEM.GetArea(WalkPosition(start))->AccessibleFrom(mapBWEM.GetArea(WalkPosition(end))))
            return DBL_MAX;

        for (auto &cpp : mapBWEM.GetPath(start, end)) {
            auto center = Position(cpp->Center());
            dist += start.getDistance(center);
            start = center;
        }

        return dist += start.getDistance(end);
    }

    double distanceNextChoke(Position start, Position end)
    {
        if (!start.isValid() || !end.isValid())
            return DBL_MAX;

        auto bwemPath = mapBWEM.GetPath(start, end);
        auto source = bwemPath.front();


        BWEB::Path newPath;
        newPath.createUnitPath(start, (Position)source->Center());
        return newPath.getDistance();
    }

    TilePosition Map::getBuildPosition(UnitType type, const TilePosition searchCenter)
    {
        auto distBest = DBL_MAX;
        auto tileBest = TilePositions::Invalid;

        // Search through each block to find the closest block and valid position
        for (auto &block : Blocks::getBlocks()) {
            set<TilePosition> placements;
            if (type.tileWidth() == 4) placements = block.getLargeTiles();
            else if (type.tileWidth() == 3) placements = block.getMediumTiles();
            else placements = block.getSmallTiles();

            for (auto &tile : placements) {
                const auto dist = tile.getDistance(searchCenter);
                if (dist < distBest && isPlaceable(type, tile))
                    distBest = dist, tileBest = tile;
            }
        }
        return tileBest;
    }

    TilePosition Map::getDefBuildPosition(UnitType type, const TilePosition searchCenter)
    {
        auto distBest = DBL_MAX;
        auto tileBest = TilePositions::Invalid;

        // Search through each wall to find the closest valid TilePosition
        for (auto &wall : Walls::getWalls()) {
            for (auto &tile : wall.getDefenses()) {
                const auto dist = tile.getDistance(searchCenter);
                if (dist < distBest && isPlaceable(type, tile))
                    distBest = dist, tileBest = tile;
            }
        }

        // Search through each station to find the closest valid TilePosition
        for (auto &station : Stations::getStations()) {
            for (auto &tile : station.getDefenseLocations()) {
                const auto dist = tile.getDistance(searchCenter);
                if (dist < distBest && isPlaceable(type, tile))
                    distBest = dist, tileBest = tile;
            }
        }
        return tileBest;
    }

    bool isOverlapping(const TilePosition here, const int width, const int height, bool ignoreBlocks)
    {
        for (auto x = here.x; x < here.x + width; x++) {
            for (auto y = here.y; y < here.y + height; y++) {
                TilePosition t(x, y);
                if (!t.isValid())
                    continue;
                if (Map::overlapGrid[x][y] > 0)
                    return true;
            }
        }
        return false;
    }

    bool isReserved(const TilePosition here, const int width, const int height)
    {
        for (auto x = here.x; x < here.x + width; x++) {
            for (auto y = here.y; y < here.y + height; y++) {
                TilePosition t(x, y);
                if (!t.isValid())
                    continue;
                if (Map::reserveGrid[x][y] > 0)
                    return true;
            }
        }
        return false;
    }

    UnitType isUsed(const TilePosition here, const int width, const int height)
    {
        for (auto x = here.x; x < here.x + width; x++) {
            for (auto y = here.y; y < here.y + height; y++) {
                TilePosition t(x, y);
                if (!t.isValid())
                    continue;
                if (Map::usedGrid[x][y] != UnitTypes::None)
                    return Map::usedGrid[x][y];
            }
        }
        return UnitTypes::None;
    }

    bool isWalkable(const TilePosition here)
    {
        return walkGrid[here.x][here.y];
    }

    int tilesWithinArea(BWEM::Area const * area, const TilePosition here, const int width, const int height)
    {
        auto cnt = 0;
        for (auto x = here.x; x < here.x + width; x++) {
            for (auto y = here.y; y < here.y + height; y++) {
                TilePosition t(x, y);
                if (!t.isValid())
                    return false;

                // Make an assumption that if it's on a chokepoint geometry, it belongs to the area provided
                if (Map::mapBWEM.GetArea(t) == area /*|| !mapBWEM.GetArea(t)*/)
                    cnt++;
            }
        }
        return cnt;
    }

    pair<Position, Position> lineOfBestFit(const BWEM::ChokePoint * choke)
    {
        Position p1, p2;
        int minX= INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
        double sumX = 0, sumY = 0;
        double sumXY = 0, sumX2 = 0, sumY2 = 0;
        for (auto geo : choke->Geometry()) {
            if (geo.x < minX) minX = geo.x;
            if (geo.y < minY) minY = geo.y;
            if (geo.x > maxX) maxX = geo.x;
            if (geo.y > maxY) maxY = geo.y;

            BWAPI::Position p = BWAPI::Position(geo) + BWAPI::Position(4, 4);
            sumX += p.x;
            sumY += p.y;
            sumXY += p.x * p.y;
            sumX2 += p.x * p.x;
            sumY2 += p.y * p.y;
            //BWAPI::Broodwar->drawBoxMap(BWAPI::Position(geo), BWAPI::Position(geo) + BWAPI::Position(9, 9), BWAPI::Colors::Black);
        }
        double xMean = sumX / choke->Geometry().size();
        double yMean = sumY / choke->Geometry().size();
        double denominator, slope, yInt;
        if ((maxY - minY) > (maxX - minX))
        {
            denominator = (sumXY - sumY * xMean);
            // handle vertical line error
            if (std::fabs(denominator) < 1.0) {
                slope = 0;
                yInt = xMean;
            }
            else {
                slope = (sumY2 - sumY * yMean) / denominator;
                yInt = yMean - slope * xMean;
            }
        }
        else {
            denominator = sumX2 - sumX * xMean;
            // handle vertical line error
            if (std::fabs(denominator) < 1.0) {
                slope = DBL_MAX;
                yInt = 0;
            }
            else {
                slope = (sumXY - sumX * yMean) / denominator;
                yInt = yMean - slope * xMean;
            }
        }


        int x1 = Position(choke->Pos(choke->end1)).x;
        int y1 = int(ceil(x1 * slope)) + int(yInt);
        p1 = Position(x1, y1);

        int x2 = Position(choke->Pos(choke->end2)).x;
        int y2 = int(ceil(x2 * slope)) + int(yInt);
        p2 = Position(x2, y2);

        return make_pair(p1, p2);
    }

    pair<Position, Position> perpendicularLine(pair<Position, Position> points, double length)
    {
        auto n1 = points.first;
        auto n2 = points.second;
        auto dist = n1.getDistance(n2);
        auto dx1 = int((n2.x - n1.x) * length / dist);
        auto dy1 = int((n2.y - n1.y) * length / dist);
        auto dx2 = int((n1.x - n2.x) * length / dist);
        auto dy2 = int((n1.y - n2.y) * length / dist);
        auto direction1 = Position(-dy1, dx1) + ((n1 + n2) / 2);
        auto direction2 = Position(-dy2, dx2) + ((n1 + n2) / 2);
        return make_pair(direction1, direction2);
    }

    bool isPlaceable(UnitType type, const TilePosition location)
    {
        // Placeable is valid if buildable and not overlapping neutrals
        // Note: Must check neutrals due to the terrain below them technically being buildable
        const auto creepCheck = type.requiresCreep() ? true : false;
        for (auto x = location.x; x < location.x + type.tileWidth(); x++) {

            if (creepCheck) {
                TilePosition tile(x, location.y + 2);
                if (!Broodwar->isBuildable(tile))
                    return false;
            }

            for (auto y = location.y; y < location.y + type.tileHeight(); y++) {                
                TilePosition tile(x, y);
                if (!tile.isValid()
                    //|| !Broodwar->isBuildable(tile)
                    || !mapBWEM.GetTile(tile).Buildable()
                    //|| !Broodwar->isWalkable(WalkPosition(tile))
                    //|| Map::usedGrid[x][y] != UnitTypes::None
                    //|| Map::reserveGrid[x][y] > 0
                    || (type.isResourceDepot() && !Broodwar->canBuildHere(tile, type)))
                    return false;
            }
        }

        return true;
    }

    void addOverlap(const TilePosition t, const int w, const int h)
    {
        for (auto x = t.x; x < t.x + w; x++) {
            for (auto y = t.y; y < t.y + h; y++)
                if (TilePosition(x, y).isValid())
                    overlapGrid[x][y] = 1;
        }
    }

    void removeOverlap(const TilePosition t, const int w, const int h)
    {
        for (auto x = t.x; x < t.x + w; x++) {
            for (auto y = t.y; y < t.y + h; y++)
                if (TilePosition(x, y).isValid())
                    overlapGrid[x][y] = 0;
        }
    }

    void removeUsed(const TilePosition t, const int w, const int h)
    {
        for (auto x = t.x; x < t.x + w; x++) {
            for (auto y = t.y; y < t.y + h; y++)
                if (TilePosition(x, y).isValid())
                    usedGrid[x][y] = UnitTypes::None;
        }
    }

    void addUsed(const TilePosition t, UnitType type)
    {
        for (auto x = t.x; x < t.x + type.tileWidth(); x++) {
            for (auto y = t.y; y < t.y + type.tileHeight(); y++)
                if (TilePosition(x, y).isValid())
                    usedGrid[x][y] = type;
        }
    }

    void addReserve(const TilePosition t, const int w, const int h)
    {
        for (auto x = t.x; x < t.x + w; x++) {
            for (auto y = t.y; y < t.y + h; y++)
                if (TilePosition(x, y).isValid())
                    reserveGrid[x][y] = 1;
        }
    }

    const BWEM::Area * getNaturalArea() { return naturalArea; }
    const BWEM::Area * getMainArea() { return mainArea; }
    const BWEM::ChokePoint * getNaturalChoke() { return naturalChoke; }
    const BWEM::ChokePoint * getMainChoke() { return mainChoke; }
    TilePosition getNaturalTile() { return naturalTile; }
    Position getNaturalPosition() { return naturalPosition; }
    TilePosition getMainTile() { return mainTile; }
    Position getMainPosition() { return mainPosition; }
}