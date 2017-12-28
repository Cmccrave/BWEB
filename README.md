# BWEB
## BWEB is currently v0.7
Broodwar Easy Builder or BWEB for short, is a BWEM based building placement addon that creates blocks of buildings for simple placement of all building types. BWEB started as a choice to switch to a standardized approach to placing buildings in a consistent method that allows for optimizing the space you have, something that Broodwar bots struggle with.

## Q: What does BWEB do?
BWEB currently creates blocks of building positions within your main that are accessible through the BWEB instance created on the start of the game.

## Q: Why use BWEB?
Building placement is a very important aspect of Broodwar. Decisions such as hiding tech, walling a choke or finding more optimal use of your space are possible using BWEB.

## Q: How do I install BWEB?
BWEB requires the use of BWEM. To install BWEB you can either: download a copy of BWEB and move the folders into your directory, or clone the git and add it to your project in Visual Studio.

## Q: How do I use BWEB?
You will need to put the onStart function into your onStart event. To draw the blocks, you will need to put the draw function into your onFrame event.

``` 
void McRaveModule::onStart()
{
  theBuilder.onStart();
}

void McRaveModule::onFrame()
{
  theBuilder.draw();
}
```
Once BWEB is initialized, you can grab positions for buildings based on their size and command your workers to build on that tile.

If you have any questions, feel free to email me at christianmccrave@gmail.com or message me on BWAPI Discord @Fawx.
BWEB is created by Christian McCrave and Daniel Maize. 
