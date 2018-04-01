#pragma once
#include "BWEB.h"

namespace BWEB
{
	using uint = unsigned int;

	using namespace BWAPI;
	using namespace std;

	struct Node
	{
		uint G, H;
		TilePosition coordinates, direction;
		Node *parent;

		Node(TilePosition tile, Node *parent = nullptr);
		uint getScore() const;
	};

	class AStar
	{
		static Node* findNodeOnList(set<Node*>&, TilePosition);
		vector<TilePosition> direction, walls;
		uint directions;
		uint manhattan(TilePosition, TilePosition) const;

	public:
		AStar();
		vector<TilePosition> findPath(TilePosition, TilePosition, bool);
	};
}
