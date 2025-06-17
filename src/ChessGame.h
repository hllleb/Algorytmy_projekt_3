#ifndef PROJEKT3_CHESSGAME_H
#define PROJEKT3_CHESSGAME_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <random>
#include "Piece.h"
#include "Move.h"
#include "Logger.h"
#include "GameState.h"
#include "TranspositionEntry.h"

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
    static const int MAX_DEPTH = 10;
    Logger logger;
    std::unordered_map<std::string, Move> openingBook; // Księga debiutów
    std::unordered_map<std::string, int> moveRepetitionCount; // Licznik powtórek ruchów
    std::mt19937 rng; // Generator losowy dla losowości ruchów
    std::unordered_map<std::string, TranspositionEntry> transpositionTable;

    // Killer moves (po dwa na każdą głębokość)
    std::vector<std::vector<Move>> killerMoves;

    const int CHECK_BONUS = 400;
    const int CHECKMATE_BONUS = 999999;
    const int PROMOTION_BONUS = 1500; // Promote as soon as possible
    const int REPETITION_PENALTY = 8000; // Heavier than before
    const int THREAT_KING_BONUS = 400; // Nearby attackers
    const int SAFE_CAPTURE_BONUS = 5000; // Safe capture
    const int KING_SAFETY_BONUS = 200; // King safety

    const int PAWN_WEIGHT = 100;
    const int KNIGHT_WEIGHT = 320;
    const int BISHOP_WEIGHT = 330;
    const int ROOK_WEIGHT = 500;
    const int QUEEN_WEIGHT = 900;
    const int KING_WEIGHT = 5000;

    const int CAPTURE_BONUS_MULTIPLIER = 100;

    int materialSum() {
        int sum = 0;
        for (int i=0; i<8; ++i) for (int j=0; j<8; ++j) {
                Piece p = board[i][j];
                int v = 0;
                switch (p.type) {
                    case PAWN: v = PAWN_WEIGHT; break;
                    case KNIGHT: v = KNIGHT_WEIGHT; break;
                    case BISHOP: v = BISHOP_WEIGHT; break;
                    case ROOK: v = ROOK_WEIGHT; break;
                    case QUEEN: v = QUEEN_WEIGHT; break;
                    default: v = 0;
                }
                if (p != EMPTY_PIECE)
                    sum += (p.color == currentPlayer ? v : -v);
            }
        return sum;
    }

    // Inicjalizacja księgi debiutów
    void initializeOpeningBook()
    {
        // Proste debiuty, np. 1. e4 e5, 1. d4 d5
        openingBook["rnbqkbnr/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKBNR w KQkq - 0 1"] = Move(6, 4, 4, 4, "e4"); // 1. e4
        openingBook["rnbqkbnr/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKBNR w KQkq e6 0 1"] = Move(6, 3, 4, 3, "d4"); // 1. e4 e5 2. d4
        openingBook["rnbqkbnr/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKBNR b KQkq - 0 1"] = Move(1, 4, 3, 4, "e5"); // 1. e4 e5
        openingBook["rnbqkbnr/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKBNR b KQkq - 0 1"] = Move(1, 3, 3, 3, "d5"); // 1. e4 e5 2. d4 d5

        // Additional openings
        openingBook["rnbqkbnr/pppp1ppp/5n2/5p2/4P3/5N2/PPPP1PPP/RNBQKBNR b KQkq - 0 1"] = Move(1, 5, 3, 5, "f5"); // 1. e4 f5 (Dutch Defense)
        openingBook["rnbqkb1r/pppp1ppp/5n2/5p2/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 1 2"] = Move(6, 5, 4, 5, "f3"); // 1. e4 f5 2. f3
        openingBook["rnbqkbnr/pppppppp/8/8/4P3/8/PPPPPPPP/RNBQKBNR b KQkq e3 0 1"] = Move(6, 2, 4, 2, "c4"); // 1. e4 c4 (unorthodox)
        openingBook["rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"] = Move(1, 2, 3, 2, "c5"); // 1. e4 e5 2. Nf3 c5 (Sicilian Defense)
        openingBook["rnbqkb1r/pp1ppppp/5n2/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq c6 0 2"] = Move(6, 3, 4, 3, "d4"); // 1. e4 c5 2. d4
        openingBook["rnbqkbnr/pp1ppppp/5n2/5p2/4P3/8/PPPPPPPP/RNBQKBNR w KQkq - 0 2"] = Move(6, 5, 4, 5, "f4"); // 1. e4 f5 2. f4 (From's Gambit)
    }

    // Generowanie klucza pozycji dla tabeli transpozycji
    std::string getPositionKey() const
    {
        std::string key;
        int emptyCount = 0;

        // Generowanie układu figur (8 rzędów)
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                Piece p = board[i][j];
                if (p == EMPTY_PIECE)
                {
                    emptyCount++;
                }
                else
                {
                    if (emptyCount > 0)
                    {
                        key += std::to_string(emptyCount);
                        emptyCount = 0;
                    }
                    key += p.color == WHITE ? toupper(pieceToChar(p.type)) : tolower(pieceToChar(p.type));
                }
            }
            if (emptyCount > 0)
            {
                key += std::to_string(emptyCount);
                emptyCount = 0;
            }
            if (i < 7) key += "/";
        }

        // Kolor, który ma ruch
        key += std::string(" ") + (currentPlayer == WHITE ? "w" : "b");

        // Prawa do roszady
        std::string castling = "";
        if (whiteCanCastleKingside) castling += "K";
        if (whiteCanCastleQueenside) castling += "Q";
        if (blackCanCastleKingside) castling += "k";
        if (blackCanCastleQueenside) castling += "q";
        key += " " + (castling.empty() ? "-" : castling);

        // Pole en passant
        if (enPassantTargetX != -1 && enPassantTargetY != -1)
        {
            key += " " + std::string(1, 'a' + enPassantTargetY) + std::to_string(8 - enPassantTargetX);
        }
        else
        {
            key += " -";
        }

        // Liczba posunięć (uproszczone, można pominąć)
        key += " 0 1";

        return key;
    }

    char pieceToChar(PieceType type) const
    {
        switch (type)
        {
            case PAWN: return 'p';
            case KNIGHT: return 'n';
            case BISHOP: return 'b';
            case ROOK: return 'r';
            case QUEEN: return 'q';
            case KING: return 'k';
            default: return '.';
        }
    }

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
                return PAWN_WEIGHT;
            case KNIGHT:
                return KNIGHT_WEIGHT;
            case BISHOP:
                return BISHOP_WEIGHT;
            case ROOK:
                return ROOK_WEIGHT;
            case QUEEN:
                return QUEEN_WEIGHT;
            case KING:
                return KING_WEIGHT;
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
        sf::String str;
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

    int getBestPromotionEval(const Move &move, int depth, int alpha, int beta, bool maximizingPlayer)
    {
        int bestEval = maximizingPlayer ? INT_MIN : INT_MAX;
        PieceType options[] = {QUEEN, ROOK, BISHOP, KNIGHT};
        for (PieceType option : options)
        {
            board[move.toX][move.toY] = Piece(option, board[move.toX][move.toY].color);
            int eval = minimax(depth, alpha, beta, !maximizingPlayer);
            if (maximizingPlayer && eval > bestEval)
                bestEval = eval;
            else if (!maximizingPlayer && eval < bestEval)
                bestEval = eval;
            if (maximizingPlayer)
                alpha = std::max(alpha, eval);
            else
                beta = std::min(beta, eval);
            if (beta <= alpha)
                break;
        }
        board[move.toX][move.toY] = Piece(options[0], board[move.toX][move.toY].color); // Przywróć domyślną promocję
        return bestEval;
    }

public:
    ChessGame() : currentPlayer(WHITE), isCheckmate(false), isStalemate(false), gameOverState(false),
                  whiteCanCastleKingside(true), whiteCanCastleQueenside(true), blackCanCastleKingside(true),
                  blackCanCastleQueenside(true), enPassantTargetX(-1), enPassantTargetY(-1),
                  isPawnPromotionPending(false), promotionX(-1), promotionY(-1), promotionChoice(QUEEN),
                  logger("chess_log.txt")
    {
        initializeBoard();
        initializeOpeningBook();
        killerMoves.resize(MAX_DEPTH + 1, std::vector<Move>(2, Move(-1,-1,-1,-1)));
    }

    PieceType getPromotionChoice() const { return promotionChoice; }
    void setPromotionChoice(const PieceType& type) { promotionChoice = type; }

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
                    if (target != EMPTY_PIECE)
                        validPieceMove = true;
                    else if (move.toX == enPassantTargetX && move.toY == enPassantTargetY)
                    {
                        int capturedPawnX = move.fromX;
                        if (isValidPosition(capturedPawnX, move.toY) && board[capturedPawnX][move.toY].type == PAWN &&
                            board[capturedPawnX][move.toY].color != piece.color)
                            validPieceMove = true;
                    }
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
            int capturedPawnX = move.toX - direction;
            if (isValidPosition(capturedPawnX, move.toY) &&
                board[capturedPawnX][move.toY].type == PAWN &&
                board[capturedPawnX][move.toY].color != piece.color)
            {
                board[capturedPawnX][move.toY] = EMPTY_PIECE;
            }
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

        board[move.toX][move.toY] = board[move.fromX][move.fromY];
        board[move.fromX][move.fromY] = EMPTY_PIECE;

        if (piece.type == PAWN && (move.toX == 0 || move.toX == 7))
        {
            isPawnPromotionPending = true;
            promotionX = move.toX;
            promotionY = move.toY;
        }

        std::string notation = generateAlgebraicNotation(move, state);
        moveHistory.emplace_back(move.fromX, move.fromY, move.toX, move.toY, notation);

        // Aktualizacja licznika powtórek ruchów
        std::string moveKey = std::to_string(move.fromX) + std::to_string(move.fromY) + std::to_string(move.toX) + std::to_string(move.toY);
        moveRepetitionCount[moveKey]++;

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

    void promotePawn()
    {
        if (isPawnPromotionPending && (promotionChoice == QUEEN || promotionChoice == ROOK || promotionChoice == BISHOP || promotionChoice == KNIGHT))
        {
            Color color = board[promotionX][promotionY].color;
            board[promotionX][promotionY] = Piece(promotionChoice, color);
            logger.log("Pawn promoted to " + std::to_string(promotionChoice), Logger::INFO);
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
        moveRepetitionCount.clear();
        transpositionTable.clear();
        killerMoves.clear();
        killerMoves.resize(MAX_DEPTH + 1, std::vector<Move>(2, Move(-1, -1, -1, -1)));
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
        int kingSafety = 0;
        int pawnStructure = 0;
        int pieceActivity = 0;
        int development = 0;
        int threatPenalty = 0; // New: penalty for pieces under threat
        int passedPawnBonus = 0; // New: bonus for passed pawns

        // Określenie fazy gry
        int totalMaterial = 0;
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
                        value = PAWN_WEIGHT;
                        break;
                    case KNIGHT:
                        value = KING_WEIGHT;
                        break;
                    case BISHOP:
                        value = BISHOP_WEIGHT;
                        break;
                    case ROOK:
                        value = ROOK_WEIGHT;
                        break;
                    case QUEEN:
                        value = QUEEN_WEIGHT;
                        break;
                    case KING:
                        value = KING_WEIGHT;
                        break;
                    default:
                        value = 0;
                        break;
                }

                totalMaterial += value;
            }
        }

        bool isOpening = moveHistory.size() < 10;
        bool isEndgame = totalMaterial < 2000;

        // Ocena figur i dodatkowych czynników
        int kingX[2] = {-1, -1}, kingY[2] = {-1, -1}; // Pozycje królów (0: białe, 1: czarne)
        int pawnCount[2][8] = {{0}}; // Liczba pionków w każdej kolumnie
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                Piece piece = board[i][j];
                if (piece == EMPTY_PIECE) continue;

                int value = 0;
                switch (piece.type)
                {
                    case PAWN: value = PAWN_WEIGHT; break;
                    case KNIGHT: value = KING_WEIGHT; break;
                    case BISHOP: value = BISHOP_WEIGHT; break;
                    case ROOK: value = ROOK_WEIGHT; break;
                    case QUEEN: value = QUEEN_WEIGHT; break;
                    case KING: value = KING_WEIGHT; break;
                    default: value = 0;
                }

                // Kontrola centrum
                if ((i == 3 || i == 4) && (j == 3 || j == 4))
                {
                    centerControl += (piece.type == PAWN ? 30 : 50);
                }

                // Bezpieczeństwo króla
                if (piece.type == KING)
                {
                    kingX[piece.color] = i;
                    kingY[piece.color] = j;

                    // Bonus for pieces attacking squares near the enemy king
                    for (int dx = -1; dx <= 1; dx++) {
                        for (int dy = -1; dy <= 1; dy++) {
                            int kx = kingX[1 - currentPlayer], ky = kingY[1 - currentPlayer];
                            int ax = kx + dx, ay = ky + dy;
                            if (isValidPosition(ax, ay) && board[ax][ay] != EMPTY_PIECE && board[ax][ay].color == currentPlayer) {
                                score += THREAT_KING_BONUS;
                            }
                        }
                    }

                    if (!isEndgame)
                    {
                        // Kara za wczesne ruszanie królem
                        if (piece.color == WHITE && i < 7 && isOpening)
                            kingSafety -= KING_SAFETY_BONUS;
                        else if (piece.color == BLACK && i > 0 && isOpening)
                            kingSafety -= KING_SAFETY_BONUS;
                        // Premia za roszadę
                        if (piece.color == WHITE && (whiteCanCastleKingside || whiteCanCastleQueenside))
                            kingSafety += KING_SAFETY_BONUS;
                        else if (piece.color == BLACK && (blackCanCastleKingside || blackCanCastleQueenside))
                            kingSafety += KING_SAFETY_BONUS;
                    }
                    else
                    {
                        // Endgame: Encourage king centralization
                        int distToCenter = std::max(std::abs(i - 3.5), std::abs(j - 3.5));
                        kingSafety -= distToCenter * 20; // Bonus for central king
                    }
                }

                // Rozwój figur
                if (isOpening && (piece.type == KNIGHT || piece.type == BISHOP))
                {
                    if (piece.color == WHITE && i == 7)
                        development -= 50; // Kara za nierozwinięte figury
                    else if (piece.color == BLACK && i == 0)
                        development -= 50;
                    else
                        development += 30;
                }

                // Aktywność figur
                if (piece.type == ROOK)
                {
                    bool openFile = true;
                    for (int k = 0; k < 8; ++k)
                        if (board[k][j].type == PAWN)
                            openFile = false;
                    if (openFile)
                        pieceActivity += 60; // Premia za wieżę na otwartej linii
                    if (isEndgame && i == (piece.color == WHITE ? 1 : 6))
                        pieceActivity += 50; // Bonus for rook on 7th rank
                }
                else if (piece.type == KNIGHT && (i == 3 || i == 4) && (j == 3 || j == 4))
                {
                    pieceActivity += 40; // Premia za skoczka w centrum
                }

                // New: Passed pawn bonus
                if (piece.type == PAWN)
                {
                    pawnCount[piece.color][j]++;
                    bool isPassed = true;
                    int direction = (piece.color == WHITE) ? -1 : 1;
                    for (int k = i + direction; k >= 0 && k < 8; k += direction)
                    {
                        if ((j > 0 && board[k][j-1].type == PAWN && board[k][j-1].color != piece.color) ||
                            (j < 7 && board[k][j+1].type == PAWN && board[k][j+1].color != piece.color) ||
                            (board[k][j].type == PAWN && board[k][j].color != piece.color))
                        {
                            isPassed = false;
                            break;
                        }
                    }
                    if (isPassed)
                    {
                        int rank = piece.color == WHITE ? 7 - i : i;
                        passedPawnBonus += 50 + rank * 20; // Bonus increases with rank
                    }

                    // Stronger bonus for advanced pawn ready for promotion
                    int promotionRank = (piece.color == WHITE) ? 0 : 7;
                    int advance = abs(i - promotionRank);
                    if (advance <= 2) {
                        score += (piece.color == WHITE ? 1 : -1) * (PROMOTION_BONUS / (advance + 1));
                    }
                }

                // Bonus za możliwość bicia
                std::vector<Move> possibleCaptures = getAllPossibleMoves(piece.color);
                for (const auto& move : possibleCaptures)
                {
                    if (board[move.toX][move.toY] != EMPTY_PIECE && board[move.toX][move.toY].color != piece.color)
                    {
                        int captureValue = getCaptureValue(move);
                        if (captureValue > 0)
                            pieceActivity += captureValue; // Premia za możliwość bicia
                    }
                }

                // New: Threat penalty for high-value pieces
                if (piece.type == QUEEN || piece.type == ROOK || piece.type == KNIGHT || piece.type == BISHOP)
                {
                    for (int x = 0; x < 8; ++x)
                    {
                        for (int y = 0; y < 8; ++y)
                        {
                            if (board[x][y] != EMPTY_PIECE && board[x][y].color != piece.color)
                            {
                                Move attackMove(x, y, i, j);
                                if (isValidAttackMove(attackMove, board[x][y].color))
                                {
                                    int pieceValue = getCaptureValue({i, j, i, j});
                                    threatPenalty -= pieceValue / 2; // Significant penalty
                                }
                            }
                        }
                    }
                }

                score += (piece.color == WHITE) ? value : -value;
            }
        }

        // Ocena struktury pionków
        for (int j = 0; j < 8; ++j)
        {
            for (int c = 0; c < 2; ++c)
            {
                Color color = static_cast<Color>(c);
                // Kara za podwójne pionki
                if (pawnCount[c][j] > 1)
                    pawnStructure -= 50 * (pawnCount[c][j] - 1);
                // Kara za izolowane pionki
                bool isolated = true;
                if (j > 0 && pawnCount[c][j - 1] > 0)
                    isolated = false;
                if (j < 7 && pawnCount[c][j + 1] > 0)
                    isolated = false;
                if (isolated && pawnCount[c][j] > 0)
                    pawnStructure -= 40;
                pawnStructure = (color == WHITE) ? pawnStructure : -pawnStructure;
            }
        }

        // Bezpieczeństwo króla: osłona pionkami
        for (int c = 0; c < 2; ++c)
        {
            Color color = static_cast<Color>(c);
            if (kingX[c] != -1)
            {
                int pawnShield = 0;
                int direction = (color == WHITE) ? -1 : 1;
                for (int dj = -1; dj <= 1; ++dj)
                {
                    int nx = kingX[c] + direction, ny = kingY[c] + dj;
                    if (isValidPosition(nx, ny) && board[nx][ny].type == PAWN && board[nx][ny].color == color)
                        pawnShield += 60;
                }
                kingSafety += (color == WHITE) ? pawnShield : -pawnShield;
            }
        }

        // Mobilność
        mobilityScore += getAllPossibleMoves(WHITE).size() * (isOpening ? 30 : 15);
        mobilityScore -= getAllPossibleMoves(BLACK).size() * (isOpening ? 30 : 15);

        // Reward for giving check to opponent, bigger for mate
        if (isInCheck(currentPlayer == WHITE ? BLACK : WHITE)) {
            score += CHECK_BONUS;
            // If opponent has no legal moves: checkmate!
            if (getAllPossibleMoves(currentPlayer == WHITE ? BLACK : WHITE).empty()) {
                score += CHECKMATE_BONUS;
            }
        }
        // Penalize being in check
        if (isInCheck(currentPlayer)) {
            score -= CHECK_BONUS;
        }

        // Heavier penalty for repeating positions
        for (const auto& [_, count] : moveRepetitionCount) {
            if (count >= 2) score -= REPETITION_PENALTY * (count-1);  // make it at least 4x larger!
            if (count >= 3) score -= 1000000; // Near-infinite penalty for 3-fold!
        }

        // Clamp insane values except in clear mate/draw
        if (score > 20000 && score < CHECKMATE_BONUS) score = 20000;
        if (score < -20000 && score > -CHECKMATE_BONUS) score = -20000;

        return score + mobilityScore + centerControl + kingSafety + pawnStructure + pieceActivity + development + threatPenalty + passedPawnBonus;
    }

    // New: Quiescence search to evaluate captures beyond depth limit
    int quiescenceSearch(int alpha, int beta, bool maximizingPlayer, int maxDepth)
    {
        int standPat = evaluateBoard();
        if (maxDepth <= 0) return standPat;
        if (maximizingPlayer)
        {
            if (standPat >= beta) return beta;
            alpha = std::max(alpha, standPat);
        }
        else
        {
            if (standPat <= alpha) return alpha;
            beta = std::min(beta, standPat);
        }

        std::vector<Move> captureMoves;
        Color player = maximizingPlayer ? currentPlayer : (currentPlayer == WHITE ? BLACK : WHITE);
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                if (board[i][j].color != player) continue;
                std::vector<Move> moves = getAllPossibleMoves(player);
                for (const auto& move : moves)
                {
                    if (board[move.toX][move.toY] != EMPTY_PIECE || (board[i][j].type == PAWN && move.toX == enPassantTargetX && move.toY == enPassantTargetY))
                        captureMoves.push_back(move);
                }
            }
        }

        std::sort(captureMoves.begin(), captureMoves.end(), [this](const Move &a, const Move &b)
        {
            return getCaptureValue(a) > getCaptureValue(b);
        });

        for (const Move &move : captureMoves)
        {
            GameState state = makeTemporaryMove(move);
            int score;
            if (board[move.toX][move.toY].type == PAWN && (move.toX == 0 || move.toX == 7))
                score = getBestPromotionEval(move, maxDepth - 1, alpha, beta, !maximizingPlayer);
            else
                score = quiescenceSearch(alpha, beta, !maximizingPlayer, maxDepth - 1);
            undoMove(state);

            if (maximizingPlayer)
            {
                alpha = std::max(alpha, score);
                if (alpha >= beta)
                    break;
            }
            else
            {
                beta = std::min(beta, score);
                if (beta <= alpha)
                    break;
            }
        }

        return maximizingPlayer ? alpha : beta;
    }

    int minimax(int depth, int alpha, int beta, bool maximizingPlayer)
    {
        if (depth == 0 )
        {
            return quiescenceSearch(alpha, beta, maximizingPlayer, 4); // Quiescence search up to depth 4
        }

        if (isGameOver())
        {
            int eval = evaluateBoard();
            if (isInCheck(maximizingPlayer ? currentPlayer : (currentPlayer == WHITE ? BLACK : WHITE)))
                eval -= 500;
            return eval;
        }

        std::string positionKey = getPositionKey();
        auto ttEntry = transpositionTable.find(positionKey);
        if (ttEntry != transpositionTable.end() && ttEntry->second.depth >= depth)
        {
            return ttEntry->second.value;
        }

        std::vector<Move> moves = getAllPossibleMoves(maximizingPlayer ? currentPlayer : (currentPlayer == WHITE ? BLACK : WHITE));
        if (moves.empty())
        {
            if (isInCheck(maximizingPlayer ? currentPlayer : (currentPlayer == WHITE ? BLACK : WHITE)))
            {
                return maximizingPlayer ? -CHECKMATE_BONUS + depth : CHECKMATE_BONUS - depth;
            }
            return 0;
        }

        // Sortowanie ruchów
        std::sort(moves.begin(), moves.end(), [this, depth](const Move &a, const Move &b)
        {
            int scoreA = getCaptureValue(a) * CAPTURE_BONUS_MULTIPLIER;
            int scoreB = getCaptureValue(b) * CAPTURE_BONUS_MULTIPLIER;
            GameState stateA = makeTemporaryMove(a);
            bool isCheckA = isInCheck(currentPlayer);
            bool isMateA = isCheckA && getAllPossibleMoves(currentPlayer).empty();
            undoMove(stateA);
            scoreA += isMateA ? 999999 : (isCheckA ? 800 : 0);
            GameState stateB = makeTemporaryMove(b);
            bool isCheckB = isInCheck(currentPlayer);
            bool isMateB = isCheckB && getAllPossibleMoves(currentPlayer).empty();
            undoMove(stateB);
            scoreB += isMateB ? 999999 : (isCheckB ? 800 : 0);
            // Rozwój figur
            if (moveHistory.size() < 10)
            {
                Piece pieceA = board[a.fromX][a.fromY];
                Piece pieceB = board[b.fromX][b.fromY];
                if (pieceA.type == KNIGHT || pieceA.type == BISHOP)
                    scoreA += 50;
                if (pieceB.type == KNIGHT || pieceB.type == BISHOP)
                    scoreB += 50;
                // Ostrożność z hetmanem
                if (pieceA.type == QUEEN)
                    scoreA -= 30;
                if (pieceB.type == QUEEN)
                    scoreB -= 30;
            }
            // Killer moves
            if (a == (killerMoves[depth][0]) || a == killerMoves[depth][1])
                scoreA += 200;
            if (b == killerMoves[depth][0] || b == killerMoves[depth][1])
                scoreB += 200;

            return scoreA > scoreB;
        });

        if (maximizingPlayer)
        {
            int maxEval = INT_MIN;
            Move bestMove;
            for (const Move &move : moves)
            {
                GameState state = makeTemporaryMove(move);
                int eval = (board[move.toX][move.toY].type == PAWN && (move.toX == 0 || move.toX == 7))
                           ? getBestPromotionEval(move, depth - 1, alpha, beta, false)
                           : minimax(depth - 1, alpha, beta, false);
                undoMove(state);

                // New: Bonus for safe captures
                if (getCaptureValue(move) > 0)
                {
                    bool isSafe = true;
                    GameState tempState = makeTemporaryMove(move);
                    for (int x = 0; x < 8; ++x)
                    {
                        for (int y = 0; y < 8; ++y)
                        {
                            if (board[x][y] != EMPTY_PIECE && board[x][y].color != currentPlayer)
                            {
                                Move attackMove(x, y, move.toX, move.toY);
                                if (isValidAttackMove(attackMove, board[x][y].color))
                                {
                                    isSafe = false;
                                    break;
                                }
                            }
                        }
                        if (!isSafe) break;
                    }
                    undoMove(tempState);
                    if (isSafe)
                        eval += SAFE_CAPTURE_BONUS; // Bonus for safe captures
                }

                if (eval > maxEval)
                {
                    maxEval = eval;
                    bestMove = move;
                }
                alpha = std::max(alpha, eval);
                if (beta <= alpha)
                {
                    if (getCaptureValue(move) == 0)
                    {
                        killerMoves[depth][1] = killerMoves[depth][0];
                        killerMoves[depth][0] = move;
                    }
                    break;
                }
            }

            int before = materialSum();
            GameState state = makeTemporaryMove(bestMove);
            int after = materialSum();
            undoMove(state);
            if (after > before) maxEval += (after - before) * 10; // Or more

            // Zapis do tabeli transpozycji
            transpositionTable[positionKey] = {maxEval, depth, bestMove};
            return maxEval;
        }
        else
        {
            int minEval = INT_MAX;
            Move bestMove;
            for (const Move &move : moves)
            {
                GameState state = makeTemporaryMove(move);
                int eval = (board[move.toX][move.toY].type == PAWN && (move.toX == 0 || move.toX == 7))
                           ? getBestPromotionEval(move, depth - 1, alpha, beta, true)
                           : minimax(depth - 1, alpha, beta, true);
                undoMove(state);

                // New: Penalty for moves exposing pieces
                if (getCaptureValue(move) == 0)
                {
                    GameState tempState = makeTemporaryMove(move);
                    for (int x = 0; x < 8; ++x)
                    {
                        for (int y = 0; y < 8; ++y)
                        {
                            if (board[x][y].type == QUEEN && board[x][y].color == currentPlayer)
                            {
                                for (int ax = 0; ax < 8; ++ax)
                                {
                                    for (int ay = 0; ay < 8; ++ay)
                                    {
                                        if (board[ax][ay] != EMPTY_PIECE && board[ax][ay].color != currentPlayer)
                                        {
                                            Move attackMove(ax, ay, x, y);
                                            if (isValidAttackMove(attackMove, board[ax][ay].color))
                                            {
                                                eval -= 4 * QUEEN_WEIGHT; // Or even 10x for extreme caution!
                                            }
                                        }
                                    }
                                }
                            }
                            else if (board[x][y] != EMPTY_PIECE && board[x][y].color != currentPlayer)
                            {
                                Move attackMove(x, y, move.toX, move.toY);
                                if (isValidAttackMove(attackMove, board[x][y].color))
                                {
                                    int pieceValue = getCaptureValue({move.toX, move.toY, move.toX, move.toY});
                                    eval -= pieceValue / 2;
                                }
                            }
                        }
                    }
                    undoMove(tempState);
                }

                if (eval < minEval)
                {
                    minEval = eval;
                    bestMove = move;
                }
                beta = std::min(beta, eval);
                if (beta <= alpha)
                {
                    if (getCaptureValue(move) == 0)
                    {
                        killerMoves[depth][1] = killerMoves[depth][0];
                        killerMoves[depth][0] = move;
                    }
                    break;
                }
            }
            // Zapis do tabeli transpozycji
            transpositionTable[positionKey] = {minEval, depth, bestMove};
            return minEval;
        }
    }

    Move iterativeDeepening(int maxDepth, float timeLimit)
    {
        Move bestMove = Move(-1, -1, -1, -1);
        int bestValue = (currentPlayer == WHITE) ? INT_MIN : INT_MAX;
        std::vector<Move> bestMoves;
        auto startTime = std::chrono::steady_clock::now();
        bool isMaximizing = (currentPlayer == WHITE);

        for (int depth = 1; depth <= maxDepth; ++depth)
        {
            int alpha = INT_MIN;
            int beta = INT_MAX;
            std::vector<Move> moves = getAllPossibleMoves(currentPlayer);
            if (moves.empty()) return Move(-1, -1, -1, -1);

            bestMoves.clear();
            std::sort(moves.begin(), moves.end(), [this](const Move &a, const Move &b)
            {
                int scoreA = getCaptureValue(a) * CAPTURE_BONUS_MULTIPLIER;
                int scoreB = getCaptureValue(b) * CAPTURE_BONUS_MULTIPLIER;
                GameState stateA = makeTemporaryMove(a);
                bool isCheckA = isInCheck(currentPlayer);
                bool isMateA = isCheckA && getAllPossibleMoves(currentPlayer).empty();
                undoMove(stateA);
                scoreA += isMateA ? 999999 : (isCheckA ? 800 : 0);
                GameState stateB = makeTemporaryMove(b);
                bool isCheckB = isInCheck(currentPlayer);
                bool isMateB = isCheckB && getAllPossibleMoves(currentPlayer).empty();
                undoMove(stateB);
                scoreB += isMateB ? 999999 : (isCheckB ? 800 : 0);
                return scoreA > scoreB;
            });

            for (const Move &move : moves)
            {
                GameState state = makeTemporaryMove(move);
                int moveValue;
                if (board[move.toX][move.toY].type == PAWN && (move.toX == 0 || move.toX == 7))
                {
                    int bestPromotionValue = isMaximizing ? INT_MIN : INT_MAX;
                    PieceType bestPromotion = QUEEN;
                    PieceType options[] = {QUEEN, ROOK, BISHOP, KNIGHT};
                    for (PieceType option : options)
                    {
                        board[move.toX][move.toY] = Piece(option, board[move.toX][move.toY].color);
                        int value = minimax(depth - 1, alpha, beta, !isMaximizing);
                        if (isMaximizing && value > bestPromotionValue)
                        {
                            bestPromotionValue = value;
                            bestPromotion = option;
                        }
                        else if (!isMaximizing && value < bestPromotionValue)
                        {
                            bestPromotionValue = value;
                            bestPromotion = option;
                        }
                    }
                    moveValue = bestPromotionValue;
                    board[move.toX][move.toY] = Piece(bestPromotion, board[move.toX][move.toY].color);
                    if (isMaximizing && moveValue > bestValue)
                    {
                        promotionChoice = bestPromotion;
                    }
                    else if (!isMaximizing && moveValue < bestValue)
                    {
                        promotionChoice = bestPromotion;
                    }
                }
                else
                {
                    moveValue = minimax(depth - 1, alpha, beta, !isMaximizing);
                }
                undoMove(state);

                if (isMaximizing)
                {
                    if (moveValue > bestValue)
                    {
                        bestValue = moveValue;
                        bestMove = move;
                        bestMoves.clear();
                        bestMoves.push_back(move);
                    }
                    else if (moveValue >= bestValue - 20)
                    {
                        bestMoves.push_back(move);
                    }
                    alpha = std::max(alpha, moveValue);
                }
                else
                {
                    if (moveValue < bestValue)
                    {
                        bestValue = moveValue;
                        bestMove = move;
                        bestMoves.clear();
                        bestMoves.push_back(move);
                    }
                    else if (moveValue <= bestValue + 20)
                    {
                        bestMoves.push_back(move);
                    }
                    beta = std::min(beta, moveValue);
                }

                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
                if (elapsed / 1000.0f > timeLimit)
                    break;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
            if (elapsed / 1000.0f > timeLimit)
                break;
        }

        if (!bestMoves.empty() && bestMoves.size() > 1)
        {
            std::vector<Move> captureMoves;
            for (const auto& move : bestMoves)
            {
                if (getCaptureValue(move) > 0)
                    captureMoves.push_back(move);
            }
            if (!captureMoves.empty())
            {
                std::uniform_int_distribution<int> dist(0, captureMoves.size() - 1);
                bestMove = captureMoves[dist(rng)];
            }
            else
            {
                std::uniform_int_distribution<int> dist(0, bestMoves.size() - 1);
                bestMove = bestMoves[dist(rng)];
            }
        }

        return bestMove;
    }

    Move getBestMove(int depth)
    {
        if (isGameOver()) return Move(-1, -1, -1, -1);

        // Sprawdzenie księgi debiutów
        std::string positionKey = getPositionKey();
        auto openingMove = openingBook.find(positionKey);
        if (openingMove != openingBook.end() && moveHistory.size() < 10)
        {
            logger.log("Using opening book move: " + openingMove->second.notation, Logger::INFO);
            return openingMove->second;
        }

        Move bestMove = iterativeDeepening(depth + 2, 5.0f); // Limit czasu 3 sekundy
        if (bestMove.fromX == -1)
        {
            std::vector<Move> moves = getAllPossibleMoves(currentPlayer);
            if (!moves.empty())
                bestMove = moves[0];
        }

        std::string moveStr = "AI move: (" + std::to_string(bestMove.fromX) + "," + std::to_string(bestMove.fromY) + ") to (" +
                              std::to_string(bestMove.toX) + "," + std::to_string(bestMove.toY) + ") with value: " + std::to_string(evaluateBoard());
        logger.log(moveStr, Logger::INFO);

        Piece piece = board[bestMove.fromX][bestMove.fromY];
        if (piece.type == PAWN && (bestMove.toX == 0 || bestMove.toX == 7))
        {
            isPawnPromotionPending = true;
            promotionX = bestMove.toX;
            promotionY = bestMove.toY;
        }

        return bestMove;
    }

    Color getCurrentPlayer() const
    {
        return currentPlayer;
    }
};

#endif //PROJEKT3_CHESSGAME_H