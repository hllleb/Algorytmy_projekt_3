#ifndef PROJEKT3_GAMESTATE_H
#define PROJEKT3_GAMESTATE_H

#include "Piece.h"
#include "Move.h"

// Struktura do przechowywania stanu gry przed tymczasowym ruchem
    struct GameState
    {
        Piece capturedPiece;
        bool whiteCanCastleKingside;
        bool whiteCanCastleQueenside;
        bool blackCanCastleKingside;
        bool blackCanCastleQueenside;
        int enPassantTargetX;
        int enPassantTargetY;
        Move move;
        Piece movedPiece;
    };

#endif //PROJEKT3_GAMESTATE_H
