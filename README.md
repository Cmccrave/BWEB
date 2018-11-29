# BWEB
### BWEB is currently v1.09
Broodwar Easy Builder or BWEB for short, is a BWEM based building placement addon. The purpose of this addon is to provide easily accesible building management. BWEB started as a decision to create a standard and simple method for bots to optimize their building space and placement.

### What does BWEB do?
BWEB has 3 classes of information, Walls, Blocks and Stations.

Walls are made by permutating through a vector of UnitTypes in as many combinations as possible and measuring a BFS path length to find a wall that is either wall tight or minimizes enemy movement. Walls can be created using any `BWAPI::UnitType` and have optional parameters to pass in; what `BWAPI::UnitType` you want to be wall tight against, a vector of `BWAPI::UnitType` for defenses, or a reserve path can be created to ensure your units can leave the `BWEM::Area` that the Wall is being created in.

Blocks are used for all building types and are modifiable for any race and build. Blocks are useful for maximizing your space for production buildings without trapping units. 

Stations are placed on every `BWEM::Base` and include defense positions that provide coverage for all your workers.

### Why use BWEB?
Building placement is a very important aspect of Broodwar. Decisions such as hiding tech, walling a choke or finding more optimal use of your space are possible using BWEB. Most Broodwar bots suffer from many issues stemming from building placement, such as; timeouts, building where it's not safe, trapping units, and lack of fast expand options due to poor wall placement.

### How do I install BWEB?
1) Clone the repository or download a copy.
2) In your source code, create a BWEB folder and store the files in there.
3) In Visual Studio, add the files to your project and edit the properties of your projects include directory to include BWEBs folder.

### How do I use BWEB?

In any file in which you need to access BWEB, add the following include:
`#include "BWEB.h"`

BWEB is accessed through the BWEB namespace. Map, Stations, Walls, Blocks and PathFinding is all accessed through their respective namespace inside the BWEB namespace.

- `Map` contains useful functions such as quick ground distance calculations, natural and main choke identifying and tracking of used tiles for buildings.
- `Stations` contains placements for `BWEM::Base`s, tracking the defense count and placements around them.
- `Walls` contains the ability to create walls at any `BWEM::Chokepoint` and `BWEM::Area`. These walls currently store the location of the `BWAPI::UnitType::TileSize` of each segment of the created wall, defense placements and an optional opening called a door.
- `Blocks` contains placements for `BWAPI::UnitType`s for a fairly optimized approach to placing production buildings. Blocks are placed with varying sizes and shapes to try and fit as many buildings into each `BWEM::Area` as possible.
- `PathFinding` provides the ability to take advantage of BWEBs blazing fast JPS to create paths.

BWEB needs to be initialized after BWEM is initialized. I would suggest including the following BWEB functions in your code:
`BWEB::onStart()`
`BWEB::Blocks::findBlocks()`

All other BWEB functions have full comments describing their use and what parameters are required or optional. GL HF!
If you have any questions, feel free to ask on BWAPI Discord.

## Changelog
1.09
- Added JPS pathfinding for unit paths. 
- Fixed a bug in returning a copy of a vector.

1.08
- Restructured BWEB, removed singleton pattern.

1.07 
- Improved Terran wall placement
- Recoded wall tight function

1.06 
- Improved wall placement, should function better on every map.
- Added a unit pathing option (may move elsewhere later).
- Small bugfixes.

1.05 
- Minor bugfixes, added Path class to setup for unit pathing.

1.04
- Paths are made using BFS instead of A*. 
- Improved performance and no memory leaks.
- Placement on all buildings improved slightly.
- Door placement improved drastically.
- Defense placement moved further back from wall when possible.
- Pylons are as central as possible.

1.03
- General improvements across the board.

1.02
- Adjusted wall placements.
- Improved block generation speed and maximizing of space.

1.01
- Minor bug fixes, some slight wall improvements

