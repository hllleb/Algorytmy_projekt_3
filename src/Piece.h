#ifndef PROJEKT3_PIECE_H
#define PROJEKT3_PIECE_H

#include "PieceType.h"
#include "Color.h"

struct Piece
{
    PieceType type;
    Color color;

    Piece() : type(static_cast<PieceType>(-1)), color(static_cast<Color>(-1))
    {
    }

    Piece(PieceType type, Color color) : type(type), color(color)
    {
    }

    bool operator==(const Piece &other) const
    {
        return type == other.type && color == other.color;
    }

    bool operator!=(const Piece &other) const
    {
        return !(*this == other);
    }
};

#endif //PROJEKT3_PIECE_H
