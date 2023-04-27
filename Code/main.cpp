#include "transform.hpp"
#include "buffer.hpp"
#include "vertex_array.hpp"
#include "program.hpp"
#include "time.hpp"
#include "render_pass.hpp"
#include "camera.hpp"
#include "file.hpp"
#include "material.hpp"
#include "texture.hpp"
#include "importer.hpp"
#include "light.hpp"
#include "physics.hpp"
#include "editor.hpp"
#include "light_window.hpp"

#include <backends/imgui_impl_glfw.h>

#include <btBulletCollisionCommon.h>

#include <stb_image.h>

#define USE_GLFW
#ifdef  USE_GLFW
    #include "GLFW/platform_factory.hpp"
    #include "GLFW/window.hpp"

    glfw::PlatformFactory platform_factory;
#else
    #include "Windows/platform_factory.hpp"

    windows::PlatformFactory platform_factory;
#endif

void key_callback(GLFWwindow* handle, int key, int, int action, int)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            ((glfw::Window*)glfwGetWindowUserPointer(handle))->close();
        }
    }
}

int main()
{
    auto platform = platform_factory.create_platform();
    auto window   = platform_factory.create_window(800, 600);

    if (!platform->init())
    {
        return -1;
    }

    if (!window->create("Playground"))
    {
        platform->release();
        return -1;
    }

    glfwSetKeyCallback(((glfw::Window*)window.get())->handle(), key_callback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        window->destroy();
        platform->release();

        return -1;
    }

    platform->vsync();

    // ==================================================================================

    auto debug_vertex_source = File::read("../debug_vert.glsl");
    auto debug_fragment_source = File::read("../debug_frag.glsl");

    Shader debug_vertex_shader {"debug_vert.glsl", GL_VERTEX_SHADER };
    debug_vertex_shader.create();
    debug_vertex_shader.source(debug_vertex_source.data());
    debug_vertex_shader.compile();

    Shader debug_fragment_shader {"debug_frag.glsl", GL_FRAGMENT_SHADER };
    debug_fragment_shader.create();
    debug_fragment_shader.source(debug_fragment_source.data());
    debug_fragment_shader.compile();

    Program debug_program;
    debug_program.create();
    debug_program.attach(&debug_vertex_shader);
    debug_program.attach(&debug_fragment_shader);
    debug_program.link();

    debug_program.detach(&debug_vertex_shader);
    debug_program.detach(&debug_fragment_shader);

    // ==================================================================================

    auto diffuse_vertex_source = File::read("../diffuse_vert.glsl");
    auto diffuse_fragment_source = File::read("../diffuse_frag.glsl");

    Shader diffuse_vertex_shader {"diffuse_vert.glsl", GL_VERTEX_SHADER };
    diffuse_vertex_shader.create();
    diffuse_vertex_shader.source(diffuse_vertex_source.data());
    diffuse_vertex_shader.compile();

    Shader diffuse_fragment_shader {"diffuse_frag.glsl", GL_FRAGMENT_SHADER };
    diffuse_fragment_shader.create();
    diffuse_fragment_shader.source(diffuse_fragment_source.data());
    diffuse_fragment_shader.compile();

    Program diffuse_program;
    diffuse_program.create();
    diffuse_program.attach(&diffuse_vertex_shader);
    diffuse_program.attach(&diffuse_fragment_shader);
    diffuse_program.link();

    diffuse_program.detach(&diffuse_vertex_shader);
    diffuse_program.detach(&diffuse_fragment_shader);

    // ==================================================================================

    auto sprite_vertex_source = File::read("../sprite_vert.glsl");
    auto sprite_fragment_source = File::read("../sprite_frag.glsl");

    Shader sprite_vertex_shader { "sprite_vert.gsl", GL_VERTEX_SHADER };
    sprite_vertex_shader.create();
    sprite_vertex_shader.source(sprite_vertex_source.data());
    sprite_vertex_shader.compile();

    Shader sprite_fragment_shader { "sprite_frag.glsl", GL_FRAGMENT_SHADER };
    sprite_fragment_shader.create();
    sprite_fragment_shader.source(sprite_fragment_source.data());
    sprite_fragment_shader.compile();

    Program sprite_program;
    sprite_program.create();
    sprite_program.attach(&sprite_vertex_shader);
    sprite_program.attach(&sprite_fragment_shader);
    sprite_program.link();

    sprite_program.detach(&sprite_vertex_shader);
    sprite_program.detach(&sprite_fragment_shader);

    // ==================================================================================

    auto x_geometry = Importer::load("../x.obj");

    // ==================================================================================

    int32_t w, h, c;

    stbi_set_flip_vertically_on_load(true);
    auto   data = stbi_load("../texture.jpeg", &w, &h, &c, 0);
    assert(data != nullptr);

    Texture test_texture { GL_TEXTURE_2D };
    test_texture.create();
    test_texture.source({ data, w, h, GL_RGB });

    test_texture.parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    test_texture.parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::free(data);

    // ==================================================================================

    std::vector<vertex_attribute> debug_vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(vertex::debug, position) },
        { 1, 3, (int32_t)offsetof(vertex::debug, color) }
    };

    VertexArray debug_vertex_array;
    debug_vertex_array.create();
    debug_vertex_array.bind();

    Buffer debug_vertex_buffer { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    debug_vertex_buffer.create();
    debug_vertex_buffer.bind();

    Buffer debug_indices_buffer { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    debug_indices_buffer.create();
    debug_indices_buffer.bind();

    debug_vertex_array.init_attributes_of_type<vertex::debug>(debug_vertex_attributes);

    // ==================================================================================

    std::vector<vertex_attribute> diffuse_vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(vertex::diffuse, position) },
        { 1, 3, (int32_t)offsetof(vertex::diffuse, normal) }
    };

    VertexArray x_vertex_array;
    x_vertex_array.create();
    x_vertex_array.bind();

    Buffer x_vertex_buffer {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    x_vertex_buffer.create();
    x_vertex_buffer.data(BufferData::make_data(x_geometry.vertices()));

    Buffer x_indices_buffer {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    x_indices_buffer.create();
    x_indices_buffer.data(BufferData::make_data(x_geometry.indices()));

    x_vertex_array.init_attributes_of_type<vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    MeshGeometry<vertex::sprite> square_geometry;

    square_geometry.add_vertex({{  128.0f,  128.0f }, { 1.0f, 1.0f } });
    square_geometry.add_vertex( {{  128.0f, -128.0f }, { 1.0f, 0.0f } });
    square_geometry.add_vertex({{ -128.0f, -128.0f }, { 0.0f, 0.0f } });
    square_geometry.add_vertex({{ -128.0f,  128.0f }, { 0.0f, 1.0f } });

    square_geometry.add_face(0, 1, 3);
    square_geometry.add_face(1, 2, 3);

    std::vector<vertex_attribute> sprite_vertex_attributes =
    {
        { 0, 2, (int32_t)offsetof(vertex::sprite, position) },
        { 1, 2, (int32_t)offsetof(vertex::sprite, uv) }
    };

    VertexArray square_vertex_array;
    square_vertex_array.create();
    square_vertex_array.bind();

    Buffer square_vertex_buffer { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_vertex_buffer.create();
    square_vertex_buffer.data(BufferData::make_data(square_geometry.vertices()));

    Buffer square_indices_buffer { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_indices_buffer.create();
    square_indices_buffer.data(BufferData::make_data(square_geometry.indices()));

    square_vertex_array.init_attributes_of_type<vertex::sprite>(sprite_vertex_attributes);

    // ==================================================================================

    Material x_material;
    x_material.diffuse = { 1.0f, 1.0f, 0.0f };

    // ==================================================================================

    Light directional_light { { 0.0f, 0.0f, 5.0f }, { 1.0f, 1.0f, 1.0f } };

    // ==================================================================================

    Buffer matrices_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    matrices_buffer.create();
    matrices_buffer.bind_at_location(0);

    Buffer material_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    material_buffer.create();
    material_buffer.bind_at_location(1);

    Buffer light_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    light_buffer.create();
    light_buffer.bind_at_location(2);

    // ==================================================================================

    const RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    // ==================================================================================

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.0f);

    std::vector<glm::mat4> matrices { 3 };

    // ==================================================================================

    Camera perspective_camera;
    Camera ortho_camera;

    Transform perspective_camera_transform;
    Transform ortho_camera_transform;

    perspective_camera_transform.translate({0.0f, 0.0f, -5.0f });

    // ==================================================================================

    Transform x_transform;
    Transform square_transform;

    square_transform.translate({ 128.0f, 128.0f, 0.0f });

    // ==================================================================================

    Physics physics;
    physics.init();

    btCollisionShape* x_shape = new btBoxShape({ 1.0f, 1.0f, 1.0f });
    physics.add_collision(1, x_shape, { });

    // ==================================================================================

    Editor editor;
    editor.init(window.get());

    LightWindow light_window;
    light_window.set_light(&directional_light);

    editor.add_window(&light_window);

    // ==================================================================================

    const Time time;
    float fov = 60.0f;

    while (!window->closed())
    {
        const float total_time = time.total_time();

        physics.debug();

        int32_t width  = window->width();
        int32_t height = window->height();

        const float ratio = (float) width / (float) height;

        perspective_camera.perspective(fov, ratio);
        ortho_camera.ortho(0.0f, (float)width, 0.0f, (float)height);

        render_pass.viewport(0, 0, width, height);

        // ==================================================================================

        if (glfwGetMouseButton(((glfw::Window*)window.get())->handle(), GLFW_MOUSE_BUTTON_1))
        {
            double xpos, ypos;
            glfwGetCursorPos(((glfw::Window*)window.get())->handle(), &xpos, &ypos);

            float y = height - ypos;

            glm::vec4 viewport { 0.0f, 0.0f, width, height };

            glm::vec3 start = glm::unProject({ xpos, y, 0.0f }, perspective_camera_transform.matrix(), perspective_camera.projection(), viewport);
            glm::vec3 end   = glm::unProject({ xpos, y, 1.0f }, perspective_camera_transform.matrix(), perspective_camera.projection(), viewport);

            glm::vec3 direction = glm::normalize(end - start);

            auto hit = physics.cast({ {start.x, start.y, start.z },
                                                                           { direction.x, direction.y, direction.z } }, 50.0f);

            if (hit.hasHit())
            {
                std::cout << "hit\n";
            }
        }

        // ==================================================================================

        editor.begin(width, height, total_time);

        ImGui::Begin("RenderPass");
        ImGui::ColorEdit3("Clear color", (float*) &clear_color, ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit3("Diffuse color", (float*) &x_material.diffuse, ImGuiColorEditFlags_NoOptions);
        ImGui::SliderFloat("Fov", &fov, 45, 120);

        //ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Begin("Texture");
        ImGui::Image((void*)(intptr_t)test_texture.handle(), { 256, 256}); // TODO fix flipped image
        ImGui::End();

        editor.end();

        // ==================================================================================

        light_buffer.data(BufferData::make_data(&directional_light));

        // ==================================================================================

        render_pass.clear_color(clear_color.x, clear_color.y, clear_color.z);
        render_pass.clear_buffers();

        // ==================================================================================

        x_transform.translate({ 0.0f, 0.0f, 0.0f })
                   .rotate({ 0.0f, 1.0f, 0.0f }, total_time)
                   .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = x_transform.matrix();
        matrices[1] = perspective_camera_transform.matrix();
        matrices[2] = perspective_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));
        material_buffer.data(BufferData::make_data(&x_material));

        diffuse_program.bind();

        x_vertex_array.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)x_geometry.indices().size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        matrices[0] = glm::mat4(1.0f);
        matrices_buffer.data(BufferData::make_data(matrices));

        const MeshGeometry<vertex::debug>& geometry = physics.physics_debug()->geometry();

        debug_program.bind();

        debug_vertex_array.bind();
        debug_vertex_buffer.data(BufferData::make_data(geometry.vertices()));
        debug_indices_buffer.data(BufferData::make_data(geometry.indices()));

        glDrawElements(GL_LINES, (int32_t)geometry.indices().size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        matrices[0] = square_transform.matrix();
        matrices[1] = ortho_camera_transform.matrix();
        matrices[2] = ortho_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));

        sprite_program.bind();
        test_texture.bind();

        square_vertex_array.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)square_geometry.indices().size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        editor.draw();

        window->update();
        platform->update();
    }

    physics.release();

    window->destroy();
    platform->release();

    return 0;
}