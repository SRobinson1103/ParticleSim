#include "Grid.hpp"

Grid::Grid(unsigned width, unsigned height)
    : width(width), height(height), cells(width* height, CellType::EMPTY)
{
}


void Grid::update(float deltaTime)
{
    std::vector<std::pair<unsigned, unsigned>> nextActiveCells;

    for (const auto& [x, y] : activeCells)
    {
        switch (cells[index(x, y)])
        {
        case CellType::SAND:
            if (y + 1 < height && cells[index(x, y + 1)] == CellType::EMPTY)
            {
                cells[index(x, y + 1)] = CellType::SAND;
                cells[index(x, y)] = CellType::EMPTY;
                nextActiveCells.emplace_back(x, y + 1);
            }
            break;

        case CellType::WATER:
            if (y + 1 < height && cells[index(x, y + 1)] == CellType::EMPTY)
            {
                cells[index(x, y + 1)] = CellType::WATER;
                cells[index(x, y)] = CellType::EMPTY;
                nextActiveCells.emplace_back(x, y + 1);
            }
            else if (x > 0 && cells[index(x - 1, y)] == CellType::EMPTY)
            {
                cells[index(x - 1, y)] = CellType::WATER;
                cells[index(x, y)] = CellType::EMPTY;
                nextActiveCells.emplace_back(x - 1, y);
            }
            else if (x + 1 < width && cells[index(x + 1, y)] == CellType::EMPTY)
            {
                cells[index(x + 1, y)] = CellType::WATER;
                cells[index(x, y)] = CellType::EMPTY;
                nextActiveCells.emplace_back(x + 1, y);
            }
            break;

        case CellType::SMOKE:
            if (y > 0 && cells[index(x, y - 1)] == CellType::EMPTY)
            {
                cells[index(x, y - 1)] = CellType::SMOKE;
                cells[index(x, y)] = CellType::EMPTY;
                nextActiveCells.emplace_back(x, y - 1);
            }
            else if (x > 0 && cells[index(x - 1, y)] == CellType::EMPTY)
            {
                cells[index(x - 1, y)] = CellType::SMOKE;
                cells[index(x, y)] = CellType::EMPTY;
                nextActiveCells.emplace_back(x - 1, y);
            }
            else if (x + 1 < width && cells[index(x + 1, y)] == CellType::EMPTY)
            {
                cells[index(x + 1, y)] = CellType::SMOKE;
                cells[index(x, y)] = CellType::EMPTY;
                nextActiveCells.emplace_back(x + 1, y);
            }
            break;

        default:
            break;
        }
    }

    activeCells = std::move(nextActiveCells);
}



void Grid::render(sf::RenderWindow& window)
{
    const float cellSize = 4.0f; // Each cell is 4x4 pixels

    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width; ++x)
        {
            sf::RectangleShape cellShape(sf::Vector2f(cellSize, cellSize));
            cellShape.setPosition(sf::Vector2f(x * cellSize, y * cellSize));

            switch (cells[index(x, y)])
            {
            case CellType::EMPTY:
                cellShape.setFillColor(sf::Color::Black);
                break;
            case CellType::SAND:
                cellShape.setFillColor(sf::Color(194, 178, 128));
                break;
            case CellType::WATER:
                cellShape.setFillColor(sf::Color::Blue);
                break;
            case CellType::SMOKE:
                cellShape.setFillColor(sf::Color(128, 128, 128));
                break;
            }

            window.draw(cellShape);
        }
    }
}

void Grid::setCell(unsigned x, unsigned y, CellType type)
{
    if (x < width && y < height)
    {
        unsigned idx = index(x, y);
        cells[idx] = type;

        // Add to active cells if it's not empty
        if (type != CellType::EMPTY)
        {
            activeCells.emplace_back(x, y);
        }
    }
}
