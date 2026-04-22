#include <chrono>

#include "BWEB.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;

namespace BWEB {

    namespace {
        map<const BWEM::ChokePoint *const, Wall> walls;
        int wallReserveGrid[256][256] = {};
        bool logInfo                  = true;

        int existingDefenseGrid[256][256];

        map<TilePosition, BWAPI::Color> testTiles;
        map<TilePosition, std::string> textTiles;

        void addWallReserve(const TilePosition t, int w, int h)
        {
            for (auto x = t.x; x < t.x + w; x++) {
                for (auto y = t.y; y < t.y + h; y++)
                    if (TilePosition(x, y).isValid())
                        wallReserveGrid[x][y] = 1;
            }
        }
        void addWallReserve(const TilePosition t, UnitType type) { addWallReserve(t, type.tileWidth(), type.tileHeight()); }

        void removeWallReserve(const TilePosition t, int w, int h)
        {
            for (auto x = t.x; x < t.x + w; x++) {
                for (auto y = t.y; y < t.y + h; y++)
                    if (TilePosition(x, y).isValid())
                        wallReserveGrid[x][y] = 0;
            }
        }
        void removeWallReserve(const TilePosition t, UnitType type) { removeWallReserve(t, type.tileWidth(), type.tileHeight()); }

        bool isWallReserved(const TilePosition here, int w, int h)
        {
            for (auto x = here.x; x < here.x + w; x++) {
                for (auto y = here.y; y < here.y + h; y++) {
                    if (TilePosition(x, y).isValid() && wallReserveGrid[x][y] > 0)
                        return true;
                }
            }
            return false;
        }
        bool isWallReserved(const TilePosition t, UnitType type) { return isWallReserved(t, type.tileWidth(), type.tileHeight()); }
    } // namespace

    void Wall::initialize()
    {
        // Set important terrain features
        pylonWall    = count(rawBuildings.begin(), rawBuildings.end(), BWAPI::UnitTypes::Protoss_Pylon) > 1;
        base         = !area->Bases().empty() ? &area->Bases().front() : nullptr;
        station      = Stations::getClosestStation(TilePosition(area->Top()));
        defenseAngle = station->getDefenseAngle();
        wallLocation = base->Location();

        if (Broodwar->self()->getRace() == Races::Protoss)
            defenseType = UnitTypes::Protoss_Photon_Cannon;
        if (Broodwar->self()->getRace() == Races::Terran)
            defenseType = UnitTypes::Terran_Missile_Turret;
        if (Broodwar->self()->getRace() == Races::Zerg)
            defenseType = UnitTypes::Zerg_Creep_Colony;

        // Stations without chokepoints (or multiple) don't get determined on start
        if (!station->isMain() && !station->isNatural()) {
            auto bestGeo = Position(choke->Center()) + Position(4, 4);
            auto geoDist = DBL_MAX;
            for (auto &geo : choke->Geometry()) {
                auto p = Position(geo) + Position(4, 4);
                if (p.getDistance(station->getBase()->Center()) < geoDist)
                    bestGeo = Position(geo) + Position(4, 4);
            }
            defenseAngle = Map::getAngle(make_pair(station->getBase()->Center(), bestGeo)) + M_PI_D2;
        }
        defenseArrangement = int(round(defenseAngle / M_PI_D4)) % 4;

        // If this is a natural wall, check if the wall can by bypassed into the main, then wall at the choke instead
        if (station && station->isNatural()) {
            auto bypassable = false;
            auto main       = Stations::getClosestMainStation(base->Center());
            if (main->getChokepoint()) {
                auto mainChokeCenter = Position(main->getChokepoint()->Center());
                auto natChokeCenter  = Position(choke->Center());
                auto distNatToMainChoke = station->getBase()->Center().getDistance(mainChokeCenter);
                auto distNatToNatChoke = station->getBase()->Center().getDistance(natChokeCenter);

                if (distNatToNatChoke >= 240.0 && distNatToMainChoke >= 240.0) {

                    // Check each point of geometry to see if it's close to the main choke
                    auto bypassCount = 0;
                    for (auto &geo : choke->Geometry()) {
                        auto p = Position(geo) + Position(4, 4);
                        if (p.getDistance(base->Center()) >= p.getDistance(mainChokeCenter))
                            bypassCount++;
                    }

                    // Wall is bypassable, need to change how it generates to defend the main as well
                    if (bypassCount >= int(choke->Geometry().size() / 4)) {
                        bypassable          = true;
                        auto mainChokeTile  = TilePosition(mainChokeCenter);
                        auto baseCenterTile = TilePosition(base->Center());

                        auto dx = 0;
                        if (defenseArrangement == 0) {
                            dx = mainChokeTile.x - baseCenterTile.x;
                            if (dx >= 2)
                                dx -= 2;
                            if (dx <= -2)
                                dx += 2;
                        }

                        auto dy = 0;
                        if (defenseArrangement == 2) {
                            dy = mainChokeTile.y - baseCenterTile.y;
                            if (dy >= 2)
                                dy -= 2;
                            if (dy <= -2)
                                dy += 2;
                        }

                        auto tiledx  = clamp(dx, -8, 8);
                        auto tiledy  = clamp(dy, -8, 8);
                        wallLocation = TilePosition(base->Location()) + TilePosition(tiledx, tiledy);

                        defenseAngle       = BWEB::Map::getAngle(Position(wallLocation) + Position(16, 16), natChokeCenter) + M_PI_D2;
                        defenseArrangement = int(round(defenseAngle / M_PI_D4)) % 4;

                        testTiles[wallLocation] = Colors::Blue;
                    }
                }
            }
        }
    }

    bool Wall::tryLocations(vector<TilePosition> &tryOrder, set<TilePosition> &insertList, UnitType type)
    {
        auto debugColor = Colors::White;
        if (type.tileWidth() == 3)
            debugColor = Colors::Grey;
        if (type.tileWidth() == 4)
            debugColor = Colors::Black;

        // Defense types place every possible tile
        if (type.tileWidth() == 2 && type.tileHeight() == 2 && type != Protoss_Pylon && type != Zerg_Spire) {
            for (auto placement : tryOrder) {
                auto tile           = wallLocation + placement;
                auto stationDefense = type.tileHeight() == 2 && type.tileWidth() == 2 && station->getDefenses().find(tile) != station->getDefenses().end();
                if ((!isWallReserved(tile, type) && BWEB::Map::isPlaceable(type, tile)) || stationDefense) {
                    insertList.insert(tile);
                    addWallReserve(tile, type);
                    return true;
                }
            }
        }

        // Other types only need 1 unique spot
        else {
            bool firstPlacement = Broodwar->self()->getRace() == Races::Protoss || (largeTiles.empty() && mediumTiles.empty() && smallTiles.empty());
            static int i        = 0;
            for (auto placement : tryOrder) {
                auto tile = wallLocation + placement;
                // textTiles[tile] = std::to_string(i);
                i++;
                testTiles[tile] = debugColor;

                // Must be adjacent to another piece or unwalkable terrain (look at only border edges)
                auto valid = type == Protoss_Pylon;
                for (int x = -1; x < type.tileWidth() + 1; x++) {
                    for (int y = -1; y < type.tileHeight() + 1; y++) {

                        // Not a border tile
                        if (x >= 0 && x < type.tileWidth() && y >= 0 && y < type.tileHeight())
                            continue;
                        // A corner
                        if ((x == -1 || x == type.tileWidth()) && (y == -1 || y == type.tileHeight()))
                            continue;

                        auto borderTile = tile + TilePosition(x, y);
                        if (borderTile.isValid()) {
                            auto notWalkable = !Broodwar->isBuildable(borderTile);
                            auto reserved    = wallReserveGrid[borderTile.x][borderTile.y] > 0;

                            if ((firstPlacement && notWalkable) || reserved) {
                                valid = true;
                            }
                        }
                    }
                }
                if (!valid)
                    continue;

                //
                if (!isWallReserved(tile, type) && BWEB::Map::isPlaceable(type, tile) && !Map::isReserved(tile, type.tileWidth(), type.tileHeight())) {
                    insertList.insert(tile);
                    addWallReserve(tile, type);
                    return true;
                }
            }
        }
        return false;
    }

    void Wall::addPieces()
    {
        // For each piece, try to place it a known distance away depending on how the angles of chokes look
        auto closestMain     = Stations::getClosestMainStation(station->getBase()->Location());
        auto mainChokeCenter = Position(closestMain->getChokepoint()->Center());
        auto flipOrder       = false; // TODO: Right now we flip the opposite way so we always can get out
        auto maintainShape   = true;

        const auto adjustOrder = [&](auto &order, auto diff) {
            for (auto &tile : order)
                tile = tile + diff;
        };

        // Iteration attempts move buildings closer
        auto iteration    = 0;
        auto maxIteration = 1;

        if (station->isNatural() && Broodwar->self()->getRace() == Races::Protoss) {
            iteration    = 0;
            maxIteration = 1;
        }

        // If this isn't a natural or main
        if (station && !station->isMain() && !station->isNatural()) {
            iteration     = 0;
            maxIteration  = 4;
            maintainShape = true;
        }

        // This is ugly
        if (station && station->isMain()) {
            iteration    = 5;
            maxIteration = 8;
        }

        for (; iteration < maxIteration; iteration += 2) {
            cleanup();
            smallTiles.clear();
            mediumTiles.clear();
            largeTiles.clear();

            // 0/8 - Horizontal
            if (defenseArrangement == 0) {
                flipVertical = base->Center().y < Position(choke->Center()).y;
                lrgOrder     = {{-6, -6}, {-5, -6}, {-4, -6}, {-3, -6}, {-2, -6}, {-1, -6}, {0, -6}, {1, -6}, {2, -6}, {3, -6}, {4, -6}, {5, -6}, {6, -6}};
                medOrder     = {{-6, -5}, {-5, -5}, {-4, -5}, {-3, -5}, {-2, -5}, {-1, -5}, {0, -5}, {1, -5}, {2, -5}, {3, -5}, {4, -5}, {5, -5}, {6, -5}};
                smlOrder     = {{-2, -5}, {-1, -5}, {0, -5}, {1, -5}, {2, -5}, {3, -5}, {4, -5}, {5, -5}};
                pylOrder     = {{0, -2}, {2, -2}};
                flipOrder    = station->isNatural() && mainChokeCenter.x > base->Center().x;

                for (int x = -5; x <= 8; x++)
                    opnOrder.push_back({x, -4});

                // Shift positions based on chokepoint offset and iteration
                auto diffX = !maintainShape ? TilePosition(choke->Center()).x - wallLocation.x : 0;
                auto diffY = -iteration;
                wallOffset = TilePosition(0, -iteration);
                adjustOrder(lrgOrder, TilePosition(diffX, diffY));
                adjustOrder(medOrder, TilePosition(diffX, diffY));
                adjustOrder(smlOrder, TilePosition(diffX, diffY));
            }

            // pi/4 - Angled
            else if (defenseArrangement == 1 || defenseArrangement == 3) {
                flipVertical   = base->Center().y < Position(choke->Center()).y;
                flipHorizontal = base->Center().x < Position(choke->Center()).x;
                lrgOrder       = {{0, -7}, {0, -6}, {-2, -5}, {-2, -4}, {-4, -3}, {-4, -2}, {-6, -1}, {-6, 0}, {-8, 1}, {-8, 2}};
                medOrder       = {{1, -6}, {1, -5}, {-1, -4}, {-1, -3}, {-3, -2}, {-3, -1}, {-5, 0}, {-5, 1}};
                smlOrder       = {{0, -4}, {-2, -2}, {-4, 0}};
                pylOrder       = {{-2, 0}, {0, -2}};

                // TODO: these flips don't always work as intended
                flipOrder = (station->isNatural() && mainChokeCenter.x < base->Center().x && mainChokeCenter.y > base->Center().y) ||
                            (station->isNatural() && mainChokeCenter.x > base->Center().x && mainChokeCenter.y < base->Center().y) ||
                            (!station->isMain() && !station->isNatural() && station->getResourceCentroid().x < station->getBase()->Center().x);

                // Shift positions based on iteration
                auto diffX = -iteration;
                auto diffY = -iteration;
                wallOffset = TilePosition(-iteration - 2, -iteration - 2);
                adjustOrder(lrgOrder, TilePosition(diffX, diffY));
                adjustOrder(medOrder, TilePosition(diffX, diffY));
                adjustOrder(smlOrder, TilePosition(diffX, diffY));

                // If this isn't a natural or main, allow it to place more large blocks as zerg
                if (station && !station->isMain() && !station->isNatural() && Broodwar->self()->getRace() == Races::Zerg) {
                    lrgOrder.push_back(TilePosition(-4, 0));
                    lrgOrder.push_back(TilePosition(0, -3));
                    lrgOrder.push_back(TilePosition(-4, -3));
                    // medOrder.push_back(TilePosition(-1, -7));
                    // medOrder.push_back(TilePosition(-3, -5));
                    // medOrder.push_back(TilePosition(-5, -3));
                    // medOrder.push_back(TilePosition(-7, -1));
                }
            }

            // pi/2 - Vertical
            else if (defenseArrangement == 2) {
                flipHorizontal = base->Center().x < Position(choke->Center()).x;
                lrgOrder       = {{-7, -4}, {-7, -3}, {-7, -2}, {-7, -1}, {-7, 0}, {-7, 1}, {-7, 2}, {-7, 3}, {-7, 4}};
                medOrder       = {{-6, -3}, {-6, -2}, {-6, -1}, {-6, 0}, {-6, 1}, {-6, 2}, {-6, 3}, {-6, 4}};
                smlOrder       = {{-5, -3}, {-5, -2}, {-5, -1}, {-5, 0}, {-5, 1}, {-5, 2}, {-5, 3}, {-5, 4}};
                pylOrder       = {{-2, 0}, {-2, 2}};

                flipOrder = station->isNatural() && mainChokeCenter.y > base->Center().y;

                // Shift positions based on chokepoint offset and iteration
                auto diffX = -iteration;
                auto diffY = !maintainShape ? TilePosition(choke->Center()).y - wallLocation.y : 0;
                wallOffset = TilePosition(-iteration, 0);
                adjustOrder(lrgOrder, TilePosition(diffX, diffY));
                adjustOrder(medOrder, TilePosition(diffX, diffY));
                adjustOrder(smlOrder, TilePosition(diffX, diffY));
            }

            // Flip them vertically / horizontally as needed
            const auto flipPositions = [&](auto &tryOrder, auto type) {
                if (flipVertical) {
                    wallOffset = TilePosition(wallOffset.x, iteration);
                    for (auto &placement : tryOrder) {
                        auto diff   = 3 - type.tileHeight();
                        placement.y = -(placement.y - diff);
                    }
                }
                if (flipHorizontal) {
                    wallOffset = TilePosition(iteration, wallOffset.y);
                    for (auto &placement : tryOrder) {
                        auto diff   = 4 - type.tileWidth();
                        placement.x = -(placement.x - diff);
                    }
                }
                if (flipOrder)
                    std::reverse(tryOrder.begin(), tryOrder.end());
            };

            // Check if positions need to be flipped vertically or horizontally
            flipPositions(smlOrder, Protoss_Pylon);
            flipPositions(pylOrder, Protoss_Pylon);
            flipPositions(medOrder, Protoss_Forge);
            flipPositions(lrgOrder, Protoss_Gateway);
            flipPositions(opnOrder, Protoss_Dragoon);

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                for (auto &building : rawBuildings) {
                    if (building.tileWidth() == 4)
                        tryLocations(lrgOrder, largeTiles, Zerg_Hatchery);
                    if (building.tileWidth() == 3)
                        tryLocations(medOrder, mediumTiles, Zerg_Evolution_Chamber);
                    if (building.tileWidth() == 2)
                        tryLocations(smlOrder, smallTiles, Zerg_Spire);
                }
            }

            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                std::reverse(lrgOrder.begin(), lrgOrder.end()); // Flip large order so that gateway is opposite of forge, center opening
                tryLocations(medOrder, mediumTiles, Protoss_Forge);
                tryLocations(lrgOrder, largeTiles, Protoss_Gateway);
                // tryLocations(smlOrder, smallTiles, Protoss_Pylon);
                tryLocations(pylOrder, smallTiles, Protoss_Pylon);
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran) {
                for (auto &building : rawBuildings) {
                    if (building.tileWidth() == 4)
                        tryLocations(lrgOrder, largeTiles, Terran_Barracks);
                    if (building.tileWidth() == 3)
                        tryLocations(medOrder, mediumTiles, Terran_Supply_Depot);
                }
            }

            // If iteration was negative, we prevent moving the defenses backwards by resetting the walloffset here
            if (iteration < 0)
                wallOffset = TilePosition(0, 0);

            // Always place cannons where the station is as P
            if (Broodwar->self()->getRace() == Races::Protoss)
                wallOffset = TilePosition(0, 0);

            // If a wall is found, we're done
            if ((getSmallTiles().size() + getMediumTiles().size() + getLargeTiles().size()) == getRawBuildings().size())
                break;
        }
    }

    void Wall::addDefenseLayer(int offset, int layer, int width)
    {
        vector<TilePosition> wallPlacements;
        int dx = 0, dy = 0, ox = 0, oy = 0;

        // Determine the direction of iteration
        if (defenseArrangement == 0) { // 0/8 - Horizontal
            dx = -1;
            dy = 0;
            oy = offset;
        }
        else if (defenseArrangement == 2) { // pi/2 - Vertical
            dx = 0;
            dy = -1;
            ox = offset;
        }
        else if (defenseArrangement == 1 || defenseArrangement == 3) { // pi/4 - Angled
            dx = 1;
            dy = -1;
            oy = offset;
            width -= 2;
        }

        const auto addToList = [&](int i) {
            int x = dx * i + ox;
            int y = dy * i + oy;
            wallPlacements.push_back({x, y});
        };

        // First priority placements (center out)
        for (int i = 0; i <= width; i++) {
            addToList(i);
        }
        for (int i = 0; i >= -width; i--) {
            addToList(i);
        }

        // Flip them vertically / horizontally as needed
        if (flipVertical) {
            for (auto &placement : wallPlacements)
                placement.y = 1 - placement.y;
        }
        if (flipHorizontal) {
            for (auto &placement : wallPlacements)
                placement.x = 2 - placement.x;
        }

        // Add wall defenses to the set
        for (auto &placement : wallPlacements) {
            auto tile = base->Location() + placement;
            if (Map::isPlaceable(defenseType, tile) && !isWallReserved(tile, defenseType) &&
                (layer == 1 || !Map::isReserved(tile, 2, 2) || station->getDefenses().find(tile) != station->getDefenses().end())) {
                addWallReserve(tile, defenseType);
                defenses[0].insert(tile);
                defenses[layer].insert(tile);
            }
        }
    }

    void Wall::addDefenses()
    {
        addDefenseLayer(-2, 1, 3);
        addDefenseLayer(-2, 2, 6);
        addDefenseLayer(0, 3, 6);
    }

    void Wall::addOpenings()
    {
        auto type      = Protoss_Dragoon;
        auto foundWall = false;
        for (auto placement : opnOrder) {
            auto tile = wallLocation + placement;
            if (isWallReserved(tile, type))
                foundWall = true;

            if (foundWall && !isWallReserved(tile, type) && BWEB::Map::isPlaceable(type, tile)) {
                openings.insert(tile);
                addWallReserve(tile, type);
            }
        }
    }

    void Wall::cleanup()
    {
        // Remove used from tiles
        for (auto &tile : smallTiles)
            removeWallReserve(tile, 2, 2);
        for (auto &tile : mediumTiles)
            removeWallReserve(tile, 3, 2);
        for (auto &tile : largeTiles)
            removeWallReserve(tile, 4, 3);
        for (auto &tile : openings)
            removeWallReserve(tile, 1, 1);
        for (auto &[_, tiles] : defenses) {
            for (auto &tile : tiles)
                removeWallReserve(tile, 2, 2);
        }
    }

    const bool Wall::requestAddedLayer()
    {
        for (auto &tile : smallTiles)
            removeWallReserve(tile, 2, 2);
        for (auto &tile : mediumTiles)
            removeWallReserve(tile, 3, 2);
        for (auto &tile : largeTiles)
            removeWallReserve(tile, 4, 3);

        smallTiles.clear();
        mediumTiles.clear();
        largeTiles.clear();

        addDefenseLayer(-4, 1, 6);
        return true;
    }

    const int Wall::getGroundDefenseCount() const
    {
        // Returns how many visible ground defensive structures exist in this Walls defense locations
        int count = 0;
        if (defenses.find(0) != defenses.end()) {
            for (auto &tile : defenses.at(0)) {
                auto type = Map::isUsed(tile);
                if (type == UnitTypes::Protoss_Photon_Cannon || type == UnitTypes::Zerg_Sunken_Colony || type == UnitTypes::Terran_Bunker)
                    count++;
            }
        }
        return count;
    }

    const int Wall::getAirDefenseCount() const
    {
        // Returns how many visible air defensive structures exist in this Walls defense locations
        int count = 0;
        if (defenses.find(0) != defenses.end()) {
            for (auto &tile : defenses.at(0)) {
                auto type = Map::isUsed(tile);
                if (type == UnitTypes::Protoss_Photon_Cannon || type == UnitTypes::Zerg_Spore_Colony || type == UnitTypes::Terran_Missile_Turret)
                    count++;
            }
        }
        return count;
    }

    const void Wall::draw()
    {
        set<Position> anglePositions;
        int color     = Broodwar->self()->getColor();
        int textColor = color == 185 ? textColor = Text::DarkGreen : Broodwar->self()->getTextColor();

        // Draw boxes around each feature
        auto drawBoxes = true;
        if (drawBoxes) {
            for (auto &tile : smallTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cWS", textColor);
            }
            for (auto &tile : mediumTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cWM", textColor);
            }
            for (auto &tile : largeTiles) {
                Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), color);
                Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cWL", textColor);
            }
            for (auto &tile : openings) {
                Broodwar->drawBoxMap(Position(tile) + Position(2, 2), Position(tile) + Position(31, 31), color, true);
            }
            for (int i = 1; i <= 4; i++) {
                for (auto &tile : defenses[i]) {
                    Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
                    Broodwar->drawTextMap(Position(tile) + Position(4, 4), "%cW", textColor);
                    Broodwar->drawTextMap(Position(tile) + Position(4, 20), "%c%d", textColor, i);
                }
            }
        }

        // Draw the line of the chokepoint
        auto p1 = choke->Pos(choke->end1);
        auto p2 = choke->Pos(choke->end2);
        Broodwar->drawLineMap(Position(p1), Position(p2), Colors::Grey);

        // Write the angle of the wall/chokepoint
        auto chokeCenter = Position(choke->Center());
        Broodwar->drawBoxMap(chokeCenter - Position(32, 16), chokeCenter + Position(32, 16), Colors::Black, true);
        Broodwar->drawBoxMap(chokeCenter - Position(32, 16), chokeCenter + Position(32, 16), Colors::Grey);
        Broodwar->drawTextMap(chokeCenter - Position(22, 8), "%c%.2f (%.2f)", Text::Grey, defenseAngle, station->getDefenseAngle());
    }
} // namespace BWEB

namespace BWEB::Walls {

    Wall *const createWall(vector<UnitType> &buildings, const BWEM::Area *area, const BWEM::ChokePoint *choke, const UnitType tightType, const vector<UnitType> &defenses, const bool openWall,
                           const bool requireTight)
    {
        ofstream writeFile;
        string buffer;
        auto timePointNow = chrono::system_clock::now();
        auto timeNow      = chrono::system_clock::to_time_t(timePointNow);

        // Print the clock position of this Wall
        auto clock = (round((Map::getAngle(make_pair(Position(area->Top()), Map::mapBWEM.Center())) - M_PI_D2) / 0.52));
        if (clock < 0)
            clock += 12;

        // Open the log file if desired and write information
        if (logInfo) {
            writeFile.open("bwapi-data/write/BWEB_Wall.txt");
            writeFile << ctime(&timeNow);
            writeFile << Broodwar->mapFileName().c_str() << endl;
            writeFile << "At: " << clock << " o'clock." << endl;
            writeFile << endl;

            writeFile << "Buildings:" << endl;
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

        // Verify not attempting to create a Wall in the same Area/ChokePoint combination
        for (auto &[_, wall] : walls) {
            if (wall.getChokePoint() == choke) {
                writeFile << "BWEB: Can't create a Wall where one already exists." << endl;
                return &wall;
            }
        }

        // Create a Wall
        Wall wall(area, choke, buildings, defenses, tightType, requireTight, openWall);

        // Verify the Wall creation was successful
        auto wallFound = (wall.getSmallTiles().size() + wall.getMediumTiles().size() + wall.getLargeTiles().size()) == wall.getRawBuildings().size();
        if (wallFound) {
            for (auto &tile : wall.getSmallTiles())
                Map::addReserve(tile, 2, 2);
            for (auto &tile : wall.getMediumTiles())
                Map::addReserve(tile, 3, 2);
            for (auto &tile : wall.getLargeTiles())
                Map::addReserve(tile, 4, 3);
            for (auto &tile : wall.getOpenings())
                Map::addReserve(tile, 1, 1);
            for (auto &tile : wall.getDefenses())
                Map::addReserve(tile, 2, 2);
        }

        // Log information
        if (logInfo) {
            double dur = std::chrono::duration<double, std::milli>(chrono::system_clock::now() - timePointNow).count();
            writeFile << "Generation Time: " << dur << "ms and " << (wallFound ? "successful." : "failed.") << endl;
            writeFile << "--------------------" << endl;
        }

        // If we found a suitable Wall, push into container and return pointer to it
        if (wallFound) {
            walls.emplace(choke, wall);
            return &walls.at(choke);
        }
        return nullptr;
    }

    Wall *const createProtossWall()
    {
        vector<UnitType> buildings = {Protoss_Forge, Protoss_Gateway, Protoss_Pylon};
        return createWall(buildings, BWEB::Stations::getStartingNatural()->getBase()->GetArea(), BWEB::Stations::getStartingNatural()->getChokepoint(), UnitTypes::None, {Protoss_Photon_Cannon}, true,
                          false);
    }

    Wall *const createTerranWall()
    {
        vector<UnitType> buildings = {Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot};
        auto tightType             = Broodwar->enemy()->getRace() == Races::Zerg ? Zerg_Zergling : Protoss_Zealot;
        return createWall(buildings, BWEB::Stations::getStartingNatural()->getBase()->GetArea(), BWEB::Stations::getStartingNatural()->getChokepoint(), tightType, {Terran_Missile_Turret}, false,
                          true);
    }

    Wall *const createZergWall()
    {
        vector<UnitType> buildings = {Zerg_Hatchery, Zerg_Evolution_Chamber};
        return createWall(buildings, BWEB::Stations::getStartingNatural()->getBase()->GetArea(), BWEB::Stations::getStartingNatural()->getChokepoint(), UnitTypes::None, {Zerg_Sunken_Colony}, true,
                          false);
    }

    Wall *const getWall(const BWEM::ChokePoint *choke)
    {
        if (!choke)
            return nullptr;

        for (auto &[_, wall] : walls) {
            if (wall.getChokePoint() == choke)
                return &wall;
        }
        return nullptr;
    }

    map<const BWEM::ChokePoint *const, Wall> &getWalls() { return walls; }

    void draw()
    {
        for (auto &[tile, color] : testTiles) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(32, 32), color);
        }
        for (auto &[tile, text] : textTiles) {
            Broodwar->drawTextMap(Position(tile) + Position(16, 16), text.c_str());
        }
        for (auto &[_, wall] : walls)
            wall.draw();
    }
} // namespace BWEB::Walls
