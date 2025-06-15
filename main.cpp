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

// Funkcja konwertująca współrzędne na notację szachową
std::string moveToAlgebraic(const Move& move) {
    std::string from = std::string(1, 'a' + move.fromY) + std::to_string(8 - move.fromX);
    std::string to = std::string(1, 'a' + move.toY) + std::to_string(8 - move.toX);
    return from + "-" + to;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(1000, 800), "Chess Game");
    ChessGame game;
    const float squareSize = 100.0f;
    int selectedX = -1, selectedY = -1;
    int depth = 4;
    std::vector<Move> possibleMoves;

    sf::Font font;
    if (!font.loadFromFile("Textures/arial.ttf"))
    {
        std::cout << "Error loading font!" << std::endl;
        return 1;
    }

    sf::Texture pieceTextures[12];
    int texture_index = 0;
    for (const auto &color: {"w", "b"})
    {
        for (const auto &figure: {"pawn", "knight", "bishop", "rook", "queen", "king"})
        {
            std::string path = std::string("Textures/") + color + "_" + figure + ".png";
            if (!pieceTextures[texture_index++].loadFromFile(path))
            {
                std::cerr << "Error loading texture: " << path << std::endl;
                return 1;
            }
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

    sf::Clock animationClock;

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
                        // Po promocji wykonaj ruch AI, jeśli to tura czarnych
                        if (game.getCurrentPlayer() == BLACK)
                        {
                            Move aiMove = game.getBestMove(depth);
                            if (aiMove.fromX != -1 && game.isValidMove(aiMove, BLACK))
                            {
                                game.makeMove(aiMove);
                            }
                            else
                            {
                                std::cerr << "AI generated invalid move!" << std::endl;
                            }
                        }
                    }
                }
                else if (selectedX == -1 || game.getPiece(selectedX, selectedY) == EMPTY_PIECE)
                {
                    int x = event.mouseButton.y / squareSize;
                    int y = event.mouseButton.x / squareSize;
                    if(game.isValidPosition(x, y))
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
                            game.makeMove(move);
                            selectedX = -1;
                            selectedY = -1;
                            possibleMoves.clear();

                            if (!game.isPromotionPending() && !game.isGameOver() && game.getCurrentPlayer() == BLACK)
                            {
                                Move aiMove = game.getBestMove(depth);
                                if (game.isValidMove(aiMove, BLACK))
                                {
                                    game.makeMove(aiMove);
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
            }
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R && game.isGameOver())
            {
                game.resetGame();
                selectedX = -1;
                selectedY = -1;
                possibleMoves.clear();
            }
        }

        window.clear(sf::Color::White);

        // Draw sidebar
        window.draw(sidebar);
        window.draw(historyTitle);

        // Draw move history
        const auto& history = game.getMoveHistory();
        for (size_t i = 0; i < history.size(); ++i)
        {
            sf::Text moveText;
            moveText.setFont(font);
            moveText.setCharacterSize(16);
            moveText.setFillColor(sf::Color::Black);
            std::string moveStr = (i % 2 == 0 ? std::to_string(i / 2 + 1) + ". " : "   ") + moveToAlgebraic(history[i]);
            moveText.setString(moveStr);
            moveText.setPosition(820, 40 + i * 20);
            window.draw(moveText);
        }

        // Draw board
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                sf::RectangleShape square(sf::Vector2f(squareSize, squareSize));
                square.setPosition(j * squareSize, i * squareSize);
                square.setFillColor((i + j) % 2 == 0 ? sf::Color(255, 206, 158) : sf::Color(209, 139, 71));
                window.draw(square);

                if (i == selectedX && j == selectedY)
                {
                    sf::RectangleShape highlight(sf::Vector2f(squareSize, squareSize));
                    highlight.setPosition(j * squareSize, i * squareSize);
                    highlight.setFillColor(sf::Color(255, 255, 0, 128));
                    window.draw(highlight);
                }

                for (const auto &move: possibleMoves)
                {
                    if (move.toX == i && move.toY == j)
                    {
                        sf::RectangleShape moveHighlight(sf::Vector2f(squareSize, squareSize));
                        moveHighlight.setPosition(j * squareSize, i * squareSize);
                        moveHighlight.setFillColor(sf::Color(0, 255, 0, 128));
                        window.draw(moveHighlight);
                    }
                }
            }
        }

        // Draw pieces
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
            {
                Piece piece = game.getPiece(i, j);
                if (piece != EMPTY_PIECE)
                {
                    int textureIndex = -1;
                    if (piece.color == WHITE)
                    {
                        textureIndex = piece.type;
                    }
                    else
                    {
                        textureIndex = piece.type + 6;
                    }

                    if (textureIndex >= 0)
                    {
                        sf::Sprite pieceSprite(pieceTextures[textureIndex]);
                        pieceSprite.setPosition(j * squareSize, i * squareSize);
                        sf::Vector2u textureSize = pieceTextures[textureIndex].getSize();
                        pieceSprite.setScale(squareSize / textureSize.x, squareSize / textureSize.y);
                        window.draw(pieceSprite);
                    }
                }
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