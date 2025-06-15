#ifndef PROJEKT3_CHESSGAME_H
#define PROJEKT3_CHESSGAME_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include "Piece.h"
#include "Move.h"
#include "Logger.h"

const Piece EMPTY_PIECE = {static_cast<PieceType>(-1), static_cast<Color>(-1)};

class ChessGame
{
private:
    Piece board[8][8];
    Color currentPlayer;
    bool isCheckmate;
    bool isStalemate;
    bool gameOverState;
    bool whiteCanCastleKingside;
    bool whiteCanCastleQueenside;
    bool blackCanCastleKingside;
    bool blackCanCastleQueenside;
    int enPassantTargetX, enPassantTargetY;
    bool isPawnPromotionPending; // Flaga wskazująca na oczekującą promocję
    int promotionX, promotionY; // Współrzędne pola promocji
    PieceType promotionChoice; // Wybrana figura dla promocji
    std::vector<Move> moveHistory;

    //Move lastMove = {-1, -1, -1, -1, false, false, QUEEN, EMPTY_PIECE};
    static const int MAX_DEPTH = 3;
    Logger logger;

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

    // Funkcja do obliczania wartości zdobytej figury dla sortowania ruchów
    int getCaptureValue(const Move &move) const
    {
        Piece target = board[move.toX][move.toY];
        if (target == EMPTY_PIECE)
        {
            return 0;
        }
        switch (target.type)
        {
            case PAWN:
                return 100;
            case KNIGHT:
                return 300;
            case BISHOP:
                return 300;
            case ROOK:
                return 500;
            case QUEEN:
                return 900;
            case KING:
                return 10000;
            default:
                return 0;
        }
    }

    std::string generateAlgebraicNotation(const Move& move, const GameState& state)
    {
        Piece piece = state.movedPiece;
        if (piece == EMPTY_PIECE) return "";

        std::string notation;
        std::string pieceSymbol;
        switch (piece.type) {
            case KNIGHT: pieceSymbol = "N"; break;
            case BISHOP: pieceSymbol = "B"; break;
            case ROOK: pieceSymbol = "R"; break;
            case QUEEN: pieceSymbol = "Q"; break;
            case KING: pieceSymbol = "K"; break;
            default: break;
        }

        if(piece.color == BLACK)
        {
            pieceSymbol[0] = tolower(pieceSymbol[0]);
        }

        notation+= pieceSymbol;

        if (piece.type == KING && abs(move.toY - move.fromY) == 2) {
            return move.toY > move.fromY ? "O-O" : "O-O-O";
        }

        if (piece.type != PAWN) {
            notation += char('a' + move.fromY);
            notation += std::to_string(8 - move.fromX);
        } else if (state.capturedPiece != EMPTY_PIECE || (move.toX == state.enPassantTargetX && move.toY == state.enPassantTargetY)) {
            notation += char('a' + move.fromY);
        }

        if (state.capturedPiece != EMPTY_PIECE || (piece.type == PAWN && move.toX == state.enPassantTargetX && move.toY == state.enPassantTargetY)) {
            notation += "x";
        }

        notation += char('a' + move.toY);
        notation += std::to_string(8 - move.toX);

        bool isPromotion = (piece.type == PAWN && (move.toX == 0 || move.toX == 7));
        std::string promotionNotation;
        if (isPromotion) {
            promotionNotation += "=";
            pieceSymbol = "";
            switch (board[move.toX][move.toY].type) {
                case QUEEN: pieceSymbol = "Q"; break;
                case ROOK: pieceSymbol = "R"; break;
                case BISHOP: pieceSymbol = "B"; break;
                case KNIGHT: pieceSymbol = "N"; break;
                default: break;
            }

            if(board[move.toX][move.toY].color == BLACK)
            {
                pieceSymbol[0] = tolower(pieceSymbol[0]);
            }

            promotionNotation += pieceSymbol;
        }

        // Sprawdzenie szacha/mata bez modyfikacji historii
        Piece tempTo = board[move.toX][move.toY];
        Piece tempFrom = board[move.fromX][move.fromY];
        board[move.toX][move.toY] = tempFrom;
        board[move.fromX][move.fromY] = EMPTY_PIECE;
        bool isCheck = isInCheck(currentPlayer == WHITE ? BLACK : WHITE);
        bool isMate = isCheck && getAllPossibleMoves(currentPlayer == WHITE ? BLACK : WHITE).empty();
        board[move.fromX][move.fromY] = tempFrom;
        board[move.toX][move.toY] = tempTo;

        if (isCheck) {
            notation += isMate ? "#" : "+";
        }

        return notation + promotionNotation;
    }

public:
    ChessGame() : currentPlayer(WHITE), isCheckmate(false), isStalemate(false), gameOverState(false),
                  whiteCanCastleKingside(true), whiteCanCastleQueenside(true), blackCanCastleKingside(true),
                  blackCanCastleQueenside(true), enPassantTargetX(-1), enPassantTargetY(-1),
                  isPawnPromotionPending(false), promotionX(-1), promotionY(-1), promotionChoice(QUEEN),
                  logger("chess_log.txt")
    {
        initializeBoard();
    }

    int getEnPassantTargetX()
    {
        return enPassantTargetX;
    };

    int getEnPassantTargetY()
    {
        return enPassantTargetY;
    };

    void initializeBoard()
    {
        for (int i = 2; i <= 5; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                board[i][j] = EMPTY_PIECE;
            }
        }

        for (int i = 0; i < 8; i++)
        {
            board[1][i] = Piece(PAWN, BLACK);
            board[6][i] = Piece(PAWN, WHITE);
        }

        board[0][0] = board[0][7] = Piece(ROOK, BLACK);
        board[0][1] = board[0][6] = Piece(KNIGHT, BLACK);
        board[0][2] = board[0][5] = Piece(BISHOP, BLACK);
        board[0][3] = Piece(QUEEN, BLACK);
        board[0][4] = Piece(KING, BLACK);

        board[7][0] = board[7][7] = Piece(ROOK, WHITE);
        board[7][1] = board[7][6] = Piece(KNIGHT, WHITE);
        board[7][2] = board[7][5] = Piece(BISHOP, WHITE);
        board[7][3] = Piece(QUEEN, WHITE);
        board[7][4] = Piece(KING, WHITE);
    }

    Piece getPiece(int x, int y) const
    {
        if (x >= 0 && x < 8 && y >= 0 && y < 8)
        {
            return board[x][y];
        }

        return EMPTY_PIECE;
    }

    static bool isValidPosition(int x, int y)
    {
        return x >= 0 && x < 8 && y >= 0 && y < 8;
    }

    bool isInCheck(Color color) // const
    {
        int kingX = -1, kingY = -1;
        bool kingFound = false;
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                if (board[i][j] != EMPTY_PIECE && board[i][j].type == KING && board[i][j].color == color)
                {
                    kingX = i;
                    kingY = j;
                    kingFound = true;
                    break;
                }
            }
        }

        if (!kingFound)
        {
            logger.log("King not found for color " + std::to_string(color) + "! Assuming checkmate.", Logger::ERROR);
            return true; // Brak króla = automatyczny mat
        }

        Color opponent = (color == WHITE) ? BLACK : WHITE;
        for (int x = 0; x < 8; ++x)
        {
            for (int y = 0; y < 8; ++y)
            {
                if (board[x][y] != EMPTY_PIECE && board[x][y].color == opponent)
                {
                    Move move = {x, y, kingX, kingY};
                    // Tymczasowo odblokuj sprawdzanie ruchu dla przeciwnika, aby wykryć atak na króla
                    if (isValidAttackMove(move, opponent))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Nowa funkcja do sprawdzania ataku, bez sprawdzania szacha
    bool isValidAttackMove(const Move &move, Color player) const
    {
        if (!isValidPosition(move.fromX, move.fromY) || !isValidPosition(move.toX, move.toY))
        {
            return false;
        }

        Piece piece = board[move.fromX][move.fromY];
        if (piece == EMPTY_PIECE || piece.color != player)
        {
            return false;
        }

        Piece target = board[move.toX][move.toY];
        if (target != EMPTY_PIECE && target.color == piece.color)
        {
            return false;
        }

        bool validPieceMove = false;
        switch (piece.type)
        {
            case PAWN:
            {
                int direction = (piece.color == WHITE) ? -1 : 1;
                if (abs(move.fromY - move.toY) == 1 && move.toX == move.fromX + direction)
                {
                    validPieceMove = true;
                }
                break;
            }
            case KNIGHT:
            {
                int dx = abs(move.toX - move.fromX);
                int dy = abs(move.toY - move.fromY);
                validPieceMove = (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
                break;
            }
            case BISHOP:
            {
                if (abs(move.toX - move.fromX) == abs(move.toY - move.fromY))
                {
                    validPieceMove = isPathClear(move.fromX, move.fromY, move.toX, move.toY);
                }
                break;
            }
            case ROOK:
            {
                if (move.toX == move.fromX || move.toY == move.fromY)
                {
                    validPieceMove = isPathClear(move.fromX, move.fromY, move.toX, move.toY);
                }
                break;
            }
            case QUEEN:
            {
                if (move.toX == move.fromX || move.toY == move.fromY ||
                    abs(move.toX - move.fromX) == abs(move.toY - move.fromY))
                {
                    validPieceMove = isPathClear(move.fromX, move.fromY, move.toX, move.toY);
                }
                break;
            }
            case KING:
            {
                int dx = abs(move.toX - move.fromX);
                int dy = abs(move.toY - move.fromY);
                validPieceMove = (dx <= 1 && dy <= 1);
                break;
            }
            default:
                return false;
        }

        return validPieceMove;
    }

    bool isPathClear(int fromX, int fromY, int toX, int toY) const
    {
        int dx = (toX == fromX) ? 0 : (toX > fromX ? 1 : -1);
        int dy = (toY == fromY) ? 0 : (toY > fromY ? 1 : -1);
        int x = fromX + dx;
        int y = fromY + dy;

        while (x != toX || y != toY)
        {
            if (!isValidPosition(x, y) || board[x][y] != EMPTY_PIECE)
            {
                return false;
            }
            x += dx;
            y += dy;
        }
        return true;
    }


    bool isValidMove(const Move &move, Color player)
    {
        if (!isValidPosition(move.fromX, move.fromY) || !isValidPosition(move.toX, move.toY))
        {
            return false;
        }

        Piece piece = board[move.fromX][move.fromY];
        if (piece == EMPTY_PIECE || piece.color != currentPlayer)
        {
            return false;
        }

        if (piece.color != player)
        {
            return false;
        }

        Piece target = board[move.toX][move.toY];
        if (target != EMPTY_PIECE)
        {
            if (target.color == piece.color)
            {
                return false;
            }
            if (target.type == KING)
            {
                return false;
            }
        }

        // Sprawdź poprawność ruchu dla danej figury
        bool validPieceMove = false;
        switch (piece.type)
        {
            case PAWN:
            {
                int direction = (piece.color == WHITE) ? -1 : 1;
                int startRow = (piece.color == WHITE) ? 6 : 1;

                if (move.fromY == move.toY && move.toX == move.fromX + direction && target == EMPTY_PIECE)
                {
                    validPieceMove = true;
                }

                if (move.fromY == move.toY && move.fromX == startRow && move.toX == move.fromX + 2 * direction &&
                    target == EMPTY_PIECE && board[move.fromX + direction][move.fromY] == EMPTY_PIECE)
                {
                    validPieceMove = true;
                }

                if (abs(move.fromY - move.toY) == 1 && move.toX == move.fromX + direction &&
                    (target != EMPTY_PIECE || (move.toX == enPassantTargetX && move.toY == enPassantTargetY)))
                {
                    validPieceMove = true;
                }

                break;
            }
            case KNIGHT:
            {
                int dx = abs(move.toX - move.fromX);
                int dy = abs(move.toY - move.fromY);
                validPieceMove = (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
                break;
            }
            case BISHOP:
            {
                if (abs(move.toX - move.fromX) == abs(move.toY - move.fromY))
                {
                    validPieceMove = isPathClear(move.fromX, move.fromY, move.toX, move.toY);
                }

                break;
            }
            case ROOK:
            {
                if (move.toX == move.fromX || move.toY == move.fromY)
                {
                    validPieceMove = isPathClear(move.fromX, move.fromY, move.toX, move.toY);
                }

                break;
            }
            case QUEEN:
            {
                if (move.toX == move.fromX || move.toY == move.fromY ||
                    abs(move.toX - move.fromX) == abs(move.toY - move.fromY))
                {
                    validPieceMove = isPathClear(move.fromX, move.fromY, move.toX, move.toY);
                }

                break;
            }
            case KING:
            {
                int dx = abs(move.toX - move.fromX);
                int dy = abs(move.toY - move.fromY);
                if (dx <= 1 && dy <= 1)
                {
                    validPieceMove = true;
                }

                if (piece.color == WHITE && move.fromX == 7 && move.fromY == 4)
                {
                    if (move.toX == 7 && move.toY == 6 && whiteCanCastleKingside && board[7][5] == EMPTY_PIECE &&
                        board[7][6] == EMPTY_PIECE && board[7][7].type == ROOK && board[7][7].color == WHITE &&
                        !isInCheck(WHITE))
                    {
                        // Sprawdź, czy pola pośrednie są szachowane
                        Piece temp = board[7][5];
                        board[7][5] = board[7][4];
                        bool check1 = isInCheck(WHITE);
                        board[7][5] = temp;
                        if (check1)
                        {
                            return false;
                        }

                        temp = board[7][6];
                        board[7][6] = board[7][4];
                        bool check2 = isInCheck(WHITE);
                        board[7][6] = temp;
                        if (check2)
                        {
                            return false;
                        }

                        validPieceMove = true;
                    }
                    else if (move.toX == 7 && move.toY == 2 && whiteCanCastleQueenside && board[7][1] == EMPTY_PIECE &&
                             board[7][2] == EMPTY_PIECE && board[7][3] == EMPTY_PIECE && board[7][0].type == ROOK &&
                             board[7][0].color == WHITE && !isInCheck(WHITE))
                    {
                        Piece temp = board[7][3];
                        board[7][3] = board[7][4];
                        bool check1 = isInCheck(WHITE);
                        board[7][3] = temp;
                        if (check1)
                        {
                            return false;
                        }

                        temp = board[7][2];
                        board[7][2] = board[7][4];
                        bool check2 = isInCheck(WHITE);
                        board[7][2] = temp;
                        if (check2)
                        {
                            return false;
                        }

                        validPieceMove = true;
                    }
                }
                else if (piece.color == BLACK && move.fromX == 0 && move.fromY == 4)
                {
                    if (move.toX == 0 && move.toY == 6 && blackCanCastleKingside && board[0][5] == EMPTY_PIECE &&
                        board[0][6] == EMPTY_PIECE && board[0][7].type == ROOK && board[0][7].color == BLACK &&
                        !isInCheck(BLACK))
                    {
                        Piece temp = board[0][5];
                        board[0][5] = board[0][4];
                        bool check1 = isInCheck(BLACK);
                        board[0][5] = temp;
                        if (check1)
                        {
                            return false;
                        }

                        temp = board[0][6];
                        board[0][6] = board[0][4];
                        bool check2 = isInCheck(BLACK);
                        board[0][6] = temp;
                        if (check2)
                        {
                            return false;
                        }

                        validPieceMove = true;
                    }
                    else if (move.toX == 0 && move.toY == 2 && blackCanCastleQueenside && board[0][1] == EMPTY_PIECE &&
                             board[0][2] == EMPTY_PIECE && board[0][3] == EMPTY_PIECE && board[0][0].type == ROOK &&
                             board[0][0].color == BLACK && !isInCheck(BLACK))
                    {
                        Piece temp = board[0][3];
                        board[0][3] = board[0][4];
                        bool check1 = isInCheck(BLACK);
                        board[0][3] = temp;
                        if (check1)
                        {
                            return false;
                        }

                        temp = board[0][2];
                        board[0][2] = board[0][4];
                        bool check2 = isInCheck(BLACK);
                        board[0][2] = temp;
                        if (check2)
                        {
                            return false;
                        }

                        validPieceMove = true;
                    }
                }

                break;
            }
            default:
                return false;
        }

        if (!validPieceMove)
        {
            return false;
        }

        // Sprawdź, czy ruch nie zostawia króla w szachu
        Piece capturedPiece = board[move.toX][move.toY];
        board[move.toX][move.toY] = board[move.fromX][move.fromY];
        board[move.fromX][move.fromY] = EMPTY_PIECE;

        bool inCheck = isInCheck(player);

        // Przywróć stan planszy
        board[move.fromX][move.fromY] = board[move.toX][move.toY];
        board[move.toX][move.toY] = capturedPiece;

        return !inCheck;
    }

    void makeMove(const Move &move)
    {
        if (!isValidMove(move, currentPlayer))
        {
            logger.log("Invalid move attempted!", Logger::ERROR);
            return;
        }

        GameState state{board[move.toX][move.toY], whiteCanCastleKingside, whiteCanCastleQueenside,
                        blackCanCastleKingside, blackCanCastleQueenside, enPassantTargetX, enPassantTargetY, move,
                        board[move.fromX][move.fromY]};

        Piece piece = board[move.fromX][move.fromY];
        int direction = (piece.color == WHITE) ? -1 : 1;
        int startRow = (piece.color == WHITE) ? 6 : 1;

        if (piece.type == KING)
        {
            if (piece.color == WHITE)
            {
                whiteCanCastleKingside = false;
                whiteCanCastleQueenside = false;
            }
            else
            {
                blackCanCastleKingside = false;
                blackCanCastleQueenside = false;
            }
        }
        if (piece.type == ROOK)
        {
            if (piece.color == WHITE)
            {
                if (move.fromX == 7 && move.fromY == 0)
                {
                    whiteCanCastleQueenside = false;
                }
                if (move.fromX == 7 && move.fromY == 7)
                {
                    whiteCanCastleKingside = false;
                }
            }
            else
            {
                if (move.fromX == 0 && move.fromY == 0)
                {
                    blackCanCastleQueenside = false;
                }
                if (move.fromX == 0 && move.fromY == 7)
                {
                    blackCanCastleKingside = false;
                }
            }
        }

        enPassantTargetX = -1;
        enPassantTargetY = -1;
        if (piece.type == PAWN && move.fromX == startRow && abs(move.toX - move.fromX) == 2)
        {
            enPassantTargetX = move.fromX + direction;
            enPassantTargetY = move.fromY;
        }
        if (piece.type == PAWN && move.toX == enPassantTargetX && move.toY == enPassantTargetY)
        {
            board[move.toX - direction][move.toY] = EMPTY_PIECE;
        }

        if (piece.type == KING && abs(move.toY - move.fromY) == 2)
        {
            if (move.toY > move.fromY)
            {
                board[move.fromX][move.fromY + 1] = board[move.fromX][7];
                board[move.fromX][7] = EMPTY_PIECE;
            }
            else
            {
                board[move.fromX][move.fromY - 1] = board[move.fromX][0];
                board[move.fromX][0] = EMPTY_PIECE;
            }
        }

        if (piece.type == PAWN && (move.toX == 0 || move.toX == 7))
        {
            isPawnPromotionPending = true;
            promotionX = move.toX;
            promotionY = move.toY;
        }

        board[move.toX][move.toY] = board[move.fromX][move.fromY];
        board[move.fromX][move.fromY] = EMPTY_PIECE;

        std::string notation = generateAlgebraicNotation(move, state);
        moveHistory.emplace_back(move.fromX, move.fromY, move.toX, move.toY, notation);
        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
        checkGameState();
    }

    // Wykonaj tymczasowy ruch i zwróć stan gry
    GameState makeTemporaryMove(const Move &move)
    {
        GameState state{board[move.toX][move.toY], whiteCanCastleKingside, whiteCanCastleQueenside,
                        blackCanCastleKingside, blackCanCastleQueenside, enPassantTargetX, enPassantTargetY, move,
                        board[move.fromX][move.fromY]};

        int direction = (state.movedPiece.color == WHITE) ? -1 : 1;
        int startRow = (state.movedPiece.color == WHITE) ? 6 : 1;

        if (state.movedPiece.type == KING)
        {
            if (state.movedPiece.color == WHITE)
            {
                whiteCanCastleKingside = false;
                whiteCanCastleQueenside = false;
            }
            else
            {
                blackCanCastleKingside = false;
                blackCanCastleQueenside = false;
            }
        }
        if (state.movedPiece.type == ROOK)
        {
            if (state.movedPiece.color == WHITE)
            {
                if (move.fromX == 7 && move.fromY == 0)
                {
                    whiteCanCastleQueenside = false;
                }
                if (move.fromX == 7 && move.fromY == 7)
                {
                    whiteCanCastleKingside = false;
                }
            }
            else
            {
                if (move.fromX == 0 && move.fromY == 0)
                {
                    blackCanCastleQueenside = false;
                }
                if (move.fromX == 0 && move.fromY == 7)
                {
                    blackCanCastleKingside = false;
                }
            }
        }

        enPassantTargetX = -1;
        enPassantTargetY = -1;
        if (state.movedPiece.type == PAWN && move.fromX == startRow && abs(move.toX - move.fromX) == 2)
        {
            enPassantTargetX = move.fromX + direction;
            enPassantTargetY = move.fromY;
        }
        if (state.movedPiece.type == PAWN && move.toX == state.enPassantTargetX && move.toY == state.enPassantTargetY)
        {
            board[move.toX - direction][move.toY] = EMPTY_PIECE;
        }

        if (state.movedPiece.type == KING && abs(move.toY - move.fromY) == 2)
        {
            if (move.toY > move.fromY)
            {
                board[move.fromX][move.fromY + 1] = board[move.fromX][7];
                board[move.fromX][7] = EMPTY_PIECE;
            }
            else
            {
                board[move.fromX][move.fromY - 1] = board[move.fromX][0];
                board[move.fromX][0] = EMPTY_PIECE;
            }
        }

        board[move.toX][move.toY] = board[move.fromX][move.fromY];
        board[move.fromX][move.fromY] = EMPTY_PIECE;

        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
        return state;
    }

    // Cofnij tymczasowy ruch
    void undoMove(const GameState &state)
    {
        board[state.move.fromX][state.move.fromY] = state.movedPiece;
        board[state.move.toX][state.move.toY] = state.capturedPiece;

        if (state.movedPiece.type == PAWN && state.move.toX == state.enPassantTargetX &&
            state.move.toY == state.enPassantTargetY)
        {
            int direction = (state.movedPiece.color == WHITE) ? 1 : -1;
            board[state.move.toX - direction][state.move.toY] = Piece(PAWN, currentPlayer == WHITE ? BLACK : WHITE);
        }

        if (state.movedPiece.type == KING && abs(state.move.toY - state.move.fromY) == 2)
        {
            if (state.move.toY > state.move.fromY)
            {
                board[state.move.fromX][7] = board[state.move.fromX][state.move.fromY + 1];
                board[state.move.fromX][state.move.fromY + 1] = EMPTY_PIECE;
            }
            else
            {
                board[state.move.fromX][0] = board[state.move.fromX][state.move.fromY - 1];
                board[state.move.fromX][state.move.fromY - 1] = EMPTY_PIECE;
            }
        }

        whiteCanCastleKingside = state.whiteCanCastleKingside;
        whiteCanCastleQueenside = state.whiteCanCastleQueenside;
        blackCanCastleKingside = state.blackCanCastleKingside;
        blackCanCastleQueenside = state.blackCanCastleQueenside;
        enPassantTargetX = state.enPassantTargetX;
        enPassantTargetY = state.enPassantTargetY;

        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
    }

    bool isPromotionPending() const
    {
        return isPawnPromotionPending;
    }

    void promotePawn(PieceType type)
    {
        if (isPawnPromotionPending && (type == QUEEN || type == ROOK || type == BISHOP || type == KNIGHT))
        {
            board[promotionX][promotionY] = Piece(type, currentPlayer == WHITE ? BLACK : WHITE);
            logger.log("Pawn promoted to " + std::to_string(type), Logger::INFO);
            isPawnPromotionPending = false;
            promotionX = -1;
            promotionY = -1;
            checkGameState();
        }
    }

    bool isGameOver() const
    {
        return gameOverState;
    }

    bool isCheckmateState() const
    {
        return isCheckmate;
    }

    bool isStalemateState() const
    {
        return isStalemate;
    }

    void checkGameState()
    {
        // Sprawdź, czy król istnieje
        int pieceCount = 0;
        int kingCount = 0;
        Color missingKingColor = static_cast<Color>(-1); // Domyślnie, dla kompilacji
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                if (board[i][j] != EMPTY_PIECE)
                {
                    pieceCount++;
                    if (board[i][j].type == KING)
                    {
                        kingCount++;
                        if (board[i][j].color == WHITE)
                        {
                            missingKingColor = BLACK;
                        }
                        else
                        {
                            missingKingColor = WHITE;
                        }
                    }
                }
            }
        }

        if (pieceCount == 2 && kingCount == 2)
        {
            isCheckmate = false;
            isStalemate = true;
            gameOverState = true;
            logger.log("Draw! Only two kings remain.", Logger::INFO);
            return;
        }

        if (kingCount < 2)
        {
            isCheckmate = true;
            isStalemate = false;
            gameOverState = true;
            logger.log(std::string(missingKingColor == WHITE ? "Black" : "White") + " wins by capturing king!",
                       Logger::SUCCESS);
            return;
        }

        std::vector<Move> moves = getAllPossibleMoves(currentPlayer);
        bool inCheck = isInCheck(currentPlayer);
        if (moves.empty())
        {
            if (inCheck)
            {
                isCheckmate = true;
                isStalemate = false;
                gameOverState = true;
                logger.log(currentPlayer == WHITE ? "Black wins by checkmate!" : "White wins by checkmate!",
                           Logger::SUCCESS);
            }
            else
            {
                isCheckmate = false;
                isStalemate = true;
                gameOverState = true;
                logger.log("Stalemate! Game is a draw.", Logger::INFO);
            }
        }
        else if (inCheck)
        {
            logger.log(currentPlayer == WHITE ? "White is in check!" : "Black is in check!", Logger::WARN);
        }
    }

    void resetGame()
    {
        initializeBoard();
        currentPlayer = WHITE;
        isCheckmate = false;
        isStalemate = false;
        gameOverState = false;
        whiteCanCastleKingside = true;
        whiteCanCastleQueenside = true;
        blackCanCastleKingside = true;
        blackCanCastleQueenside = true;
        enPassantTargetX = -1;
        enPassantTargetY = -1;
        isPawnPromotionPending = false;
        promotionX = -1;
        promotionY = -1;
        promotionChoice = QUEEN;
        moveHistory.clear();
        logger.log("Game reset.", Logger::INFO);
    }

    const std::vector<Move> &getMoveHistory() const
    {
        return moveHistory;
    }

    std::vector<Move> getAllPossibleMoves(Color player)
    {
        std::vector<Move> moves;
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                if (board[i][j].color != player)
                {
                    continue;
                }

                Piece piece = board[i][j];
                switch (piece.type)
                {
                    case PAWN:
                    {
                        int direction = (piece.color == WHITE) ? -1 : 1;
                        int startRow = (piece.color == WHITE) ? 6 : 1;

                        // Ruch o jedno pole do przodu
                        if (isValidPosition(i + direction, j))
                        {
                            Move move(i, j, i + direction, j);
                            if (isValidMove(move, player))
                            {
                                moves.push_back(move);
                            }
                        }

                        // Ruch o dwa pola do przodu
                        if (i == startRow && isValidPosition(i + 2 * direction, j))
                        {
                            Move move(i, j, i + 2 * direction, j);
                            if (isValidMove(move, player))
                            {
                                moves.push_back(move);
                            }
                        }

                        // Bicie w lewo
                        if (isValidPosition(i + direction, j - 1))
                        {
                            Move move(i, j, i + direction, j - 1);
                            if (isValidMove(move, player))
                            {
                                moves.push_back(move);
                            }
                        }

                        // Bicie w prawo
                        if (isValidPosition(i + direction, j + 1))
                        {
                            Move move(i, j, i + direction, j + 1);
                            if (isValidMove(move, player))
                            {
                                moves.push_back(move);
                            }
                        }
                        break;
                    }
                    case KNIGHT:
                    {
                        int knightMoves[8][2] = {{-2, -1},
                                                 {-2, 1},
                                                 {-1, -2},
                                                 {-1, 2},
                                                 {1,  -2},
                                                 {1,  2},
                                                 {2,  -1},
                                                 {2,  1}};
                        for (const auto &m: knightMoves)
                        {
                            int ni = i + m[0], nj = j + m[1];
                            if (isValidPosition(ni, nj))
                            {
                                Move move(i, j, ni, nj);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                            }
                        }
                        break;
                    }
                    case BISHOP:
                    {
                        for (int d = -1; d <= 1; d += 2)
                        {
                            for (int e = -1; e <= 1; e += 2)
                            {
                                for (int k = 1; k < 8; k++)
                                {
                                    int ni = i + k * d, nj = j + k * e;
                                    if (!isValidPosition(ni, nj))
                                    {
                                        break;
                                    }
                                    Move move(i, j, ni, nj);
                                    if (isValidMove(move, player))
                                    {
                                        moves.push_back(move);
                                    }
                                    if (board[ni][nj] != EMPTY_PIECE)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case ROOK:
                    {
                        for (int d = -1; d <= 1; d += 2)
                        {
                            for (int k = 1; k < 8; k++)
                            {
                                int ni = i + k * d, nj = j;
                                if (!isValidPosition(ni, nj))
                                {
                                    break;
                                }
                                Move move(i, j, ni, nj);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                                if (board[ni][nj] != EMPTY_PIECE)
                                {
                                    break;
                                }
                            }
                            for (int k = 1; k < 8; k++)
                            {
                                int ni = i, nj = j + k * d;
                                if (!isValidPosition(ni, nj))
                                {
                                    break;
                                }
                                Move move(i, j, ni, nj);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                                if (board[ni][nj] != EMPTY_PIECE)
                                {
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    case QUEEN:
                    {
                        for (int d = -1; d <= 1; d += 2)
                        {
                            for (int e = -1; e <= 1; e += 2)
                            {
                                for (int k = 1; k < 8; k++)
                                {
                                    int ni = i + k * d, nj = j + k * e;
                                    if (!isValidPosition(ni, nj))
                                    {
                                        break;
                                    }
                                    Move move(i, j, ni, nj);
                                    if (isValidMove(move, player))
                                    {
                                        moves.push_back(move);
                                    }
                                    if (board[ni][nj] != EMPTY_PIECE)
                                    {
                                        break;
                                    }
                                }
                            }
                            for (int k = 1; k < 8; k++)
                            {
                                int ni = i + k * d, nj = j;
                                if (!isValidPosition(ni, nj))
                                {
                                    break;
                                }
                                Move move(i, j, ni, nj);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                                if (board[ni][nj] != EMPTY_PIECE)
                                {
                                    break;
                                }
                            }
                            for (int k = 1; k < 8; k++)
                            {
                                int ni = i, nj = j + k * d;
                                if (!isValidPosition(ni, nj))
                                {
                                    break;
                                }
                                Move move(i, j, ni, nj);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                                if (board[ni][nj] != EMPTY_PIECE)
                                {
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    case KING:
                    {
                        for (int di = -1; di <= 1; di++)
                        {
                            for (int dj = -1; dj <= 1; dj++)
                            {
                                if (di == 0 && dj == 0)
                                {
                                    continue;
                                }
                                int ni = i + di, nj = j + dj;
                                if (isValidPosition(ni, nj))
                                {
                                    Move move(i, j, ni, nj);
                                    if (isValidMove(move, player))
                                    {
                                        moves.push_back(move);
                                    }
                                }
                            }
                        }
                        // Roszada
                        if (piece.color == WHITE && i == 7 && j == 4)
                        {
                            if (whiteCanCastleKingside)
                            {
                                Move move(7, 4, 7, 6);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                            }
                            if (whiteCanCastleQueenside)
                            {
                                Move move(7, 4, 7, 2);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                            }
                        }
                        else if (piece.color == BLACK && i == 0 && j == 4)
                        {
                            if (blackCanCastleKingside)
                            {
                                Move move(0, 4, 0, 6);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                            }
                            if (blackCanCastleQueenside)
                            {
                                Move move(0, 4, 0, 2);
                                if (isValidMove(move, player))
                                {
                                    moves.push_back(move);
                                }
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        return moves;
    }

    int evaluateBoard()
    {
        int score = 0;
        int mobilityScore = 0;
        int centerControl = 0;
        int kingPenalty = 0;

        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                Piece piece = board[i][j];
                if (piece == EMPTY_PIECE)
                {
                    continue;
                }

                int value = 0;
                switch (piece.type)
                {
                    case PAWN:
                        value = 100;
                        break;
                    case KNIGHT:
                        value = 320;
                        break;
                    case BISHOP:
                        value = 330;
                        break;
                    case ROOK:
                        value = 500;
                        break;
                    case QUEEN:
                        value = 900;
                        break;
                    case KING:
                        value = 20000;
                        break;
                    default:
                        value = 0;
                }

                // Kontrola centrum (pola d4, d5, e4, e5)
                if ((i == 3 || i == 4) && (j == 3 || j == 4))
                {
                    centerControl += (piece.type == PAWN ? 10 : 20);
                }

                if (piece.type == KING)
                {
                    // Kara za wczesne ruchy królem w centrum
                    if (piece.color == WHITE && i < 7 && moveHistory.size() < 10)
                    {
                        kingPenalty -= 50;
                    }
                    else if (piece.color == BLACK && i > 0 && moveHistory.size() < 10)
                    {
                        kingPenalty -= 50;
                    }
                }

                score += (piece.color == WHITE) ? (value + centerControl + kingPenalty) : -(value + centerControl +
                                                                                            kingPenalty);
                centerControl = 0; // Reset dla następnej figury
                kingPenalty = 0;
            }
        }

        // Mobilność
        mobilityScore += getAllPossibleMoves(WHITE).size() * 5;
        mobilityScore -= getAllPossibleMoves(BLACK).size() * 5;

        return score + mobilityScore;
    }

    int minimax(int depth, int alpha, int beta, bool maximizingPlayer)
    {
        if (depth == 0 || isGameOver())
        {
            return evaluateBoard();
        }

        std::vector<Move> moves = getAllPossibleMoves(
                maximizingPlayer ? currentPlayer : (currentPlayer == WHITE ? BLACK : WHITE));
        if (moves.empty())
        {
            if (isInCheck(maximizingPlayer ? currentPlayer : (currentPlayer == WHITE ? BLACK : WHITE)))
            {
                return maximizingPlayer ? INT_MIN : INT_MAX;
            }
            return 0; // Pat
        }

        if (maximizingPlayer)
        {
            int maxEval = INT_MIN;
            for (const Move &move: moves)
            {
                Piece temp = board[move.toX][move.toY];
                board[move.toX][move.toY] = board[move.fromX][move.fromY];
                board[move.fromX][move.fromY] = EMPTY_PIECE;

                // Obsługa promocji w minimax
                PieceType originalPromotion = promotionChoice;
                bool wasPromotion = false;
                if (board[move.toX][move.toY].type == PAWN && (move.toX == 0 || move.toX == 7))
                {
                    wasPromotion = true;
                    int bestPromotionEval = INT_MIN;
                    PieceType bestPromotion = QUEEN;
                    PieceType options[] = {QUEEN, ROOK, BISHOP, KNIGHT};
                    for (PieceType option: options)
                    {
                        board[move.toX][move.toY] = Piece(option, board[move.toX][move.toY].color);
                        int eval = minimax(depth - 1, alpha, beta, false);
                        if (eval > bestPromotionEval)
                        {
                            bestPromotionEval = eval;
                            bestPromotion = option;
                        }
                    }
                    board[move.toX][move.toY] = Piece(bestPromotion, board[move.toX][move.toY].color);
                    maxEval = std::max(maxEval, bestPromotionEval);
                    alpha = std::max(alpha, bestPromotionEval);
                }
                else
                {
                    int eval = minimax(depth - 1, alpha, beta, false);
                    maxEval = std::max(maxEval, eval);
                    alpha = std::max(alpha, eval);
                }

                board[move.fromX][move.fromY] = board[move.toX][move.toY];
                board[move.toX][move.toY] = temp;
                promotionChoice = originalPromotion;

                if (wasPromotion)
                {
                    board[move.toX][move.toY] = Piece(PAWN, board[move.fromX][move.fromY].color);
                }

                if (beta <= alpha)
                {
                    break;
                }
            }
            return maxEval;
        }
        else
        {
            int minEval = INT_MAX;
            for (const Move &move: moves)
            {
                Piece temp = board[move.toX][move.toY];
                board[move.toX][move.toY] = board[move.fromX][move.fromY];
                board[move.fromX][move.fromY] = EMPTY_PIECE;

                // Obsługa promocji w minimax
                PieceType originalPromotion = promotionChoice;
                bool wasPromotion = false;
                if (board[move.toX][move.toY].type == PAWN && (move.toX == 0 || move.toX == 7))
                {
                    wasPromotion = true;
                    int bestPromotionEval = INT_MAX;
                    PieceType bestPromotion = QUEEN;
                    PieceType options[] = {QUEEN, ROOK, BISHOP, KNIGHT};
                    for (PieceType option: options)
                    {
                        board[move.toX][move.toY] = Piece(option, board[move.toX][move.toY].color);
                        int eval = minimax(depth - 1, alpha, beta, true);
                        if (eval < bestPromotionEval)
                        {
                            bestPromotionEval = eval;
                            bestPromotion = option;
                        }
                    }
                    board[move.toX][move.toY] = Piece(bestPromotion, board[move.toX][move.toY].color);
                    minEval = std::min(minEval, bestPromotionEval);
                    beta = std::min(beta, bestPromotionEval);
                }
                else
                {
                    int eval = minimax(depth - 1, alpha, beta, true);
                    minEval = std::min(minEval, eval);
                    beta = std::min(beta, eval);
                }

                board[move.fromX][move.fromY] = board[move.toX][move.toY];
                board[move.toX][move.toY] = temp;
                promotionChoice = originalPromotion;

                if (wasPromotion)
                {
                    board[move.toX][move.toY] = Piece(PAWN, board[move.fromX][move.fromY].color);
                }

                if (beta <= alpha)
                {
                    break;
                }
            }
            return minEval;
        }
    }

    Move getBestMove(int depth)
    {
        if (isGameOver())
        {
            return Move(-1, -1, -1, -1);
        }

        std::vector<Move> moves = getAllPossibleMoves(currentPlayer);

        if (moves.empty())
        {
            return Move(-1, -1, -1, -1);
        }

        // Sortuj ruchy: najpierw bicie figur (od najcenniejszych), potem reszta
        std::sort(moves.begin(), moves.end(), [this](const Move &a, const Move &b)
        {
            return getCaptureValue(a) > getCaptureValue(b);
        });

        Move bestMove = moves[0];
        int bestValue = INT_MIN;
        int alpha = INT_MIN;
        int beta = INT_MAX;
        PieceType bestPromotion = QUEEN;

        for (const Move &move: moves)
        {
            Piece temp = board[move.toX][move.toY];
            Piece movingPiece = board[move.fromX][move.fromY];
            board[move.toX][move.toY] = movingPiece;
            board[move.fromX][move.fromY] = EMPTY_PIECE;


            int moveValue = INT_MIN;
            bool isPromotionMove = (movingPiece.type == PAWN && (move.toX == 0 || move.toX == 7));
            if (isPromotionMove)
            {
                int bestPromotionValue = INT_MIN;
                PieceType options[] = {QUEEN, ROOK, BISHOP, KNIGHT};
                for (PieceType option: options)
                {
                    board[move.toX][move.toY] = Piece(option, movingPiece.color);
                    int promotionValue = minimax(depth - 1, alpha, beta, false);
                    if (promotionValue > bestPromotionValue)
                    {
                        bestPromotionValue = promotionValue;
                        bestPromotion = option;
                    }
                }
                moveValue = bestPromotionValue;
            }
            else
            {
                moveValue = minimax(depth - 1, alpha, beta, false);
            }

            board[move.fromX][move.fromY] = movingPiece;
            board[move.toX][move.toY] = temp;

            if (moveValue > bestValue)
            {
                bestValue = moveValue;
                bestMove = move;
                if (isPromotionMove)
                {
                    promotionChoice = bestPromotion;
                }
            }
            alpha = std::max(alpha, moveValue);

            if (beta <= alpha)
            {
                break;
            }
        }

        // Logowanie ruchu AI
        std::string moveStr =
                "AI move: (" + std::to_string(bestMove.fromX) + "," + std::to_string(bestMove.fromY) + ") to (" +
                std::to_string(bestMove.toX) + "," + std::to_string(bestMove.toY) + ") with value: " +
                std::to_string(bestValue);
        logger.log(moveStr, Logger::INFO);
        // Obsługa promocji
        Piece piece = board[bestMove.fromX][bestMove.fromY];
        if (piece.type == PAWN && (bestMove.toX == 0 || bestMove.toX == 7))
        {
            isPawnPromotionPending = true;
            promotionX = bestMove.toX;
            promotionY = bestMove.toY;
            promotePawn(promotionChoice);
        }

        return bestMove;
    }

    Color getCurrentPlayer() const
    {
        return currentPlayer;
    }
    //Piece getPiece(int x, int y) const { return board[x][y]; }
};

#endif //PROJEKT3_CHESSGAME_H
