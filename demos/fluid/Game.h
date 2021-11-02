#ifndef GAME_H
#define GAME_H

#include "World6.h"


struct Game {
    private:
        bool game_should_close = false;

        static constexpr float world_size_y = 100.0f;
        Vec<2> window_size;
        Vec<2> world_size;
        Matrix4f mvp;
        World world;

        bool pressed_s = false;
        bool pressed_r = false;
        bool pressed_lmb = false;
        bool pressed_rmb = false;
        bool pressed_mmb = false;
        bool pressed_space = false;
        bool physics_on = false;

        Vec<2> cursor;

        static Matrix4f mvp_for_world_size(const Vec<2>& world_size) noexcept {
            return Matrix4f::orthographic(0.0f, world_size[0], 0.0f, world_size[1], -1.0f, 1.0f);
        }

        static Vec<2> world_size_for_aspect(float aspect_w_h) noexcept {
            return Vec<2>{ world_size_y * aspect_w_h, world_size_y };
        }

        Vec<2> cursor_to_world_coord(const Vec<2>& cursor) const noexcept {
            return Vec<2>{ cursor[0] / window_size[0] * world_size[0], world_size[1] - cursor[1] / window_size[1] * world_size[1] };
        }

        static Vec<2> get_cursor(GLFWwindow* window) noexcept {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            return Vec<2>{ static_cast<float>(xpos), static_cast<float>(ypos) };
        }

    public:
        Game(unsigned int width, unsigned int height) noexcept
                : window_size(Vec<2>{ 1.0f * width, 1.0f * height }),
                  world_size(world_size_for_aspect(static_cast<float>(width) / static_cast<float>(height))),
                  mvp(mvp_for_world_size(world_size)),
                  world(World{ world_size }) {
            world.set_mvp(mvp);
        }

        void update_and_render(float dt) noexcept {
            world.update_and_render(dt, cursor_to_world_coord(cursor));
        }

        void register_input(GLFWwindow* window) noexcept {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) game_should_close = true;

            cursor = get_cursor(window);

            if ((glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) && physics_on) {
                physics_on = false;
                world.flip_physics();
            } else if ((glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) && !physics_on) {
                physics_on = true;
                world.flip_physics();
            }
        }

        void reset_dimensions(unsigned int width, unsigned int height) noexcept {
            window_size = Vec<2>{ 1.0f * width, 1.0f * height };
            const float aspect_w_h = static_cast<float>(width) / static_cast<float>(height);
            world_size = world_size_for_aspect(aspect_w_h);
            mvp = mvp_for_world_size(world_size);
            world.set_mvp(mvp);
            world.set_size(world_size);
        }

        bool should_close() const noexcept {
            return game_should_close;
        }
};


#endif
