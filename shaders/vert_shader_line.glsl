#version 450 core

layout(location = 0) in vec2 position;

out vec3 fragColor;

bool isMultipleOf0_25(float value)
{
    float epsilon = 0.001;
    float remainder = mod(value, 0.25);

    return abs(remainder) < epsilon || abs(remainder - 0.25) < epsilon;
}

void main()
{
    fragColor = vec3(0.0, 0.0, 0.0);
    if (isMultipleOf0_25(position.x) && isMultipleOf0_25(position.y))
    {
        fragColor = vec3(1.0, 1.0, 1.0);
    }

    gl_Position = vec4(position, 0.0, 1.0);
}
