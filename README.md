# BWEB
## BWEB is currently v0.8
Broodwar Easy Builder or BWEB for short, is a BWEM based building placement addon that creates blocks of buildings for simple placement of all building types. BWEB started as a choice to switch to a standardized approach to placing buildings in a consistent method that allows for optimizing the space you have, something that Broodwar bots struggle with.

## Q: What does BWEB do?
BWEB currently creates blocks of building positions within your main that are accessible through the BWEB instance created on the start of the game.

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
namespace { auto & BWEB = BWEBClass::Instance(); }

```

You will need to put the onStart function into your onStart event. To draw the blocks, you will need to put the draw function into your onFrame event.

``` 
void McRaveModule::onStart()
{
  BWEB.onStart();
}

void McRaveModule::onFrame()
{
  BWEB.draw();
}
```
All other BWEB functions have comments describing their use and what parameters are required or optional. GL HF!

If you have any questions, feel free to email me at christianmccrave@gmail.com or message me on BWAPI Discord @Fawx.
BWEB is created by Christian McCrave and Daniel Maize. 
