#version 450 core

void main()
{
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint idx = y * gridWidth + x;

    // For debugging, write a unique pattern to the grid
    if (idx % 2 == 0)
        grid[idx] = 1; // Mark even indices
    else
        grid[idx] = 2; // Mark odd indices
}