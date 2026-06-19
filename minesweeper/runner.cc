#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <set>
#include <thread>
#include <chrono>
#include <print> // C++23 printing
#include "minesweeper.h"

// Constants
constexpr int HEIGHT = 8;
constexpr int WIDTH = 8;
constexpr int MINES = 8;

// Colors
const sf::Color BLACK(0, 0, 0);
const sf::Color GRAY(180, 180, 180);
const sf::Color WHITE(255, 255, 255);

// Helper function to perfectly center SFML Text inside a given rectangle
void centerText(sf::Text& text, const sf::FloatRect& bounds) {
    sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    text.setPosition(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
}

int main() {
    // Create Game Window
    constexpr int width = 600;
    constexpr int height = 400;
    sf::RenderWindow window(sf::VideoMode(width, height), "Minesweeper");

    // Fonts
    sf::Font smallFont, mediumFont, largeFont;
    if (!smallFont.loadFromFile("assets/fonts/OpenSans-Regular.ttf") ||
        !mediumFont.loadFromFile("assets/fonts/OpenSans-Regular.ttf") ||
        !largeFont.loadFromFile("assets/fonts/OpenSans-Regular.ttf")) {
        std::println(stderr, "Error loading fonts! Ensure 'assets/fonts/OpenSans-Regular.ttf' exists.");
        return -1;
    }

    // Add Images (Textures and Sprites)
    sf::Texture flagTex, mineTex;
    if (!flagTex.loadFromFile("assets/images/flag.png") ||
        !mineTex.loadFromFile("assets/images/mine.png")) {
        std::println(stderr, "Error loading images! Ensure 'assets/images/' contains flag.png and mine.png.");
        return -1;
    }

    // Compute board size
    constexpr float BOARD_PADDING = 20.0f;
    float board_width = ((2.0f / 3.0f) * width) - (BOARD_PADDING * 2);
    float board_height = height - (BOARD_PADDING * 2);
    float cell_size = std::min(board_width / WIDTH, board_height / HEIGHT);
    sf::Vector2f board_origin(BOARD_PADDING, BOARD_PADDING);

    // Scale Sprites to match cell size
    sf::Sprite flagSprite(flagTex), mineSprite(mineTex);
    flagSprite.setScale(cell_size / flagTex.getSize().x, cell_size / flagTex.getSize().y);
    mineSprite.setScale(cell_size / mineTex.getSize().x, cell_size / mineTex.getSize().y);

    // Create game and AI agent
    Minesweeper game(HEIGHT, WIDTH, MINES);
    MinesweeperAI ai(HEIGHT, WIDTH);

    // Keep track of states
    std::set<Cell> revealed;
    std::set<Cell> flags;
    bool lost = false;
    bool instructions = true;

    // Cache static UI rectangles
    sf::FloatRect buttonRectBounds(width / 4.0f, (3.0f / 4.0f) * height, width / 2.0f, 50.0f);
    sf::FloatRect aiButtonBounds((2.0f / 3.0f) * width + BOARD_PADDING, (1.0f / 3.0f) * height - 50, (width / 3.0f) - BOARD_PADDING * 2, 50);
    sf::FloatRect resetButtonBounds((2.0f / 3.0f) * width + BOARD_PADDING, (1.0f / 3.0f) * height + 20, (width / 3.0f) - BOARD_PADDING * 2, 50);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(BLACK);

        if (instructions) {
            // Title
            sf::Text title("Play Minesweeper", largeFont, 40);
            title.setFillColor(WHITE);
            centerText(title, sf::FloatRect(0, 0, width, 100)); // Centered top
            window.draw(title);

            // Rules
            const char* rules[] = {
                "Click a cell to reveal it.",
                "Right-click a cell to mark it as a mine.",
                "Mark all mines successfully to win!"
            };
            for (int i = 0; i < 3; ++i) {
                sf::Text line(rules[i], smallFont, 20);
                line.setFillColor(WHITE);
                centerText(line, sf::FloatRect(0, 150.0f + 30.0f * i - 15.0f, width, 30.0f));
                window.draw(line);
            }

            // Play game button
            sf::RectangleShape playButton(sf::Vector2f(buttonRectBounds.width, buttonRectBounds.height));
            playButton.setPosition(buttonRectBounds.left, buttonRectBounds.top);
            playButton.setFillColor(WHITE);
            window.draw(playButton);

            sf::Text playText("Play Game", mediumFont, 28);
            playText.setFillColor(BLACK);
            centerText(playText, buttonRectBounds);
            window.draw(playText);

            // Check if play button clicked
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (buttonRectBounds.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                    instructions = false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(300));
                }
            }

            window.display();
            continue;
        }

        // Draw Board & calculate cell bounds mapping
        std::vector<std::vector<sf::FloatRect>> cellBounds(HEIGHT, std::vector<sf::FloatRect>(WIDTH));
        for (int i = 0; i < HEIGHT; ++i) {
            for (int j = 0; j < WIDTH; ++j) {
                sf::FloatRect rect(
                    board_origin.x + j * cell_size,
                    board_origin.y + i * cell_size,
                    cell_size, cell_size
                );
                cellBounds[i][j] = rect;

                // Draw cell background
                sf::RectangleShape cellShape(sf::Vector2f(cell_size, cell_size));
                cellShape.setPosition(rect.left, rect.top);
                cellShape.setFillColor(GRAY);
                cellShape.setOutlineThickness(-3.0f); // Negative draws outline inside
                cellShape.setOutlineColor(WHITE);
                window.draw(cellShape);

                Cell currentCell{i, j};

                // Add mine, flag, or number if needed
                if (game.is_mine(currentCell) && lost) {
                    mineSprite.setPosition(rect.left, rect.top);
                    window.draw(mineSprite);
                } else if (flags.contains(currentCell)) {
                    flagSprite.setPosition(rect.left, rect.top);
                    window.draw(flagSprite);
                } else if (revealed.contains(currentCell)) {
                    sf::Text neighbors(std::to_string(game.nearby_mines(currentCell)), smallFont, 20);
                    neighbors.setFillColor(BLACK);
                    centerText(neighbors, rect);
                    window.draw(neighbors);
                }
            }
        }

        // AI Move Button
        sf::RectangleShape aiButton(sf::Vector2f(aiButtonBounds.width, aiButtonBounds.height));
        aiButton.setPosition(aiButtonBounds.left, aiButtonBounds.top);
        aiButton.setFillColor(WHITE);
        window.draw(aiButton);

        sf::Text aiText("AI Move", mediumFont, 28);
        aiText.setFillColor(BLACK);
        centerText(aiText, aiButtonBounds);
        window.draw(aiText);

        // Reset Button
        sf::RectangleShape resetButton(sf::Vector2f(resetButtonBounds.width, resetButtonBounds.height));
        resetButton.setPosition(resetButtonBounds.left, resetButtonBounds.top);
        resetButton.setFillColor(WHITE);
        window.draw(resetButton);

        sf::Text resetText("Reset", mediumFont, 28);
        resetText.setFillColor(BLACK);
        centerText(resetText, resetButtonBounds);
        window.draw(resetText);

        // Display Status Text
        std::string statusStr = lost ? "Lost" : (game.mines == flags ? "Won" : "");
        sf::Text statusText(statusStr, mediumFont, 28);
        statusText.setFillColor(WHITE);
        centerText(statusText, sf::FloatRect((5.0f / 6.0f) * width - 50.0f, (2.0f / 3.0f) * height - 20.0f, 100.0f, 40.0f));
        window.draw(statusText);

        std::optional<Cell> move = std::nullopt;

        // Interaction Logic
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        float mx = static_cast<float>(mousePos.x);
        float my = static_cast<float>(mousePos.y);

        if (sf::Mouse::isButtonPressed(sf::Mouse::Right) && !lost) {
            for (int i = 0; i < HEIGHT; ++i) {
                for (int j = 0; j < WIDTH; ++j) {
                    Cell c{i, j};
                    if (cellBounds[i][j].contains(mx, my) && !revealed.contains(c)) {
                        if (flags.contains(c)) {
                            flags.erase(c);
                        } else {
                            flags.insert(c);
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }
                }
            }
        } else if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (aiButtonBounds.contains(mx, my) && !lost) {
                move = ai.make_safe_move();
                if (!move.has_value()) {
                    move = ai.make_random_move();
                    if (!move.has_value()) {
                        flags = ai.mines; 
                        std::println("No moves left to make.");
                    } else {
                        std::println("No known safe moves, AI making random move.");
                    }
                } else {
                    std::println("AI making safe move.");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            } 
            else if (resetButtonBounds.contains(mx, my)) {
                game = Minesweeper(HEIGHT, WIDTH, MINES);
                ai = MinesweeperAI(HEIGHT, WIDTH);
                revealed.clear();
                flags.clear();
                lost = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            } 
            else if (!lost) {
                for (int i = 0; i < HEIGHT; ++i) {
                    for (int j = 0; j < WIDTH; ++j) {
                        Cell c{i, j};
                        if (cellBounds[i][j].contains(mx, my) && !flags.contains(c) && !revealed.contains(c)) {
                            move = c;
                            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Debounce click
                        }
                    }
                }
            }
        }

        // Apply Move and Update Knowledge
        if (move.has_value()) {
            if (game.is_mine(move.value())) {
                lost = true;
            } else {
                int nearby = game.nearby_mines(move.value());
                revealed.insert(move.value());
                ai.add_knowledge(move.value(), nearby);
            }
        }

        window.display();
    }

    return 0;
}
