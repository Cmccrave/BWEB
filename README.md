# BWEB
## BWEB is currently v0.9
Broodwar Easy Builder or BWEB for short, is a BWEM based building placement addon. The purpose of this addon is to provide easily accesible building management. BWEB started as a decision to create a standard and simple method for bots to optimize their building space and placement.

## Q: What does BWEB do?
BWEB has 3 classes of information, Walls, Blocks and Stations. Walls are currently only generated for the natural expansion, blocks are limited to the main area, stations are generated on every BWEM Base location. Blocks are used for all building types and are modifiable for any race and build. Stations include defense positions that provide coverage for all your workers. Walls provide wall in placements that defend against early rushes or allow safe fast expands.

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

You will need to put the onStart function into your onStart event. To draw the blocks, you will need to put the draw function into your onFrame event.

``` 
void McRaveModule::onStart()
{
  mapBWEB.onStart();
}

void McRaveModule::onFrame()
{
  mapBWEB.draw();
}
```
All other BWEB functions have comments describing their use and what parameters are required or optional. GL HF!

If you have any questions, feel free to email me at christianmccrave@gmail.com or message me on BWAPI Discord @Fawx.
BWEB is created by Christian McCrave and Daniel Maize. 
