#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <SFML/Graphics.hpp>

class Particle {
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    float lifetime; // Lifetime in seconds

    Particle(sf::Vector2f pos, sf::Vector2f vel, float life)
        : position(pos), velocity(vel), lifetime(life) {}
};

#endif // PARTICLE_HPP
