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
#include "combine_geometry.hpp"

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

    auto diffuse_vert_source = File::read<char>("../glsl/diffuse.vert.glsl");
    auto diffuse_vert_instance_source = File::read<char>("../glsl/diffuse_instance.vert.glsl");
    auto diffuse_frag_source = File::read<char>("../glsl/diffuse.frag.glsl");

    auto diffuse_vert_binary_source = File::read<std::byte>("../spv/diffuse.vert.spv");
    auto diffuse_instance_vert_binary_source = File::read<std::byte>("../spv/diffuse_instance.vert.spv");
    auto diffuse_frag_binary_source = File::read<std::byte>("../spv/diffuse.frag.spv");

    Shader diffuse_vert_shader {"diffuse.vert.glsl", GL_VERTEX_SHADER };
    diffuse_vert_shader.create();
    diffuse_vert_shader.source(diffuse_vert_binary_source);

    Shader diffuse_vert_instance_shader {"diffuse_instance.vert.glsl", GL_VERTEX_SHADER };
    diffuse_vert_instance_shader.create();
    diffuse_vert_instance_shader.source(diffuse_instance_vert_binary_source);

    Shader diffuse_frag_shader {"diffuse.frag.glsl", GL_FRAGMENT_SHADER };
    diffuse_frag_shader.create();
    diffuse_frag_shader.source(diffuse_frag_binary_source);

    Program diffuse_program;
    diffuse_program.create();
    diffuse_program.attach(&diffuse_vert_shader);
    diffuse_program.attach(&diffuse_frag_shader);
    diffuse_program.link();

    Program diffuse_instance_program;
    diffuse_instance_program.create();
    diffuse_instance_program.attach(&diffuse_vert_instance_shader);
    diffuse_instance_program.attach(&diffuse_frag_shader);
    diffuse_instance_program.link();

    diffuse_program.detach(&diffuse_vert_shader);
    diffuse_program.detach(&diffuse_frag_shader);

    diffuse_instance_program.detach(&diffuse_vert_instance_shader);
    diffuse_instance_program.detach(&diffuse_frag_shader);

    diffuse_vert_shader.destroy();
    diffuse_vert_instance_shader.destroy();
    diffuse_frag_shader.destroy();

    // ==================================================================================

    auto sprite_vert_source = File::read<char>("../glsl/sprite.vert.glsl");
    auto sprite_frag_source = File::read<char>("../glsl/sprite.frag.glsl");

    Shader sprite_vert_shader {"sprite.vert.gsl", GL_VERTEX_SHADER };
    sprite_vert_shader.create();
    sprite_vert_shader.source(sprite_vert_source.data());

    Shader sprite_frag_shader {"sprite.frag.glsl", GL_FRAGMENT_SHADER };
    sprite_frag_shader.create();
    sprite_frag_shader.source(sprite_frag_source.data());

    Program sprite_program;
    sprite_program.create();
    sprite_program.attach(&sprite_vert_shader);
    sprite_program.attach(&sprite_frag_shader);
    sprite_program.link();

    sprite_program.detach(&sprite_vert_shader);
    sprite_program.detach(&sprite_frag_shader);

    sprite_vert_shader.destroy();
    sprite_frag_shader.destroy();

    // ==================================================================================

    auto playground_geometries = MeshImporter::load("../playground.obj");

    CombineGeometry scene_geometry;
    scene_geometry.combine(playground_geometries);

    auto cube_mesh_part     = scene_geometry.submeshes()[0];
    auto cylinder_mesh_part = scene_geometry.submeshes()[1];

    // ==================================================================================

    auto bricks_texture_data = TextureImporter::load("../bricks.jpeg");

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
        { 0, 3, GL_FLOAT, (int32_t)offsetof(mesh_vertex::diffuse, position) },
        { 1, 3, GL_FLOAT, (int32_t)offsetof(mesh_vertex::diffuse, normal) }
    };

    VertexArray scene_vao;
    scene_vao.create();
    scene_vao.bind();

    Buffer scene_vbo { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    scene_vbo.create();
    scene_vbo.data(BufferData::make_data(scene_geometry.vertices()));

    Buffer scene_ibo { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    scene_ibo.create();
    scene_ibo.data(BufferData::make_data(scene_geometry.faces()));

    scene_vao.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================

    vertex_attributes sprite_vertex_attributes =
    {
        { 0, 2, GL_FLOAT, (int32_t)offsetof(mesh_vertex::sprite, position) },
        { 1, 2, GL_FLOAT, (int32_t)offsetof(mesh_vertex::sprite, uv) }
    };

    MeshGeometry<mesh_vertex::sprite, primitive::triangle> square_geometry;

    square_geometry.begin(4, 2);
    square_geometry.add_vertex({{  128.0f,  128.0f }, { 1.0f, 1.0f } });
    square_geometry.add_vertex({{  128.0f, -128.0f }, { 1.0f, 0.0f } });
    square_geometry.add_vertex({{ -128.0f, -128.0f }, { 0.0f, 0.0f } });
    square_geometry.add_vertex({{ -128.0f,  128.0f }, { 0.0f, 1.0f } });

    square_geometry.add_face({ 0, 1, 3 });
    square_geometry.add_face({ 1, 2, 3 });
    square_geometry.end();

    auto square_mesh_part = square_geometry.get_mesh_part();

    VertexArray square_vao;
    square_vao.create();
    square_vao.bind();

    Buffer square_vbo {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_vbo.create();
    square_vbo.data(BufferData::make_data(square_geometry.vertices()));

    Buffer square_ibo {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_ibo.create();
    square_ibo.data(BufferData::make_data(square_geometry.faces()));

    square_vao.init_attributes_of_type<mesh_vertex::sprite>(sprite_vertex_attributes);

    // ==================================================================================

    Material cube_material     { { 1.0f, 1.0f, 0.0f } };
    Material cylinder_material { { 0.0f, 1.0f, 0.0f } };

    // ==================================================================================

    Light directional_light { { 0.0f, 0.0f, 5.0f }, { 1.0f, 1.0f, 1.0f } };

    // ==================================================================================

    Buffer matrices_ubo {GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    matrices_ubo.create();
    matrices_ubo.bind_at_location(0);

    Buffer material_ubo {GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    material_ubo.create();
    material_ubo.bind_at_location(1);

    Buffer light_ubo {GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    light_ubo.create();
    light_ubo.bind_at_location(2);

    // ==================================================================================

    RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    rgb clear_color { 0.45f, 0.55f, 0.60f };
    render_pass.clear_color(clear_color);

    // ==================================================================================

    std::vector<glm::mat4> matrices          { 3 };
    std::vector<glm::mat4> matrices_instance { 9 };

    // ==================================================================================

    Camera ortho_camera;
    Camera scene_camera { 60.0f };

    Transform ortho_camera_transform;
    Transform scene_camera_transform;

    vec3 scene_camera_position {0.0f, 0.0f, -8.0f };
         scene_camera_transform.translate(scene_camera_position);

    // ==================================================================================

    Transform cube_transform;
    Transform cylinder_transform;
    Transform square_transform;

    vec3 cube_position     { -2.0f, 0.0f, 0.0f };
    vec3 cylinder_position { 2.0f, 0.0f, 0.0f };

    cylinder_transform.translate(cylinder_position);
    square_transform.translate({ 128.0f, 128.0f, 0.0f });

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
    camera_window.set_camera(&scene_camera);
    camera_window.set_transform(&scene_camera_transform, scene_camera_position);

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

        scene_camera.resize((float)width, (float)height);
        ortho_camera.resize((float)width, (float)height);

        // ==================================================================================

        if (input->mouse_pressed(window.get(), input::Button::Left))
        {
            vec2 mouse_position = input->mouse_position(window.get());

            auto ray = scene_camera.screen_to_world(scene_camera_transform.matrix(), mouse_position);
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

        render_pass.viewport({ 0, 0 }, { width, height });
        render_pass.clear_buffers();

        // ==================================================================================

        matrices[0] = square_transform.matrix();
        matrices[1] = ortho_camera_transform.matrix();
        matrices[2] = ortho_camera.projection();

        matrices_ubo.data(BufferData::make_data(matrices));

        sprite_program.bind();
        bricks_texture.bind();

        square_vao.bind();
        glDrawElements(GL_TRIANGLES, square_mesh_part.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(square_mesh_part.index));

        // ==================================================================================

        cube_transform.translate(cube_position)
                      .rotate({ 0.0f, 1.0f, 0.0f }, total_time);

        matrices[0] = cube_transform.matrix();
        matrices[1] = scene_camera_transform.matrix();
        matrices[2] = scene_camera.projection();

        matrices_ubo.sub_data(BufferData::make_data(matrices));
        material_ubo.data(BufferData::make_data(&cube_material));
        light_ubo.data(BufferData::make_data(&directional_light));

        diffuse_program.bind();

        scene_vao.bind();
        glDrawElements(GL_TRIANGLES, cube_mesh_part.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(cube_mesh_part.index));

        // ==================================================================================

        matrices_ubo.sub_data(BufferData::make_data(&cylinder_transform.matrix()));
        material_ubo.sub_data(BufferData::make_data(&cylinder_material));

        glDrawElements(GL_TRIANGLES, cylinder_mesh_part.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(cylinder_mesh_part.index));

        // ==================================================================================

        editor.draw(&matrices_ubo);

        window->update();
        platform->update();
    }

    physics.release();

    window->destroy();
    platform->release();

    return 0;
}