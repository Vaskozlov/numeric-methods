#include <deque>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <mv/application_2d.hpp>
#include <mv/color.hpp>
#include <mv/gl/axes_2d.hpp>
#include <mv/gl/shaders/color_shader.hpp>
#include <mv/gl/shape/prism.hpp>
#include <mv/shader.hpp>
#include <mvl/mvl.hpp>

class DifferentialEquations : public mv::Application2D
{
    using EulerStep = std::tuple<std::size_t, float, float, float>;

    constexpr static auto windowTitleBufferSize = 128;
    constexpr static glm::vec3 defaultCameraPosition{0.0F, 0.0F, 10.0F};

    std::array<char, windowTitleBufferSize> imguiWindowBuffer{};
    std::string input{"y + (1 + x) * y * y"};

    mv::gl::shape::Prism prism{0.03F, 20.0F, 4};
    mv::gl::shape::Axes2D plot{12, 0.009F};

    mv::Shader *colorShader = mv::gl::getColorShader();

    ImFont *font{};
    double pressTime = 0.0;
    float fontScale = 0.5F;

    float leftX = 1.0F;
    float rightX = 1.5F;
    float step = 0.2F;
    float epsilon = 1e-4F;
    float startY = -1.0F;

public:
    using Application2D::Application2D;

    auto init() -> void override
    {
        mv::Application2D::init();

        prism.loadData();
        prism.vbo.bind();
        prism.vao.bind(0, 3, GL_FLOAT, sizeof(glm::vec3), 0);

        plot.loadData();
        plot.vbo.bind();
        plot.vao.bind(0, 3, GL_FLOAT, sizeof(glm::vec3), 0);

        font = loadFont();

        setClearColor(mv::Color::LIGHT_GRAY);
        ImGui::StyleColorsLight();

        camera.setPosition(defaultCameraPosition);
    }

    auto update() -> void override
    {
        mv::Application2D::update();

        fmt::format_to_n(
            imguiWindowBuffer.data(),
            imguiWindowBuffer.size(),
            "Настройки. FPS: {:#.4}###SettingWindowTitle",
            ImGui::GetIO().Framerate);

        ImGui::Begin(imguiWindowBuffer.data());
        ImGui::PushFont(font);
        ImGui::SetWindowFontScale(fontScale);

        ImGui::InputText("Equation", &input);

        if (ImGui::Button("Compute")) {
            solveSimpleEuler();
            solveRungeKutta();
            solveMilna();
        }

        ImGui::SliderFloat("Left x", &leftX, -10.0F, rightX);
        ImGui::SliderFloat("Right x", &rightX, leftX, 10.0F);
        ImGui::SliderFloat("Step (h)", &step, 1e-4F, 1.0F);
        ImGui::SliderFloat("Epsilon", &epsilon, 1e-6F, 1e-1F, "%.1e");

        const auto start_condition_text = fmt::format("y({})", leftX);
        ImGui::InputFloat(start_condition_text.c_str(), &startY);

        ImGui::PopFont();
        ImGui::End();

        const auto viewMatrix = getResultedViewMatrix();

        colorShader->use();

        colorShader->setMat4("projection", viewMatrix);

        colorShader->setMat4(
            "model", glm::translate(glm::mat4(1.0F), glm::vec3{0.0F, 0.0F, -0.02F}));

        colorShader->setVec4("elementColor", mv::Color::BLACK);

        plot.draw();

        colorShader->setVec4("elementColor", mv::Color::FOREST);
        prism.drawAt(*colorShader, {leftX, 0.0F, 0.0F});

        colorShader->setVec4("elementColor", mv::Color::NAVY);
        prism.drawAt(*colorShader, {rightX, 0.0F, 0.0F});
    }

    auto processInput() -> void override
    {
        constexpr static auto key_press_delay = 0.2;

        Application2D::processInput();

        const auto left_shift_pressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        const auto key_g_pressed = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;

        if (left_shift_pressed && key_g_pressed) {
            const auto mode = glfwGetInputMode(window, GLFW_CURSOR);
            const double new_press_time = glfwGetTime();

            if (new_press_time - pressTime < key_press_delay) {
                return;
            }

            pressTime = new_press_time;
            firstMouse = true;
            isMouseShowed = mode == GLFW_CURSOR_DISABLED;

            glfwSetInputMode(
                window, GLFW_CURSOR, isMouseShowed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
    }

    auto onMouseRelativeMovement(const double delta_x, const double delta_y) -> void override
    {
        const auto scale = camera.getZoom() / 9000.0F;

        camera.relativeMove(camera.getUp() * static_cast<float>(delta_y) * scale);
        camera.relativeMove(camera.getRight() * static_cast<float>(delta_x) * scale);
    }

    auto onScroll(const double x_offset, const double y_offset) -> void override
    {
        const auto scale = static_cast<double>(camera.getZoom()) / 180.0;
        Application2D::onScroll(x_offset * scale, y_offset * scale);
    }

private:
    auto doRungeKuttaIteration(const mvl::ast::MathNode *equation, const float x, const float y)
        -> std::tuple<float, float, float, float, float>
    {
        const auto k1 =
            step
            * static_cast<float>(equation->compute(static_cast<double>(x), static_cast<double>(y)));

        const auto k2 =
            step
            * static_cast<float>(equation->compute(
                static_cast<double>(x + step / 2.0F), static_cast<double>(y + k1 / 2.0F)));

        const auto k3 =
            step
            * static_cast<float>(equation->compute(
                static_cast<double>(x + step / 2.0F), static_cast<double>(y + k2 / 2.0F)));

        const auto k4 = step
                        * static_cast<float>(equation->compute(
                            static_cast<double>(x + step), static_cast<double>(y + k3)));

        const auto new_y = y + 1.0F / 6.0F * (k1 + 2.0F * k2 + 2.0F * k3 + k4);

        return std::make_tuple(new_y, k1, k2, k3, k4);
    }

    auto solveSimpleEuler() -> void
    {
        const auto xEnd = rightX;
        auto x = leftX;
        auto y = startY;
        const auto input_copy = input;
        auto equation = mvl::constructRoot(input_copy);
        std::size_t iteration = 0;

        fmt::println("Solving simple Euler:");
        fmt::println("{:>4} {:>8} {:>8} {:>12}", "i", "x_i", "y_i", "f(x_i, y_i)");

        while (x < xEnd + step) {
            const auto f = static_cast<float>(
                equation->compute(static_cast<double>(x), static_cast<double>(y)));

            fmt::println("{:>4} {:>8.3} {:>8.3} {:>12.3}", iteration, x, y, f);

            y = y + step * f;
            x += step;
            ++iteration;
        }
    }

    auto solveRungeKutta() -> void
    {
        const auto xEnd = rightX;
        auto x = leftX;
        auto y = startY;
        const auto input_copy = input;
        auto equation = mvl::constructRoot(input_copy);
        std::size_t iteration = 0;

        fmt::println("Solving Runge-Kutta:");
        fmt::println(
            "{:>4} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8}", "i", "x_i", "y_i", "k1", "k2", "k3", "k4");

        while (x < xEnd + step) {
            auto [new_y, k1, k2, k3, k4] = doRungeKuttaIteration(equation.get(), x, y);

            fmt::println(
                "{:>4} {:>8.3} {:>8.3} {:>8.3} {:>8.3} {:>8.3} {:>8.3}",
                iteration,
                x,
                y,
                k1,
                k2,
                k3,
                k4);

            y = new_y;
            x += step;
            ++iteration;
        }
    }

    auto solveMilna() -> void
    {
        const auto xEnd = rightX;
        auto x = leftX;
        auto y = startY;
        const auto input_copy = input;
        auto equation = mvl::constructRoot(input_copy);
        std::size_t iteration = 0;

        std::deque<float> y_values{y};
        std::deque<float> f_values{static_cast<float>(equation->compute(x, y))};

        fmt::println("Solving Milna:");
        fmt::println("Solving simple Euler:");
        fmt::println("{:>4} {:>8} {:>8} {:>12}", "i", "x_i", "y_i", "f(x_i, y_i)");

        while (x < xEnd + step && iteration < 3) {
            auto [new_y, k1, k2, k3, k4] = doRungeKuttaIteration(equation.get(), x, y);

            fmt::println(
                "{:>4} {:>8.3} {:>8.3} {:>8.3} ", iteration, x, y, equation->compute(x, y));

            y_values.emplace_front(new_y);
            f_values.emplace_front(equation->compute(x + step, new_y));

            y = new_y;
            x += step;
            ++iteration;
        }

        fmt::println("Iterations with correction");

        while (x < xEnd + step) {
            auto predicted_y =
                y_values.at(3)
                + 4.0F / 3.0F * step
                      * (2.0F * f_values.at(0) - f_values.at(1) + 2.0F * f_values.at(2));

            auto corrected_y = predicted_y;

            do {
                predicted_y = corrected_y;

                const auto f = static_cast<float>(
                    equation->compute(static_cast<double>(x + step), static_cast<double>(predicted_y)));

                corrected_y =
                    y_values.at(1) + step / 3.0F * (f_values.at(1) + 4 * f_values.at(0) + f);
            } while (std::abs(corrected_y - predicted_y) >= epsilon);

            y_values.pop_back();
            f_values.pop_back();

            y_values.emplace_front(corrected_y);
            f_values.emplace_front(static_cast<float>(
                equation->compute(static_cast<double>(x + step), static_cast<double>(corrected_y))));

            fmt::println("{:>4} {:>8.3} {:>8.3} {:>12.3}", iteration, x, y, f_values.front());

            y = corrected_y;
            x += step;
            ++iteration;
        }
    }
};

auto main() -> int
{
    DifferentialEquations application(1000, 800, "Lab6");
    application.run();
}
