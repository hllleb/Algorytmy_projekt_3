#ifndef PROJEKT3_TRANSPOSITIONENTRY_H
#define PROJEKT3_TRANSPOSITIONENTRY_H

#include "Move.h"

// Struktura dla tabeli transpozycji
struct TranspositionEntry
{
    int value;
    int depth;
    Move bestMove;
};

#endif //PROJEKT3_TRANSPOSITIONENTRY_H
