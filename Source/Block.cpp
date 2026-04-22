#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB {

    set<TilePosition> debugTiles;

    void Block::draw()
    {
        int color     = Broodwar->self()->getColor();
        int textColor = color == 185 ? textColor = Text::DarkGreen : Broodwar->self()->getTextColor();

        // Draw boxes around each feature
        for (auto &tile : smallTiles) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(65, 65), color);
            Broodwar->drawTextMap(Position(tile) + Position(52, 52), "%cB", textColor);
        }
        for (auto &tile : mediumTiles) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(97, 65), color);
            Broodwar->drawTextMap(Position(tile) + Position(84, 52), "%cB", textColor);
        }
        for (auto &tile : largeTiles) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 97), color);
            Broodwar->drawTextMap(Position(tile) + Position(116, 84), "%cB", textColor);
        }
        for (auto &tile : wideTiles) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(129, 65), color);
            Broodwar->drawTextMap(Position(tile) + Position(116, 52), "%cB", textColor);
        }
        for (auto &tile : debugTiles) {
            Broodwar->drawBoxMap(Position(tile), Position(tile) + Position(31, 31), Colors::Yellow);
        }

        Broodwar->drawTextMap(Position(tile), "%d, %d", w, h);
    }
} // namespace BWEB

namespace BWEB::Blocks {

    namespace {
        vector<Block> allBlocks;
        int blockGrid[256][256];
        vector<TilePosition> availableTiles;
        TilePosition firstSupplyBlock;

        void addToBlockGrid(TilePosition start, TilePosition end)
        {
            for (int x = start.x; x < end.x; x++) {
                for (int y = start.y; y < end.y; y++)
                    blockGrid[x][y] = 1;
            }
        }

        struct PieceCount {
            map<Piece, int> pieces;
            int count(Piece p) { return pieces[p]; }
        };

        // TODO: Use piece per station instead
        map<const BWEM::Area *const, PieceCount> piecePerArea;

        int countPieces(vector<Piece> pieces, Piece type)
        {
            auto count = 0;
            for (auto &piece : pieces) {
                if (piece == type)
                    count++;
            }
            return count;
        }
    } // namespace

    bool validTile(TilePosition &t, Station station)
    {
        const auto &area = Map::mapBWEM.GetArea(t);
        if (!area)
            return false;

        // Shared area
        if (area == station.getBase()->GetArea())
            return true;

        // Tiles in empty extensions of the main base
        if (area->Bases().empty() && find(area->AccessibleNeighbours().begin(), area->AccessibleNeighbours().end(), station.getBase()->GetArea()) != area->AccessibleNeighbours().end())
            return true;

        // Tiles at the bottom of the map are buildable but dont have an area
        auto tileAbove = t - TilePosition(0, 1);
        return (tileAbove.isValid() && Map::mapBWEM.GetArea(tileAbove) && Map::mapBWEM.GetArea(tileAbove) == station.getBase()->GetArea());
    }

    vector<TilePosition> wherePieces(TilePosition tile, vector<Piece> pieces)
    {
        vector<TilePosition> pieceLayout;
        auto rowHeight = 0;
        auto rowWidth  = 0;
        int w = 0, h = 0;
        auto here = tile;
        for (auto &p : pieces) {
            if (p == Piece::Small) {
                pieceLayout.push_back(here);
                here += TilePosition(2, 0);
                rowWidth += 2;
                rowHeight = std::max(rowHeight, 2);
            }
            if (p == Piece::Medium) {
                pieceLayout.push_back(here);
                here += TilePosition(3, 0);
                rowWidth += 3;
                rowHeight = std::max(rowHeight, 2);
            }
            if (p == Piece::Large) {
                pieceLayout.push_back(here);
                here += TilePosition(4, 0);
                rowWidth += 4;
                rowHeight = std::max(rowHeight, 3);
            }
            if (p == Piece::Wide) {
                pieceLayout.push_back(here);
                here += TilePosition(4, 0);
                rowWidth += 4;
                rowHeight = std::max(rowHeight, 2);
            }
            if (p == Piece::Addon) {
                pieceLayout.push_back(here + TilePosition(0, 1));
                here += TilePosition(2, 0);
                rowWidth += 2;
                rowHeight = std::max(rowHeight, 2);
            }
            if (p == Piece::Row) {
                w = std::max(w, rowWidth);
                h += rowHeight;
                rowWidth  = 0;
                rowHeight = 0;
                here      = tile + TilePosition(0, h);
            }
            if (p == Piece::Space) {
                here += TilePosition(1, 0);
                rowWidth += 1;
                rowHeight = std::max(rowHeight, 2);
            }
        }
        return pieceLayout;
    }

    vector<Piece> whichPieces(int width, int height)
    {
        vector<Piece> pieces;

        // Zerg Block pieces
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (height == 2) {
                if (width == 2)
                    pieces = {Piece::Small};
                if (width == 3)
                    pieces = {Piece::Medium};
                if (width == 4)
                    pieces = {Piece::Wide};
            }
            else if (height == 3) {
                if (width == 4)
                    pieces = {Piece::Large};
            }
        }

        // Protoss Block pieces
        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (height == 2) {
                if (width == 2)
                    pieces = {Piece::Small};
                if (width == 5)
                    pieces = {Piece::Small, Piece::Medium};
            }
            else if (height == 4) {
                if (width == 5)
                    pieces = {Piece::Small, Piece::Medium, Piece::Row, Piece::Small, Piece::Medium};
            }
            else if (height == 5) {
                if (width == 4)
                    pieces = {Piece::Large, Piece::Row, Piece::Small, Piece::Small};
                if (width == 8) {
                    pieces = {Piece::Small, Piece::Medium, Piece::Medium, Piece::Row, Piece::Large, Piece::Large};
                }
                if (width == 16) {
                    pieces = {Piece::Small, Piece::Medium, Piece::Medium, Piece::Small, Piece::Medium, Piece::Medium, Piece::Row, Piece::Large, Piece::Large, Piece::Large, Piece::Large};
                }
            }
            else if (height == 6) {
                if (width == 10)
                    pieces = {Piece::Large, Piece::Addon, Piece::Large, Piece::Row, Piece::Large, Piece::Small, Piece::Large};
                if (width == 18)
                    pieces = {Piece::Large, Piece::Large, Piece::Addon, Piece::Large, Piece::Large, Piece::Row, Piece::Large, Piece::Large, Piece::Small, Piece::Large, Piece::Large};
            }
            else if (height == 8) {
                if (width == 5)
                    pieces = {Piece::Large, Piece::Row, Piece::Small, Piece::Medium, Piece::Row, Piece::Large};
                if (width == 4)
                    pieces = {Piece::Large, Piece::Row, Piece::Small, Piece::Small, Piece::Row, Piece::Large};
                if (width == 8)
                    pieces = {Piece::Large, Piece::Large, Piece::Row, Piece::Space, Piece::Space, Piece::Small, Piece::Small, Piece::Space, Piece::Space, Piece::Row, Piece::Large, Piece::Large};
            }
        }

        // Terran Block pieces
        if (Broodwar->self()->getRace() == Races::Terran) {
            if (height == 2) {
                if (width == 3)
                    pieces = {Piece::Medium};
            }
            else if (height == 3) {
                if (width == 6)
                    pieces = {Piece::Large, Piece::Addon};
            }
            else if (height == 6) {
                if (width == 6)
                    pieces = {Piece::Large, Piece::Addon, Piece::Row, Piece::Large, Piece::Addon};
                if (width == 10)
                    pieces = {Piece::Large, Piece::Large, Piece::Addon, Piece::Row, Piece::Large, Piece::Large, Piece::Addon};
            }
            else if (height == 9) {
                if (width == 6)
                    pieces = {Piece::Large, Piece::Addon, Piece::Row, Piece::Large, Piece::Addon, Piece::Row, Piece::Large, Piece::Addon};
                if (width == 10)
                    pieces = {Piece::Large, Piece::Large, Piece::Addon, Piece::Row, Piece::Large, Piece::Large, Piece::Addon, Piece::Row, Piece::Large, Piece::Large, Piece::Addon};
            }
        }
        return pieces;
    }

    multimap<TilePosition, Piece> generatePieceLayout(vector<Piece> pieces, vector<TilePosition> layout)
    {
        multimap<TilePosition, Piece> pieceLayout;
        auto pieceItr  = pieces.begin();
        auto layoutItr = layout.begin();
        while (pieceItr != pieces.end() && layoutItr != layout.end()) {
            if (*pieceItr == Piece::Row || *pieceItr == Piece::Space) {
                pieceItr++;
                continue;
            }
            pieceLayout.insert(make_pair(*layoutItr, *pieceItr));
            pieceItr++;
            layoutItr++;
        }
        return pieceLayout;
    }

    bool canAddBlock(const TilePosition here, const int width, const int height, multimap<TilePosition, Piece> &pieces, BlockType type)
    {
        const auto blockWalkable = [&](const TilePosition &t) {
            return (t.x < here.x || t.x > here.x + width || t.y < here.y || t.y > here.y + height) && !blockGrid[t.x][t.y] && !BWEB::Map::isReserved(t);
        };

        const auto blockExists = [&](const TilePosition &t) { return blockGrid[t.x][t.y + 3]; };

        const auto productionReachable = [&](TilePosition start) {
            if (!Map::mapBWEM.GetArea(here))
                return false;
            for (auto &choke : Map::mapBWEM.GetArea(here)->ChokePoints()) {
                auto path    = BWEB::Path(start + TilePosition(0, 3), TilePosition(choke->Center()), UnitTypes::Protoss_Dragoon, false, false);
                auto maxDist = path.getSource().getDistance(path.getTarget());
                auto maxDim  = max(width, height);
                path.generateJPS([&](const TilePosition &t) { return path.terrainWalkable(t) && blockWalkable(t) && t.getDistance(path.getTarget()) < maxDist + 5; });
                Pathfinding::clearCacheFully();
                if (!path.isReachable())
                    return false;
            }
            return true;
        };

        // Check if placing a Hatchery within this Block cannot be done due to resources around it
        for (auto &[tile, piece] : pieces) {
            if (piece == Piece::Large && Broodwar->self()->getRace() == BWAPI::Races::Zerg && !Broodwar->canBuildHere(tile, UnitTypes::Zerg_Hatchery))
                return false;
        }

        // Check if a Block of specified size would overlap any bases, resources or other blocks
        auto ff = ((type != BlockType::Supply && Broodwar->self()->getRace() != BWAPI::Races::Zerg) || type == BlockType::Production) ? 1 : 0;

        for (auto x = here.x - ff; x < here.x + width + ff; x++) {
            for (auto y = here.y - ff; y < here.y + height + ff; y++) {
                const TilePosition t(x, y);
                if (!t.isValid())
                    return false;
                if (type == BlockType::Supply && Map::mapBWEM.GetTile(t).MinAltitude() > 170)
                    return false;
                if (!Map::mapBWEM.GetTile(t).Buildable() || Map::isReserved(t))
                    return false;
            }
        }

        // Can't place too close to main choke
        if (type == BlockType::Supply) {
            auto center = Position(here) + Position(width * 32, height * 32);
            if (center.getDistance(Position(Stations::getStartingMain()->getChokepoint()->Center())) < 160.0)
                return false;
        }

        return true;
    }

    void createBlock(TilePosition here, multimap<TilePosition, Piece> &pieceLayout, int width, int height, BlockType type)
    {
        Block newBlock(here, pieceLayout, width, height, type);
        const auto &area = Map::mapBWEM.GetArea(here);
        allBlocks.push_back(newBlock);
        addToBlockGrid(here, here + TilePosition(width, height));
        for (auto &[_, piece] : pieceLayout)
            piecePerArea[area].pieces[piece]++;

        // Store pieces - extra spaces for non Zerg buildings
        for (auto &[placement, piece] : pieceLayout) {
            if (piece == Piece::Small) {
                BWEB::Map::addReserve(placement, 2, 2);
            }
            if (piece == Piece::Medium) {
                BWEB::Map::addReserve(placement, 3, 2);
            }
            if (piece == Piece::Large) {
                auto tile = (Broodwar->self()->getRace() != Races::Zerg) ? (placement - TilePosition(1, 1)) : placement;
                auto ff   = (Broodwar->self()->getRace() != Races::Zerg) ? 2 : 0;
                BWEB::Map::addReserve(tile, 4 + ff, 3 + ff);
            }
            if (piece == Piece::Wide) {
                BWEB::Map::addReserve(placement, 4, 2);
            }
            if (piece == Piece::Addon) {
                auto tile = (placement - TilePosition(1, 1));
                auto ff   = 2;
                BWEB::Map::addReserve(tile, 2 + ff, 2 + ff);
            }
        }
    }

    void initialize()
    {
        for (auto &station : Stations::getStations()) {
            addToBlockGrid(station.getBase()->Location(), station.getBase()->Location() + TilePosition(4, 3));

            if (station.isMain())
                Map::addReserve(station.getBase()->Location() - TilePosition(1, 1), 6, 5);

            for (auto &secondary : station.getSecondaryLocations()) {
                addToBlockGrid(secondary, secondary + TilePosition(4, 3));
                if (station.isMain())
                    Map::addReserve(secondary - TilePosition(1, 1), 6, 5);
            }
            for (auto &def : station.getDefenses())
                addToBlockGrid(def, def + TilePosition(2, 2));
            for (auto &mineral : station.getBase()->Minerals()) {
                auto halfway = (mineral->Pos() + station.getBase()->Center()) / 2;
                addToBlockGrid(TilePosition(halfway) - TilePosition(1, 1), TilePosition(halfway) + TilePosition(1, 1));
            }
        }
    }

    template <typename T, typename C> //
    bool findBlock(Position start, BlockType type, int width, int height, vector<TilePosition> searchTiles, T conditions, C callback)
    {
        const auto race = Broodwar->self()->getRace();
        auto tileStart  = TilePosition(start);

        const auto creepOnCorners = [&](TilePosition here, int width, int height) {
            return Broodwar->hasCreep(here) && Broodwar->hasCreep(here + TilePosition(width - 1, 0)) && Broodwar->hasCreep(here + TilePosition(0, height - 1)) &&
                   Broodwar->hasCreep(here + TilePosition(width - 1, height - 1));
        };

        for (int w = width; w > 1; w--) {
            for (int h = height; h > 1; h--) {
                for (auto tile : searchTiles) {
                    const auto blockCenter = Position(tile) + Position(w * 16, h * 16);

                    // Check if we have pieces and a layout to use
                    const auto pieces = whichPieces(w, h);
                    const auto layout = wherePieces(tile, pieces);
                    if (!tile.isValid() || pieces.empty() || layout.empty() || !conditions(tile, pieces))
                        continue;
                    if (race == Races::Zerg && !creepOnCorners(tile, w, h) && type != BlockType::Production)
                        continue;

                    multimap<TilePosition, Piece> pieceLayout = generatePieceLayout(pieces, layout);

                    if (canAddBlock(tile, w, h, pieceLayout, type)) {
                        createBlock(tile, pieceLayout, w, h, type);
                        callback(tile);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void findStartBlocks()
    {
        const auto race        = Broodwar->self()->getRace();
        const auto firstStart  = Stations::getStartingMain()->getBase()->Center();
        const auto secondStart = (Position(Stations::getStartingMain()->getChokepoint()->Center()) + Stations::getStartingMain()->getBase()->Center()) / 2;
        const auto &area       = Stations::getStartingMain()->getBase()->GetArea();

        const auto startOk = [&](TilePosition tile, const std::vector<BWEB::Piece> pieces) {
            const auto smallCount  = countPieces(pieces, Piece::Small);
            const auto mediumCount = countPieces(pieces, Piece::Medium);
            const auto largeCount  = countPieces(pieces, Piece::Large);
            const auto wideCount   = countPieces(pieces, Piece::Wide);

            // Zerg needs 5 medium, 1 small, 1 wide
            if (race == Races::Zerg) {
                if ((mediumCount + smallCount + wideCount) != 1 || (piecePerArea[area].pieces[Piece::Small] >= 1 && smallCount > 0) ||
                    (piecePerArea[area].pieces[Piece::Medium] >= 5 && mediumCount > 0) || (piecePerArea[area].pieces[Piece::Wide] >= 1 && wideCount > 0))
                    return false;
            }

            // Protoss needs ~4 medium minimum
            if (race == Races::Protoss) {
                if (largeCount < 2 || mediumCount < 1)
                    return false;
            }

            // Terran doesn't really have minimums
            if (race == Races::Terran) {
                if (largeCount < 1)
                    return false;
            }
            return true;
        };

        const auto defOk           = [&](TilePosition tile, const std::vector<BWEB::Piece> pieces) { return true; };
        const auto successCallback = [&](TilePosition tile) { return; };

        // Sort available tiles by distance to starting main
        vector<TilePosition> tilesDistStart = availableTiles;
        vector<TilePosition> tilesDistChoke = availableTiles;
        auto startingMainTile               = Stations::getStartingMain()->getBase()->Location();
        auto startingMainChokeTile          = TilePosition(Stations::getStartingMain()->getChokepoint()->Center());

        sort(tilesDistStart.begin(), tilesDistStart.end(), [&](auto &p1, auto &p2) { return p1.getDistance(startingMainTile) < p2.getDistance(startingMainTile); });
        sort(tilesDistChoke.begin(), tilesDistChoke.end(), [&](auto &p1, auto &p2) { return p1.getDistance(startingMainChokeTile) < p2.getDistance(startingMainChokeTile); });

        // P needs 2 start blocks
        // T needs 1 block
        // Z will repeat finding a block until it fails
        if (race == Races::Zerg)
            while (findBlock(firstStart, BlockType::Start, 4, 3, tilesDistStart, startOk, successCallback))
                ;
        else if (race == Races::Terran) {
            findBlock(secondStart, BlockType::Defensive, 3, 2, tilesDistChoke, defOk, successCallback);
            findBlock(secondStart, BlockType::Start, 10, 6, tilesDistStart, startOk, successCallback);
        }
        else {
            findBlock(firstStart, BlockType::Start, 8, 5, tilesDistStart, startOk, successCallback);
            findBlock(secondStart, BlockType::Start, 8, 5, tilesDistStart, startOk, successCallback);
        }
    }

    void findProductionBlocks()
    {
        const auto race = Broodwar->self()->getRace();

        const auto productionOk = [&](TilePosition tile, const std::vector<BWEB::Piece> pieces) {
            const auto smallCount  = countPieces(pieces, Piece::Small);
            const auto mediumCount = countPieces(pieces, Piece::Medium);
            const auto largeCount  = countPieces(pieces, Piece::Large);
            const auto wideCount   = countPieces(pieces, Piece::Wide);

            const auto &area = Map::mapBWEM.GetArea(tile);

            // Protoss caps large pieces in the main at 16 if we don't have necessary medium pieces
            if (race == Races::Protoss) {
                if ((largeCount > 0 && piecePerArea[area].pieces[Piece::Large] >= 16 && piecePerArea[area].pieces[Piece::Medium] < 8) ||
                    (mediumCount > 0 && piecePerArea[area].pieces[Piece::Medium] >= 12) || (smallCount > 0 && mediumCount == 0 && largeCount == 0) ||
                    (largeCount > 0 && piecePerArea[area].pieces[Piece::Large] >= 12))
                    return false;
            }

            // Zerg production placement is just solo hatcheries
            if (race == Races::Zerg) {
                if ((mediumCount + smallCount + wideCount > 0) || (piecePerArea[area].pieces[Piece::Large] + largeCount >= 6))
                    return false;
            }

            // Terran only need about 20 depot spots and 12 production spots
            if (race == Races::Terran) {
                if (mediumCount > 0 || (largeCount > 0 && piecePerArea[area].pieces[Piece::Large] >= 12))
                    return false;
            }
            return true;
        };

        const auto successCallback = [&](TilePosition tile) { return; };

        // Sort available tiles by distance to closest main
        for (auto &station : Stations::getStations()) {
            if (station.isMain() && station.getChokepoint()) {
                vector<TilePosition> tilesDistMain;
                for (auto &t : availableTiles) {
                    if (validTile(t, station))
                        tilesDistMain.push_back(t);
                }

                auto desiredCenter = TilePosition(station.getChokepoint()->Center());
                if (race == Races::Zerg) {
                    desiredCenter += station.getBase()->Location();
                    desiredCenter /= 2;
                }

                sort(tilesDistMain.begin(), tilesDistMain.end(), [&](auto &p1, auto &p2) { return p1.getDistance(desiredCenter) < p2.getDistance(desiredCenter); });

                while (findBlock(station.getBase()->Center(), BlockType::Production, 20, 20, tilesDistMain, productionOk, successCallback))
                    ;
            }
        }
    }

    void findSupplyBlocks()
    {
        auto firstSupplyTile   = TilePositions::Invalid;
        const auto race        = Broodwar->self()->getRace();
        const auto supplyWidth = (race == Races::Protoss) ? 2 : 3;

        // Zerg supply is overlords
        if (race == Races::Zerg)
            return;

        const auto supplyOk = [&](TilePosition tile, const std::vector<BWEB::Piece> pieces) {
            const auto &area = Map::mapBWEM.GetArea(tile);
            if ((supplyWidth == 2 && piecePerArea[area].pieces[Piece::Small] >= 16) || (supplyWidth == 3 && piecePerArea[area].pieces[Piece::Medium] >= 25))
                return false;
            return true;
        };

        const auto successCallback = [&](TilePosition tile) { return; };

        // Sort available tiles by distance to closest main
        for (auto &station : Stations::getStations()) {
            if (station.isMain() && station.getChokepoint()) {
                vector<TilePosition> tilesDistMain;
                for (auto &t : availableTiles) {
                    if (validTile(t, station))
                        tilesDistMain.push_back(t);
                }

                auto desiredCenter = TilePosition(station.getChokepoint()->Center());
                auto centerTile    = TilePosition(Map::mapBWEM.Center());
                sort(tilesDistMain.begin(), tilesDistMain.end(),
                     [&](auto &p1, auto &p2) { return p1.getDistance(desiredCenter) + p1.getDistance(centerTile) > p2.getDistance(desiredCenter) + p2.getDistance(centerTile); });

                while (findBlock(station.getBase()->Center(), BlockType::Supply, supplyWidth, 2, tilesDistMain, supplyOk, successCallback))
                    ;
            }
        }
    }

    void findTechBlocks()
    {
        // TODO: look at natural tiles, find something as far as possible from the choke to store 2-3 medium spots
    }

    void generateTiles()
    {
        // TODO: Store tiles by station, only allowing main/natural
        for (int x = 0; x < Broodwar->mapWidth(); x++) {
            for (int y = 0; y < Broodwar->mapHeight(); y++) {
                const TilePosition t(x, y);
                if (!Broodwar->isBuildable(t))
                    continue;
                availableTiles.push_back(t);
            }
        }
    }

    void eraseBlock(const TilePosition here)
    {
        for (auto it = allBlocks.begin(); it != allBlocks.end(); ++it) {
            auto &block = *it;
            if (here.x >= block.getTilePosition().x && here.x < block.getTilePosition().x + block.width() && here.y >= block.getTilePosition().y &&
                here.y < block.getTilePosition().y + block.height()) {
                allBlocks.erase(it);
                return;
            }
        }
    }

    void findBlocks()
    {
        initialize();

        // TODO: call these functions by stations
        generateTiles();
        findStartBlocks();
        findProductionBlocks();
        findSupplyBlocks();
        findTechBlocks();
        Pathfinding::clearCacheFully();
    }

    void draw()
    {
        for (auto &block : allBlocks)
            block.draw();
    }

    vector<Block> &getBlocks() { return allBlocks; }

    Block *getClosestBlock(TilePosition here)
    {
        auto distBest    = DBL_MAX;
        Block *bestBlock = nullptr;
        for (auto &block : allBlocks) {
            const auto tile = block.getTilePosition() + TilePosition(block.width() / 2, block.height() / 2);
            const auto dist = here.getDistance(tile);

            if (dist < distBest) {
                distBest  = dist;
                bestBlock = &block;
            }
        }
        return bestBlock;
    }
} // namespace BWEB::Blocks