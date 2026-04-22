#pragma once
#include <set>
#include <BWAPI.h>
#include <bwem.h>

namespace BWEB {

    using namespace BWAPI;
    using namespace std;
    using namespace BWEB;

    class Wall
    {
        UnitType tightType;
        UnitType defenseType;
        TilePosition wallLocation;
        vector<TilePosition> smlOrder, medOrder, lrgOrder, opnOrder, pylOrder;
        set<TilePosition> smallTiles, mediumTiles, largeTiles, openings;
        map<int, set<TilePosition>> defenses;
        vector<UnitType> rawBuildings, rawDefenses;
        vector<const BWEM::Area *> accessibleNeighbors;
        const BWEM::Area * area = nullptr;
        const BWEM::ChokePoint * choke = nullptr;
        const BWEM::Base * base = nullptr;
        const BWEB::Station * station = nullptr;
        double defenseAngle;
        bool valid, pylonWall, openWall, requireTight, flatRamp, angledChoke;
        int defenseArrangement;
        TilePosition wallOffset = TilePosition(0,0);

        bool flipHorizontal = false;
        bool flipVertical   = false;

        void initialize();
        void addPieces();
        void addDefenses();
        void addDefenseLayer(int, int, int);
        void cleanup();

        void addOpenings();
        bool tryLocations(vector<TilePosition>&, set<TilePosition>&, UnitType);

    public:
        Wall(const BWEM::Area * _area, const BWEM::ChokePoint * _choke, vector<UnitType> _buildings, vector<UnitType> _defenses, UnitType _tightType, bool _requireTight, bool _openWall) {
            area                = _area;
            choke               = _choke;
            rawBuildings        = _buildings;
            rawDefenses         = _defenses;
            tightType           = _tightType;
            requireTight        = _requireTight;
            openWall            = _openWall;

            // Create Wall layout and find basic features
            initialize();
            addPieces();
            addDefenses();
            addOpenings();
            cleanup();
        }

        /// <summary> Adds a piece at the TilePosition based on the UnitType. </summary>
        void addToWallPieces(TilePosition here, UnitType building) {
            if (building.tileWidth() >= 4)
                largeTiles.insert(here);
            else if (building.tileWidth() >= 3)
                mediumTiles.insert(here);
            else if (building.tileWidth() >= 2)
                smallTiles.insert(here);
        }

        /// <summary> Returns the Station this wall is close to if one exists. </summary>
        const Station * getStation() const { return station; }

        /// <summary> Returns the Chokepoint associated with this Wall. </summary>
        const BWEM::ChokePoint * getChokePoint() const { return choke; }

        /// <summary> Returns the Area associated with this Wall. </summary>
        const BWEM::Area * getArea() const { return area; }

        /// <summary> Returns the defense locations associated with this Wall. </summary>
        const set<TilePosition>& getDefenses(int row = 0) const {
            auto ptr = defenses.find(row);
            if (ptr != defenses.end())
                return (ptr->second);
            return {};
        }

        /// <summary> Returns the TilePosition belonging to the opening of the wall. </summary>
        const set<TilePosition>& getOpenings() const { return openings; }

        /// <summary> Returns the TilePosition belonging to large UnitType buildings. </summary>
        const set<TilePosition>& getLargeTiles() const { return largeTiles; }

        /// <summary> Returns the TilePosition belonging to medium UnitType buildings. </summary>
        const set<TilePosition>& getMediumTiles() const { return mediumTiles; }

        /// <summary> Returns the TilePosition belonging to small UnitType buildings. </summary>
        const set<TilePosition>& getSmallTiles() const { return smallTiles; }

        /// <summary> Returns the raw vector of the buildings the wall was initialzied with. </summary>
        const vector<UnitType>& getRawBuildings() const { return rawBuildings; }

        /// <summary> Returns the raw vector of the defenses the wall was initialzied with. </summary>
        const vector<UnitType>& getRawDefenses() const { return rawDefenses; }

        /// <summary> Returns true if the Wall only contains Pylons. </summary>
        const bool isPylonWall() const { return pylonWall; }

        /// <summary> Returns true if an additional defense layer was planned, can only be called once.
        const bool requestAddedLayer();

        /// <summary> Returns the number of ground defenses associated with this Wall. </summary>
        const int getGroundDefenseCount() const;

        /// <summary> Returns the number of air defenses associated with this Wall. </summary>
        const int getAirDefenseCount() const;

        /// <summary> Returns the defense angle of this Wall.
        double getDefenseAngle() { return defenseAngle; }

        /// <summary> Draws all the features of the Wall. </summary>
        const void draw();
    };

    namespace Walls {

        /// <summary> Returns a map containing every Wall keyed by Chokepoint. </summary>
        map<const BWEM::ChokePoint * const, Wall>& getWalls();

        /// <summary> Returns a pointer to a Wall if it has been created at the given ChokePoint. </summary>
        /// <param name="choke"> The Chokepoint that the Wall blocks. </param>
        Wall * const getWall(const BWEM::ChokePoint *);

        /// <summary> Returns the closest Wall to the given Point. </summary>
        template<typename T>
        Wall * const getClosestWall(T h)
        {
            const auto here = Position(h);
            auto distBest = DBL_MAX;
            Wall * bestWall = nullptr;
            for (auto &[_, wall] : getWalls()) {
                const auto dist = here.getDistance(Position(wall.getChokePoint()->Center()));

                if (dist < distBest) {
                    distBest = dist;
                    bestWall = &wall;
                }
            }
            return bestWall;
        }

        /// <summary> <para> Given a vector of UnitTypes, an Area and a Chokepoint, finds an optimal wall placement, returns a valid pointer if a Wall was created. </para>
        /// <para> Note: Highly recommend that only Terran walls attempt to be walled tight, as most Protoss and Zerg wallins have gaps to allow your units through.</para>
        /// <para> BWEB makes tight walls very differently from non tight walls and will only create a tight wall if it is completely tight. </para></summary>
        /// <param name="buildings"> A Vector of UnitTypes that you want the Wall to consist of. </param>
        /// <param name="area"> The Area that you want the Wall to be contained within. </param>
        /// <param name="choke"> The Chokepoint that you want the Wall to block. </param>
        /// <param name="tight"> (Optional) Decides whether this Wall intends to be walled around a specific UnitType. </param>
        /// <param name="defenses"> (Optional) A Vector of UnitTypes that you want the Wall to have defenses consisting of. </param>
        /// <param name="openWall"> (Optional) Set as true if you want an opening in the wall for unit movement. </param>
        /// <param name="requireTight"> (Optional) Set as true if you want pixel perfect placement. </param>
        Wall * const createWall(vector<UnitType>& buildings, const BWEM::Area * area, const BWEM::ChokePoint * choke, UnitType tight = UnitTypes::None, const vector<UnitType>& defenses ={}, bool openWall = false, bool requireTight = false);

        /// I only added this to support Alchemist because it's a trash fucking map.
        Wall * const createHardWall(multimap<UnitType, TilePosition>&, const BWEM::Area * const, const BWEM::ChokePoint *);

        /// <summary><para> Creates a Forge Fast Expand at the natural. </para>
        /// <para> Places 1 Forge, 1 Gateway, 1 Pylon and Cannons. </para></summary>
        Wall * const createProtossWall();

        /// <summary><para> Creates a "Sim City" of Zerg buildings at the natural. </para>
        /// <para> Places Sunkens, 1 Evolution Chamber and 1 Hatchery. </para>
        Wall * const createZergWall();

        /// <summary><para> Creates a full wall of Terran buildings at the main choke. </para>
        /// <para> Places 2 Depots and 1 Barracks. </para>
        Wall * const createTerranWall();

        /// <summary> Calls the draw function for each Wall that exists. </summary>
        void draw();
    }
}
