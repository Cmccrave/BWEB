#include "BWEB.h"

using namespace std;
using namespace BWAPI;

namespace BWEB {

    void Block::draw()
    {
        int color = Broodwar->self()->getColor();
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
    }
}

namespace BWEB::Blocks
{
    namespace {
        vector<Block> allBlocks;
        map<const BWEM::Area *, int> typePerArea;
        map<Piece, int> mainPieces;

        int countPieces(vector<Piece> pieces, Piece type)
        {
            auto count = 0;
            for (auto &piece : pieces) {
                if (piece == type)
                    count++;
            }
            return count;
        }

        vector<Piece> whichPieces(int width, int height, bool faceUp = false, bool faceLeft = false)
        {
            vector<Piece> pieces;

            // Zerg Block pieces
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (height == 2) {
                    if (width == 2)
                        pieces ={ Piece::Small };
                    if (width == 3)
                        pieces ={ Piece::Medium };
                    if (width == 5)
                        pieces ={ Piece::Small, Piece::Medium };
                }
                else if (height == 3) {
                    if (width == 4)
                        pieces ={ Piece::Large };
                }
                else if (height == 4) {
                    if (width == 3)
                        pieces ={ Piece::Medium, Piece::Row, Piece::Medium };
                    if (width == 5)
                        pieces ={ Piece::Small, Piece::Medium, Piece::Row, Piece::Small, Piece::Medium };
                }
                else if (height == 6) {
                    if (width == 5)
                        pieces ={ Piece::Small, Piece::Medium, Piece::Row, Piece::Medium, Piece::Small, Piece::Row, Piece::Small, Piece::Medium };
                }
            }

            // Protoss Block pieces
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (height == 2) {
                    if (width == 5)
                        pieces ={ Piece::Small, Piece::Medium };
                }
                else if (height == 4) {
                    if (width == 5)
                        pieces ={ Piece::Small, Piece::Medium, Piece::Row, Piece::Small, Piece::Medium };
                }
                else if (height == 5) {
                    if (width == 4)
                        pieces ={ Piece::Large, Piece::Row, Piece::Small, Piece::Small };
                    if (width == 8) {
                        if (faceLeft) {
                            if (faceUp)
                                pieces ={ Piece::Large, Piece::Large, Piece::Row, Piece::Medium, Piece::Medium, Piece::Small };
                            else
                                pieces ={ Piece::Medium, Piece::Medium, Piece::Small, Piece::Row, Piece::Large, Piece::Large };
                        }
                        else {
                            if (faceUp)
                                pieces ={ Piece::Large, Piece::Large, Piece::Row, Piece::Small, Piece::Medium, Piece::Medium };
                            else
                                pieces ={ Piece::Small, Piece::Medium, Piece::Medium, Piece::Row, Piece::Large, Piece::Large };
                        }
                    }
                }
                else if (height == 6) {
                    if (width == 10)
                        pieces ={ Piece::Large, Piece::Addon, Piece::Large, Piece::Row, Piece::Large, Piece::Small, Piece::Large };
                    if (width == 18)
                        pieces ={ Piece::Large, Piece::Large, Piece::Addon, Piece::Large, Piece::Large, Piece::Row, Piece::Large, Piece::Large, Piece::Small, Piece::Large, Piece::Large };
                }
                else if (height == 8) {
                    if (width == 8)
                        pieces ={ Piece::Large, Piece::Large, Piece::Row, Piece::Small, Piece::Small, Piece::Small, Piece::Small, Piece::Row, Piece::Large, Piece::Large };
                    if (width == 5)
                        pieces ={ Piece::Large, Piece::Row, Piece::Small, Piece::Medium, Piece::Row, Piece::Large };
                }
            }

            // Terran Block pieces
            if (Broodwar->self()->getRace() == Races::Terran) {
                if (height == 2) {
                    if (width == 3)
                        pieces ={ Piece::Medium };
                    if (width == 6)
                        pieces ={ Piece::Medium, Piece::Medium };
                }
                else if (height == 4) {
                    if (width == 3)
                        pieces ={ Piece::Medium, Piece::Row, Piece::Medium };
                }
                else if (height == 6) {
                    if (width == 3)
                        pieces ={ Piece::Medium, Piece::Row, Piece::Medium, Piece::Row, Piece::Medium };
                }
                else if (height == 3) {
                    if (width == 6)
                        pieces ={ Piece::Large, Piece::Addon };
                }
                else if (height == 4) {
                    if (width == 6)
                        pieces ={ Piece::Medium, Piece::Medium, Piece::Row, Piece::Medium, Piece::Medium };
                    if (width == 9)
                        pieces ={ Piece::Medium, Piece::Medium, Piece::Medium, Piece::Row, Piece::Medium, Piece::Medium, Piece::Medium };
                }
                else if (height == 5) {
                    if (width == 6)
                        pieces ={ Piece::Large, Piece::Addon, Piece::Row, Piece::Medium, Piece::Medium };
                }
                else if (height == 6) {
                    if (width == 6)
                        pieces ={ Piece::Large, Piece::Addon, Piece::Row, Piece::Large, Piece::Addon };
                }
            }
            return pieces;
        }

        bool canAddBlock(const TilePosition here, const int width, const int height)
        {
            // Check if a block of specified size would overlap any bases, resources or other blocks
            for (auto x = here.x - 1; x < here.x + width + 1; x++) {
                for (auto y = here.y - 1; y < here.y + height + 1; y++) {
                    const TilePosition t(x, y);
                    if (!t.isValid() || !Map::mapBWEM.GetTile(t).Buildable() || Map::isReserved(t))
                        return false;
                }
            }
            return true;
        }

        bool canAddProxyBlock(const TilePosition here, const int width, const int height)
        {
            // Check if a proxy block of specified size is not buildable here
            for (auto x = here.x - 1; x < here.x + width + 1; x++) {
                for (auto y = here.y - 1; y < here.y + height + 1; y++) {
                    const TilePosition t(x, y);
                    if (!t.isValid() || !Map::mapBWEM.GetTile(t).Buildable() || !Broodwar->isWalkable(WalkPosition(t)))
                        return false;
                }
            }
            return true;
        }

        void insertBlock(TilePosition here, vector<Piece> pieces)
        {
            Block newBlock(here, pieces);
            allBlocks.push_back(newBlock);
            Map::addReserve(here, newBlock.width(), newBlock.height());
        }

        void insertProxyBlock(TilePosition here, vector<Piece> pieces)
        {
            Block newBlock(here, pieces, true);
            allBlocks.push_back(newBlock);
            Map::addReserve(here, newBlock.width(), newBlock.height());
        }

        void insertDefensiveBlock(TilePosition here, vector<Piece> pieces)
        {
            Block newBlock(here, pieces, false, true);
            allBlocks.push_back(newBlock);
            Map::addReserve(here, newBlock.width(), newBlock.height());
        }

        void findMainStartBlocks()
        {
            const auto race = Broodwar->self()->getRace();
            const auto firstStart = Map::getMainPosition();
            const auto secondStart = race != Races::Zerg ? (Position(Map::getMainChoke()->Center()) + Map::getMainPosition()) / 2 : Map::getMainPosition();

            const auto creepOnCorners = [&](TilePosition here, int width, int height) {
                return Broodwar->hasCreep(here) && Broodwar->hasCreep(here + TilePosition(width - 1, 0)) && Broodwar->hasCreep(here + TilePosition(0, height - 1)) && Broodwar->hasCreep(here + TilePosition(width - 1, height - 1));
            };

            const auto searchStart = [&](Position start) {
                auto tileStart = TilePosition(start);
                auto tileBest = TilePositions::Invalid;
                auto distBest = DBL_MAX;
                vector<Piece> piecesBest;

                for (int i = 10; i > 0; i--) {
                    for (int j = 10; j > 0; j--) {

                        // Try to find a block near our starting location
                        for (auto x = tileStart.x - 15; x <= tileStart.x + 15; x++) {
                            for (auto y = tileStart.y - 15; y <= tileStart.y + 15; y++) {
                                const TilePosition tile(x, y);

                                const auto blockCenter = Position(tile) + Position(i * 16, j * 16);
                                const auto dist = blockCenter.getDistance(start);
                                const auto blockFacesLeft = (blockCenter.x < Map::getMainPosition().x);
                                const auto blockFacesUp = (blockCenter.y < Map::getMainPosition().y);

                                // Check if we have pieces to use
                                const auto pieces = whichPieces(i, j, blockFacesUp, blockFacesLeft);
                                if (pieces.empty())
                                    continue;

                                // Check if we have creep as Zerg
                                if (race == Races::Zerg && !creepOnCorners(tile, i, j))
                                    continue;

                                const auto smallCount = countPieces(pieces, Piece::Small);
                                const auto mediumCount = countPieces(pieces, Piece::Medium);
                                const auto largeCount = countPieces(pieces, Piece::Large);

                                if (!tile.isValid()
                                    || mediumCount < 1
                                    || (race == Races::Zerg && smallCount == 0 && mediumCount == 0)
                                    || (race == Races::Protoss && largeCount < 2)
                                    || (race == Races::Terran && largeCount < 1))
                                    continue;

                                if (dist < distBest && canAddBlock(tile, i, j)) {
                                    piecesBest = pieces;
                                    distBest = dist;
                                    tileBest = tile;
                                }
                            }
                        }

                        if (tileBest.isValid() && canAddBlock(tileBest, i, j)) {
                            if (Map::mapBWEM.GetArea(tileBest) == Map::getMainArea()) {
                                for (auto &piece : piecesBest)
                                    mainPieces[piece]++;
                            }
                            insertBlock(tileBest, piecesBest);
                        }
                    }
                }
            };

            searchStart(firstStart);
            searchStart(secondStart);
        }

        void findMainDefenseBlock()
        {
            if (Broodwar->self()->getRace() == Races::Zerg)
                return;

            // Added a block that allows a good shield battery placement or bunker placement
            auto tileBest = TilePositions::Invalid;
            auto start = TilePosition(Map::getMainChoke()->Center());
            auto distBest = DBL_MAX;
            for (auto x = start.x - 12; x <= start.x + 16; x++) {
                for (auto y = start.y - 12; y <= start.y + 16; y++) {
                    const TilePosition tile(x, y);
                    const auto blockCenter = Position(tile) + Position(80, 32);
                    const auto dist = (blockCenter.getDistance((Position)Map::getMainChoke()->Center()));

                    if (!tile.isValid()
                        || Map::mapBWEM.GetArea(tile) != Map::getMainArea()
                        || dist < 96.0)
                        continue;

                    if (dist < distBest && canAddBlock(tile, 5, 2)) {
                        tileBest = tile;
                        distBest = dist;
                    }
                }
            }

            if (tileBest.isValid())
                insertDefensiveBlock(tileBest, { Piece::Small, Piece::Medium });
        }

        void findProductionBlocks()
        {
            multimap<double, TilePosition> tilesByPathDist;
            int totalMedium = 0;
            int totalLarge = 0;

            // Calculate distance for each tile to our natural choke, we want to place bigger blocks closer to the chokes
            for (int y = 0; y < Broodwar->mapHeight(); y++) {
                for (int x = 0; x < Broodwar->mapWidth(); x++) {
                    const TilePosition t(x, y);
                    if (t.isValid() && Broodwar->isBuildable(t)) {
                        const auto p = Position(x * 32, y * 32);
                        const auto dist = (Map::getNaturalChoke() && Broodwar->self()->getRace() != Races::Zerg) ? p.getDistance(Position(Map::getNaturalChoke()->Center())) : p.getDistance(Map::getMainPosition());
                        tilesByPathDist.insert(make_pair(dist, t));
                    }
                }
            }

            // Iterate every tile
            for (int i = 20; i > 0; i--) {
                for (int j = 20; j > 0; j--) {

                    // Check if we have pieces to use
                    const auto pieces = whichPieces(i, j);
                    if (pieces.empty())
                        continue;

                    const auto smallCount = countPieces(pieces, Piece::Small);
                    const auto mediumCount = countPieces(pieces, Piece::Medium);
                    const auto largeCount = countPieces(pieces, Piece::Large);

                    for (auto &[_, tile] : tilesByPathDist) {

                        // Protoss caps large pieces in the main at 12 if we don't have necessary medium pieces
                        if (Broodwar->self()->getRace() == Races::Protoss) {
                            if (largeCount > 0 && Map::mapBWEM.GetArea(tile) == Map::getMainArea() && mainPieces[Piece::Large] >= 12 && mainPieces[Piece::Medium] < 10)
                                continue;
                        }

                        // Zerg only need 4 medium pieces and 2 small piece
                        if (Broodwar->self()->getRace() == Races::Zerg) {
                            if ((mediumCount > 0 && mainPieces[Piece::Medium] >= 4)
                                || (smallCount > 0 && mainPieces[Piece::Small] >= 2))
                                continue;
                        }

                        // Terran only need about 20 depot spots
                        if (Broodwar->self()->getRace() == Races::Terran) {
                            if (mediumCount > 0 && mainPieces[Piece::Medium] >= 20)
                                continue;
                        }

                        if (canAddBlock(tile, i, j)) {
                            insertBlock(tile, pieces);

                            totalMedium += mediumCount;
                            totalLarge += largeCount;

                            if (Map::mapBWEM.GetArea(tile) == Map::getMainArea()) {
                                for (auto &piece : pieces)
                                    mainPieces[piece]++;
                            }
                        }
                    }
                }
            }
        }

        void findProxyBlock()
        {
            // For base-specific locations, avoid all areas likely to be traversed by worker scouts
            set<const BWEM::Area*> areasToAvoid;
            for (auto &first : Map::mapBWEM.StartingLocations()) {
                for (auto &second : Map::mapBWEM.StartingLocations()) {
                    if (first == second)
                        continue;

                    for (auto &choke : Map::mapBWEM.GetPath(Position(first), Position(second))) {
                        areasToAvoid.insert(choke->GetAreas().first);
                        areasToAvoid.insert(choke->GetAreas().second);
                    }
                }

                // Also add any areas that neighbour each start location
                auto baseArea = Map::mapBWEM.GetNearestArea(first);
                for (auto &area : baseArea->AccessibleNeighbours())
                    areasToAvoid.insert(area);
            }

            // Gather the possible enemy start locations
            vector<TilePosition> enemyStartLocations;
            for (auto &start : Map::mapBWEM.StartingLocations()) {
                if (Map::mapBWEM.GetArea(start) != Map::getMainArea())
                    enemyStartLocations.push_back(start);
            }

            // Check if this block is in a good area
            const auto goodArea = [&](TilePosition t) {
                for (auto &start : enemyStartLocations) {
                    if (Map::mapBWEM.GetArea(t) == Map::mapBWEM.GetArea(start))
                        return false;
                }
                for (auto &area : areasToAvoid) {
                    if (Map::mapBWEM.GetArea(t) == area)
                        return false;
                }
                return true;
            };

            // Check if there's a blocking neutral between the positions to prevent bad pathing
            const auto blockedPath = [&](Position source, Position target) {
                for (auto &choke : Map::mapBWEM.GetPath(source, target)) {
                    if (Map::isUsed(TilePosition(choke->Center())) != UnitTypes::None)
                        return true;
                }
                return false;
            };

            // Find the best locations
            TilePosition tileBest = TilePositions::Invalid;
            auto distBest = DBL_MAX;
            for (int x = 0; x < Broodwar->mapWidth(); x++) {
                for (int y = 0; y < Broodwar->mapHeight(); y++) {
                    const TilePosition topLeft(x, y);
                    const TilePosition botRight(x + 8, y + 5);

                    if (!topLeft.isValid()
                        || !botRight.isValid()
                        || !canAddProxyBlock(topLeft, 8, 5))
                        continue;

                    const Position blockCenter = Position(topLeft) + Position(160, 96);

                    // Consider each start location
                    auto dist = 0.0;
                    for (auto &base : enemyStartLocations) {
                        const auto baseCenter = Position(base) + Position(64, 48);
                        dist += Map::getGroundDistance(blockCenter, baseCenter);
                        if (blockedPath(blockCenter, baseCenter)) {
                            dist = DBL_MAX;
                            break;
                        }
                    }

                    // Bonus for placing in a good area
                    if (goodArea(topLeft) && goodArea(botRight))
                        dist = log(dist);

                    if (dist < distBest) {
                        distBest = dist;
                        tileBest = topLeft;
                    }
                }
            }

            // Add the blocks
            if (canAddProxyBlock(tileBest, 8, 5))
                insertProxyBlock(tileBest, { Piece::Large, Piece::Large, Piece::Row, Piece::Small, Piece::Small, Piece::Small, Piece::Small });
        }
    }

    void eraseBlock(const TilePosition here)
    {
        for (auto &it = allBlocks.begin(); it != allBlocks.end(); ++it) {
            auto &block = *it;
            if (here.x >= block.getTilePosition().x && here.x < block.getTilePosition().x + block.width() && here.y >= block.getTilePosition().y && here.y < block.getTilePosition().y + block.height()) {
                allBlocks.erase(it);
                return;
            }
        }
    }

    void findBlocks()
    {
        findMainDefenseBlock();
        findMainStartBlocks();
        findProxyBlock();
        findProductionBlocks();
    }

    void draw()
    {
        for (auto &block : allBlocks)
            block.draw();
    }

    vector<Block>& getBlocks() {
        return allBlocks;
    }

    Block * getClosestBlock(TilePosition here)
    {
        auto distBest = DBL_MAX;
        Block * bestBlock = nullptr;
        for (auto &block : allBlocks) {
            const auto tile = block.getTilePosition() + TilePosition(block.width() / 2, block.height() / 2);
            const auto dist = here.getDistance(tile);

            if (dist < distBest) {
                distBest = dist;
                bestBlock = &block;
            }
        }
        return bestBlock;
    }
}