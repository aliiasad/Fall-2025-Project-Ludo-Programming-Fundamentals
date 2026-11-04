// ludo board game using sfml 3.x
// no stl containers, no structs/classes, all state lives in global c-style arrays
// and all the logic is in free functions
//
// layout of this file (search for the banner to jump to a section):
//   1. includes
//   2. constants
//   3. board layout tables
//   4. player colours
//   5. global game state
//   6. function prototypes (signatures only, grouped the same way as the definitions)
//   7. main()
//   8. function definitions (grouped the same way as the prototypes)

// ================================================================
// 1. INCLUDES
// ================================================================
#include <SFML/Graphics.hpp>    // window, shapes, text, colours, clock
#include <iostream>
#include <cstdlib>              // rand, srand
#include <ctime>                // time (seed for rand)
#include <cstring>              // strlen, memmove
#include <cstdio>               // snprintf, fopen, fprintf, fscanf
using namespace std;

// ================================================================
// 2. CONSTANTS
// ================================================================

// game rules
const int PLAYERS = 4; // Red = 0, Green = 1, Yellow = 2, Blue = 3
const int PIECES = 4; // pieces per player
const int TOTAL_MAIN = 52; // cells on the shared circular track
const int HOME_SIZE = 6; // cells in a home column, index 5 is the finish cell

// how pieceMoves works (the whole movement system is built on this):
//   0..51   piece is on the main track, its cell is (spawnIndex[p] + pieceMoves) % 52
//   52..57  piece is in its own home column, column index = pieceMoves - 52
//   57      piece has reached the finish (pieceState becomes 3)

// board drawing (pixels)
const float CELL_SIZE = 52.f; // one cell of the 15x15 grid
const float GRID_OFFSET_X = 60.f; // left margin of the board
const float GRID_OFFSET_Y = 65.f; // top margin of the board

// window size
const unsigned int WIN_WIDTH = 900;
const unsigned int WIN_HEIGHT = 1000;

// dice roll animation
const float ANIM_DURATION = 0.75f; // how long the dice keeps flickering (seconds)
const float ANIM_FRAME_SPEED = 0.07f; // time each flicker face stays (seconds)

// ai waits a little before acting so the player can follow what happens (seconds)
const float AI_ROLL_DELAY = 0.6f;
const float AI_MOVE_DELAY = 0.4f;

// ================================================================
// 3. BOARD LAYOUT TABLES
// ================================================================

// the main track as (row, col) pairs on the 15x15 grid, index 0 is red's spawn cell
// and the track goes around the board in the direction the pieces travel
const int trackRow[52] = {
    13, 12, 11, 10, 9,              // 0-4    up the left side of the bottom arm
    8,  8,  8,  8,  8,  8,          // 5-10   left along row 8
    7,  6,                          // 11-12  up the left edge
    6,  6,  6,  6,  6,              // 13-17  right along row 6
    5,  4,  3,  2,  1,              // 18-22  up the left side of the top arm
    0,  0,  0,                      // 23-25  across the top
    1,  2,  3,  4,  5,              // 26-30  down the right side of the top arm
    6,  6,  6,  6,  6,  6,          // 31-36  right along row 6
    7,  8,                          // 37-38  down the right edge
    8,  8,  8,  8,  8,              // 39-43  left along row 8
    9,  10, 11, 12, 13,             // 44-48  down the right side of the bottom arm
    14, 14, 14                      // 49-51  across the bottom
};

const int trackCol[52] = {
    6,  6,  6,  6,  6,              // 0-4
    5,  4,  3,  2,  1,  0,          // 5-10
    0,  0,                          // 11-12
    1,  2,  3,  4,  5,              // 13-17
    6,  6,  6,  6,  6,              // 18-22
    6,  7,  8,                      // 23-25
    8,  8,  8,  8,  8,              // 26-30
    9,  10, 11, 12, 13, 14,         // 31-36
    14, 14,                         // 37-38
    13, 12, 11, 10, 9,              // 39-43
    8,  8,  8,  8,  8,              // 44-48
    8,  7,  6                       // 49-51
};

// each player's private home column, 6 cells, the last one (index 5) is the finish
// red goes up col 7, green goes right along row 7, yellow goes down col 7, blue goes left along row 7
const int homeRow[PLAYERS][HOME_SIZE] = {
    { 13, 12, 11, 10,  9,  8 },     // red
    {  7,  7,  7,  7,  7,  7 },     // green
    {  1,  2,  3,  4,  5,  6 },     // yellow
    {  7,  7,  7,  7,  7,  7 }      // blue
};
const int homeCol[PLAYERS][HOME_SIZE] = {
    {  7,  7,  7,  7,  7,  7 },     // red
    {  1,  2,  3,  4,  5,  6 },     // green
    {  7,  7,  7,  7,  7,  7 },     // yellow
    { 13, 12, 11, 10,  9,  8 }      // blue
};

// the 4 resting squares inside each player's yard (where pieces wait for a 6)
const int baseRow[PLAYERS][PIECES] = {
    { 10, 10, 12, 12 },             // red    (bottom left yard)
    {  1,  1,  3,  3 },             // green  (top left yard)
    {  1,  1,  3,  3 },             // yellow (top right yard)
    { 10, 10, 12, 12 }              // blue   (bottom right yard)
};
const int baseCol[PLAYERS][PIECES] = {
    {  1,  3,  1,  3 },             // red
    {  1,  3,  1,  3 },             // green
    { 10, 12, 10, 12 },             // yellow
    { 10, 12, 10, 12 }              // blue
};

// track index where each player's piece enters the board after rolling a 6
const int spawnIndex[PLAYERS] = { 0, 13, 26, 39 };

// safe cells, nobody can be captured here: the 4 spawn cells + the 4 star cells (8, 21, 34, 47)
const int safeSpots[8] = { 0, 8, 13, 21, 26, 34, 39, 47 };

// ================================================================
// 4. PLAYER COLOURS
// ================================================================
// three shades per player: normal (pieces, spawn cells), dark (outlines, finish cell)
// and light (yard background)
const sf::Color playerColor[PLAYERS] = {
    sf::Color(210,  45,  45),   // red
    sf::Color( 40, 170,  40),   // green
    sf::Color(210, 190,  30),   // yellow
    sf::Color( 40,  90, 210)    // blue
};
const sf::Color playerDark[PLAYERS] = {
    sf::Color(140,  15,  15),
    sf::Color( 10, 110,  10),
    sf::Color(140, 130,  10),
    sf::Color( 10,  50, 140)
};
const sf::Color playerLight[PLAYERS] = {
    sf::Color(255, 180, 180),
    sf::Color(180, 255, 180),
    sf::Color(255, 255, 160),
    sf::Color(180, 200, 255)
};

// ================================================================
// 5. GLOBAL GAME STATE
// ================================================================

// ---------- screen / menu state ----------
// currentScreen: 0 = menu, 1 = colour select, 2 = game, 3 = win, 4 = pause, 5 = settings
int currentScreen = 0;
int gameMode = 0; // 0 = player vs player, 1 = player vs computer
int humanPlayerID = 0; // which colour the human plays in player vs computer
int winnerID = -1; // -1 until somebody finishes all 4 pieces
bool soundEnabled = true; // only a settings toggle, no audio is played in this file

// ---------- piece state (first index = player, second index = piece) ----------
int pieceState[PLAYERS][PIECES]; // 0 = in base, 1 = on main track, 2 = in home column, 3 = finished
int piecePos[PLAYERS][PIECES]; // track index (0-51) when state is 1, home column index (0-5) when state is 2
int pieceMoves[PLAYERS][PIECES]; // total steps taken since leaving base, see the pieceMoves note above
bool hasKilled[PLAYERS][PIECES]; // reserved flag for capture bookkeeping, reset in initializeGame

// mainOccupant[i] = encode(player, piece) of the piece last placed on track cell i, -1 if empty
int mainOccupant[TOTAL_MAIN];

// ---------- backup of piece state (used to undo a turn after three 6s) ----------
int bk_pieceState[PLAYERS][PIECES];
int bk_piecePos[PLAYERS][PIECES];
int bk_pieceMoves[PLAYERS][PIECES];
int bk_mainOccupant[TOTAL_MAIN];

// ---------- turn state ----------
int currentPlayer = 0; // whose turn it is (0-3)
int currentDice = 1; // last rolled value
int consecutiveSixes = 0; // 6s rolled in a row by the current player
bool waitingForRoll = true; // true while the game is waiting for a roll
bool waitingForMove = false; // true while the game is waiting for a piece to be chosen
bool bonusTurn = false; // set by a capture or a finish, grants another roll

// pieces of the current player that can legally move with the current roll
int highlightedPieces[PIECES];
int highlightedCount = 0;

// how many pieces each player has finished
int finishedCount[PLAYERS];

// ---------- dice animation ----------
sf::Clock diceAnimClock; // restarted whenever an animation starts
bool diceAnimating = false; // true while the dice is flickering
float diceAnimTime = 0.f; // total time spent animating (stops at ANIM_DURATION)
float diceFrameAcc = 0.f; // time since the flicker face last changed
int diceAnimFace = 1; // face shown while flickering

// ---------- ai timers ----------
sf::Clock aiClock; // measures the thinking delay
bool aiThinking = false; // ai is waiting before it rolls
bool aiMoving = false; // ai is waiting before it moves a piece

// ---------- game log (last few events shown under the board) ----------
const int LOG_MAX = 7;
char gameLog[LOG_MAX][160];
int logCount = 0;

// ================================================================
// 6. FUNCTION PROTOTYPES
// ================================================================

// ---------- piece encoding and log ----------
int encode(int p, int pc);
void decode(int code, int &p, int &pc);
void logMsg(const char *msg);
const char *playerName(int p);

// ---------- game setup and backup ----------
void initializeGame();
void saveState();
void restoreState();

// ---------- board queries ----------
bool isSafeSpot(int trackIdx);
int countAtTrack(int trackIdx);
int ownerAtTrack(int trackIdx);
bool hasOwnPieceAt(int p, int trackIdx);

// ---------- move rules ----------
bool canMove(int p, int pc, int dice);
int getValidMoves(int p, int dice, int validPCs[]);
void movePiece(int p, int pc, int dice);
bool checkWin(int p);

// ---------- turn flow ----------
void nextPlayer();
void afterRoll(int dice);
void handlePieceSelected(int pc);

// ---------- ai ----------
bool isAI(int p);
void aiDoMove();

// ---------- save / load to file ----------
void saveGame();
bool loadGame();

// ---------- sfml drawing helpers ----------
sf::Vector2f cellCentre(int row, int col);
sf::Vector2f trackCentre(int idx);
sf::Vector2f homeCentre(int p, int i);
sf::Vector2f baseCentre(int p, int pc);
void drawCell(sf::RenderWindow &w, int row, int col, sf::Color fill,
sf::Color outline = sf::Color(160, 160, 160));
void drawDot(sf::RenderWindow &w, float x, float y, float size, float dx, float dy);
void drawDice(sf::RenderWindow &w, int face, float x, float y);
void drawButton(sf::RenderWindow &w, float x, float y, float bw, float bh,
const char *label, sf::Font &font,
sf::Color bg = sf::Color(70, 70, 200), unsigned int sz = 20);

// ---------- sfml board and piece drawing ----------
void drawBoard(sf::RenderWindow &w);
void drawPieces(sf::RenderWindow &w);

// ---------- sfml screens ----------
float panelY();
float ctrlY();
void drawMenu(sf::RenderWindow &w, sf::Font &font);
void drawColorSelect(sf::RenderWindow &w, sf::Font &font);
void drawGameScreen(sf::RenderWindow &w, sf::Font &font);
void drawWinScreen(sf::RenderWindow &w, sf::Font &font);
void drawPauseScreen(sf::RenderWindow &w, sf::Font &font);
void drawSettingsScreen(sf::RenderWindow &w, sf::Font &font);

// ---------- mouse input ----------
bool inRect(float mx, float my, float rx, float ry, float rw, float rh);
int pieceAtPos(float mx, float my);
void handleMenuClick(float mx, float my);
void handleColorSelectClick(float mx, float my);
void handleGameClick(float mx, float my);
void handlePauseClick(float mx, float my);
void handleSettingsClick(float mx, float my);
void handleWinClick(float mx, float my);

// ---------- per frame update ----------
void update(float dt);

// ================================================================
// 7. MAIN
// ================================================================
int main()
{
    // seed the random generator once so dice rolls differ every run
    srand(static_cast<unsigned>(time(nullptr)));

    // sf::RenderWindow is the window we draw into
    // sf::VideoMode({w, h}) sets the size in pixels
    // Close | Titlebar gives a fixed size window (no resize or maximise button)
    sf::RenderWindow window(sf::VideoMode({WIN_WIDTH, WIN_HEIGHT}), "Ludo Board Game",
                            sf::Style::Close | sf::Style::Titlebar);

    // locks the loop to 60 frames per second so the game does not eat the whole cpu
    window.setFramerateLimit(60);

    // sfml has no default font, so try a few common system paths until one opens
    // openFromFile returns false when the file does not exist
    sf::Font font;
    bool fontLoaded = false;
    const char *fontPaths[] = {
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        nullptr     // end marker
    };
    for (int i = 0; fontPaths[i] != nullptr; i++) {
        if (font.openFromFile(fontPaths[i])) { fontLoaded = true; break; }
    }
    if (!fontLoaded)
        cout << "Warning: could not load a system font. Text will not display.\n";

    initializeGame();

    // frameClock measures how long the last frame took (delta time)
    sf::Clock frameClock;

    // main loop: events -> update -> draw, once per frame
    while (window.isOpen()) {

        // restart() returns the time since the last restart and starts counting again
        float dt = frameClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;   // cap it so a long freeze (window drag etc) does not make timers jump

        // ---------- events ----------
        // pollEvent gives one pending event at a time and ends when the queue is empty
        while (const auto event = window.pollEvent()) {

            // is<T>() checks the type of the event, close button pressed
            if (event->is<sf::Event::Closed>())
                window.close();

            // getIf<T>() returns a pointer to the event data if it is that type, else nullptr
            if (const auto *mousePress = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePress->button == sf::Mouse::Button::Left) {
                    float mx = static_cast<float>(mousePress->position.x);
                    float my = static_cast<float>(mousePress->position.y);

                    // send the click to the handler of whatever screen is showing
                    switch (currentScreen) {
                    case 0: handleMenuClick(mx, my);        break;
                    case 1: handleColorSelectClick(mx, my); break;
                    case 2: handleGameClick(mx, my);        break;
                    case 3: handleWinClick(mx, my);         break;
                    case 4: handlePauseClick(mx, my);       break;
                    case 5: handleSettingsClick(mx, my);    break;
                    }
                }
            }
        }

        // ---------- update ----------
        // timers and ai only run during the game (not on menu, pause or win screens)
        if (currentScreen == 2 || diceAnimating)
            update(dt);

        // ---------- draw ----------
        switch (currentScreen) {
        case 0: drawMenu(window, font);           break;
        case 1: drawColorSelect(window, font);    break;
        case 2: drawGameScreen(window, font);     break;
        case 3: drawWinScreen(window, font);      break;
        case 4:     // pause is an overlay, so the game is drawn first and the dim panel on top
            drawGameScreen(window, font);
            drawPauseScreen(window, font);
            break;
        case 5: drawSettingsScreen(window, font); break;
        }

        // display() swaps the buffers and shows everything drawn this frame
        window.display();
    }

    return 0;
}

// ================================================================
// 8. FUNCTION DEFINITIONS
// ================================================================

// ----------------------------------------------------------------
// piece encoding and log
// ----------------------------------------------------------------

// packs (player, piece) into one int, e.g. player 2 piece 3 -> 23
// used by mainOccupant so a track cell can remember who stands on it
int encode(int p, int pc)
{
    return p * 10 + pc;
}

// reverse of encode, results go back through the reference parameters
void decode(int code, int &p, int &pc)
{
    p = code / 10;
    pc = code % 10;
}

// adds a line to the game log, when the log is full the oldest line is dropped
// snprintf is used instead of strcpy so a long message can never overflow the buffer
void logMsg(const char *msg)
{
    if (logCount < LOG_MAX) {
        snprintf(gameLog[logCount], sizeof(gameLog[logCount]), "%s", msg);
        logCount++;
    }
    else {
        // memmove shifts every line one slot up (overlapping memory so memcpy is not safe)
        for (int i = 0; i < LOG_MAX - 1; i++)
            memmove(gameLog[i], gameLog[i + 1], sizeof(gameLog[i]));
        snprintf(gameLog[LOG_MAX - 1], sizeof(gameLog[LOG_MAX - 1]), "%s", msg);
    }
}

const char *playerName(int p)
{
    if (p == 0) return "Red";
    if (p == 1) return "Green";
    if (p == 2) return "Yellow";
    return "Blue";
}

// ----------------------------------------------------------------
// game setup and backup
// ----------------------------------------------------------------

// puts every piece back in base and resets the whole turn state, called when a new game starts
void initializeGame()
{
    for (int p = 0; p < PLAYERS; p++) {
        finishedCount[p] = 0;
        for (int pc = 0; pc < PIECES; pc++) {
            pieceState[p][pc] = 0;
            piecePos[p][pc] = 0;
            pieceMoves[p][pc] = 0;
            hasKilled[p][pc] = false;
        }
    }
    for (int i = 0; i < TOTAL_MAIN; i++)
        mainOccupant[i] = -1;

    currentPlayer = 0;
    currentDice = 1;
    consecutiveSixes = 0;
    waitingForRoll = true;
    waitingForMove = false;
    bonusTurn = false;
    winnerID = -1;
    logCount = 0;
    diceAnimating = false;
    diceAnimTime = 0.f;
    diceFrameAcc = 0.f;
    aiThinking = false;
    aiMoving = false;
    highlightedCount = 0;
}

// copies the live piece arrays into the bk_ arrays
// called when the first 6 of a streak is rolled, so three 6s can be undone
void saveState()
{
    for (int p = 0; p < PLAYERS; p++)
        for (int pc = 0; pc < PIECES; pc++) {
            bk_pieceState[p][pc] = pieceState[p][pc];
            bk_piecePos[p][pc] = piecePos[p][pc];
            bk_pieceMoves[p][pc] = pieceMoves[p][pc];
        }
    for (int i = 0; i < TOTAL_MAIN; i++)
        bk_mainOccupant[i] = mainOccupant[i];
}

// copies the backup back into the live arrays
// finishedCount is rebuilt by counting state 3 pieces because it is not backed up itself
void restoreState()
{
    for (int p = 0; p < PLAYERS; p++) {
        finishedCount[p] = 0;
        for (int pc = 0; pc < PIECES; pc++) {
            pieceState[p][pc] = bk_pieceState[p][pc];
            piecePos[p][pc] = bk_piecePos[p][pc];
            pieceMoves[p][pc] = bk_pieceMoves[p][pc];
            if (pieceState[p][pc] == 3)
                finishedCount[p]++;
        }
    }
    for (int i = 0; i < TOTAL_MAIN; i++)
        mainOccupant[i] = bk_mainOccupant[i];
}

// ----------------------------------------------------------------
// board queries
// ----------------------------------------------------------------

// true if the track cell is a spawn cell or a star cell (no capturing there)
bool isSafeSpot(int trackIdx)
{
    for (int i = 0; i < 8; i++)
        if (safeSpots[i] == trackIdx)
            return true;
    return false;
}

// how many pieces (of any colour) stand on a track cell
// only state 1 counts because base, home column and finished pieces are not on the track
int countAtTrack(int trackIdx)
{
    int cnt = 0;
    for (int p = 0; p < PLAYERS; p++)
        for (int pc = 0; pc < PIECES; pc++)
            if (pieceState[p][pc] == 1 && piecePos[p][pc] == trackIdx)
                cnt++;
    return cnt;
}

// first player found standing on the cell, -1 if it is empty
// used by the ai to see if a move would capture something
int ownerAtTrack(int trackIdx)
{
    int owner = -1;
    for (int p = 0; p < PLAYERS; p++)
        for (int pc = 0; pc < PIECES; pc++)
            if (pieceState[p][pc] == 1 && piecePos[p][pc] == trackIdx) {
                if (owner == -1)
                    owner = p;
            }
    return owner;
}

// true if player p already has a piece on the cell (a player cannot stack on their own piece)
bool hasOwnPieceAt(int p, int trackIdx)
{
    for (int pc = 0; pc < PIECES; pc++)
        if (pieceState[p][pc] == 1 && piecePos[p][pc] == trackIdx)
            return true;
    return false;
}

// ----------------------------------------------------------------
// move rules
// ----------------------------------------------------------------

// decides if piece pc of player p is allowed to move by dice, nothing is changed here
bool canMove(int p, int pc, int dice)
{
    int st = pieceState[p][pc];

    // finished pieces never move again
    if (st == 3)
        return false;

    // in base: only a 6 lets the piece out, and the spawn cell must not hold our own piece
    if (st == 0) {
        if (dice != 6)
            return false;
        if (hasOwnPieceAt(p, spawnIndex[p]))
            return false;
        return true;
    }

    // on the main track
    if (st == 1) {
        int newMoves = pieceMoves[p][pc] + dice;

        // 57 is the finish, anything above it is an overshoot
        if (newMoves > 51 + HOME_SIZE)
            return false;

        // still on the main track, so the landing cell must not hold our own piece
        if (newMoves <= 51) {
            int newPos = (spawnIndex[p] + newMoves) % TOTAL_MAIN;
            if (hasOwnPieceAt(p, newPos))
                return false;
        }
        // entering the home column is always fine as long as we do not overshoot
        return true;
    }

    // already in the home column, can only move if it does not go past index 5
    if (st == 2) {
        int curIdx = pieceMoves[p][pc] - 52;
        return (curIdx + dice <= 5);
    }

    return false;
}

// fills validPCs with every piece of player p that can move, returns how many there are
int getValidMoves(int p, int dice, int validPCs[])
{
    int count = 0;
    for (int pc = 0; pc < PIECES; pc++)
        if (canMove(p, pc, dice))
            validPCs[count++] = pc;
    return count;
}

// actually performs a move that canMove already approved
// three cases depending on where the piece is: base, main track or home column
void movePiece(int p, int pc, int dice)
{
    char msg[160];
    int st = pieceState[p][pc];

    // ---------- case 1: leaving base ----------
    // the piece is placed on its spawn cell and the dice value is not used as steps
    if (st == 0) {
        int sp = spawnIndex[p];
        pieceState[p][pc] = 1;
        piecePos[p][pc] = sp;
        pieceMoves[p][pc] = 0;
        mainOccupant[sp] = encode(p, pc);
        snprintf(msg, sizeof(msg), "%s piece %d exits base!", playerName(p), pc + 1);
        logMsg(msg);
        return;
    }

    // ---------- case 2: on the main track ----------
    if (st == 1) {
        int oldPos = piecePos[p][pc];
        int newMoves = pieceMoves[p][pc] + dice;

        // free the old cell, but if another piece is still standing there it becomes the occupant
        if (mainOccupant[oldPos] == encode(p, pc))
            mainOccupant[oldPos] = -1;
        for (int q = 0; q < PLAYERS; q++)
            for (int qc = 0; qc < PIECES; qc++)
                if (!(q == p && qc == pc) &&
                    pieceState[q][qc] == 1 &&
                    piecePos[q][qc] == oldPos)
                    mainOccupant[oldPos] = encode(q, qc);

        // stays on the main track
        if (newMoves <= 51) {
            int newPos = (spawnIndex[p] + newMoves) % TOTAL_MAIN;

            // capture: every opponent piece on the landing cell goes back to base
            // (not on safe cells), a capture also gives the mover a bonus turn
            if (!isSafeSpot(newPos)) {
                for (int q = 0; q < PLAYERS; q++) {
                    if (q == p) continue;
                    for (int qc = 0; qc < PIECES; qc++) {
                        if (pieceState[q][qc] == 1 && piecePos[q][qc] == newPos) {
                            pieceState[q][qc] = 0;
                            piecePos[q][qc] = 0;
                            pieceMoves[q][qc] = 0;
                            mainOccupant[newPos] = -1;
                            snprintf(msg, sizeof(msg),
                                     "%s piece %d captures %s piece %d!",
                                     playerName(p), pc + 1, playerName(q), qc + 1);
                            logMsg(msg);
                            bonusTurn = true;
                        }
                    }
                }
            }

            piecePos[p][pc] = newPos;
            pieceMoves[p][pc] = newMoves;
            mainOccupant[newPos] = encode(p, pc);
            snprintf(msg, sizeof(msg), "%s piece %d -> track %d",
                     playerName(p), pc + 1, newPos);
            logMsg(msg);
        }
        // leaves the main track and goes into the home column
        else {
            int homeIdx = newMoves - 52;
            pieceMoves[p][pc] = newMoves;
            piecePos[p][pc] = homeIdx;

            // index 5 is the finish cell, finishing also gives a bonus turn
            if (homeIdx == 5) {
                pieceState[p][pc] = 3;
                finishedCount[p]++;
                bonusTurn = true;
                snprintf(msg, sizeof(msg), "%s piece %d FINISHED!",
                         playerName(p), pc + 1);
            }
            else {
                pieceState[p][pc] = 2;
                snprintf(msg, sizeof(msg), "%s piece %d enters home col %d",
                         playerName(p), pc + 1, homeIdx);
            }
            logMsg(msg);
        }
        return;
    }

    // ---------- case 3: already in the home column ----------
    if (st == 2) {
        int newIdx = (pieceMoves[p][pc] - 52) + dice;
        pieceMoves[p][pc] += dice;
        piecePos[p][pc] = newIdx;

        if (newIdx == 5) {
            pieceState[p][pc] = 3;
            finishedCount[p]++;
            bonusTurn = true;
            snprintf(msg, sizeof(msg), "%s piece %d FINISHED!", playerName(p), pc + 1);
        }
        else {
            snprintf(msg, sizeof(msg), "%s piece %d at home col %d",
                     playerName(p), pc + 1, newIdx);
        }
        logMsg(msg);
    }
}

// a player wins when all 4 of their pieces have finished
bool checkWin(int p)
{
    return finishedCount[p] == PIECES;
}

// ----------------------------------------------------------------
// turn flow
// ----------------------------------------------------------------

// passes the turn to the next player and resets everything that belongs to one turn
void nextPlayer()
{
    bonusTurn = false;
    consecutiveSixes = 0;
    currentPlayer = (currentPlayer + 1) % PLAYERS;
    waitingForRoll = true;
    waitingForMove = false;
    highlightedCount = 0;
    diceAnimating = false;
    aiThinking = false;
    aiMoving = false;

    // skip players who have already finished all pieces
    for (int t = 0; t < PLAYERS; t++) {
        bool allDone = true;
        for (int pc = 0; pc < PIECES; pc++)
            if (pieceState[currentPlayer][pc] != 3) { allDone = false; break; }
        if (!allDone) break;
        currentPlayer = (currentPlayer + 1) % PLAYERS;
    }

    char msg[80];
    snprintf(msg, sizeof(msg), "--- %s's turn ---", playerName(currentPlayer));
    logMsg(msg);
}

// runs right after the dice animation ends with the rolled value
// handles the three sixes rule, then works out which pieces can move
void afterRoll(int dice)
{
    currentDice = dice;

    if (dice == 6) {
        consecutiveSixes++;

        // snapshot before the first six so a streak of three can be undone
        if (consecutiveSixes == 1)
            saveState();

        // third six in a row: undo everything done in the streak and lose the turn
        if (consecutiveSixes >= 3) {
            restoreState();
            logMsg("Three 6s! All moves cancelled.");
            consecutiveSixes = 0;
            bonusTurn = false;
            nextPlayer();
            return;
        }
    }
    else {
        consecutiveSixes = 0;
    }

    int validPCs[PIECES];
    int count = getValidMoves(currentPlayer, dice, validPCs);

    // nothing can move, so the turn is lost
    if (count == 0) {
        logMsg("No valid moves - turn skipped.");
        nextPlayer();
        return;
    }

    // remember the movable pieces (drawPieces highlights them, handlePieceSelected checks against them)
    for (int i = 0; i < count; i++)
        highlightedPieces[i] = validPCs[i];
    highlightedCount = count;
    waitingForRoll = false;
    waitingForMove = true;
}

// called when a piece is chosen, by a human click or by the ai
// moves the piece, checks for a win, then decides if the same player rolls again
void handlePieceSelected(int pc)
{
    if (!waitingForMove)
        return;

    // only pieces in the highlighted list can be chosen
    bool valid = false;
    for (int i = 0; i < highlightedCount; i++)
        if (highlightedPieces[i] == pc) { valid = true; break; }
    if (!valid)
        return;

    movePiece(currentPlayer, pc, currentDice);
    highlightedCount = 0;
    waitingForMove = false;

    // game over, switch to the win screen
    if (checkWin(currentPlayer)) {
        winnerID = currentPlayer;
        currentScreen = 3;
        char msg[80];
        snprintf(msg, sizeof(msg), "*** %s WINS the game! ***", playerName(currentPlayer));
        logMsg(msg);
        return;
    }

    // extra turn after a six, a capture or a finish, otherwise the next player goes
    if (currentDice == 6 || bonusTurn) {
        waitingForRoll = true;
        bonusTurn = false;
        char msg[80];
        snprintf(msg, sizeof(msg), "%s gets another turn!", playerName(currentPlayer));
        logMsg(msg);
    }
    else {
        nextPlayer();
    }
}

// ----------------------------------------------------------------
// ai
// ----------------------------------------------------------------

// in player vs player nobody is ai, in player vs computer everyone except the human is
bool isAI(int p)
{
    if (gameMode == 0)
        return false;
    return (p != humanPlayerID);
}

// picks a piece for the ai and plays it
// starts from a random valid piece, then the loop overrides it with better choices:
//   1. finish a piece (stops the search at once)
//   2. capture an opponent (stops the search at once)
//   3. take a piece out of base (can still be overridden by a later 1 or 2)
void aiDoMove()
{
    int validPCs[PIECES];
    int count = getValidMoves(currentPlayer, currentDice, validPCs);
    if (count == 0) {
        nextPlayer();
        return;
    }

    int best = validPCs[rand() % count];

    for (int i = 0; i < count; i++) {
        int pc = validPCs[i];

        // priority 1: a home column piece that lands exactly on the finish
        if (pieceState[currentPlayer][pc] == 2) {
            int newIdx = (pieceMoves[currentPlayer][pc] - 52) + currentDice;
            if (newIdx == 5) { best = pc; break; }
        }

        // priority 2: a track piece that lands on an unsafe cell holding an opponent
        if (pieceState[currentPlayer][pc] == 1) {
            int newMoves = pieceMoves[currentPlayer][pc] + currentDice;
            if (newMoves <= 51) {
                int newPos = (spawnIndex[currentPlayer] + newMoves) % TOTAL_MAIN;
                if (!isSafeSpot(newPos) && ownerAtTrack(newPos) >= 0 &&
                    ownerAtTrack(newPos) != currentPlayer) {
                    best = pc;
                    break;
                }
            }
        }

        // priority 3: get a piece out of base
        if (pieceState[currentPlayer][pc] == 0 && currentDice == 6)
            best = pc;
    }

    handlePieceSelected(best);
    aiMoving = false;
}

// ----------------------------------------------------------------
// save / load to file
// ----------------------------------------------------------------

// writes the game to a plain text file, one group of values per line
// the order here must match the order in loadGame exactly
void saveGame()
{
    FILE *f = fopen("ludo_save.dat", "w");
    if (!f) {
        logMsg("Save failed!");
        return;
    }

    fprintf(f, "%d %d %d\n", currentScreen, gameMode, humanPlayerID);
    fprintf(f, "%d %d %d\n", currentPlayer, currentDice, consecutiveSixes);
    fprintf(f, "%d %d %d\n", (int)waitingForRoll, (int)waitingForMove, (int)bonusTurn);
    fprintf(f, "%d\n", winnerID);

    // one line per player: state, pos, moves of each of the 4 pieces
    for (int p = 0; p < PLAYERS; p++) {
        for (int pc = 0; pc < PIECES; pc++)
            fprintf(f, "%d %d %d  ", pieceState[p][pc], piecePos[p][pc], pieceMoves[p][pc]);
        fprintf(f, "\n");
    }
    for (int p = 0; p < PLAYERS; p++)
        fprintf(f, "%d ", finishedCount[p]);
    fprintf(f, "\n");
    for (int i = 0; i < TOTAL_MAIN; i++)
        fprintf(f, "%d ", mainOccupant[i]);
    fprintf(f, "\n");

    // the log is saved too, count first and then each line
    fprintf(f, "%d\n", logCount);
    for (int i = 0; i < logCount; i++)
        fprintf(f, "%s\n", gameLog[i]);

    fclose(f);
    logMsg("Game saved.");
}

// reads the file written by saveGame, returns false if there is no save file
bool loadGame()
{
    FILE *f = fopen("ludo_save.dat", "r");
    if (!f) {
        logMsg("Load failed: no save file found.");
        return false;
    }

    fscanf(f, "%d %d %d", &currentScreen, &gameMode, &humanPlayerID);
    fscanf(f, "%d %d %d", &currentPlayer, &currentDice, &consecutiveSixes);

    // bools are stored as ints in the file, so read into ints and convert
    int wr, wm, bt;
    fscanf(f, "%d %d %d", &wr, &wm, &bt);
    waitingForRoll = (bool)wr;
    waitingForMove = (bool)wm;
    bonusTurn = (bool)bt;
    fscanf(f, "%d", &winnerID);

    for (int p = 0; p < PLAYERS; p++)
        for (int pc = 0; pc < PIECES; pc++)
            fscanf(f, "%d %d %d", &pieceState[p][pc], &piecePos[p][pc], &pieceMoves[p][pc]);

    for (int p = 0; p < PLAYERS; p++)
        fscanf(f, "%d", &finishedCount[p]);
    for (int i = 0; i < TOTAL_MAIN; i++)
        fscanf(f, "%d", &mainOccupant[i]);

    // the log lines contain spaces so fgets is used and the trailing newline is cut off
    fscanf(f, "%d\n", &logCount);
    for (int i = 0; i < logCount; i++) {
        fgets(gameLog[i], (int)sizeof(gameLog[i]), f);
        int len = (int)strlen(gameLog[i]);
        if (len > 0 && gameLog[i][len - 1] == '\n')
            gameLog[i][len - 1] = '\0';
    }
    fclose(f);

    // animation, ai and highlight state are not saved, so reset them
    highlightedCount = 0;
    diceAnimating = false;
    aiThinking = false;
    aiMoving = false;

    // fresh backup so the three sixes rule cannot restore data from before the load
    saveState();

    // the save may have been made while a piece had to be chosen, so rebuild the highlight list
    // if no piece can move after all, fall back to rolling again
    if (waitingForMove) {
        int validPCs[PIECES];
        int count = getValidMoves(currentPlayer, currentDice, validPCs);
        if (count > 0) {
            for (int i = 0; i < count; i++)
                highlightedPieces[i] = validPCs[i];
            highlightedCount = count;
        }
        else {
            waitingForMove = false;
            waitingForRoll = true;
        }
    }

    // always resume on the game screen, even if it was saved from the pause screen
    currentScreen = 2;
    return true;
}

// ----------------------------------------------------------------
// sfml drawing helpers
// ----------------------------------------------------------------

// the board is a 15x15 grid, this turns a (row, col) into the pixel position of that cell's middle
sf::Vector2f cellCentre(int row, int col)
{
    return sf::Vector2f(
        GRID_OFFSET_X + col * CELL_SIZE + CELL_SIZE * 0.5f,
        GRID_OFFSET_Y + row * CELL_SIZE + CELL_SIZE * 0.5f);
}

// shortcuts that look the grid cell up in the tables and return its centre
sf::Vector2f trackCentre(int idx)
{
    return cellCentre(trackRow[idx], trackCol[idx]);
}

sf::Vector2f homeCentre(int p, int i)
{
    return cellCentre(homeRow[p][i], homeCol[p][i]);
}

sf::Vector2f baseCentre(int p, int pc)
{
    return cellCentre(baseRow[p][pc], baseCol[p][pc]);
}

// draws one square cell of the grid
// sf::RectangleShape is a filled rectangle, it is made 1px smaller than the cell so a gap shows between cells
// setPosition = top left corner, setFillColor = inside colour,
// setOutlineThickness / setOutlineColor = border, w.draw(shape) puts it on the window
void drawCell(sf::RenderWindow &w, int row, int col, sf::Color fill, sf::Color outline)
{
    sf::RectangleShape rect(sf::Vector2f(CELL_SIZE - 1.f, CELL_SIZE - 1.f));
    rect.setPosition({GRID_OFFSET_X + col * CELL_SIZE + 0.5f,
                      GRID_OFFSET_Y + row * CELL_SIZE + 0.5f});
    rect.setFillColor(fill);
    rect.setOutlineThickness(1.f);
    rect.setOutlineColor(outline);
    w.draw(rect);
}

// draws one dice dot, (dx, dy) is the dot position as a fraction (0 to 1) of the dice size
// sf::CircleShape(radius) makes a circle, its position is the top left of the bounding box
// so the radius is subtracted to centre the dot on the point
void drawDot(sf::RenderWindow &w, float x, float y, float size, float dx, float dy)
{
    float r = size * 0.09f;
    sf::CircleShape c(r);
    c.setFillColor(sf::Color(30, 30, 30));
    c.setPosition({x + dx * size - r, y + dy * size - r});
    w.draw(c);
}

// draws a dice square with the dots of the given face (1-6) at pixel position (x, y)
// the dot positions are fractions of the dice, 0.27 / 0.50 / 0.73 are left-middle-right (or top-middle-bottom)
void drawDice(sf::RenderWindow &w, int face, float x, float y)
{
    const float S = 60.f; // dice size in pixels

    sf::RectangleShape box(sf::Vector2f(S, S));
    box.setPosition({x, y});
    box.setFillColor(sf::Color::White);
    box.setOutlineThickness(2.f);
    box.setOutlineColor(sf::Color(60, 60, 60));
    w.draw(box);

    switch (face) {
    case 1:
        drawDot(w, x, y, S, 0.50f, 0.50f);
        break;
    case 2:
        drawDot(w, x, y, S, 0.27f, 0.27f);
        drawDot(w, x, y, S, 0.73f, 0.73f);
        break;
    case 3:
        drawDot(w, x, y, S, 0.27f, 0.27f);
        drawDot(w, x, y, S, 0.50f, 0.50f);
        drawDot(w, x, y, S, 0.73f, 0.73f);
        break;
    case 4:
        drawDot(w, x, y, S, 0.27f, 0.27f); drawDot(w, x, y, S, 0.73f, 0.27f);
        drawDot(w, x, y, S, 0.27f, 0.73f); drawDot(w, x, y, S, 0.73f, 0.73f);
        break;
    case 5:
        drawDot(w, x, y, S, 0.27f, 0.27f); drawDot(w, x, y, S, 0.73f, 0.27f);
        drawDot(w, x, y, S, 0.50f, 0.50f);
        drawDot(w, x, y, S, 0.27f, 0.73f); drawDot(w, x, y, S, 0.73f, 0.73f);
        break;
    case 6:
        drawDot(w, x, y, S, 0.27f, 0.22f); drawDot(w, x, y, S, 0.73f, 0.22f);
        drawDot(w, x, y, S, 0.27f, 0.50f); drawDot(w, x, y, S, 0.73f, 0.50f);
        drawDot(w, x, y, S, 0.27f, 0.78f); drawDot(w, x, y, S, 0.73f, 0.78f);
        break;
    }
}

// draws a clickable button, the click itself is checked separately with inRect
// sf::Text needs a font, setString = what it says, setCharacterSize = font size in pixels
// getLocalBounds gives the size of the text so it can be centred inside the button
// (bounds.position is the small offset the font adds before the first glyph, so it is subtracted)
void drawButton(sf::RenderWindow &w, float x, float y, float bw, float bh,
const char *label, sf::Font &font, sf::Color bg, unsigned int sz)
{
    sf::RectangleShape btn(sf::Vector2f(bw, bh));
    btn.setPosition({x, y});
    btn.setFillColor(bg);
    btn.setOutlineThickness(2.f);
    btn.setOutlineColor(sf::Color::White);
    w.draw(btn);

    sf::Text txt(font);
    txt.setString(label);
    txt.setCharacterSize(sz);
    txt.setFillColor(sf::Color::White);
    sf::FloatRect b = txt.getLocalBounds();
    txt.setPosition({x + (bw - b.size.x) * 0.5f - b.position.x,
                     y + (bh - b.size.y) * 0.5f - b.position.y});
    w.draw(txt);
}

// ----------------------------------------------------------------
// sfml board and piece drawing
// ----------------------------------------------------------------

// draws the whole board in layers, later steps are drawn over earlier ones
void drawBoard(sf::RenderWindow &w)
{
    // ---------- step 1: base colour of every cell ----------
    for (int r = 0; r < 15; r++) {
        for (int c = 0; c < 15; c++) {
            sf::Color col(248, 248, 248); // default is a near white track cell

            // the four 6x6 yards, each in the light shade of its player
            if (r <= 5 && c <= 5)  col = playerLight[1];    // green
            if (r <= 5 && c >= 9)  col = playerLight[2];    // yellow
            if (r >= 9 && c <= 5)  col = playerLight[0];    // red
            if (r >= 9 && c >= 9)  col = playerLight[3];    // blue

            // white 4x4 area inside each yard where the pieces sit
            if (r >= 1 && r <= 4 && c >= 1 && c <= 4)       col = sf::Color(255, 255, 255);
            if (r >= 1 && r <= 4 && c >= 10 && c <= 13)     col = sf::Color(255, 255, 255);
            if (r >= 10 && r <= 13 && c >= 1 && c <= 4)     col = sf::Color(255, 255, 255);
            if (r >= 10 && r <= 13 && c >= 10 && c <= 13)   col = sf::Color(255, 255, 255);

            // centre 3x3, the triangles in step 5 are drawn over this
            if (r >= 6 && r <= 8 && c >= 6 && c <= 8)       col = sf::Color(230, 230, 230);

            drawCell(w, r, c, col);
        }
    }

    // ---------- step 2: home columns ----------
    // the alpha value (last number of sf::Color) makes the colour see through, 160 out of 255
    for (int p = 0; p < PLAYERS; p++) {
        for (int i = 0; i < HOME_SIZE - 1; i++)
            drawCell(w, homeRow[p][i], homeCol[p][i],
                     sf::Color(playerColor[p].r, playerColor[p].g, playerColor[p].b, 160));
        drawCell(w, homeRow[p][5], homeCol[p][5], playerDark[p]);   // finish cell is darker
    }

    // ---------- step 3: spawn cells in the player's colour ----------
    for (int p = 0; p < PLAYERS; p++)
        drawCell(w, trackRow[spawnIndex[p]], trackCol[spawnIndex[p]], playerColor[p]);

    // ---------- step 4: star cells (safe, but not a spawn cell) ----------
    int stars[4] = { 8, 21, 34, 47 };
    for (int i = 0; i < 4; i++)
        drawCell(w, trackRow[stars[i]], trackCol[stars[i]], sf::Color(255, 220, 80));

    // ---------- step 5: centre triangles, one per player ----------
    // cx, cy = top left of the centre area, cw, ch = its size, mx, my = its exact middle
    float cx = GRID_OFFSET_X + 6 * CELL_SIZE;
    float cy = GRID_OFFSET_Y + 6 * CELL_SIZE;
    float cw = 3 * CELL_SIZE;
    float ch = 3 * CELL_SIZE;
    float mx = cx + cw * 0.5f, my = cy + ch * 0.5f;

    // sf::ConvexShape is a polygon made from any points, setPointCount says how many
    // and setPoint(i, position) places each corner, here 3 points make a triangle
    // the same shape object is reused, only the colour and the points are changed
    sf::ConvexShape tri;
    tri.setPointCount(3);

    // bottom triangle, red
    tri.setFillColor(sf::Color(playerColor[0].r, playerColor[0].g, playerColor[0].b, 200));
    tri.setPoint(0, sf::Vector2f(cx,      cy + ch));
    tri.setPoint(1, sf::Vector2f(cx + cw, cy + ch));
    tri.setPoint(2, sf::Vector2f(mx,      my));
    w.draw(tri);

    // left triangle, green
    tri.setFillColor(sf::Color(playerColor[1].r, playerColor[1].g, playerColor[1].b, 200));
    tri.setPoint(0, sf::Vector2f(cx, cy));
    tri.setPoint(1, sf::Vector2f(cx, cy + ch));
    tri.setPoint(2, sf::Vector2f(mx, my));
    w.draw(tri);

    // top triangle, yellow
    tri.setFillColor(sf::Color(playerColor[2].r, playerColor[2].g, playerColor[2].b, 200));
    tri.setPoint(0, sf::Vector2f(cx,      cy));
    tri.setPoint(1, sf::Vector2f(cx + cw, cy));
    tri.setPoint(2, sf::Vector2f(mx,      my));
    w.draw(tri);

    // right triangle, blue
    tri.setFillColor(sf::Color(playerColor[3].r, playerColor[3].g, playerColor[3].b, 200));
    tri.setPoint(0, sf::Vector2f(cx + cw, cy));
    tri.setPoint(1, sf::Vector2f(cx + cw, cy + ch));
    tri.setPoint(2, sf::Vector2f(mx,      my));
    w.draw(tri);

    // white circle on top of the triangle corners in the middle
    // setOrigin moves the point the shape is positioned by, so the circle is centred on (mx, my)
    sf::CircleShape homeCircle(CELL_SIZE * 0.9f);
    homeCircle.setFillColor(sf::Color(255, 255, 255, 220));
    homeCircle.setOrigin({CELL_SIZE * 0.9f, CELL_SIZE * 0.9f});
    homeCircle.setPosition({mx, my});
    w.draw(homeCircle);

    // ---------- step 6: star symbols ----------
    // a CircleShape with a second argument is a regular polygon with that many sides, 5 sides = pentagon
    // (used as a simple star marker)
    for (int i = 0; i < 4; i++) {
        sf::CircleShape star(CELL_SIZE * 0.22f, 5);
        star.setFillColor(sf::Color(200, 120, 0));
        star.setOrigin({CELL_SIZE * 0.22f, CELL_SIZE * 0.22f});
        sf::Vector2f sc = trackCentre(stars[i]);
        star.setPosition({sc.x, sc.y});
        w.draw(star);
    }
}

// draws every piece that is not finished
// the screen position depends on the state: base slot, track cell or home column cell
void drawPieces(sf::RenderWindow &w)
{
    float r = CELL_SIZE * 0.32f; // piece radius

    for (int p = 0; p < PLAYERS; p++) {
        for (int pc = 0; pc < PIECES; pc++) {
            int st = pieceState[p][pc];
            if (st == 3)
                continue;   // finished pieces are not drawn

            sf::Vector2f centre;

            if (st == 0) {
                centre = baseCentre(p, pc);
            }
            else if (st == 1) {
                centre = trackCentre(piecePos[p][pc]);

                // several pieces on one cell would hide each other, so each one is shifted
                // into its own corner of a small 2x2 pattern (myIdx = how many pieces come before this one)
                int cnt = countAtTrack(piecePos[p][pc]);
                int myIdx = 0;
                if (cnt > 1) {
                    bool found = false;
                    for (int q = 0; q < PLAYERS && !found; q++)
                        for (int qc = 0; qc < PIECES && !found; qc++) {
                            if (q == p && qc == pc) { found = true; break; }
                            if (pieceState[q][qc] == 1 && piecePos[q][qc] == piecePos[p][pc])
                                myIdx++;
                        }
                    float ox[4] = { -r * 0.55f, r * 0.55f, -r * 0.55f, r * 0.55f };
                    float oy[4] = { -r * 0.55f, -r * 0.55f, r * 0.55f, r * 0.55f };
                    if (myIdx < 4) {
                        centre.x += ox[myIdx];
                        centre.y += oy[myIdx];
                    }
                }
            }
            else {  // st == 2
                centre = homeCentre(p, piecePos[p][pc]);
            }

            // a piece is highlighted if it is the current player's and is in the valid move list
            bool highlighted = false;
            if (waitingForMove && currentPlayer == p) {
                for (int i = 0; i < highlightedCount; i++)
                    if (highlightedPieces[i] == pc) { highlighted = true; break; }
            }

            // soft shadow, a transparent black circle shifted 2px down and right
            sf::CircleShape shadow(r);
            shadow.setFillColor(sf::Color(0, 0, 0, 55));
            shadow.setPosition({centre.x - r + 2.f, centre.y - r + 2.f});
            w.draw(shadow);

            // piece body, thicker white outline when it can be clicked
            sf::CircleShape circle(r);
            circle.setFillColor(playerColor[p]);
            circle.setOutlineThickness(highlighted ? 3.f : 2.f);
            circle.setOutlineColor(highlighted ? sf::Color::White : playerDark[p]);
            circle.setPosition({centre.x - r, centre.y - r});
            w.draw(circle);

            // yellow ring around clickable pieces (sf::Color::Transparent = no fill, only the outline shows)
            if (highlighted) {
                sf::CircleShape ring(r + 5.f);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(2.f);
                ring.setOutlineColor(sf::Color(255, 255, 100, 200));
                ring.setPosition({centre.x - r - 5.f, centre.y - r - 5.f});
                w.draw(ring);
            }
        }
    }
}

// ----------------------------------------------------------------
// sfml screens
// ----------------------------------------------------------------

// top of the control panel, just under the 15 rows of the board
float panelY()
{
    return GRID_OFFSET_Y + 15.f * CELL_SIZE + 8.f;
}

// y position of the dice, roll button, pause button and log (below the turn bar)
float ctrlY()
{
    return panelY() + 50.f;
}

// screen 0, main menu
// w.clear(colour) wipes the previous frame with a background colour, it is the first call of every screen
void drawMenu(sf::RenderWindow &w, sf::Font &font)
{
    w.clear(sf::Color(25, 25, 55));

    // title, setStyle(sf::Text::Bold) makes it bold, x is calculated to centre it on the window
    sf::Text title(font);
    title.setString("L U D O");
    title.setCharacterSize(80);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color::White);
    sf::FloatRect tb = title.getLocalBounds();
    title.setPosition({(WIN_WIDTH - tb.size.x) * 0.5f - tb.position.x, 70.f});
    w.draw(title);

    // underline made of four short bars, one per player colour
    float lineY = 165.f;
    float lineW = 400.f, lineX = (WIN_WIDTH - lineW) * 0.5f;
    for (int i = 0; i < 4; i++) {
        sf::RectangleShape seg(sf::Vector2f(lineW * 0.25f - 4.f, 6.f));
        seg.setPosition({lineX + i * lineW * 0.25f + 2.f, lineY});
        seg.setFillColor(playerColor[i]);
        w.draw(seg);
    }

    // the button positions here must match the ones in handleMenuClick
    float bw = 300.f, bh = 55.f, bx = (WIN_WIDTH - bw) * 0.5f;
    drawButton(w, bx, 230.f, bw, bh, "Player vs Player",   font, sf::Color(60, 140, 60));
    drawButton(w, bx, 310.f, bw, bh, "Player vs Computer", font, sf::Color(60,  90, 180));
    drawButton(w, bx, 390.f, bw, bh, "Load Game",          font, sf::Color(90,  90,  90));
    drawButton(w, bx, 470.f, bw, bh, "Settings",           font, sf::Color(110, 70, 130));
}

// screen 1, colour select (only reached from player vs computer)
void drawColorSelect(sf::RenderWindow &w, sf::Font &font)
{
    w.clear(sf::Color(25, 25, 55));

    sf::Text title(font);
    title.setString("Choose Your Color");
    title.setCharacterSize(42);
    title.setFillColor(sf::Color::White);
    sf::FloatRect tb = title.getLocalBounds();
    title.setPosition({(WIN_WIDTH - tb.size.x) * 0.5f - tb.position.x, 80.f});
    w.draw(title);

    // four big colour squares with the colour name under each one
    const char *names[4] = { "Red", "Green", "Yellow", "Blue" };
    for (int i = 0; i < 4; i++) {
        float bx = 80.f + i * 185.f, by = 220.f;
        sf::RectangleShape box(sf::Vector2f(140.f, 140.f));
        box.setPosition({bx, by});
        box.setFillColor(playerColor[i]);
        box.setOutlineThickness(4.f);
        box.setOutlineColor(sf::Color::White);
        w.draw(box);

        sf::Text lbl(font);
        lbl.setString(names[i]);
        lbl.setCharacterSize(22);
        lbl.setFillColor(sf::Color::White);
        sf::FloatRect lb = lbl.getLocalBounds();
        lbl.setPosition({bx + (140.f - lb.size.x) * 0.5f - lb.position.x, by + 150.f});
        w.draw(lbl);
    }

    drawButton(w, (WIN_WIDTH - 200.f) * 0.5f, 430.f, 200.f, 50.f,
               "Back", font, sf::Color(110, 50, 50));
}

// screen 2, the game itself: board, pieces and the control panel below the board
void drawGameScreen(sf::RenderWindow &w, sf::Font &font)
{
    w.clear(sf::Color(200, 200, 200));

    drawBoard(w);
    drawPieces(w);

    float py = panelY(), cy = ctrlY();

    // ---------- turn bar in the current player's colour ----------
    sf::RectangleShape bar(sf::Vector2f(WIN_WIDTH - 20.f, 42.f));
    bar.setPosition({10.f, py});
    bar.setFillColor(playerColor[currentPlayer]);
    bar.setOutlineThickness(2.f);
    bar.setOutlineColor(sf::Color(80, 80, 80));
    w.draw(bar);

    char turnStr[64];
    snprintf(turnStr, sizeof(turnStr), "%s's Turn%s",
             playerName(currentPlayer),
             isAI(currentPlayer) ? "  [AI]" : "");
    sf::Text turnTxt(font);
    turnTxt.setString(turnStr);
    turnTxt.setCharacterSize(22);
    turnTxt.setFillColor(sf::Color::White);
    turnTxt.setPosition({16.f, py + 10.f});
    w.draw(turnTxt);

    // small badge in the bar for every player who has finished at least one piece
    for (int p = 0; p < PLAYERS; p++) {
        if (finishedCount[p] > 0) {
            sf::RectangleShape badge(sf::Vector2f(70.f, 18.f));
            float bx = WIN_WIDTH - 80.f - (PLAYERS - 1 - p) * 76.f;
            badge.setPosition({bx, py + 12.f});
            badge.setFillColor(playerColor[p]);
            w.draw(badge);

            char fc[16];
            snprintf(fc, sizeof(fc), "%s %d/4", playerName(p), finishedCount[p]);
            sf::Text ft(font);
            ft.setString(fc);
            ft.setCharacterSize(12);
            ft.setFillColor(sf::Color::White);
            ft.setPosition({bx + 3.f, py + 14.f});
            w.draw(ft);
        }
    }

    // ---------- dice (shows a random face while animating, the real value otherwise) ----------
    int face = diceAnimating ? diceAnimFace : currentDice;
    drawDice(w, face, 16.f, cy);

    // ---------- roll button or a hint, depending on what the game is waiting for ----------
    if (waitingForRoll && !isAI(currentPlayer) && !diceAnimating) {
        drawButton(w, 90.f, cy, 160.f, 60.f, "Roll Dice", font, sf::Color(60, 140, 60));
    }
    else if (waitingForMove && !isAI(currentPlayer)) {
        sf::Text hint(font);
        hint.setString("Click a piece to move");
        hint.setCharacterSize(17);
        hint.setFillColor(sf::Color(40, 40, 130));
        hint.setPosition({90.f, cy + 20.f});
        w.draw(hint);
    }
    else if (diceAnimating || (isAI(currentPlayer))) {
        sf::Text thinking(font);
        thinking.setString("Thinking...");
        thinking.setCharacterSize(17);
        thinking.setFillColor(sf::Color(100, 100, 100));
        thinking.setPosition({90.f, cy + 20.f});
        w.draw(thinking);
    }

    // ---------- warning for the three sixes rule ----------
    if (consecutiveSixes > 0) {
        char sxt[24];
        snprintf(sxt, sizeof(sxt), "Sixes: %d / 3", consecutiveSixes);
        sf::Text sx(font);
        sx.setString(sxt);
        sx.setCharacterSize(15);
        sx.setFillColor(sf::Color(200, 50, 50));
        sx.setPosition({16.f, cy + 64.f});
        w.draw(sx);
    }

    // ---------- pause button (position must match handleGameClick) ----------
    drawButton(w, WIN_WIDTH - 115.f, cy, 105.f, 40.f,
               "Pause", font, sf::Color(90, 90, 90), 18);

    // ---------- game log box, shows the 5 newest lines ----------
    float logX = 265.f, logW = WIN_WIDTH - logX - 125.f;
    sf::RectangleShape logBg(sf::Vector2f(logW, 110.f));
    logBg.setPosition({logX, cy});
    logBg.setFillColor(sf::Color(215, 215, 215));
    logBg.setOutlineThickness(1.f);
    logBg.setOutlineColor(sf::Color(160, 160, 160));
    w.draw(logBg);

    sf::Text logHdr(font);
    logHdr.setString("Game Log:");
    logHdr.setCharacterSize(13);
    logHdr.setFillColor(sf::Color(70, 70, 70));
    logHdr.setStyle(sf::Text::Bold);
    logHdr.setPosition({logX + 4.f, cy + 2.f});
    w.draw(logHdr);

    int startIdx = logCount - 5;
    if (startIdx < 0)
        startIdx = 0;
    for (int i = startIdx; i < logCount; i++) {
        sf::Text lt(font);
        lt.setString(gameLog[i]);
        lt.setCharacterSize(12);
        lt.setFillColor(sf::Color(40, 40, 40));
        lt.setPosition({logX + 4.f, cy + 18.f + (float)(i - startIdx) * 18.f});
        w.draw(lt);
    }
}

// screen 3, winner announcement
void drawWinScreen(sf::RenderWindow &w, sf::Font &font)
{
    w.clear(sf::Color(15, 15, 35));

    sf::Text title(font);
    title.setString("WINNER!");
    title.setCharacterSize(80);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color::Yellow);
    sf::FloatRect tb = title.getLocalBounds();
    title.setPosition({(WIN_WIDTH - tb.size.x) * 0.5f - tb.position.x, 130.f});
    w.draw(title);

    // the winner's name is drawn in their own colour
    sf::Text nameT(font);
    nameT.setString(playerName(winnerID));
    nameT.setCharacterSize(60);
    nameT.setFillColor(winnerID >= 0 ? playerColor[winnerID] : sf::Color::White);
    sf::FloatRect nb = nameT.getLocalBounds();
    nameT.setPosition({(WIN_WIDTH - nb.size.x) * 0.5f - nb.position.x, 240.f});
    w.draw(nameT);

    sf::Text sub(font);
    sub.setString("wins the game!");
    sub.setCharacterSize(36);
    sub.setFillColor(sf::Color(210, 210, 210));
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setPosition({(WIN_WIDTH - sb.size.x) * 0.5f - sb.position.x, 320.f});
    w.draw(sub);

    drawButton(w, (WIN_WIDTH - 260.f) * 0.5f, 440.f, 260.f, 56.f,
               "Main Menu", font, sf::Color(70, 70, 200));
}

// screen 4, pause menu
// no clear() here, it is drawn on top of the game screen that main() already drew
void drawPauseScreen(sf::RenderWindow &w, sf::Font &font)
{
    // semi transparent black layer over the whole window to dim the game
    sf::RectangleShape dim(sf::Vector2f((float)WIN_WIDTH, (float)WIN_HEIGHT));
    dim.setFillColor(sf::Color(0, 0, 0, 155));
    w.draw(dim);

    sf::RectangleShape panel(sf::Vector2f(320.f, 290.f));
    float px = (WIN_WIDTH - 320.f) * 0.5f;
    float py2 = (WIN_HEIGHT - 290.f) * 0.5f;
    panel.setPosition({px, py2});
    panel.setFillColor(sf::Color(45, 45, 75));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color::White);
    w.draw(panel);

    sf::Text pt(font);
    pt.setString("PAUSED");
    pt.setCharacterSize(38);
    pt.setStyle(sf::Text::Bold);
    pt.setFillColor(sf::Color::White);
    sf::FloatRect ptb = pt.getLocalBounds();
    pt.setPosition({(WIN_WIDTH - ptb.size.x) * 0.5f - ptb.position.x, py2 + 18.f});
    w.draw(pt);

    // button positions must match handlePauseClick
    float bx = (WIN_WIDTH - 210.f) * 0.5f, by = py2 + 80.f;
    drawButton(w, bx, by,         210.f, 48.f, "Resume",    font, sf::Color(60, 140, 60));
    drawButton(w, bx, by + 65.f,  210.f, 48.f, "Save Game", font, sf::Color(60,  90, 180));
    drawButton(w, bx, by + 130.f, 210.f, 48.f, "Main Menu", font, sf::Color(150, 60, 60));
}

// screen 5, settings
void drawSettingsScreen(sf::RenderWindow &w, sf::Font &font)
{
    w.clear(sf::Color(25, 25, 55));

    sf::Text title(font);
    title.setString("Settings");
    title.setCharacterSize(50);
    title.setFillColor(sf::Color::White);
    sf::FloatRect tb = title.getLocalBounds();
    title.setPosition({(WIN_WIDTH - tb.size.x) * 0.5f - tb.position.x, 100.f});
    w.draw(title);

    // the label and the colour of the button both follow soundEnabled
    char soundLabel[24];
    snprintf(soundLabel, sizeof(soundLabel), "Sound: %s", soundEnabled ? "ON" : "OFF");
    drawButton(w, (WIN_WIDTH - 220.f) * 0.5f, 240.f, 220.f, 56.f,
               soundLabel, font,
               soundEnabled ? sf::Color(60, 140, 60) : sf::Color(140, 60, 60));

    drawButton(w, (WIN_WIDTH - 220.f) * 0.5f, 380.f, 220.f, 56.f,
               "Back", font, sf::Color(110, 50, 50));
}

// ----------------------------------------------------------------
// mouse input
// ----------------------------------------------------------------

// true if the point (mx, my) is inside the rectangle, used to detect button clicks
bool inRect(float mx, float my, float rx, float ry, float rw, float rh)
{
    return mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh;
}

// finds which of the current player's pieces was clicked, -1 if none
// a click counts when it is within the piece circle (distance check using squared values
// so no square root is needed, the 1.5 multiplier makes the click area a bit forgiving)
int pieceAtPos(float mx, float my)
{
    float r = CELL_SIZE * 0.38f;
    for (int pc = 0; pc < PIECES; pc++) {
        int st = pieceState[currentPlayer][pc];
        if (st == 3)
            continue;

        sf::Vector2f c;
        if      (st == 0) c = baseCentre(currentPlayer, pc);
        else if (st == 1) c = trackCentre(piecePos[currentPlayer][pc]);
        else              c = homeCentre(currentPlayer, piecePos[currentPlayer][pc]);

        float dx = mx - c.x, dy = my - c.y;
        if (dx * dx + dy * dy <= r * r * 1.5f)
            return pc;
    }
    return -1;
}

// menu: start a pvp game, go to colour select for pvc, load a save, or open settings
void handleMenuClick(float mx, float my)
{
    float bw = 300.f, bh = 55.f, bx = (WIN_WIDTH - bw) * 0.5f;

    if (inRect(mx, my, bx, 230.f, bw, bh)) {    // player vs player
        gameMode = 0;
        initializeGame();
        currentScreen = 2;
        logMsg("--- Red's turn ---");
    }
    if (inRect(mx, my, bx, 310.f, bw, bh)) {    // player vs computer
        gameMode = 1;
        currentScreen = 1;
    }
    if (inRect(mx, my, bx, 390.f, bw, bh)) {    // load game
        if (!loadGame())
            logMsg("No save file found.");
    }
    if (inRect(mx, my, bx, 470.f, bw, bh)) {    // settings
        currentScreen = 5;
    }
}

// colour select: the clicked square becomes the human player, then the game starts
void handleColorSelectClick(float mx, float my)
{
    for (int i = 0; i < 4; i++) {
        float bx = 80.f + i * 185.f;
        if (inRect(mx, my, bx, 220.f, 140.f, 140.f)) {
            humanPlayerID = i;
            initializeGame();
            currentScreen = 2;
            logMsg("--- Red's turn ---");
            return;
        }
    }
    if (inRect(mx, my, (WIN_WIDTH - 200.f) * 0.5f, 430.f, 200.f, 50.f))
        currentScreen = 0;
}

// game: pause button, roll button (starts the dice animation) or a piece click
void handleGameClick(float mx, float my)
{
    float cy = ctrlY();

    if (inRect(mx, my, WIN_WIDTH - 115.f, cy, 105.f, 40.f)) {
        currentScreen = 4;
        return;
    }

    // the roll itself is not done here, update() finishes the animation and then calls afterRoll
    if (waitingForRoll && !isAI(currentPlayer) && !diceAnimating) {
        if (inRect(mx, my, 90.f, cy, 160.f, 60.f)) {
            diceAnimating = true;
            diceAnimTime = 0.f;
            diceFrameAcc = 0.f;
            diceAnimClock.restart();
            return;
        }
    }

    // ai pieces cannot be clicked, only a human choosing a piece gets through
    if (waitingForMove && !isAI(currentPlayer)) {
        int pc = pieceAtPos(mx, my);
        if (pc >= 0)
            handlePieceSelected(pc);
    }
}

// pause: resume, save, or quit to menu
void handlePauseClick(float mx, float my)
{
    float bx = (WIN_WIDTH - 210.f) * 0.5f;
    float by = ((WIN_HEIGHT - 290.f) * 0.5f) + 80.f;

    if (inRect(mx, my, bx, by,         210.f, 48.f)) currentScreen = 2;
    if (inRect(mx, my, bx, by + 65.f,  210.f, 48.f)) saveGame();
    if (inRect(mx, my, bx, by + 130.f, 210.f, 48.f)) currentScreen = 0;
}

// settings: sound toggle and back button
void handleSettingsClick(float mx, float my)
{
    if (inRect(mx, my, (WIN_WIDTH - 220.f) * 0.5f, 240.f, 220.f, 56.f))
        soundEnabled = !soundEnabled;
    if (inRect(mx, my, (WIN_WIDTH - 220.f) * 0.5f, 380.f, 220.f, 56.f))
        currentScreen = 0;
}

// win screen: back to menu
void handleWinClick(float mx, float my)
{
    if (inRect(mx, my, (WIN_WIDTH - 260.f) * 0.5f, 440.f, 260.f, 56.f))
        currentScreen = 0;
}

// ----------------------------------------------------------------
// per frame update
// ----------------------------------------------------------------

// runs once per frame with dt = seconds since the last frame
// handles the dice animation (for humans and ai) and the ai turn
void update(float dt)
{
    // ---------- dice animation ----------
    // the face changes every ANIM_FRAME_SPEED seconds, after ANIM_DURATION the real
    // random value is picked and the game continues in afterRoll
    if (diceAnimating) {
        diceAnimTime += dt;
        diceFrameAcc += dt;
        if (diceFrameAcc >= ANIM_FRAME_SPEED) {
            diceFrameAcc -= ANIM_FRAME_SPEED;
            diceAnimFace = (diceAnimFace % 6) + 1;
        }
        if (diceAnimTime >= ANIM_DURATION) {
            diceAnimating = false;
            diceAnimTime = 0.f;
            int rolled = rand() % 6 + 1;
            char msg[64];
            snprintf(msg, sizeof(msg), "%s rolled %d", playerName(currentPlayer), rolled);
            logMsg(msg);
            afterRoll(rolled);
        }
        return;
    }

    if (currentScreen != 2) return;
    if (!isAI(currentPlayer)) return;

    // ---------- ai rolls ----------
    // first frame starts the delay timer, once AI_ROLL_DELAY has passed the same dice animation a human gets is started
    if (waitingForRoll && !aiThinking) {
        aiThinking = true;
        aiClock.restart();
    }
    if (aiThinking && waitingForRoll &&
        aiClock.getElapsedTime().asSeconds() >= AI_ROLL_DELAY)
    {
        aiThinking = false;
        diceAnimating = true;
        diceAnimTime = 0.f;
        diceFrameAcc = 0.f;
        diceAnimClock.restart();
    }

    // ---------- ai moves ----------
    // same idea, a short delay after the roll so the player can see the dice value before the piece moves
    if (waitingForMove && !aiMoving) {
        aiMoving = true;
        aiClock.restart();
    }
    if (aiMoving && waitingForMove &&
        aiClock.getElapsedTime().asSeconds() >= AI_MOVE_DELAY)
    {
        aiDoMove();
    }
}