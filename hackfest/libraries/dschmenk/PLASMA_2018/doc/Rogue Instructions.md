# PLASMA goes ROGUE

## Introduction

This version of ROGUE is somewhat different than others. It is very simple in most ways, but I have developed a (I think) unique visibility algorithm that runs extremely fast. Fast enough to run interpreted by the PLASMA VM on a 1 MHz 6502, and space efficient enough to allow for large (in the future) dungeons. The unique feature of this ROGUE is that lighting becomes critical and strategic. You are in dark catacombs, after all. You enter with a lit lamp, throwing off a circle of light. There are also torches throughout the catacombs that light up a small surrounding circle of light. Other items in the catacombs are mana (health+energy increase), a key, a raft, and gold. You will also encounter a number of enemies that will track you down to try and kill you. You will also encounter doors, locked doors, windows, water, and crevasses.

## Strategy

As you travel through the catacombs, you must watch your health, energy, and lamp oil levels. Once health reaches zero, you are dead. As energy reaches zero, your vision will narrow and you will no longer be able to run. When the lamp oil runs out, you will be cast into darkness. If you see any torches in your field of vision, you can navigate to them. Taking the torch will extinguish the torch and replenish some of your lamp oil. Note that as you travel through the catacombs, your map of what you have seen will automatically fill in. But, if you are in the dark, you cannot read your map. You must turn on your lamp or get next to a torch before you can read the map again. If you are in the dark and can’t see any torches in your field of vision, you are in complete darkness. It is easy to lose your bearings. As such, the absolute direction movements no longer work (NSEW) - you will end up in a random direction if you try. However, the relative turns, left/right and forward/backward controls continue to work (that you can do in the dark).

Being in the dark can be advantageous, however. All the enemies in the catacombs can see you if you are in light, just as you can see them. If you are in darkness, they can't see you, and you can move around without being tracked. Don't run into them! Also, don't fall off a crevasse. You will hear certain noises giving you feedback on what is going on. A simple beep when you run into walls. A groan when an enemy moves towards you. A bleep when you pick an item up. Other noises when you fall over an edge or win a battle. These can be used strategically when moving in the dark.

Health will slowly improve as you move around. However, energy is depleted as you move. Mana will increase both health and energy. If health is already at 100, it won’t go any higher. Same for energy, but it is important to keep both high. When energy goes low, you can no longer move quickly and your field-of-vision narrows. When health goes to zero, you are dead.

## Tile Description

As ROGUE uses the text screen for display, a little creativity is required to interpret the map. These are the characters you will see and what the represent. Once you get the hang of it, it will be just like looking at the unencoded Matrix.
```
Screen Character      Represents
   #                    Wall
   .                    Floor
   :                    Window (barred opening)
   +                    Door
   %                    Locked Door (need key to open)
   ' ' space            Crevasse (pit - don't fall in)
   =                    Exit
   -                    Entrance
   *                    Torch
   &                    Mana
   ,                    Key (yep, hard to spot)
   @                    Raft (need to cross water)
   <<<                  Water
   >>>                  Water (you will drown without raft)
   $                    Gold

Flashing               Entity
   T                    Thief
   O                    Ogre
   Z                    Zombie
   R                    Rogue

 Player               Facing Direction
   ^                    North
   \                    NE
   >                    East
   /                    SE
   v                    South
   \                    SW
   <                    West
   /                    NW
```
Tiles in light are inverse. Entities are displayed only when lit and in field of view. The map is only visible when lit, i.e lamp is on or standing next to a torch.

## Interaction
```
Keyboard commands        Action
   Q                    Run (Quick)
   W up-arrow           Forward
   S down-arrow         Backward
   A left-arrow         Turn left
   D right-arrow        Turn right
   I                    Move N
   J                    Move W
   K                    Move E
   M                    Move S
   < ,                  Turn lamp down
   > .                  Turn lamp up/on
   O                    Turn lamp off
   Space-bar            Open door
   Return               Pick up item
   X                    Exit (die)
```
Whenever you and an enemy end up on the same tile, battle commences. As you win fights, your skill increases, improving your attack effectiveness. As you advance through the catacombs, the enemies become more powerful. You will need to replenish health and energy with mana. Don't forget, the alternative to fighting is stealth in the darkness. During battle, you have the option to run. If you have low energy, you won't get very far. Also, when fighting, you get turned around so you can't depend on the direction you were facing before fighting. Running ('Q'uick) will get you away from enemies but will use much more energy.

If you should die, restart the game by typing:
```
+rogue
```

## Map Levels

Level maps are up to 62x62 in size (plus a wall boundary for an effective 64x64 map size). They can be smaller than this. The game will end when it tries to load an non-existent level. Levels start at file name “LEVEL0“ and can go all the way to “LEVEL9“, but must be sequential.

There are two levels included on the disk, and an empty level for you to use as a template. You can edit the map levels, and add your own. They are simple ASCII text files. The included sandbox editor can edit the maps right on the disk. type:
```
-sandbox level.empty
```
after exiting from ROGUE. Make your changes and save it as LEVEL0" to "LEVEL9". The next free level is currently "LEVEL2". You may also edit an existing level:
```
-sandbox level0
```
for instance.

