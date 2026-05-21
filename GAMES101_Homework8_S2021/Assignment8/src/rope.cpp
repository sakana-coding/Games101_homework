#include <iostream>
#include <vector>

#include "CGL/vector2D.h"

#include "mass.h"
#include "rope.h"
#include "spring.h"
const float damping_factor = 5e-5;
namespace CGL
{

Rope::Rope(
    Vector2D start, Vector2D end, int num_nodes, float node_mass, float k, vector<int> pinned_nodes)
{

    for (int i = 0; i < num_nodes; i++)
    {
        masses.push_back(new Mass(
            start + (end - start) * (static_cast<float>(i) / (num_nodes - 1)), node_mass, false));
    }
    for (auto& i : pinned_nodes)
    {
        masses[i]->pinned = true;
    }
    for (int i = 0; i < num_nodes - 1; i++)
    {
        springs.push_back(new Spring(masses[i], masses[i + 1], k));
    }
}

void Rope::simulateEuler(float delta_t, Vector2D gravity)
{
    for (auto& s : springs)
    {
        Vector2D m1_to_m2 = s->m2->position - s->m1->position;
        float len = m1_to_m2.norm();
        Vector2D f = s->k * (m1_to_m2 / len) * (len - s->rest_length);
        s->m1->forces += f;
        s->m2->forces -= f;
    }

    for (auto& m : masses)
    {
        if (!m->pinned)
        {
            m->forces += m->mass * gravity;
            Vector2D a = m->forces / m->mass;
            m->velocity += a * delta_t;
            m->velocity *= (1.0f - 0.01f * delta_t); // global damping
            m->position += m->velocity * delta_t;
        }

        // Reset all forces on each mass
        m->forces = Vector2D(0, 0);
    }
}

void Rope::simulateVerlet(float delta_t, Vector2D gravity)
{
    for (auto& s : springs)
    {
        Vector2D m1_to_m2 = s->m2->position - s->m1->position;
        float len = m1_to_m2.norm();
        Vector2D f = s->k * (m1_to_m2 / len) * (len - s->rest_length);
        s->m1->forces += f;
        s->m2->forces -= f;
    }

    for (auto& m : masses)
    {
        if (!m->pinned)
        {
            Vector2D temp_position = m->position;
            m->forces += m->mass * gravity;
            Vector2D a = m->forces / m->mass;
            m->position = temp_position + (1 - damping_factor)*(temp_position - m->last_position) +
                          a * delta_t * delta_t;
            m->last_position = temp_position;
            // TODO (Part 4): Add global Verlet damping
        }
        m->forces = Vector2D(0, 0);
    }
}
} // namespace CGL
