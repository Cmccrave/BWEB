#include "BWEB.h"
#include <chrono> 

using namespace std;
using namespace BWAPI;

namespace BWEB
{
    namespace {
        vector<Wall> walls;
        bool logInfo = true;

        int failedPlacement = 0;
        int failedAngle = 0;
        int failedPath = 0;
        int failedTight = 0;
        int failedSpawn = 0;
        int failedPower = 0;
    }

    namespace Walls {

        void addToWall(UnitType building, Wall& wall, UnitType tight, bool openWall)
        {
            // TODO
        }

        Wall* createWall(vector<UnitType>& buildings, const BWEM::Area * area, const BWEM::ChokePoint * choke, const UnitType tightType, const vector<UnitType>& defenses, const bool openWall, const bool requireTight)
        {
            ofstream writeFile;
            string buffer;
            auto timePointNow = chrono::system_clock::now();
            auto timeNow = chrono::system_clock::to_time_t(timePointNow);

            // Create the name of the Area to print out
            string areaName;
            if (area == Map::getMainArea())
                areaName = "Main Area";
            else if (area == Map::getNaturalArea())
                areaName = "Natural Area";
            else
                areaName = "%d, %d", Position(area->Top());

            // Create the name of the ChokePoint to print out
            string chokeName;
            if (choke == Map::getMainChoke())
                chokeName = "Main Choke";
            else if (choke == Map::getNaturalChoke())
                chokeName = "Natural Choke";
            else
                chokeName = "%d, %d", Position(choke->Center());

            // Open the log file if desired and write information
            if (logInfo) {
                writeFile.open("bwapi-data/write/BWEB.txt", std::ios::app);
                writeFile << ctime(&timeNow);
                writeFile << Broodwar->mapFileName().c_str() << endl;
                writeFile << "Placing Wall at " << chokeName << " in the " << areaName << endl;
                writeFile << endl;

                writeFile << "Wall Buildings:" << endl;
                for (auto &building : buildings)
                    writeFile << building.c_str() << endl;
                writeFile << endl;
            }

            // Verify inputs are correct
            if (!area) {
                writeFile << "BWEB: Can't create a wall without a valid BWEM::Area" << endl;
                return nullptr;
            }

            if (!choke) {
                writeFile << "BWEB: Can't create a wall without a valid BWEM::Chokepoint" << endl;
                return nullptr;
            }

            if (buildings.empty()) {
                writeFile << "BWEB: Can't create a wall with an empty vector of UnitTypes." << endl;
                return nullptr;
            }

            // Verify not attempting to create a Wall in the same Area/ChokePoint combination
            for (auto &wall : walls) {
                if (wall.getArea() == area && wall.getChokePoint() == choke) {
                    writeFile << "BWEB: Can't create a Wall where one already exists." << endl;
                    return &wall;
                }
            }

            // Create a Wall
            Wall wall(area, choke, buildings, defenses, tightType, requireTight, openWall);

            // Log information
            if (logInfo) {
                writeFile << "Failure Reasons:" << endl;
                writeFile << "Power: " << failedPower << endl;
                writeFile << "Angle: " << failedAngle << endl;
                writeFile << "Placement: " << failedPlacement << endl;
                writeFile << "Tight: " << failedTight << endl;
                writeFile << "Path: " << failedPath << endl;
                writeFile << "Spawn: " << failedSpawn << endl;
                writeFile << endl;

                double dur = std::chrono::duration <double, std::milli>(chrono::system_clock::now() - timePointNow).count();
                writeFile << "Generation Time: " << dur << endl;
                writeFile << "--------------------" << endl;
            }

            // Verify the Wall creation was successful
            auto wallFound = (wall.getSmallTiles().size() + wall.getMediumTiles().size() + wall.getLargeTiles().size()) == wall.getRawBuildings().size();

            // If we found a suitable Wall, push into container and return pointer to it
            if (wallFound) {
                walls.push_back(wall);
                return &walls.back();
            }
            return nullptr;
        }

        Wall* createFFE()
        {
            vector<UnitType> buildings ={ UnitTypes::Protoss_Forge, UnitTypes::Protoss_Gateway, UnitTypes::Protoss_Pylon };
            vector<UnitType> defenses(6, UnitTypes::Protoss_Photon_Cannon);

            return createWall(buildings, Map::getNaturalArea(), Map::getNaturalChoke(), UnitTypes::Zerg_Zergling, defenses, true, false);
        }

        Wall* createZSimCity()
        {
            vector<UnitType> buildings ={ UnitTypes::Zerg_Hatchery, UnitTypes::Zerg_Evolution_Chamber, UnitTypes::Zerg_Evolution_Chamber };
            vector<UnitType> defenses(6, UnitTypes::Zerg_Sunken_Colony);

            return createWall(buildings, Map::getNaturalArea(), Map::getNaturalChoke(), UnitTypes::None, defenses, true, false);
        }

        Wall* createTWall()
        {
            vector<UnitType> buildings ={ UnitTypes::Terran_Supply_Depot, UnitTypes::Terran_Supply_Depot, UnitTypes::Terran_Barracks };
            vector<UnitType> defenses;
            auto type = Broodwar->enemy() && Broodwar->enemy()->getRace() == Races::Protoss ? UnitTypes::Protoss_Zealot : UnitTypes::Zerg_Zergling;

            return createWall(buildings, Map::getMainArea(), Map::getMainChoke(), type, defenses, false, true);
        }

        Wall* getClosestWall(TilePosition here)
        {
            auto distBest = DBL_MAX;
            Wall * bestWall = nullptr;
            for (auto &wall : walls) {
                const auto dist = here.getDistance(TilePosition(wall.getChokePoint()->Center()));

                if (dist < distBest) {
                    distBest = dist;
                    bestWall = &wall;
                }
            }
            return bestWall;
        }

        Wall* getWall(const BWEM::Area * area, const BWEM::ChokePoint * choke)
        {
            if (!area && !choke)
                return nullptr;

            for (auto &wall : walls) {
                if ((!area || wall.getArea() == area) && (!choke || wall.getChokePoint() == choke))
                    return &wall;
            }
            return nullptr;
        }

        vector<Wall>& getWalls() {
            return walls;
        }

        void draw()
        {
            for (auto &wall : walls)
                wall.draw();
        }
    }

    void Wall::checkPathPoints()
    {
        auto distBest = DBL_MAX;
        startTile = initialStart;
        endTile = initialEnd;

        const auto neighbourArea = [&](const BWEM::Area * area) {
            for (auto subArea : this->getArea()->AccessibleNeighbours()) {
                if (area == subArea)
                    return true;
            }
            return false;
        };

        if (initialStart.isValid() && (!Map::isWalkable(initialStart) || Map::isUsed(initialStart) != UnitTypes::None || Map::isReserved(initialStart) != 0)) {
            for (auto x = initialStart.x - 2; x < initialStart.x + 2; x++) {
                for (auto y = initialStart.y - 2; y < initialStart.y + 2; y++) {
                    TilePosition t(x, y);
                    const auto dist = t.getDistance(endTile);

                    if (!t.isValid())
                        continue;
                    if (Map::isUsed(initialStart) != UnitTypes::None || !Map::isWalkable(t) || Map::isReserved(t))
                        continue;
                    if (!neighbourArea(Map::mapBWEM.GetArea(t)))
                        continue;

                    if (Map::mapBWEM.GetArea(t) == this->getArea() && dist < distBest) {
                        startTile = t;
                        distBest = dist;
                    }
                }
            }
        }

        distBest = 0.0;
        if (initialEnd.isValid() && (!Map::isWalkable(initialEnd) || Map::isUsed(initialStart) != UnitTypes::None || Map::isReserved(endTile) != 0)) {
            for (auto x = initialEnd.x - 4; x < initialEnd.x + 4; x++) {
                for (auto y = initialEnd.y - 4; y < initialEnd.y + 4; y++) {
                    TilePosition t(x, y);
                    const auto dist = t.getDistance(startTile);
                    if (!t.isValid())
                        continue;
                    if (Map::isUsed(initialStart) != UnitTypes::None || !Map::isWalkable(t) || Map::isReserved(t))
                        continue;
                    if (!neighbourArea(Map::mapBWEM.GetArea(t)))
                        continue;

                    if (Map::mapBWEM.GetArea(t) && dist > distBest) {
                        endTile = t;
                        distBest = dist;
                    }
                }
            }
        }
    }

    void Wall::initializePathPoints()
    {
        const auto choke = this->getChokePoint();
        const auto line = Map::lineOfBestFit(choke);
        const auto perpLine = Map::perpendicularLine(line, 160.0);
        const auto pathEnd = !perpLine.first.isValid() || Map::mapBWEM.GetArea(TilePosition(perpLine.first)) == this->getArea() ? perpLine.second : perpLine.first;

        if (choke == Map::getNaturalChoke()) {
            initialStart = TilePosition(Map::getMainChoke()->Center());
            initialEnd = TilePosition(pathEnd);
        }

        // Default the pathing to the top of the area and towards the center of the map
        else {
            initialStart = TilePosition(perpLine.first);
            initialEnd = TilePosition(perpLine.second);
        }

        startTile = initialStart;
        endTile = initialEnd;
    }

    Path Wall::findOpeningInWall()
    {
        // Check that the path points are possible to reach
        checkPathPoints();
        const auto startCenter = Position(startTile) + Position(16, 16);
        const auto endCenter = Position(endTile) + Position(16, 16);

        // Get a new path
        BWEB::Path newPath;
        newPath.createWallPath(startCenter, endCenter, false, jpsDist);
        return newPath;
    }

    bool Wall::powerCheck(const UnitType type, const TilePosition here)
    {
        if (type != UnitTypes::Protoss_Pylon || this->isPylonWall())
            return true;

        // TODO: Create a generic BWEB function that takes 2 tiles and tells you if the 1st tile will power the 2nd tile
        for (auto &[tileLayout, typeLayout] : currentLayout) {
            if (typeLayout == UnitTypes::Protoss_Pylon)
                continue;

            if (typeLayout.tileWidth() == 4) {
                auto powersThis = false;
                if (tileLayout.y - here.y == -5 || tileLayout.y - here.y == 4) {
                    if (tileLayout.x - here.x >= -4 && tileLayout.x - here.x <= 1)
                        powersThis = true;
                }
                if (tileLayout.y - here.y == -4 || tileLayout.y - here.y == 3) {
                    if (tileLayout.x - here.x >= -7 && tileLayout.x - here.x <= 4)
                        powersThis = true;
                }
                if (tileLayout.y - here.y == -3 || tileLayout.y - here.y == 2) {
                    if (tileLayout.x - here.x >= -8 && tileLayout.x - here.x <= 5)
                        powersThis = true;
                }
                if (tileLayout.y - here.y >= -2 && tileLayout.y - here.y <= 1) {
                    if (tileLayout.x - here.x >= -8 && tileLayout.x - here.x <= 6)
                        powersThis = true;
                }
                if (!powersThis)
                    return false;
            }
            else {
                auto powersThis = false;
                if (tileLayout.y - here.y == 4) {
                    if (tileLayout.x - here.x >= -3 && tileLayout.x - here.x <= 2)
                        powersThis = true;
                }
                if (tileLayout.y - here.y == -4 || tileLayout.y - here.y == 3) {
                    if (tileLayout.x - here.x >= -6 && tileLayout.x - here.x <= 5)
                        powersThis = true;
                }
                if (tileLayout.y - here.y >= -3 && tileLayout.y - here.y <= 2) {
                    if (tileLayout.x - here.x >= -7 && tileLayout.x - here.x <= 6)
                        powersThis = true;
                }
                if (!powersThis)
                    return false;
            }
        }
        return true;
    }

    bool Wall::angleCheck(const UnitType type, const TilePosition here)
    {
        const auto centerHere = Position(here) + Position(type.tileWidth() * 16, type.tileHeight() * 16);

        // If we want a closed wall or we moved the starting iteration position, we don't care the angle of the buildings
        if (!openWall)
            return true;

        // Check if the angle is okay
        for (auto &[tileLayout, typeLayout] : currentLayout) {

            const auto centerPiece = Position(tileLayout) + Position(typeLayout.tileWidth() * 16, typeLayout.tileHeight() * 16);
            const auto wallAngle = Map::getAngle(make_pair(centerPiece, centerHere));

            if (abs(chokeAngle - wallAngle) > 25.0)
                return false;
        }
        return true;
    }

    bool Wall::placeCheck(const UnitType type, const TilePosition here)
    {
        const auto center = Position(here) + Position(type.tileWidth() * 16, type.tileHeight() * 16);

        if ((!openWall && type != UnitTypes::Terran_Barracks && center.getDistance(Position(this->getChokePoint()->Center())) < 64.0)
            || Map::isReserved(here, type.tileWidth(), type.tileHeight(), true)
            || !Map::isPlaceable(type, here)
            || Map::tilesWithinArea(this->getArea(), here, type.tileWidth(), type.tileHeight()) < 1)
            return false;
        return true;
    }

    bool Wall::tightCheck(const UnitType type, const TilePosition here)
    {
        // If this is a powering pylon and we are not making a pylon wall
        if (type == UnitTypes::Protoss_Pylon && !this->isPylonWall() && !requireTight)
            return true;

        // Dimensions of current buildings UnitType
        const auto dimL = (type.tileWidth() * 16) - type.dimensionLeft();
        const auto dimR = (type.tileWidth() * 16) - type.dimensionRight() - 1;
        const auto dimU = (type.tileHeight() * 16) - type.dimensionUp();
        const auto dimD = (type.tileHeight() * 16) - type.dimensionDown() - 1;
        const auto walkHeight = type.tileHeight() * 4;
        const auto walkWidth = type.tileWidth() * 4;

        // Dimension of UnitType to check tightness for
        const auto vertTight = (tightType == UnitTypes::None) ? 64 : tightType.height();
        const auto horizTight = (tightType == UnitTypes::None) ? 64 : tightType.width();

        // Checks each side of the building to see if it is valid for walling purposes
        const auto checkL = dimL < horizTight;
        const auto checkR = dimR < horizTight;
        const auto checkU = dimU < vertTight;
        const auto checkD = dimD < vertTight;

        // Figures out how many extra tiles we can check tightness for
        const auto extraL = this->isPylonWall() ? 0 : max(0, (horizTight - dimL) / 8);
        const auto extraR = this->isPylonWall() ? 0 : max(0, (horizTight - dimR) / 8);
        const auto extraU = this->isPylonWall() ? 0 : max(0, (vertTight - dimU) / 8);
        const auto extraD = this->isPylonWall() ? 0 : max(0, (vertTight - dimD) / 8);

        // Setup boundary WalkPositions to check for tightness
        const auto left =  WalkPosition(here) - WalkPosition(1 + extraL, 0);
        const auto right = WalkPosition(here) + WalkPosition(walkWidth + extraR, 0);
        const auto up =  WalkPosition(here) - WalkPosition(0, 1 + extraU);
        const auto down =  WalkPosition(here) + WalkPosition(0, walkHeight + extraD);

        // Used for determining if the tightness we found is suitable
        const auto firstBuilding = currentLayout.size() == 0;
        const auto lastBuilding = currentLayout.size() == (this->getRawBuildings().size() - 1);
        auto terrainTight = false;
        auto parentTight = false;

        // Functions for each dimension check
        const auto gapRight = [&](UnitType parent) {
            return (parent.tileWidth() * 16) - parent.dimensionLeft() + dimR;
        };
        const auto gapLeft = [&](UnitType parent) {
            return (parent.tileWidth() * 16) - parent.dimensionRight() - 1 + dimL;
        };
        const auto gapUp = [&](UnitType parent) {
            return (parent.tileHeight() * 16) - parent.dimensionDown() - 1 + dimU;
        };
        const auto gapDown = [&](UnitType parent) {
            return (parent.tileHeight() * 16) - parent.dimensionUp() + dimD;
        };

        // Check if the building is terrain tight when placed here
        const auto terrainTightCheck = [&](WalkPosition w, bool check) {
            const auto t = TilePosition(w);

            // If the walkposition is invalid or unwalkable
            if (tightType != UnitTypes::None && check && (!w.isValid() || !Broodwar->isWalkable(w)))
                return true;

            // If we don't care about walling tight and the tile isn't walkable
            if (!requireTight && !Map::isWalkable(t))
                return true;

            // If there's a mineral field or geyser here
            if (Map::isUsed(t).isResourceContainer())
                return true;

            return false;
        };

        // Iterate vertical tiles adjacent of this placement
        const auto checkVerticalSide = [&](WalkPosition start, bool check, const auto gap) {
            for (auto x = start.x - 1; x < start.x + walkWidth + 1; x++) {
                const WalkPosition w(x, start.y);
                const auto t = TilePosition(w);
                const auto parent = Map::isUsed(t);
                const auto parentTightCheck = parent != UnitTypes::None ? gap(parent) < vertTight : false;
                const auto cornerCheck = x < start.x || x >= start.x + walkWidth;

                // Check if it's tight with the terrain
                if (!terrainTight && terrainTightCheck(w, check))
                    terrainTight = true;

                // Check if it's tight with a parent
                if (!cornerCheck && !parentTight && parentTightCheck)
                    parentTight = true;
            }
        };

        // Iterate horizontal tiles adjacent of this placement
        const auto checkHorizontalSide = [&](WalkPosition start, bool check, const auto gap) {
            for (auto y = start.y - 1; y < start.y + walkHeight + 1; y++) {
                const WalkPosition w(start.x, y);
                const auto t = TilePosition(w);
                const auto parent = Map::isUsed(t);
                const auto parentTightCheck = parent != UnitTypes::None ? gap(parent) < horizTight : false;
                const auto cornerCheck = y < start.y || y >= start.y + walkHeight;

                // Check if it's tight with the terrain
                if (!terrainTight && terrainTightCheck(w, check))
                    terrainTight = true;

                // Check if it's tight with a parent
                if (!cornerCheck && !parentTight && parentTightCheck)
                    parentTight = true;
            }
        };

        // For each side, check if it's terrain tight or tight with any adjacent buildings
        checkVerticalSide(up, checkU, gapUp);
        checkVerticalSide(down, checkD, gapDown);
        checkHorizontalSide(left, checkL, gapLeft);
        checkHorizontalSide(right, checkR, gapRight);

        // If we want a closed wall, we need all buildings to be tight at the tightness resolution...
        if (!openWall) {
            if (!lastBuilding && !firstBuilding)	// ...to the parent if not first building
                return parentTight;
            if (firstBuilding)						// ...to the terrain if first building
                return terrainTight;
            if (lastBuilding)						// ...to the parent and terrain if last building
                return (terrainTight && parentTight);
        }

        // If we want an open wall, we need this building to be tight at tile resolution to a parent or terrain
        else if (openWall)
            return (terrainTight || parentTight);
        return false;
    }

    bool Wall::spawnCheck(const UnitType type, const TilePosition here)
    {
        const auto startCenter = Position(startTile) + Position(16, 16);
        const auto endCenter = Position(endTile) + Position(16, 16);
        Path pathOut;

        // Check if we can lift the Barracks to get out
        if (Broodwar->self()->getRace() == Races::Terran) {
            pathOut.createWallPath(startCenter, endCenter, true, jpsDist);
            if (!pathOut.isReachable())
                return false;
        }
        return true;
    }

    void Wall::addOpening()
    {
        // Add a door if we want an open wall
        if (openWall) {

            // Set any tiles on the path as reserved so we don't build on them
            auto currentPath = findOpeningInWall();

            // Check which tile is closest to each part on the path, set as door
            auto distBest = DBL_MAX;
            for (auto &chokeTile : Map::getChokeTiles(this->getChokePoint())) {
                for (auto pathTile : currentPath.getTiles()) {
                    const auto dist = chokeTile.getDistance(pathTile);
                    if (dist < distBest) {
                        distBest = dist;
                        opening = chokeTile;
                    }
                }
            }

            // If the line of tiles we made is empty, assign closest path tile to closest to wall centroid
            if (Map::getChokeTiles(this->getChokePoint()).empty()) {
                for (auto pathTile : currentPath.getTiles()) {
                    const auto p = Position(pathTile);
                    const auto dist = this->getCentroid().getDistance(p);
                    if (dist < distBest) {
                        distBest = dist;
                        opening = pathTile;
                    }
                }
            }
        }
    }

    void Wall::addCentroid()
    {
        auto currentCentroid = Position(0, 0);
        auto sizeWall = int(this->getRawBuildings().size());
        for (auto &[tile, type] : bestLayout) {
            if (type != UnitTypes::Protoss_Pylon)
                currentCentroid += Position(tile) + Position(type.tileSize()) / 2;
            else
                sizeWall--;
        }

        // Set a centroid if it's only a pylon wall
        if (sizeWall == 0) {
            sizeWall = bestLayout.size();
            for (auto &[tile, type] : bestLayout)
                currentCentroid += Position(tile) + Position(type.tileSize()) / 2;
        }
        centroid = (currentCentroid / sizeWall);
    }

    void Wall::addDefenses()
    {
        // Find the furthest non pylon building to the chokepoint
        auto furthest = 0.0;
        for (auto &tile : this->getLargeTiles()) {
            const auto center = Position(tile) + Position(64, 48);
            const auto closestGeo = Map::getClosestChokeTile(this->getChokePoint(), center);
            const auto dist = center.getDistance(closestGeo);
            if (dist > furthest)
                furthest = dist;
        }
        for (auto &tile : this->getMediumTiles()) {
            const auto center = Position(tile) + Position(48, 32);
            const auto closestGeo = Map::getClosestChokeTile(this->getChokePoint(), center);
            const auto dist = center.getDistance(closestGeo);
            if (dist > furthest)
                furthest = dist;
        }

        for (auto &building : this->getRawDefenses()) {

            const auto start = TilePosition(this->getCentroid());
            const auto doorCenter = Position(this->getOpening()) + Position(16, 16);
            const auto isDefense = building == UnitTypes::Protoss_Photon_Cannon || building == UnitTypes::Terran_Missile_Turret || building == UnitTypes::Terran_Bunker;

            // Iterate around wall centroid to find a suitable position
            auto distBest = DBL_MAX;
            auto tileBest = TilePositions::Invalid;
            for (auto x = start.x - 8; x <= start.x + 8; x++) {
                for (auto y = start.y - 8; y <= start.y + 8; y++) {
                    const TilePosition t(x, y);
                    const auto center = Position(t) + Position(32, 32);

                    if (!t.isValid()
                        || Map::isReserved(t, building.tileWidth(), building.tileHeight())
                        || !Map::isPlaceable(building, t)
                        || Map::tilesWithinArea(this->getArea(), t, 2, 2) == 0
                        || (isDefense && (center.getDistance(Position(this->getChokePoint()->Center())) < furthest - 64.0 || center.getDistance(doorCenter) < 64.0)))
                        continue;

                    const auto closestGeo = Map::getClosestChokeTile(this->getChokePoint(), center);
                    const auto dist = center.getDistance(closestGeo);
                    const auto tooClose = isDefense && dist < furthest;

                    if (dist < distBest && !tooClose) {
                        Map::addUsed(t, building);
                        auto pathOut = findOpeningInWall();
                        if ((openWall && pathOut.isReachable()) || !openWall) {
                            tileBest = t;
                            distBest = dist;
                        }
                        Map::removeUsed(t, building.tileWidth(), building.tileHeight());
                    }
                }
            }

            // If tile is valid, add to wall
            if (tileBest.isValid()) {
                this->addToWall(tileBest, building);
                Map::addReserve(tileBest, 2, 2);
            }

            // Otherwise we can't place anymore
            else
                break;
        }

        // Add a reserved path
        auto currentPath = findOpeningInWall();
        for (auto &tile : currentPath.getTiles())
            Map::addReserve(tile, 1, 1);
    }

    void Wall::initialize()
    {
        chokeAngle = Map::getAngle(Map::lineOfBestFit(this->getChokePoint()));
        pylonWall = count(rawBuildings.begin(), rawBuildings.end(), BWAPI::UnitTypes::Protoss_Pylon) > 1;

        // Create a path for limiting BFS exploration
        Path jpsPath;
        initializePathPoints();
        checkPathPoints();
        jpsPath.createUnitPath(Position(startTile), Position(endTile));
        jpsDist = jpsPath.getDistance();

        if (!jpsPath.isReachable())
            return;

        auto start = TilePosition(this->getChokePoint()->Center());
        auto areaTop = TilePosition(this->getArea()->Top());
        auto movedStart = false;

        // Sort all the pieces and iterate over them to find the best wall - by Hannes
        if (find(this->getRawBuildings().begin(), this->getRawBuildings().end(), UnitTypes::Protoss_Pylon) != this->getRawBuildings().end()) {
            sort(this->getRawBuildings().begin(), this->getRawBuildings().end(), [](UnitType l, UnitType r) { return (l == UnitTypes::Protoss_Pylon) < (r == UnitTypes::Protoss_Pylon); }); // Moves pylons to end
            sort(this->getRawBuildings().begin(), find(this->getRawBuildings().begin(), this->getRawBuildings().end(), UnitTypes::Protoss_Pylon)); // Sorts everything before pylons
        }
        else
            sort(this->getRawBuildings().begin(), this->getRawBuildings().end());

        // If there is a base in this area and we're really far from it, move within 8 tiles of it            
        if (!this->getArea()->Bases().empty()) {
            auto startCenter = Position(start) + Position(16, 16);
            auto distBest = DBL_MAX;

            // Iterate 3x3 around the current TilePosition and try to get within 5 tiles
            while (startCenter.getDistance(this->getArea()->Bases().front().Center()) > 160.0) {
                for (int x = start.x - 1; x <= start.x + 1; x++) {
                    for (int y = start.y - 1; y <= start.y + 1; y++) {
                        const TilePosition t(x, y);
                        if (!t.isValid())
                            continue;

                        const auto p = Position(t) + Position(16, 16);
                        const auto dist = p.getDistance(Position(this->getArea()->Bases().front().Center()));

                        if (dist < distBest) {
                            distBest = dist;
                            start = t;
                            startCenter = p;
                            movedStart = true;
                        }
                    }
                }
            }
        }

        // If the start position isn't buildable, move towards the top of this area to find a buildable location
        while (!Broodwar->isBuildable(start)) {
            auto distBest = DBL_MAX;
            const auto initialStart = start;
            for (int x = initialStart.x - 1; x <= initialStart.x + 1; x++) {
                for (int y = initialStart.y - 1; y <= initialStart.y + 1; y++) {
                    const TilePosition t(x, y);
                    if (!t.isValid())
                        continue;

                    const auto p = Position(t);
                    const auto dist = p.getDistance(Position(areaTop));

                    if (dist < distBest) {
                        distBest = dist;
                        start = t;
                        movedStart = true;
                    }
                }
            }
        }

        const auto scoreWall = [&] {

            // Create a path searching for an opening
            auto pathOut = findOpeningInWall();

            // If we want an open wall and it's not reachable, or we want a closed wall and it is reachable
            if ((openWall && !pathOut.isReachable()) || (!openWall && pathOut.isReachable())) {
                failedPath++;
                return;
            }

            // Find distance for each piece to the closest choke tile
            auto dist = 1.0;
            for (auto &[tile, type] : currentLayout) {
                const auto center = Position(tile) + Position(type.tileWidth() * 16, type.tileHeight() * 16);
                const auto chokeDist = Map::getClosestChokeTile(this->getChokePoint(), center).getDistance(center);
                dist += chokeDist;
            }

            // Score wall based on path sizes and distances
            const auto score = !openWall ? dist : pathOut.getDistance() / max(1.0, dist);

            if (score > bestWallScore) {
                bestLayout = currentLayout;
                bestWallScore = score;
            }
        };

        function<void(TilePosition)> recursiveCheck = [&](TilePosition start) -> void {
            const auto radius = 8;
            const auto type = *typeIterator;

            for (auto x = start.x - radius; x < start.x + radius; x++) {
                for (auto y = start.y - radius; y < start.y + radius; y++) {
                    const TilePosition tile(x, y);
                    const auto center = Position(tile) + Position(type.tileWidth() * 16, type.tileHeight() * 16);

                    if (!tile.isValid())
                        continue;

                    if (!this->powerCheck(type, tile)) {
                        failedPower++;
                        continue;
                    }
                    if (!this->angleCheck(type, tile)) {
                        failedAngle++;
                        continue;
                    }
                    if (!this->placeCheck(type, tile)) {
                        failedPlacement++;
                        continue;
                    }
                    if (!this->tightCheck(type, tile)) {
                        failedTight++;
                        continue;
                    }
                    if (!this->spawnCheck(type, tile)) {
                        failedSpawn++;
                        continue;
                    }

                    // 1) Store the current type, increase the iterator
                    currentLayout[tile] = type;
                    Map::addUsed(tile, type);
                    typeIterator++;

                    // 2) If at the end, score wall
                    if (typeIterator == this->getRawBuildings().end())
                        scoreWall();
                    else
                        recursiveCheck(start);

                    // 3) Erase this current placement and repeat
                    if (typeIterator != this->getRawBuildings().begin())
                        typeIterator--;

                    currentLayout.erase(tile);
                    Map::removeUsed(tile, type.tileWidth(), type.tileHeight());
                }
            }
        };

        // For each permutation, try to make a wall combination that is better than the current best
        do {
            currentLayout.clear();
            typeIterator = this->getRawBuildings().begin();
            recursiveCheck(start);
        } while (next_permutation(this->getRawBuildings().begin(), find(this->getRawBuildings().begin(), this->getRawBuildings().end(), UnitTypes::Protoss_Pylon)));

        for (auto &[tile, type] : bestLayout) {
            this->addToWall(tile, type);
            Map::addReserve(tile, type.tileWidth(), type.tileHeight());
        }
    }

    void Wall::draw()
    {
        set<Position> anglePositions;
        int color = Broodwar->self()->getColor();
        int textColor = color == 185 ? textColor = Text::DarkGreen : Broodwar->self()->getTextColor();

        // Draw boxes around each TilePosition
        for (auto &tile : this->getSmallTiles()) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
            anglePositions.insert(Position(tile) + Position(32, 32));
        }
        for (auto &tile : this->getMediumTiles()) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), color);
            anglePositions.insert(Position(tile) + Position(48, 32));
        }
        for (auto &tile : this->getLargeTiles()) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), color);
            anglePositions.insert(Position(tile) + Position(64, 48));
        }
        for (auto &tile : this->getDefenses())
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);

        // Draw angles of all Wall pieces
        for (auto &pos1 : anglePositions) {
            for (auto &pos2 : anglePositions) {
                if (pos1 == pos2)
                    continue;
                const auto angle = Map::getAngle(make_pair(pos1, pos2));

                Broodwar->drawLineMap(pos1, pos2, color);
                Broodwar->drawTextMap((pos1 + pos2) / 2, "%c%.2f", textColor, angle);
            }
        }

        // Draw other Wall features
        Broodwar->drawBoxMap(Position(this->getOpening()), Position(this->getOpening()) + Position(33, 33), color, true);
        Broodwar->drawCircleMap(Position(this->getCentroid()) + Position(16, 16), 8, color, true);

        // Draw the line and angle of the ChokePoint
        auto p1 = this->getChokePoint()->Pos(this->getChokePoint()->end1);
        auto p2 = this->getChokePoint()->Pos(this->getChokePoint()->end2);
        auto line1 = Map::lineOfBestFit(this->getChokePoint());
        auto angle = Map::getAngle(line1);
        Broodwar->drawTextMap(Position(this->getChokePoint()->Center()), "%c%.2f", Text::Grey, angle);
        Broodwar->drawLineMap(Position(p1), Position(p2), Colors::Grey);

        // Draw the path points        
        Broodwar->drawCircleMap(Position(startTile), 6, Colors::Purple, true);
        Broodwar->drawCircleMap(Position(endTile), 6, Colors::Yellow, false);
    }
}
