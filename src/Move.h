#ifndef PROJEKT3_MOVE_H
#define PROJEKT3_MOVE_H

#include "PieceType.h"
#include "Piece.h"

struct Move
{
    int fromX, fromY, toX, toY;
    std::string notation;

    Move() {}

    Move(int fx, int fy, int tx, int ty, std::string notation = "") : fromX(fx), fromY(fy), toX(tx), toY(ty), notation(notation)
    {
    }

    bool operator==(const Move &other) const
    {
        return fromX == other.fromX && fromY == other.fromY && toX == other.toX && toY == other.toY;
    }
};

#endif //PROJEKT3_MOVE_H
