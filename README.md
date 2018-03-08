# BWEB
## BWEB is currently v0.11
Broodwar Easy Builder or BWEB for short, is a BWEM based building placement addon. The purpose of this addon is to provide easily accesible building management. BWEB started as a decision to create a standard and simple method for bots to optimize their building space and placement.

## Q: What does BWEB do?
BWEB has 3 classes of information, Walls, Blocks and Stations. Walls are created by creating segments with any UnitType you want to pass into it. It attempts to find the best placement with the option to check for it being wall tight. Blocks are chunks containing building sizes. Stations contain defense locations, an averaged positions of all the resources and a pointer to the BWEM::Base.

Walls are currently only generated for the natural expansion, Blocks are created in all BWEM::Areas, and Stations are generated on every BWEM::Base.

## Q: Why use BWEB?
Building placement is a very important aspect of Broodwar. Decisions such as hiding tech, walling a choke or finding more optimal use of your space are possible using BWEB.

## Q: How do I install BWEB?
BWEB requires the use of BWEM. To install BWEB you can either: download a copy of BWEB and move the folders into your directory, or clone the git and add it to your project in Visual Studio.

## Q: How do I use BWEB?

In your main header file, you will need to include the BWEB header files.

```
#include "..\BWEB\BWEB.h"
```

There is a singleton instance accessor function that allows you to access BWEB easily, this will also go in your main header file.

```
namespace { auto & mapBWEB = BWEB::Map::Instance(); }

```

You will need to put the onStart function into your onStart event after BWEM initialization.

``` 
void McRaveModule::onStart()
  mapBWEB.onStart();
```
You will need to put onCreate, onMorph and onDestroy from BWEB into their respective events in your code as well if you wish to take advantage of how BWEB handles TilePositions that are used.

``` 
void McRaveModule::onUnitCreate(Unit unit)
  mapBWEB.onUnitCreate(unit);

void McRaveModule::onUnitDestroy(Unit unit)
  mapBWEB.onUnitDestroy(unit);
  
void McRaveModule::onUnitMorph(Unit unit)
  mapBWEB.onUnitMorph(unit);
```

(Optional) To draw the blocks, walls, and stations, you will need to put the draw function into your onFrame event.

```
void McRaveModule::onFrame()
  mapBWEB.draw();
```

All other BWEB functions have comments describing their use and what parameters are required or optional. GL HF!

If you have any questions, feel free to email me at christianmccrave@gmail.com or message me on BWAPI Discord @Fawx.
BWEB is created by Christian McCrave and Daniel Maize. 
