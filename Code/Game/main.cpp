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
#include "light.hpp"
#include "physics.hpp"
#include "physics_shapes.hpp"
#include "mesh_importer.hpp"
#include "texture_importer.hpp"

// ==================================================================================

#include "editor.hpp"

#include "assets/material_window.hpp"
#include "assets/texture_window.hpp"
#include "components/camera_window.hpp"
#include "components/light_window.hpp"
#include "render_pass_window.hpp"

// ==================================================================================

#define USE_GLFW
#ifdef  USE_GLFW
    #include "glfw/platform_factory.hpp"
    #include "glfw/platform.hpp"

    glfw::PlatformFactory platform_factory;
#else
    #include "Windows/platform_factory.hpp"

    windows::PlatformFactory platform_factory;
#endif

int main()
{
    int32_t width  = 1024;
    int32_t height = 768;

    auto platform = platform_factory.create_platform();
    auto window   = platform_factory.create_window("Playground", { width, height });
    auto input    = platform_factory.create_input();

    if (!platform->init())
    {
        return -1;
    }

    if (!window->create())
    {
        platform->release();
        return -1;
    }

    if (!glfw::Platform::init_context())
    {
        window->destroy();
        platform->release();

        return -1;
    }

    platform->vsync();

    // ==================================================================================

    auto diffuse_vertex_source = File::read("../diffuse_vert.glsl");
    auto diffuse_fragment_source = File::read("../diffuse_frag.glsl");

    Shader diffuse_vertex_shader {"diffuse_vert.glsl", GL_VERTEX_SHADER };
    diffuse_vertex_shader.create();
    diffuse_vertex_shader.source(diffuse_vertex_source.data());

    Shader diffuse_fragment_shader {"diffuse_frag.glsl", GL_FRAGMENT_SHADER };
    diffuse_fragment_shader.create();
    diffuse_fragment_shader.source(diffuse_fragment_source.data());

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

    Shader sprite_fragment_shader { "sprite_frag.glsl", GL_FRAGMENT_SHADER };
    sprite_fragment_shader.create();
    sprite_fragment_shader.source(sprite_fragment_source.data());

    Program sprite_program;
    sprite_program.create();
    sprite_program.attach(&sprite_vertex_shader);
    sprite_program.attach(&sprite_fragment_shader);
    sprite_program.link();

    sprite_program.detach(&sprite_vertex_shader);
    sprite_program.detach(&sprite_fragment_shader);

    // ==================================================================================

    auto geometries = MeshImporter::load("../playground.obj");

    auto cube_geometry     = geometries[0];
    auto cylinder_geometry = geometries[1];

    auto bricks_texture_data = TextureImporter::load("../texture.jpeg");

    // ==================================================================================

    Texture bricks_texture {GL_TEXTURE_2D };
    bricks_texture.create();
    bricks_texture.source(bricks_texture_data);

    bricks_texture.parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    bricks_texture.parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    bricks_texture_data.release();

    // ==================================================================================

    vertex_attributes diffuse_vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(mesh_vertex::diffuse, position) },
        { 1, 3, (int32_t)offsetof(mesh_vertex::diffuse, normal) }
    };

    VertexArray cube_vertex_array;
    cube_vertex_array.create();
    cube_vertex_array.bind();

    Buffer cube_vertex_buffer { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    cube_vertex_buffer.create();
    cube_vertex_buffer.data(BufferData::make_data(cube_geometry.vertices()));

    Buffer cube_indices_buffer { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    cube_indices_buffer.create();
    cube_indices_buffer.data(BufferData::make_data(cube_geometry.faces()));

    cube_vertex_array.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    VertexArray cylinder_vertex_array;
    cylinder_vertex_array.create();
    cylinder_vertex_array.bind();

    Buffer cylinder_vertex_buffer { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    cylinder_vertex_buffer.create();
    cylinder_vertex_buffer.data(BufferData::make_data(cylinder_geometry.vertices()));

    Buffer cylinder_indices_buffer { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    cylinder_indices_buffer.create();
    cylinder_indices_buffer.data(BufferData::make_data(cylinder_geometry.faces()));

    cylinder_vertex_array.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    MeshGeometry<mesh_vertex::sprite, triangle> square_geometry;

    square_geometry.begin();
    square_geometry.add_vertex({{  128.0f,  128.0f }, { 1.0f, 1.0f } });
    square_geometry.add_vertex( {{  128.0f, -128.0f }, { 1.0f, 0.0f } });
    square_geometry.add_vertex({{ -128.0f, -128.0f }, { 0.0f, 0.0f } });
    square_geometry.add_vertex({{ -128.0f,  128.0f }, { 0.0f, 1.0f } });

    square_geometry.add_face({ 0, 1, 3 });
    square_geometry.add_face({ 1, 2, 3 });
    square_geometry.end();

    vertex_attributes sprite_vertex_attributes =
    {
        { 0, 2, (int32_t)offsetof(mesh_vertex::sprite, position) },
        { 1, 2, (int32_t)offsetof(mesh_vertex::sprite, uv) }
    };

    VertexArray square_vertex_array;
    square_vertex_array.create();
    square_vertex_array.bind();

    Buffer square_vertex_buffer { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_vertex_buffer.create();
    square_vertex_buffer.data(BufferData::make_data(square_geometry.vertices()));

    Buffer square_indices_buffer { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_indices_buffer.create();
    square_indices_buffer.data(BufferData::make_data(square_geometry.faces()));

    square_vertex_array.init_attributes_of_type<mesh_vertex::sprite>(sprite_vertex_attributes);

    // ==================================================================================

    Material cube_material { { 1.0f, 1.0f, 0.0f } };
    Material cylinder_material { { 0.0f, 1.0f, 0.0f } };

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

    RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    rgb clear_color { 0.45f, 0.55f, 0.60f };
    render_pass.clear_color(clear_color.r, clear_color.g, clear_color.b);

    // ==================================================================================

    std::vector<glm::mat4> matrices { 3 };

    // ==================================================================================

    Camera perspective_camera { 60.0f };
    Camera ortho_camera;

    Transform perspective_camera_transform;
    Transform ortho_camera_transform;

    vec3 perspective_camera_position { 0.0f, 0.0f, -12.0f };
    perspective_camera_transform.translate(perspective_camera_position);

    // ==================================================================================

    Transform cube_transform;
    Transform cylinder_transform;
    Transform square_transform;

    vec3 cube_position { -2.0f, 0.0f, 0.0f };
    vec3 cylinder_position { 2.0f, 0.0f, 0.0f };

    square_transform.translate({ 128.0f, 128.0f, 0.0f });
    cylinder_transform.translate(cylinder_position);

    // ==================================================================================

    Physics physics;
    physics.init();

    auto cube_shape = PhysicsShapes::create_box({ 1.0f, 1.0f, 1.0f });
    physics.add_collision(1, cube_shape, cube_position);

    // ==================================================================================

    Editor editor;
    editor.init(window.get(), &physics);

    LightWindow light_window;
    light_window.set_light(&directional_light);

    MaterialWindow material_window;
    material_window.set_material(&cube_material);

    TextureWindow texture_window;
    texture_window.set_texture(&bricks_texture, bricks_texture_data);

    CameraWindow camera_window;
    camera_window.set_camera(&perspective_camera);
    camera_window.set_transform(&perspective_camera_transform, perspective_camera_position);

    RenderPassWindow render_pass_window;
    render_pass_window.set_render_pass(&render_pass, clear_color);

    editor.add_window(&light_window);
    editor.add_window(&material_window);
    editor.add_window(&texture_window);
    editor.add_window(&camera_window);
    editor.add_window(&render_pass_window);

    // ==================================================================================

    const Time time;

    while (!window->closed())
    {
        physics.compute_debug_geometry();

        const float total_time = time.total_time();

        width  = window->size().width;
        height = window->size().height;

        perspective_camera.resize((float)width, (float)height);
        ortho_camera.resize((float)width, (float)height);

        // ==================================================================================

        if (input->mouse_pressed(window.get(), input::Button::Left))
        {
            vec2 mouse_position = input->mouse_position(window.get());

            auto ray = perspective_camera.screen_to_world(perspective_camera_transform.matrix(), mouse_position);
            auto hit = physics.cast(ray, 50.0f);

            if (hit.hasHit())
            {
                std::cout << "hit\n";
            }
        }

        if (input->key_pressed(window.get(), input::Key::Escape))
        {
            window->close();
        }

        // ==================================================================================

        editor.begin(width, height, total_time);
        editor.end();

        // ==================================================================================

        render_pass.viewport(0, 0, width, height);
        render_pass.clear_buffers();

        // ==================================================================================

        matrices[0] = square_transform.matrix();
        matrices[1] = ortho_camera_transform.matrix();
        matrices[2] = ortho_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));

        sprite_program.bind();
        bricks_texture.bind();

        square_vertex_array.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)square_geometry.faces().size() * 3, GL_UNSIGNED_INT, 0);

        // ==================================================================================

        cube_transform.translate(cube_position)
                      .rotate({ 0.0f, 1.0f, 0.0f }, total_time);

        matrices[0] = cube_transform.matrix();
        matrices[1] = perspective_camera_transform.matrix();
        matrices[2] = perspective_camera.projection();

        matrices_buffer.sub_data(BufferData::make_data(matrices));
        material_buffer.data(BufferData::make_data(&cube_material));
        light_buffer.data(BufferData::make_data(&directional_light));

        diffuse_program.bind();

        cube_vertex_array.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)cube_geometry.faces().size() * 3, GL_UNSIGNED_INT, 0);

        // ==================================================================================

        matrices[0] = cylinder_transform.matrix();
        matrices_buffer.sub_data(BufferData::make_data(matrices));
        material_buffer.sub_data(BufferData::make_data(&cylinder_material));

        cylinder_vertex_array.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)cylinder_geometry.faces().size() * 3, GL_UNSIGNED_INT, 0);

        // ==================================================================================

        editor.draw(&matrices_buffer);

        window->update();
        platform->update();
    }

    physics.release();

    window->destroy();
    platform->release();

    return 0;
}