#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <cmath>

#include "Color.h"
#include "PieceType.h"
#include "Piece.h"
#include "Move.h"
#include "Logger.h"
#include "ChessGame.h"

//// Funkcja konwertująca współrzędne na notację szachową
//std::string moveToAlgebraic(const ChessGame& game, const Move& move) {
//    std::string from = std::string(1, 'a' + move.fromY) + std::to_string(8 - move.fromX);
//    std::string to = std::string(1, 'a' + move.toY) + std::to_string(8 - move.toX);
//    return from + "-" + to;
//}

std::string moveToAlgebraic(ChessGame& game, const Move& move) {
    Piece piece = game.getPiece(move.fromX, move.fromY);
    if (piece == EMPTY_PIECE) return "";

    std::string notation;
    // Oznacz figurę (oprócz pionka)
    switch (piece.type) {
        case KNIGHT: notation += "N"; break;
        case BISHOP: notation += "B"; break;
        case ROOK: notation += "R"; break;
        case QUEEN: notation += "Q"; break;
        case KING: notation += "K"; break;
        default: break; // Pionek
    }

    // Roszada
    if (piece.type == KING && abs(move.toY - move.fromY) == 2) {
        return move.toY > move.fromY ? "O-O" : "O-O-O";
    }

    // Pole początkowe (tylko jeśli potrzebne, uproszczenie)
    if (piece.type != PAWN) {
        notation += char('a' + move.fromY);
        notation += std::to_string(8 - move.fromX);
    } else if (game.getPiece(move.toX, move.toY) != EMPTY_PIECE || (move.toX == game.getEnPassantTargetX() && move.toY == game.getEnPassantTargetY())) {
        notation += char('a' + move.fromY);
    }

    // Bicie
    if (game.getPiece(move.toX, move.toY) != EMPTY_PIECE || (piece.type == PAWN && move.toX == game.getEnPassantTargetX() && move.toY == game.getEnPassantTargetY())) {
        notation += "x";
    }

    // Pole docelowe
    notation += char('a' + move.toY);
    notation += std::to_string(8 - move.fromX);

    // Promocja (sprawdzamy po ruchu)
    bool isPromotion = (piece.type == PAWN && (move.toX == 0 || move.toX == 7));
    std::string promotionNotation;
    if (isPromotion) {
        promotionNotation += "=";
        switch (game.getPiece(move.toX, move.toY).type) { // Po promocji
            case QUEEN: promotionNotation += "Q"; break;
            case ROOK: promotionNotation += "R"; break;
            case BISHOP: promotionNotation += "B"; break;
            case KNIGHT: promotionNotation += "N"; break;
            default: break;
        }
    }

    // Sprawdź szach/mat
    auto state = game.makeTemporaryMove(move);
    bool isCheck = game.isInCheck(game.getCurrentPlayer());
    bool isMate = isCheck && game.getAllPossibleMoves(game.getCurrentPlayer()).empty();
    game.undoMove(state);

    if (isCheck) {
        notation += isMate ? "#" : "+";
    }

    return notation + promotionNotation;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(1000, 800), "Chess Game");
    ChessGame game;
    const float squareSize = 100.0f;
    int selectedX = -1, selectedY = -1;
    int depth = 4;
    std::vector<Move> possibleMoves;
    bool isAnimating = false;
    bool isWaitingForAIMove = false;
    sf::Clock animationClock, aiMoveDelayClock;
    float animationDuration = 0.5f; // 0.5 sekundy
    Move animatingMove = {0, 0, 0, 0};
    Piece animationPiece = EMPTY_PIECE;
    Move lastOpponentMove = {-1, -1, -1, -1};
    float historyOffset = 0.0f; // Przesunięcie historii

    sf::Font font;
    if (!font.loadFromFile("Textures/arial.ttf"))
    {
        std::cout << "Error loading font!" << std::endl;
        return 1;
    }

    // Etykiety planszy (1-8, a-h)
    std::vector<sf::Text> boardLabels;
    for (int i = 0; i < 8; ++i) {
        // Liczby 1-8
        sf::Text numLabel;
        numLabel.setFont(font);
        numLabel.setString(std::to_string(8 - i));
        numLabel.setCharacterSize(20);
        numLabel.setFillColor(sf::Color::Black);
        numLabel.setPosition(3, i * squareSize);
        boardLabels.push_back(numLabel);

        // Litery a-h
        sf::Text letterLabel;
        letterLabel.setFont(font);
        letterLabel.setString(std::string(1, char('a' + i)));
        letterLabel.setCharacterSize(20);
        letterLabel.setFillColor(sf::Color::Black);
        letterLabel.setPosition(i * squareSize + squareSize - 12, 777);
        boardLabels.push_back(letterLabel);
    }

    sf::Texture pieceTextures[12];
    int texture_index = 0;
    for (const auto &color: {"w", "b"})
    {
        for (const auto &figure: {"pawn", "knight", "bishop", "rook", "queen", "king"})
        {
            std::string path = std::string("Textures/") + color + "_" + figure + ".png";
            if (!pieceTextures[texture_index].loadFromFile(path))
            {
                std::cerr << "Error loading texture: " << path << std::endl;
                return 1;
            }

            texture_index++;
        }
    }

    sf::Text statusText;
    statusText.setFont(font);
    statusText.setCharacterSize(24);
    statusText.setFillColor(sf::Color::Red);
    statusText.setPosition(10, 10);
    statusText.setOutlineColor(sf::Color::Black);
    statusText.setOutlineThickness(1);

    sf::RectangleShape statusBackground(sf::Vector2f(400, 40));
    statusBackground.setPosition(5, 5);
    statusBackground.setFillColor(sf::Color(255, 255, 255, 200));

    sf::Text resetText;
    resetText.setFont(font);
    resetText.setString("Restart Game");
    resetText.setCharacterSize(20);
    resetText.setFillColor(sf::Color::Black);
    resetText.setPosition(820, 700);

    sf::RectangleShape resetButton(sf::Vector2f(160, 50));
    resetButton.setPosition(820, 700);
    resetButton.setFillColor(sf::Color(200, 200, 200));

    sf::RectangleShape sidebar(sf::Vector2f(200, 800));
    sidebar.setPosition(800, 0);
    sidebar.setFillColor(sf::Color(220, 220, 220));

    sf::Text historyTitle;
    historyTitle.setFont(font);
    historyTitle.setString("Move History");
    historyTitle.setCharacterSize(20);
    historyTitle.setFillColor(sf::Color::Black);
    historyTitle.setPosition(820, 10);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                if (game.isGameOver())
                {
                    // Kliknięcie przycisku resetu
                    if (event.mouseButton.x >= 820 && event.mouseButton.x <= 980 && event.mouseButton.y >= 700 &&
                        event.mouseButton.y <= 750)
                    {
                        game.resetGame();
                        selectedX = -1;
                        selectedY = -1;
                        possibleMoves.clear();
                        isAnimating = false;
                        lastOpponentMove = {-1, -1, -1, -1};
                        historyOffset = 0.0f;
                    }
                }
                else if (game.isPromotionPending())
                {
                    // Obsługa wyboru figury promocji
                    int x = event.mouseButton.x;
                    int y = event.mouseButton.y;
                    if (y >= 350 && y <= 450)
                    { // Obszar paska promocji
                        if (x >= 200 && x < 300)
                        { // Królowa
                            game.promotePawn(QUEEN);
                        }
                        else if (x >= 300 && x < 400)
                        { // Wieża
                            game.promotePawn(ROOK);
                        }
                        else if (x >= 400 && x < 500)
                        { // Goniec
                            game.promotePawn(BISHOP);
                        }
                        else if (x >= 500 && x < 600)
                        { // Skoczek
                            game.promotePawn(KNIGHT);
                        }
                        if (!game.isGameOver() && game.getCurrentPlayer() == BLACK && !isAnimating)
                        {
                            isWaitingForAIMove = true;
                            aiMoveDelayClock.restart();
                        }
                    }
                }
                else if (!isAnimating && !isWaitingForAIMove)
                {
                    if (selectedX == -1 || game.getPiece(selectedX, selectedY) == EMPTY_PIECE)
                    {
                        int x = event.mouseButton.y / squareSize;
                        int y = event.mouseButton.x / squareSize;
                        if (game.isValidPosition(x, y))
                        {
                            Piece piece = game.getPiece(x, y);
                            if (piece != EMPTY_PIECE && piece.color == WHITE)
                            {
                                selectedX = x;
                                selectedY = y;
                                possibleMoves.clear();
                                for (int i = 0; i < 8; i++)
                                {
                                    for (int j = 0; j < 8; j++)
                                    {
                                        Move move = {selectedX, selectedY, i, j};
                                        if (game.isValidMove(move, WHITE))
                                        {
                                            possibleMoves.push_back(move);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                selectedX = -1;
                                selectedY = -1;
                                possibleMoves.clear();
                            }
                        }
                    }
                    else
                    {
                        int x = event.mouseButton.y / squareSize;
                        int y = event.mouseButton.x / squareSize;
                        if(game.isValidPosition(x, y))
                        {
                            Move move = {selectedX, selectedY, x, y};
                            if (game.isValidMove(move, WHITE))
                            {
                                animatingMove = move;
                                animationPiece = game.getPiece(move.fromX, move.fromY);
                                isAnimating = true;
                                animationClock.restart();
                                game.makeMove(move);
                                selectedX = -1;
                                selectedY = -1;
                                possibleMoves.clear();

                                if (!game.isPromotionPending() && !game.isGameOver() && game.getCurrentPlayer() == BLACK)
                                {
                                    isWaitingForAIMove = true;
                                    aiMoveDelayClock.restart();
                                }
                            }
                            else
                            {
                                selectedX = -1;
                                selectedY = -1;
                                possibleMoves.clear();
                            }
                        }
                    }
                }
            }
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R && game.isGameOver())
            {
                game.resetGame();
                selectedX = -1;
                selectedY = -1;
                possibleMoves.clear();
                isAnimating = false;
                lastOpponentMove = {-1, -1, -1, -1};
                historyOffset = 0.0f;
            }
            else if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.x >= 800 && event.mouseWheelScroll.x <= 1000)
            {
                historyOffset += event.mouseWheelScroll.delta * 20; // Przewijanie o 20 pikseli na skok
                if (historyOffset < 0) historyOffset = 0;
                const auto& history = game.getMoveHistory();
                int maxOffset = std::max(0, static_cast<int>((history.size() + 1) / 2 * 20 - 760));
                if (historyOffset > maxOffset) historyOffset = maxOffset;
            }
        }

        // Obsługa ruchu AI po pauzie
        if (isWaitingForAIMove && aiMoveDelayClock.getElapsedTime().asSeconds() >= 1.0f && !isAnimating)
        {
            Move aiMove = game.getBestMove(depth);
            if (aiMove.fromX != -1 && game.isValidMove(aiMove, BLACK))
            {
                animatingMove = aiMove;
                animationPiece = game.getPiece(aiMove.fromX, aiMove.fromY);
                isAnimating = true;
                animationClock.restart();
                lastOpponentMove = aiMove;
                game.makeMove(aiMove);
            }
            else
            {
                std::cerr << "AI generated invalid move!" << std::endl;
            }
            isWaitingForAIMove = false;
        }

        window.clear(sf::Color::White);

        // Draw sidebar
        window.draw(sidebar);
        window.draw(historyTitle);

        // Draw move history
//        const auto& history = game.getMoveHistory();
//        for (size_t i = 0; i < history.size(); ++i)
//        {
//            sf::Text moveText;
//            moveText.setFont(font);
//            moveText.setCharacterSize(16);
//            moveText.setFillColor(sf::Color::Black);
//            std::string moveStr = (i % 2 == 0 ? std::to_string(i / 2 + 1) + ". " : "   ") + moveToAlgebraic(history[i]);
//            moveText.setString(moveStr);
//            moveText.setPosition(820, 40 + i * 20);
//            window.draw(moveText);
//        }

        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                sf::RectangleShape square(sf::Vector2f(squareSize, squareSize));
                square.setPosition(j * squareSize, i * squareSize);
                square.setFillColor((i + j) % 2 == 0 ? sf::Color(255, 206, 158) : sf::Color(209, 139, 71));
                window.draw(square);
            }
        }

        // Podświetlenie ostatniego ruchu AI
        if (lastOpponentMove.fromX != -1)
        {
            sf::RectangleShape fromHighlight(sf::Vector2f(squareSize, squareSize));
            fromHighlight.setPosition(lastOpponentMove.fromY * squareSize, lastOpponentMove.fromX * squareSize);
            fromHighlight.setFillColor(sf::Color(0, 0, 255, 128));
            window.draw(fromHighlight);

            sf::RectangleShape toHighlight(sf::Vector2f(squareSize, squareSize));
            toHighlight.setPosition(lastOpponentMove.toY * squareSize, lastOpponentMove.toX * squareSize);
            toHighlight.setFillColor(sf::Color(0, 0, 255, 128));
            window.draw(toHighlight);
        }

        // Podświetlenie wybranego pola
        if (selectedX != -1 && selectedY != -1)
        {
            sf::RectangleShape highlight(sf::Vector2f(squareSize, squareSize));
            highlight.setPosition(selectedY * squareSize, selectedX * squareSize);
            highlight.setFillColor(sf::Color(255, 255, 0, 128));
            window.draw(highlight);
        }

        // Podświetlenie możliwych ruchów
        for (const auto &move : possibleMoves)
        {
            sf::RectangleShape moveHighlight(sf::Vector2f(squareSize, squareSize));
            moveHighlight.setPosition(move.toY * squareSize, move.toX * squareSize);
            moveHighlight.setFillColor(sf::Color(0, 255, 0, 128));
            window.draw(moveHighlight);
        }

        // Rysowanie figur (z wyjątkiem animowanej)
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                if (isAnimating && (i == animatingMove.fromX && j == animatingMove.fromY || i == animatingMove.toX && j == animatingMove.toY))
                    continue;
                Piece piece = game.getPiece(i, j);
                if (piece != EMPTY_PIECE)
                {
                    int textureIndex = piece.color == WHITE ? piece.type : piece.type + 6;
                    sf::Sprite pieceSprite(pieceTextures[textureIndex]);
                    pieceSprite.setPosition(j * squareSize + squareSize * 0.125f, i * squareSize + squareSize * 0.125f);
                    sf::Vector2u textureSize = pieceTextures[textureIndex].getSize();
                    pieceSprite.setScale(0.75f * squareSize / textureSize.x, 0.75f * squareSize / textureSize.y);
                    window.draw(pieceSprite);
                }
            }
        }

        // Animacja ruchu
        if (isAnimating)
        {
            float t = animationClock.getElapsedTime().asSeconds() / animationDuration;
            if (t >= 1.0f)
            {
                isAnimating = false;
                t = 1.0f;
            }
            float fromX = animatingMove.fromY * squareSize;
            float fromY = animatingMove.fromX * squareSize;
            float toX = animatingMove.toY * squareSize;
            float toY = animatingMove.toX * squareSize;
            float posX = fromX + (toX - fromX) * t;
            float posY = fromY + (toY - fromY) * t;

            int textureIndex = animationPiece.color == WHITE ? animationPiece.type : animationPiece.type + 6;
            sf::Sprite pieceSprite(pieceTextures[textureIndex]);
            pieceSprite.setPosition(posX + squareSize * 0.1f, posY + squareSize * 0.1f);
            sf::Vector2u textureSize = pieceTextures[textureIndex].getSize();
            pieceSprite.setScale(0.8 * squareSize / textureSize.x, 0.8 * squareSize / textureSize.y);
            window.draw(pieceSprite);
        }

        // Rysowanie etykiet planszy
        for (const auto& label : boardLabels) {
            window.draw(label);
        }

        // Historia ruchów
        const auto& history = game.getMoveHistory();
        for (size_t i = 0; i < history.size(); ++i)
        {
            sf::Text moveText;
            moveText.setFont(font);
            moveText.setCharacterSize(16);
            moveText.setFillColor(sf::Color::Black);
            std::string moveStr = (i % 2 == 0 ? std::to_string(i / 2 + 1) + ". " : "   ") + moveToAlgebraic(game, history[i]);
            moveText.setString(moveStr);
            float yPos = 40 + (i / 2 * 20) + (i % 2) * 10 - historyOffset;
            if (yPos >= 40 && yPos < 800)
            {
                moveText.setPosition(820 + (i % 2) * 80, yPos);
                window.draw(moveText);
            }
        }

        if (game.isPromotionPending())
        {
            sf::RectangleShape promotionBackground(sf::Vector2f(400, 100));
            promotionBackground.setPosition(200, 350);
            promotionBackground.setFillColor(sf::Color(200, 200, 200, 255));
            window.draw(promotionBackground);

            // Draw promotion options (Queen, Rook, Bishop, Knight)
            int promotionOptions[] = {QUEEN, ROOK, BISHOP, KNIGHT};
            for (int i = 0; i < 4; ++i)
            {
                sf::Sprite pieceSprite(pieceTextures[promotionOptions[i]]); // Białe figury
                pieceSprite.setPosition(200 + i * 100, 350);
                pieceSprite.setScale(100.0f / pieceTextures[promotionOptions[i]].getSize().x,
                                     100.0f / pieceTextures[promotionOptions[i]].getSize().y);
                window.draw(pieceSprite);
            }
        }

        // Draw game status with animation
        float alpha = 128 + 127 * std::sin(animationClock.getElapsedTime().asSeconds() * 2);
        if (game.isCheckmateState())
        {
            statusText.setString(game.getCurrentPlayer() == WHITE ? "Checkmate! Black wins!" : "Checkmate! White wins!");
            statusText.setFillColor(sf::Color(255, 0, 0, static_cast<sf::Uint8>(alpha)));
        }
        else if (game.isStalemateState())
        {
            statusText.setString("Stalemate! Draw!");
            statusText.setFillColor(sf::Color(0, 0, 255, static_cast<sf::Uint8>(alpha)));
        }
        else if (game.isInCheck(game.getCurrentPlayer()))
        {
            statusText.setString(game.getCurrentPlayer() == WHITE ? "White is in check!" : "Black is in check!");
            statusText.setFillColor(sf::Color(255, 0, 0, static_cast<sf::Uint8>(alpha)));
        }
        else
        {
            statusText.setString("");
            statusText.setFillColor(sf::Color(0, 0, 0, 0));
        }
        window.draw(statusBackground);
        window.draw(statusText);

        // Draw reset button if game is over
        if (game.isGameOver())
        {
            window.draw(resetButton);
            window.draw(resetText);
        }

        window.display();
    }
    return 0;
}