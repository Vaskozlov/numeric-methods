#include <deque>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <isl/linalg/linspace.hpp>
#include <limits>
#include <mv/application_2d.hpp>
#include <mv/color.hpp>
#include <mv/gl/axes_2d.hpp>
#include <mv/gl/instances_holder.hpp>
#include <mv/gl/shaders/color_shader.hpp>
#include <mv/gl/shaders/shader_with_positioning.hpp>
#include <mv/gl/shape/plot_2d.hpp>
#include <mv/gl/shape/prism.hpp>
#include <mv/gl/shape/sphere.hpp>
#include <mv/shader.hpp>
#include <mvl/mvl.hpp>
#include <valarray>

class DifferentialEquations : public mv::Application2D
{
    using EulerStep = std::tuple<std::size_t, float, float, float>;

    constexpr static auto windowTitleBufferSize = 128;
    constexpr static glm::vec3 defaultCameraPosition{0.0F, 0.0F, 10.0F};

    std::array<char, windowTitleBufferSize> imguiWindowBuffer{};
    std::string input{"y + (1 + x) * y * y"};
    std::string realFunction = "-1/x";

    mv::gl::shape::Prism prism{0.03F, 20.0F, 4};
    mv::gl::shape::Axes2D plot{12, 0.009F};
    mv::gl::shape::Plot2D functionPlot;
    mv::gl::shape::Sphere sphere{1.0F};

    mv::gl::InstancesHolder<mv::gl::InstanceParameters> spheres;

    std::valarray<float> functionX;
    std::valarray<float> functionY;

    mv::Shader *colorShader = mv::gl::getColorShader();
    mv::Shader *shaderWithPositioning = mv::gl::getShaderWithPositioning();

    ImFont *font{};
    double pressTime = 0.0;
    float fontScale = 0.5F;
    float sphereRadius = 0.04F;

    float leftX = 1.0F;
    float rightX = 1.5F;
    float step = 0.1F;
    float epsilon = 1e-2F;
    float startY = -1.0F;

    std::array<bool, 3> showMethods{true, true, true};

    std::vector<mv::gl::InstanceParameters> eulerSpheres;
    std::vector<mv::gl::InstanceParameters> rungeKuttaSpheres;
    std::vector<mv::gl::InstanceParameters> milnaSpheres;

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

        functionPlot.loadData();
        functionPlot.vbo.bind();
        functionPlot.vao.bind(0, 3, GL_FLOAT, sizeof(glm::vec3), 0);

        sphere.loadData();
        sphere.vbo.bind();
        sphere.vao.bind(0, 3, GL_FLOAT, sizeof(glm::vec3), 0);

        spheres.vbo.bind();
        sphere.vao.bindInstanceParameters(1, 1);

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
        ImGui::InputText("Real function", &realFunction);

        ImGui::SliderFloat("Left x", &leftX, -10.0F, rightX);
        ImGui::SliderFloat("Right x", &rightX, leftX, 10.0F);
        ImGui::SliderFloat("Step (h)", &step, 1e-4F, 0.1F);
        ImGui::SliderFloat("Epsilon", &epsilon, 1e-2F, 0.2F, "%.1e");

        if (ImGui::SliderFloat("Radius", &sphereRadius, 0.1F, 2.0F)) {
            drawFunction();
        }

        const auto start_condition_text = fmt::format("y({})", leftX);
        ImGui::InputFloat(start_condition_text.c_str(), &startY);

        if (ImGui::Button("Compute")) {
            compute();
        }

        ImGui::SameLine();

        if (ImGui::Button("Draw")) {
            drawFunction();
        }

        if (ImGui::Checkbox("Show Euler", &showMethods[0])) {
            compute();
        }

        if (ImGui::Checkbox("Show Runge-Kutta", &showMethods[1])) {
            compute();
        }

        if (ImGui::Checkbox("Show Milna", &showMethods[2])) {
            compute();
        }

        ImGui::PopFont();
        ImGui::End();

        const auto viewMatrix = getResultedViewMatrix();

        colorShader->use();

        colorShader->setMat4("projection", viewMatrix);

        colorShader->setMat4(
            "model", glm::translate(glm::mat4(1.0F), glm::vec3{0.0F, 0.0F, -0.02F}));

        colorShader->setVec4("elementColor", mv::Color::BLACK);

        plot.draw();

        colorShader->setMat4("model", glm::mat4(1.0F));

        colorShader->setVec4("elementColor", mv::Color::ORANGE);
        functionPlot.draw();

        colorShader->setVec4("elementColor", mv::Color::FOREST);
        prism.drawAt(*colorShader, {leftX, 0.0F, 0.0F});

        colorShader->setVec4("elementColor", mv::Color::NAVY);
        prism.drawAt(*colorShader, {rightX, 0.0F, 0.0F});

        shaderWithPositioning->use();

        sphere.vao.bind();
        shaderWithPositioning->setMat4("projection", viewMatrix);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, sphere.vertices.size(), spheres.models.size());
    }

    auto compute() -> void
    {
        spheres.models.clear();

        try {
            if (showMethods[0]) {
                eulerSpheres.clear();
                const auto first = solveSimpleEuler(step, rightX);
                const auto second = solveSimpleEuler(step / 2.0F, rightX, false);

                spheres.models.insert_range(spheres.models.end(), eulerSpheres);

                fmt::println("Euler method r: {:e}", (first - second) / (2.0F - 1.0F));
            }

            if (showMethods[1]) {
                rungeKuttaSpheres.clear();

                const auto first = solveRungeKutta(step, rightX);
                const auto second = solveRungeKutta(step / 2.0, rightX, false);

                spheres.models.insert_range(spheres.models.end(), rungeKuttaSpheres);

                fmt::println("Runge-Kutta method r: {:e}", (first - second) / (16.0 - 1.0));
            }

            if (showMethods[2]) {
                milnaSpheres.clear();
                solveMilna(step, rightX);
                spheres.models.insert_range(spheres.models.end(), milnaSpheres);
            }
        } catch (...) {
        }

        spheres.loadData();
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
    auto addSphere(
        std::vector<mv::gl::InstanceParameters> &container, const mv::Color color,
        const glm::vec3 position, const float scale = 1.0F) const -> void
    {
        container.emplace_back(
            color,
            glm::scale(glm::translate(glm::mat4(1.0F), position), glm::vec3{sphereRadius * scale}));
    }

    auto drawFunction() -> void
    {
        const auto function_copy = realFunction;
        functionX = isl::linalg::linspace(leftX, rightX, 500);
        functionY.resize(functionX.size());

        auto root = mvl::constructRoot(function_copy);

        for (std::size_t i = 0; i < functionX.size(); ++i) {
            functionY[i] = static_cast<float>(root->compute(functionX[i], 0.0));
        }

        functionPlot.fill(functionX, functionY);
        functionPlot.loadData();
    }

    auto doRungeKuttaIteration(const mvl::ast::MathNode *equation, const double x, const double y)
        -> std::tuple<double, double, double, double, double>
    {
        const auto k1 =
            step
            * static_cast<double>(equation->compute(static_cast<double>(x), static_cast<double>(y)));

        const auto k2 =
            step
            * static_cast<double>(equation->compute(
                static_cast<double>(x + step / 2.0), static_cast<double>(y + k1 / 2.0)));

        const auto k3 =
            step
            * static_cast<double>(equation->compute(
                static_cast<double>(x + step / 2.0), static_cast<double>(y + k2 / 2.0)));

        const auto k4 = step
                        * static_cast<double>(equation->compute(
                            static_cast<double>(x + step), static_cast<double>(y + k3)));

        const auto new_y = y + 1.0F / 6.0F * (k1 + 2.0F * k2 + 2.0F * k3 + k4);

        return std::make_tuple(new_y, k1, k2, k3, k4);
    }

    auto solveSimpleEuler(double h, double x_end, bool add_spheres = true) -> double
    {
        auto x = static_cast<double>(leftX);
        auto y = static_cast<double>(startY);
        const auto input_copy = input;
        auto equation = mvl::constructRoot(input_copy);
        std::size_t iteration = 0;

        const auto real_function_copy = realFunction;
        ccl::parser::ast::SharedNode<mvl::ast::MathNode> real_root;

        try {
            real_root = mvl::constructRoot(real_function_copy);
        } catch (...) {
        }

        if (add_spheres) {
            fmt::println("Solving simple Euler:");
            fmt::println(
                "{:>4} {:>8} {:>8} {:>12} {:>8} {:>8}",
                "i",
                "x_i",
                "y_i",
                "f(x_i, y_i)",
                "Δ",
                "ΔC");
        }

        while (x <= x_end && iteration < 2000) {
            const auto f = static_cast<double>(
                equation->compute(x, y));

            if (add_spheres) {
                const auto runge_value =
                    std::abs(y - solveSimpleEuler(h / 2.0, x, false)) / (2.0 - 1.0);

                if (runge_value > epsilon) {
                    eulerSpheres.clear();
                    return solveSimpleEuler(h / 4.0F, x_end, add_spheres);
                }

                fmt::println(
                    "{:>4} {:>8.3} {:>8.3} {:>12.3} {:>8.3} {:>8.3}",
                    iteration,
                    x,
                    y,
                    f,
                    real_root == nullptr ? std::numeric_limits<double>::quiet_NaN()
                                         : std::abs(y - real_root->compute(x, 0)),
                    runge_value);

                addSphere(eulerSpheres, mv::Color::MAGENTA, {x, y, 0.0});
            }

            y = y + h * f;
            x += h;
            ++iteration;
        }

        if (add_spheres) {
            addSphere(eulerSpheres, mv::Color::MAGENTA, {x, y, 0.0});

            const auto method_error = real_root == nullptr
                                          ? std::numeric_limits<double>::quiet_NaN()
                                          : std::abs(y - real_root->compute(x, 0));

            fmt::println("{:>4} {:>8.3} {:>8.3} {:>8.3}", iteration, x, y, method_error);
        }

        return y;
    }

    auto solveRungeKutta(double h, double x_end, bool add_spheres = true, std::size_t depth = 0) -> double
    {
        if (depth > 16) {
            return 0.0;
        }

        auto x = leftX;
        auto y = startY;
        const auto input_copy = input;
        auto equation = mvl::constructRoot(input_copy);
        std::size_t iteration = 0;

        const auto real_function_copy = realFunction;
        ccl::parser::ast::SharedNode<mvl::ast::MathNode> real_root;

        try {
            real_root = mvl::constructRoot(real_function_copy);
        } catch (...) {
        }

        if (add_spheres) {
            fmt::println("Solving Runge-Kutta:");
            fmt::println(
                "{:>4} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8} {:>8}",
                "i",
                "x_i",
                "y_i",
                "k1",
                "k2",
                "k3",
                "k4",
                "Δ",
                "ΔC");
        }

        while (x <= x_end && iteration < 1000) {
            auto [new_y, k1, k2, k3, k4] = doRungeKuttaIteration(equation.get(), x, y);

            if (add_spheres) {
                const auto runge_value =
                    std::abs(y - solveRungeKutta(h / 2.0, x, false, depth + 1)) / (16.0 - 1.0);

                const auto absolute_error = real_root == nullptr
                                                ? std::numeric_limits<double>::quiet_NaN()
                                                : std::abs(y - real_root->compute(x, 0));

                if (runge_value > epsilon) {
                    rungeKuttaSpheres.clear();
                    return solveRungeKutta(h / 4.0F, x_end, add_spheres, depth + 1);
                }

                fmt::println(
                    "{:>4} {:>8.3} {:>8.3} {:>8.3} {:>8.3} {:>8.3} {:>8.3} {:>8.3} {:>8.3}",
                    iteration,
                    x,
                    y,
                    k1,
                    k2,
                    k3,
                    k4,
                    absolute_error,
                    runge_value);

                addSphere(rungeKuttaSpheres, mv::Color::AQUA, {x, y, 0.0F});
            }

            y = new_y;
            x += h;
            ++iteration;
        }

        if (add_spheres) {
            const auto absolute_error = real_root == nullptr
                                            ? std::numeric_limits<double>::quiet_NaN()
                                            : std::abs(y - real_root->compute(x, 0));

            fmt::println("{:>4} {:>8.3} {:>8.3} {:>8.3}", iteration, x, y, absolute_error);

            addSphere(rungeKuttaSpheres, mv::Color::RED, {x, y, 0.0F});
        }

        return y;
    }

    auto solveMilna(double h, double x_end, bool add_spheres = true) -> double
    {
        auto x = leftX;
        auto y = startY;
        const auto input_copy = input;
        auto equation = mvl::constructRoot(input_copy);
        std::size_t iteration = 0;

        std::deque<double> y_values{y};
        std::deque<double> f_values{static_cast<double>(equation->compute(x, y))};

        const auto real_function_copy = realFunction;
        ccl::parser::ast::SharedNode<mvl::ast::MathNode> real_root;

        try {
            real_root = mvl::constructRoot(real_function_copy);
        } catch (...) {
        }

        if (add_spheres) {
            fmt::println("Solving Milna:");
            fmt::println("Solving Runge-Kutta:");
            fmt::println("{:>4} {:>8} {:>8} {:>12} {:>8}", "i", "x_i", "y_i", "f(x_i, y_i)", "Δ");
        }

        while (x <= x_end && iteration < 3 && iteration < 1000) {
            auto [new_y, k1, k2, k3, k4] = doRungeKuttaIteration(equation.get(), x, y);

            if (add_spheres) {
                const auto runge_value =
                    std::abs(y - solveRungeKutta(h / 2.0, x, false)) / (16.0 - 1.0);

                if (runge_value > epsilon) {
                    milnaSpheres.clear();
                    return solveMilna(h / 4.0F, x_end, add_spheres);
                }

                const auto absolute_error = real_root == nullptr
                                                ? std::numeric_limits<double>::quiet_NaN()
                                                : std::abs(y - real_root->compute(x, 0));

                fmt::println(
                    "{:>4} {:>8.3} {:>8.3} {:>12.3} {:>8.3}",
                    iteration,
                    x,
                    y,
                    equation->compute(x, y),
                    absolute_error);

                addSphere(milnaSpheres, mv::Color::RED, {x, y, 0.0F});
            }

            y_values.emplace_front(new_y);
            f_values.emplace_front(equation->compute(x + h, new_y));

            y = new_y;
            x += h;
            ++iteration;
        }

        if (add_spheres) {
            addSphere(milnaSpheres, mv::Color::RED, {x, y, 0.0F});

            fmt::println("Iterations with correction");
        }

        while (x <= x_end&& iteration < 1000) {
            auto predicted_y =
                y_values.at(3)
                + 4.0F / 3.0F * step
                      * (2.0F * f_values.at(0) - f_values.at(1) + 2.0F * f_values.at(2));

            auto corrected_y = predicted_y;

            do {
                predicted_y = corrected_y;

                const auto f = static_cast<double>(equation->compute(
                    static_cast<double>(x + step), static_cast<double>(predicted_y)));

                corrected_y =
                    y_values.at(1) + step / 3.0F * (f_values.at(1) + 4 * f_values.at(0) + f);
            } while (std::abs(corrected_y - predicted_y) >= epsilon);

            y_values.pop_back();
            f_values.pop_back();

            y_values.emplace_front(corrected_y);
            f_values.emplace_front(static_cast<double>(
                equation->compute(static_cast<double>(x + h), static_cast<double>(corrected_y))));

            if (add_spheres) {
                const auto absolute_error = real_root == nullptr
                                                ? std::numeric_limits<double>::quiet_NaN()
                                                : std::abs(y - real_root->compute(x, 0));

                fmt::println(
                    "{:>4} {:>8.3} {:>8.3} {:>12.3} {:>8.3}",
                    iteration,
                    x,
                    y,
                    f_values.front(),
                    absolute_error);

                addSphere(milnaSpheres, mv::Color::RED, {x, y, 0.0F});
            }

            y = corrected_y;
            x += h;
            ++iteration;
        }

        if (add_spheres) {
            fmt::println(
                "{:>4} {:>8.3} {:>8.3} {:>12.3} {:>8.3}",
                iteration,
                x,
                y,
                f_values.front(),
                real_root == nullptr ? std::numeric_limits<double>::quiet_NaN()
                                     : std::abs(y - real_root->compute(x, 0)));

            addSphere(milnaSpheres, mv::Color::RED, {x, y, 0.0F});
        }

        return y;
    }
};

auto main() -> int
{
    DifferentialEquations application(1000, 800, "Lab6");
    application.run();
}
