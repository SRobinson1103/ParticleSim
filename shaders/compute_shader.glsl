#version 450 core

layout(local_size_x = 1, local_size_y = 1) in;

layout(std430, binding = 0) buffer GridData {
    uint grid[];
};

uniform int gridWidth;
uniform int gridHeight;

const uint EMPTY = 0;
const uint SAND = 1;
const uint WATER = 2;
const uint SMOKE = 3;

void main()
{
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint idx = y * gridWidth + x;

    if (grid[idx] == SAND)
    {
        // Sand falls down if the cell below is empty
        uint below = idx + gridWidth;
        if (y + 1 < gridHeight && grid[below] == EMPTY)
        {
            grid[below] = SAND;
            grid[idx] = EMPTY;
        }
    }
    else if (grid[idx] == WATER)
    {
        // Water flows downward if possible
        uint below = idx + gridWidth;
        uint left = idx - 1;
        uint right = idx + 1;

        if (y + 1 < gridHeight && grid[below] == EMPTY)
        {
            grid[below] = WATER;
            grid[idx] = EMPTY;
        }
        else if (x > 0 && grid[left] == EMPTY)
        {
            grid[left] = WATER;
            grid[idx] = EMPTY;
        }
        else if (x + 1 < gridWidth && grid[right] == EMPTY)
        {
            grid[right] = WATER;
            grid[idx] = EMPTY;
        }
    }
    else if (grid[idx] == SMOKE)
    {
        // Smoke rises if the cell above is empty
        uint above = idx - gridWidth;
        if (y > 0 && grid[above] == EMPTY)
        {
            grid[above] = SMOKE;
            grid[idx] = EMPTY;
        }
    }
}
