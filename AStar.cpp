#include "AStar.h"

using namespace std::placeholders;

namespace BWEB
{
	Node::Node(const TilePosition tile, Node *newParent)
	{
		parent = newParent;
		coordinates = tile;
		G = H = 0;
	}

	uint Node::getScore() const
	{
		return G + H;
	}

	AStar::AStar() : directions(0)
	{
		direction =
		{
			{0, 1}, {1, 0}, {0, -1}, {-1, 0},
			{-1, -1}, {1, 1}, {-1, 1}, {1, -1}
		};
	}

	vector<TilePosition> AStar::findPath(const TilePosition source, const TilePosition target, bool walling)
	{
		Node *current = nullptr;
		set<Node*> openSet, closedSet;
		openSet.insert(new Node(source));
		directions = 4;

		while (!openSet.empty())
		{
			current = *openSet.begin();
			for (auto node : openSet)
			{
				if (node->getScore() <= current->getScore())
					current = node;
			}

			if (current->coordinates == target) break;

			closedSet.insert(current);
			openSet.erase(find(openSet.begin(), openSet.end(), current));

			for (uint i = 0; i < directions; ++i)
			{
				auto tile(current->coordinates + direction[i]);

				// Detection collision or skip tiles already added to closed set
				if (!tile.isValid() || BWEB::Map::Instance().overlapGrid[tile.x][tile.y] > 0 || !BWEB::Map::isWalkable(tile) || findNodeOnList(closedSet, tile)) continue;
				if (BWEB::Map::Instance().overlapsCurrentWall(tile) != UnitTypes::None) continue;
				if (BWEM::Map::Instance().GetArea(tile) && BWEM::Map::Instance().GetArea(tile) != BWEM::Map::Instance().GetArea(source) && BWEM::Map::Instance().GetArea(tile) != BWEM::Map::Instance().GetArea(target)) continue;

				// Cost function?
				const auto totalCost = current->G + ((i < 4) ? 10 : 14);

				// Checks if the node has been made already, if not it creates one
				auto successor = findNodeOnList(openSet, tile);
				if (successor == nullptr)
				{
					successor = new Node(tile, current);
					successor->G = totalCost;
					successor->H = manhattan(successor->coordinates, target);
					successor->direction = direction[i];
					openSet.insert(successor);
				}
				// If the node exists, update its cost and parent node
				else if (totalCost < successor->G)
				{
					successor->parent = current;
					successor->G = totalCost;
				}
			}
		}

		vector<TilePosition> path;
		if (current->coordinates != target)
			return path;

		while (current != nullptr)
		{
			path.push_back(current->coordinates);
			current = current->parent;
		}

		openSet.clear();
		closedSet.clear();
		return path;
	}

	Node* AStar::findNodeOnList(set<Node*>& nodes, const TilePosition coordinates)
	{
		for (auto node : nodes) {
			if (node->coordinates == coordinates) {
				return node;
			}
		}
		return nullptr;
	}

	uint AStar::manhattan(const TilePosition source, const TilePosition target) const
	{
		return abs(source.x - target.x) + abs(source.y - target.y);
	}
}