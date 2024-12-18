#ifndef PLAYER_H
#define PLAYER_H

#define MAX_TRIES 8

typedef struct {
    char guess[5]; // Holds the 4 colors + null terminator
    int nB;        // Correct colors in correct positions (black)
    int nW;        // Correct colors in wrong positions (white)
} Trial;

#endif