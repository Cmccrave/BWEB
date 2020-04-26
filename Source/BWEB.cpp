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

        bool drawReserveOverlap, drawUsed, drawWalk, drawArea;

        map<BWAPI::Key, bool> lastKeyState;
        map<const BWEM::ChokePoint *, set<TilePosition>> chokeTiles;
        map<const BWEM::ChokePoint *, pair<Position, Position>> chokeLines;

        int overlapGrid[256][256] ={};
        UnitType usedGrid[256][256] ={};
        bool walkGrid[256][256] ={};
        bool logInfo = true;

        void findLines()
        {
            for (auto &area : mapBWEM.Areas()) {
                for (auto &choke : area.ChokePoints()) {
                    Position p1, p2;
                    int minX= INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
                    double sumX = 0, sumY = 0;
                    double sumXY = 0, sumX2 = 0, sumY2 = 0;

                    for (auto &geo : choke->Geometry()) {
                        if (geo.x < minX) minX = geo.x;
                        if (geo.y < minY) minY = geo.y;
                        if (geo.x > maxX) maxX = geo.x;
                        if (geo.y > maxY) maxY = geo.y;

                        BWAPI::Position p = BWAPI::Position(geo);
                        sumX += p.x;
                        sumY += p.y;
                        sumXY += p.x * p.y;
                        sumX2 += p.x * p.x;
                        sumY2 += p.y * p.y;
                    }
                    double xMean = sumX / double(choke->Geometry().size());
                    double yMean = sumY / double(choke->Geometry().size());
                    double denominator, slope, yInt;
                    if ((maxY - minY) > (maxX - minX)) {
                        denominator = (sumXY - sumY * xMean);

                        // Handle vertical line error
                        if (std::fabs(denominator) < 16000.0) {
                            slope = 0;
                            yInt = yMean;
                        }
                        else {
                            slope = (sumY2 - sumY * yMean) / denominator;
                            yInt = yMean - slope * xMean;
                        }
                    }
                    else {
                        denominator = sumX2 - sumX * xMean;

                        // Handle vertical line error
                        if (std::fabs(denominator) < 1.0) {
                            slope = DBL_MAX;
                            yInt = yMean;
                        }
                        else {
                            slope = (sumXY - sumX * yMean) / denominator;
                            yInt = yMean - slope * xMean;
                        }
                    }

                    int x1 = Position(choke->Pos(choke->end1)).x;
                    int y1 = int(round(double(x1) * slope)) + int(round(yInt));
                    p1 = Position(x1, y1);

                    int x2 = Position(choke->Pos(choke->end2)).x;
                    int y2 = int(round(double(x2) * slope)) + int(round(yInt));
                    p2 = Position(x2, y2);

                    // In case we failed
                    if (p1 == p2 || !p1.isValid() || !p2.isValid()) {
                        p1 = Position(choke->Pos(choke->end1));
                        p2 = Position(choke->Pos(choke->end2));
                    }
                    chokeLines[choke] = make_pair(p1, p2);
                }
            }
        }

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
                        naturalPosition = Position(naturalTile) + Position(64, 48);
                    }
                }
            }

            if (!naturalArea)
                naturalArea = mainArea;
        }

        void findMainChoke()
        {
            // Add all main chokes to a set
            set<BWEM::ChokePoint const *> mainChokes;
            for (auto &choke : mainArea->ChokePoints())
                mainChokes.insert(choke);
            if (mainChokes.size() == 1) {
                mainChoke = *mainChokes.begin();
                return;
            }

            // Add all natural chokes to a set
            set<BWEM::ChokePoint const *> naturalChokes;
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
            if (!naturalPosition.isValid()) {
                naturalChoke = mainChoke;
                return;
            }

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
            for (auto unit : Broodwar->getNeutralUnits()) {
                if (unit && unit->exists() && unit->getType().topSpeed() == 0.0)
                    addReserve(unit->getTilePosition(), unit->getType().tileWidth(), unit->getType().tileHeight());
                if (unit->getType().isBuilding())
                    addUsed(unit->getTilePosition(), unit->getType());
            }
        }
    }

    void easyWrite(string stuff)
    {
        if (logInfo) {
            ofstream writeFile;
            writeFile.open("bwapi-data/write/BWEB_Log.txt", std::ios::app);
            writeFile << stuff << endl;
        }
    }

    void draw()
    {
        WalkPosition mouse(Broodwar->getMousePosition() + Broodwar->getScreenPosition());
        auto mouseArea = mouse.isValid() ? mapBWEM.GetArea(mouse) : nullptr;
        auto k1 = Broodwar->getKeyState(BWAPI::Key::K_1);
        auto k2 = Broodwar->getKeyState(BWAPI::Key::K_2);
        auto k3 = Broodwar->getKeyState(BWAPI::Key::K_3);
        auto k4 = Broodwar->getKeyState(BWAPI::Key::K_4);

        drawReserveOverlap = k1 && k1 != lastKeyState[BWAPI::Key::K_1] ? !drawReserveOverlap : drawReserveOverlap;
        drawUsed = k2 && k2 != lastKeyState[BWAPI::Key::K_2] ? !drawUsed : drawUsed;
        drawWalk = k3 && k3 != lastKeyState[BWAPI::Key::K_3] ? !drawWalk : drawWalk;
        drawArea = k4 && k4 != lastKeyState[BWAPI::Key::K_4] ? !drawArea : drawArea;

        lastKeyState[BWAPI::Key::K_1] = k1;
        lastKeyState[BWAPI::Key::K_2] = k2;
        lastKeyState[BWAPI::Key::K_3] = k3;
        lastKeyState[BWAPI::Key::K_4] = k4;

        // Detect a keypress for drawing information
        if (drawReserveOverlap || drawUsed || drawWalk || drawArea) {

            for (auto x = 0; x < Broodwar->mapWidth(); x++) {
                for (auto y = 0; y < Broodwar->mapHeight(); y++) {
                    const TilePosition t(x, y);

                    // Draw boxes around TilePositions that are reserved or overlapping important map features
                    if (drawReserveOverlap) {
                        if (overlapGrid[x][y] >= 1)
                            Broodwar->drawBoxMap(Position(t) + Position(4, 4), Position(t) + Position(29, 29), Colors::Grey, false);
                    }

                    // Draw boxes around TilePositions that are used
                    if (drawUsed) {
                        const auto type = usedGrid[x][y];
                        if (type != UnitTypes::None)
                            Broodwar->drawBoxMap(Position(t) + Position(8, 8), Position(t) + Position(25, 25), Colors::Black, true);
                    }

                    // Draw boxes around fully walkable TilePositions
                    if (drawWalk) {
                        if (walkGrid[x][y])
                            Broodwar->drawBoxMap(Position(t), Position(t) + Position(33, 33), Colors::Black, false);
                    }

                    // Draw boxes around any TilePosition that shares an Area with mouses current Area
                    if (drawArea) {
                        if (mapBWEM.GetArea(t) == mouseArea)
                            Broodwar->drawBoxMap(Position(t), Position(t) + Position(33, 33), Colors::Green, false);
                    }
                }
            }
        }

        Walls::draw();
        Blocks::draw();
        Stations::draw();
    }

    void onStart()
    {
        // Initializes usedGrid and walkGrid
        for (int x = 0; x < Broodwar->mapWidth(); x++) {
            for (int y = 0; y < Broodwar->mapHeight(); y++) {

                usedGrid[x][y] = UnitTypes::None;

                auto cnt = 0;
                for (int dx = x * 4; dx < (x * 4) + 4; dx++) {
                    for (int dy = y * 4; dy < (y * 4) + 4; dy++) {
                        const auto w = WalkPosition(dx, dy);
                        if (w.isValid() && Broodwar->isWalkable(w))
                            cnt++;
                    }
                }

                if (cnt >= 14)
                    walkGrid[x][y] = true;
            }
        }

        for (auto gas : Broodwar->getGeysers()) {
            for (int x = gas->getTilePosition().x; x < gas->getTilePosition().x + 4; x++) {
                for (int y = gas->getTilePosition().y; y < gas->getTilePosition().y + 2; y++) {
                    walkGrid[x][y] = false;
                }
            }
        }

        for (auto &area : mapBWEM.Areas()) {
            for (auto &choke : area.ChokePoints()) {
                for (auto &geo : choke->Geometry())
                    chokeTiles[choke].insert(TilePosition(geo));
            }
        }

        findNeutrals();
        findMain();
        findNatural();
        findMainChoke();
        findNaturalChoke();
        findLines();
    }

    void onUnitDiscover(const Unit unit)
    {
        const auto tile = unit->getTilePosition();
        const auto type = unit->getType();

        const auto gameStart = Broodwar->getFrameCount() == 0;
        const auto okayToAdd = (unit->getType().isBuilding() && !unit->isFlying())
            || (gameStart && unit->getType().topSpeed() == 0.0);

        // Add used tiles
        if (okayToAdd) {
            for (auto x = tile.x; x < tile.x + type.tileWidth(); x++) {
                for (auto y = tile.y; y < tile.y + type.tileHeight(); y++) {
                    TilePosition t(x, y);
                    if (!t.isValid())
                        continue;
                    usedGrid[x][y] = type;
                }
            }

            // Clear pathfinding cache
            Pathfinding::clearCache();
        }
    }

    void onUnitDestroy(const Unit unit)
    {
        const auto tile = unit->getTilePosition();
        const auto type = unit->getType();

        const auto gameStart = Broodwar->getFrameCount() == 0;
        const auto okayToRemove = (unit->getType().isBuilding() && !unit->isFlying())
            || (!gameStart && unit->getType().topSpeed() == 0.0);

        // Add used tiles
        if (okayToRemove) {
            for (auto x = tile.x; x < tile.x + type.tileWidth(); x++) {
                for (auto y = tile.y; y < tile.y + type.tileHeight(); y++) {
                    TilePosition t(x, y);
                    if (!t.isValid())
                        continue;
                    usedGrid[x][y] = UnitTypes::None;
                }
            }

            // Clear pathfinding cache
            Pathfinding::clearCache();
        }
    }

    void onUnitMorph(const Unit unit)
    {
        onUnitDiscover(unit);
    }

    void addReserve(const TilePosition t, const int w, const int h)
    {
        for (auto x = t.x; x < t.x + w; x++) {
            for (auto y = t.y; y < t.y + h; y++)
                if (TilePosition(x, y).isValid())
                    overlapGrid[x][y] = 1;
        }
    }

    void removeReserve(const TilePosition t, const int w, const int h)
    {
        for (auto x = t.x; x < t.x + w; x++) {
            for (auto y = t.y; y < t.y + h; y++)
                if (TilePosition(x, y).isValid())
                    overlapGrid[x][y] = 0;
        }
    }

    bool isReserved(const TilePosition here, const int width, const int height)
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

    void addUsed(const TilePosition t, UnitType type)
    {
        for (auto x = t.x; x < t.x + type.tileWidth(); x++) {
            for (auto y = t.y; y < t.y + type.tileHeight(); y++)
                if (TilePosition(x, y).isValid())
                    usedGrid[x][y] = type;
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

    bool isPlaceable(UnitType type, const TilePosition location)
    {
        if (type.requiresCreep()) {
            for (auto x = location.x; x < location.x + type.tileWidth(); x++) {
                const TilePosition creepTile(x, location.y + type.tileHeight());
                if (!Broodwar->isBuildable(creepTile))
                    return false;
            }
        }

        if (type.isResourceDepot() && !Broodwar->canBuildHere(location, type))
            return false;

        for (auto x = location.x; x < location.x + type.tileWidth(); x++) {
            for (auto y = location.y; y < location.y + type.tileHeight(); y++) {

                const TilePosition tile(x, y);
                if (!tile.isValid()
                    || !Broodwar->isBuildable(tile)
                    || !Broodwar->isWalkable(WalkPosition(tile))
                    || Map::isUsed(tile) != UnitTypes::None)
                    return false;
            }
        }
        return true;
    }

    int tilesWithinArea(BWEM::Area const * area, const TilePosition here, const int width, const int height)
    {
        auto cnt = 0;
        for (auto x = here.x; x < here.x + width; x++) {
            for (auto y = here.y; y < here.y + height; y++) {
                TilePosition t(x, y);
                if (!t.isValid())
                    return false;

                if (Map::mapBWEM.GetArea(t) == area)
                    cnt++;
            }
        }
        return cnt;
    }

    template <class T>
    double getGroundDistance(T s, T e)
    {
        Position start(s), end(e);
        auto dist = 0.0;
        auto last = start;

        const auto validatePoint = [&](WalkPosition w) {
            auto distBest = 0.0;
            auto posBest = Position(w);
            for (auto x = w.x - 1; x < w.x + 1; x++) {
                for (auto y = w.y - 1; y < w.y + 1; y++) {
                    WalkPosition w(x, y);
                    if (!w.isValid()
                        || !mapBWEM.GetArea(w))
                        continue;

                    auto p = Position(w);
                    auto dist = p.getDistance(mapBWEM.Center());
                    if (dist > distBest) {
                        distBest = dist;
                        posBest = p;
                    }
                }
            }
            return posBest;
        };

        // Return DBL_MAX if not valid path points or not walkable path points
        if (!start.isValid() || !end.isValid())
            return DBL_MAX;

        // Check if we're in a valid area, if not try to find a different nearby WalkPosition
        if (!mapBWEM.GetArea(WalkPosition(start)))
            start = validatePoint(WalkPosition(start));
        if (!mapBWEM.GetArea(WalkPosition(end)))
            end = validatePoint(WalkPosition(end));

        // If not valid still, return DBL_MAX
        if (!start.isValid()
            || !end.isValid()
            || !mapBWEM.GetArea(WalkPosition(start))
            || !mapBWEM.GetArea(WalkPosition(end))
            || !mapBWEM.GetArea(WalkPosition(start))->AccessibleFrom(mapBWEM.GetArea(WalkPosition(end))))
            return DBL_MAX;

        // Find the closest chokepoint node
        const auto accurateClosestNode = [&](const BWEM::ChokePoint * cp) {
            return getClosestChokeTile(cp, start);
        };

        const auto fastClosestNode = [&](const BWEM::ChokePoint * cp) {
            auto bestPosition = cp->Center();
            auto bestDist = DBL_MAX;

            const auto n1 = Position(cp->Pos(cp->end1));
            const auto n2 = Position(cp->Pos(cp->end2));
            const auto n3 = Position(cp->Center());

            const auto d1 = n1.getDistance(last);
            const auto d2 = n2.getDistance(last);
            const auto d3 = n3.getDistance(last);

            return d1 < d2 ? (d1 < d3 ? n1 : n3) : (d2 < d3 ? n2 : n3);
        };

        // For each chokepoint, add the distance to the closest chokepoint node
        auto first = true;
        for (auto &cpp : mapBWEM.GetPath(start, end)) {

            auto large = cpp->Pos(cpp->end1).getDistance(cpp->Pos(cpp->end2)) > 40;

            auto next = first && !large ? accurateClosestNode(cpp) : fastClosestNode(cpp);
            dist += next.getDistance(last);
            last = next;
            first = false;
        }

        return dist += last.getDistance(end);
    }

    Position getClosestChokeTile(const BWEM::ChokePoint * choke, Position here)
    {
        auto best = DBL_MAX;
        auto posBest = Positions::Invalid;
        for (auto &tile : Map::getChokeTiles(choke)) {
            const auto p = Position(tile) + Position(16, 16);
            const auto dist = p.getDistance(here);
            if (dist < best) {
                posBest = p;
                best = dist;
            }
        }
        return posBest;
    }

    set<TilePosition> getChokeTiles(const BWEM::ChokePoint * choke)
    {
        if (choke)
            return chokeTiles[choke];
        return {};
    }

    pair<Position, Position> lineOfBestFit(const BWEM::ChokePoint * choke)
    {
        if (choke)
            return chokeLines[choke];
        return {};
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

    TilePosition getBuildPosition(UnitType type, const TilePosition searchCenter)
    {
        auto distBest = DBL_MAX;
        auto tileBest = TilePositions::Invalid;

        // Search through each block to find the closest block and valid position
        for (auto &block : Blocks::getBlocks()) {
            set<TilePosition> placements;

            if (type.tileWidth() == 4)
                placements = block.getLargeTiles();
            else if (type.tileWidth() == 3)
                placements = block.getMediumTiles();
            else
                placements = block.getSmallTiles();

            for (auto &tile : placements) {
                const auto dist = tile.getDistance(searchCenter);
                if (dist < distBest && isPlaceable(type, tile)) {
                    distBest = dist;
                    tileBest = tile;
                }
            }
        }
        return tileBest;
    }

    TilePosition getDefBuildPosition(UnitType type, const TilePosition searchCenter)
    {
        auto distBest = DBL_MAX;
        auto tileBest = TilePositions::Invalid;

        // Search through each wall to find the closest valid TilePosition
        for (auto &[_,wall] : Walls::getWalls()) {
            for (auto &tile : wall.getDefenses()) {
                const auto dist = tile.getDistance(searchCenter);
                if (dist < distBest && isPlaceable(type, tile)) {
                    distBest = dist;
                    tileBest = tile;
                }
            }
        }

        // Search through each station to find the closest valid TilePosition
        for (auto &station : Stations::getStations()) {
            for (auto &tile : station.getDefenseLocations()) {
                const auto dist = tile.getDistance(searchCenter);
                if (dist < distBest && isPlaceable(type, tile)) {
                    distBest = dist;
                    tileBest = tile;
                }
            }
        }
        return tileBest;
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