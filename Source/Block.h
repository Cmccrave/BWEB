#pragma once
#include <set>
#include <BWAPI.h>

namespace BWEB {

    enum class Piece {
        Small, Medium, Large, Addon, Row
    };

    class Block
    {
        int w = 0, h = 0;
        BWAPI::TilePosition t;
        std::set <BWAPI::TilePosition> smallTiles, mediumTiles, largeTiles;
    public:
        Block() : w(0), h(0) {};
        Block(const BWAPI::TilePosition tile, std::vector<Piece> pieces) {
            t = tile;
            BWAPI::TilePosition here = tile;
            int rowHeight = 0;
            int rowWidth = 0;

            for (auto &p : pieces) {
                if (p == Piece::Small) {
                    smallTiles.insert(here);
                    here += BWAPI::TilePosition(2, 0);
                    rowWidth += 2;
                    rowHeight = std::max(rowHeight, 2);
                }
                if (p == Piece::Medium) {
                    mediumTiles.insert(here);
                    here += BWAPI::TilePosition(3, 0);
                    rowWidth += 3;
                    rowHeight = std::max(rowHeight, 2);
                }
                if (p == Piece::Large) {
                    largeTiles.insert(here);
                    here += BWAPI::TilePosition(4, 0);
                    rowWidth += 4;
                    rowHeight = std::max(rowHeight, 3);
                }
                if (p == Piece::Addon) {
                    smallTiles.insert(here + BWAPI::TilePosition(0, 1));
                    here += BWAPI::TilePosition(2, 0);
                    rowWidth += 2;
                    rowHeight = std::max(rowHeight, 2);
                }
                if (p == Piece::Row) {                    
                    w = std::max(w, rowWidth);
                    h += rowHeight;
                    rowWidth = 0;
                    rowHeight = 0;
                    here = tile + BWAPI::TilePosition(0, h);
                }
            }

            // In case there is no row piece
            w = std::max(w, rowWidth);
            h += rowHeight;
        }

        int width() const { return w; }
        int height() const { return h; }

        /// Returns the top left tile position of this block
        BWAPI::TilePosition getTilePosition() const { return t; }

        /// Returns the const set of tilepositions that belong to 2x2 (small) buildings
        std::set<BWAPI::TilePosition> getSmallTiles() const { return smallTiles; }

        /// Returns the const set of tilepositions that belong to 3x2 (medium) buildings
        std::set<BWAPI::TilePosition> getMediumTiles() const { return mediumTiles; }

        /// Returns the const set of tilepositions that belong to 4x3 (large) buildings
        std::set<BWAPI::TilePosition> getLargeTiles() const { return largeTiles; }

        void insertSmall(const BWAPI::TilePosition here) { smallTiles.insert(here); }
        void insertMedium(const BWAPI::TilePosition here) { mediumTiles.insert(here); }
        void insertLarge(const BWAPI::TilePosition here) { largeTiles.insert(here); }
    };

    namespace Blocks {

        /// <summary> Initializes the building of every BWEB::Block on the map, call it only once per game. </summary>
        void findBlocks();

        /// <summary> Draws all BWEB Blocks. </summary>
        void draw();

        /// <summary> Erases any blocks at the specified TilePosition. </summary>
        /// <param name="here"> The TilePosition that you want to delete any BWEB::Block that exists here. </param>
        void eraseBlock(BWAPI::TilePosition here);

        /// <summary> Returns a vector containing every Block </summary>
        std::vector<Block>& getBlocks();

        /// <summary> Returns the closest BWEB::Block to the given TilePosition. </summary>
        Block* getClosestBlock(BWAPI::TilePosition);
    }
}
