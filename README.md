# Ludo Board Game (C++ / SFML)

A 2D Ludo board game for 2–4 players, built with C++ and SFML 3.x.

**Fall 2025 Programming Fundamentals Group Project**
By **Asad Ali**, **Hajra Shehzad** and **Husnain Maauz**

---

## Features

- 4 players: Red, Green, Yellow and Blue
- Two modes: **Player vs Player** and **Player vs Computer** (choose your colour)
- Animated dice roll
- Highlighted pieces showing which moves are legal
- Game log of recent events under the board
- Pause menu, settings screen, win screen
- Save and load game (`ludo_save.dat`)

## Game Rules

- Each player has **4 pieces**. All pieces start in the player's base (yard).
- Roll a **6** to bring a piece out onto its spawn cell. The 6 is used up by leaving the base.
- Pieces travel clockwise around the shared **52-cell track**, then turn into their own **6-cell home column**.
- A piece finishes when it reaches the last cell of its home column. It needs an **exact roll**; if the roll is too high, that piece cannot move.
- You cannot land on a cell already occupied by **your own** piece.
- Landing on an **opponent's piece** sends it back to its base (capture).
- **Safe cells** cannot be captured on: the 4 spawn cells and the 4 star cells.
- You get **another turn** if you roll a 6, capture a piece, or finish a piece.
- Rolling **three 6s in a row** cancels all moves made in that streak and ends your turn.
- If no piece can legally move, your turn is skipped.
- The first player to get **all 4 pieces home wins**.

## Built With

- C++ (C++17)
- [SFML 3.x](https://www.sfml-dev.org/) (Graphics, Window, System)
- No STL containers, structs or classes: all game state lives in global C-style arrays and the logic is in free functions.

## Dependencies

You need a C++17 compiler and **SFML 3.x** (SFML 2.x will not work, the API is different).

**Windows (MSYS2 MinGW64)**
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sfml
```

## Build and Run

Run these in the MSYS2 MinGW64 terminal:
```bash
g++ -std=c++17 main.cpp -o ludo.exe -lsfml-graphics -lsfml-window -lsfml-system
./ludo.exe
```

If SFML is installed in a custom folder, add `-I<path>/include -L<path>/lib`.

## Notes

- SFML has no built-in font, so the game tries common system fonts (Arial on Windows). If none is found, a warning is printed and text will not display.
- The save file `ludo_save.dat` is created in the folder you run the game from.

## Project Structure

```
.
├── main.cpp    # entire game: constants, board tables, game state, logic, drawing, input
└── README.md
```
