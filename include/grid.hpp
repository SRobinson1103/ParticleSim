#ifndef GRID_HPP
#define GRID_HPP

#include <vector>
#include <SFML/Graphics.hpp>

// Particle types
enum class CellType
{
    EMPTY,
    SAND,
    WATER,
    SMOKE
};

class Grid
{
public:
    Grid(unsigned width, unsigned height);

    void update(float deltaTime);
    void render(sf::RenderWindow& window);
    void setCell(unsigned x, unsigned y, CellType type);

private:
    unsigned width, height;
    std::vector<CellType> cells; // Flat 1D grid
    std::vector<std::pair<unsigned, unsigned>> activeCells; // Track active cells

    unsigned index(unsigned x, unsigned y) const { return y * width + x; }
};

#endif
