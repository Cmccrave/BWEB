#include "AStar.h"

using namespace std::placeholders;

namespace BWEB
{
	Node::Node(TilePosition tile, Node *newParent)
	{
		parent = newParent;
		coordinates = tile;
		G = H = 0;
	}

	uint Node::getScore()
	{
		return G + H;
	}

	AStar::AStar()
	{
		direction =
		{
			{ 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 },
			{ -1, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 }
		};
	}

	vector<TilePosition> AStar::findPath(TilePosition source, TilePosition target, bool diagonal)
	{
		Node *current = nullptr;
		set<Node*> openSet, closedSet;
		openSet.insert(new Node(source));
		directions = diagonal ? 8 : 4;

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
			openSet.erase(std::find(openSet.begin(), openSet.end(), current));

			for (uint i = 0; i < directions; ++i)
			{
				TilePosition tile(current->coordinates + direction[i]);

				// Detection collision or skip tiles already added to closed set
				if (!tile.isValid() || BWEB::BWEBUtil().overlapsBlocks(tile) || BWEB::BWEBUtil().overlapsStations(tile) || BWEB::BWEBUtil().overlapsNeutrals(tile) || BWEB::BWEBUtil().overlapsWalls(tile) || !BWEB::BWEBUtil().isWalkable(tile) || findNodeOnList(closedSet, tile)) continue;

				// Cost function?
				uint totalCost = current->G + ((i < 4) ? 10 : 14) + (500 * (current->direction != direction[i])) + (BWEM::Map::Instance().GetTile(tile).MinAltitude());

				//if (BWEM::Map::Instance().GetArea(tile) != BWEB::Map::Instance().getMainArea() && BWEM::Map::Instance().GetArea(tile) != BWEB::Map::Instance().getNaturalArea())
				//	totalCost = current->G + ((i < 4) ? 10 : 14) + (3 * (current->direction == direction[i]));

				// Checks if the node has been made already, if not it creates one
				Node *successor = findNodeOnList(openSet, tile);
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
		while (current != nullptr)
		{
			path.push_back(current->coordinates);
			current = current->parent;
		}

		openSet.clear();
		closedSet.clear();
		return path;
	}

	Node* AStar::findNodeOnList(set<Node*>& nodes_, TilePosition coordinates_)
	{
		for (auto node : nodes_) {
			if (node->coordinates == coordinates_) {
				return node;
			}
		}
		return nullptr;
	}

	uint AStar::manhattan(TilePosition source, TilePosition target)
	{
		return abs(source.x - target.x) + abs(source.y - target.y);
	}
}