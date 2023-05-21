#include "transform.hpp"
#include "buffer.hpp"
#include "vertex_array.hpp"
#include "shader.hpp"
#include "time.hpp"
#include "render_pass.hpp"
#include "camera.hpp"
#include "file.hpp"
#include "material.hpp"
#include "light.hpp"
#include "physics_world.hpp"
#include "physics_shapes.hpp"
#include "combine_geometry.hpp"
#include "sampler.hpp"
#include "resource_manager.hpp"

#include "importers/mesh_importer.hpp"
#include "importers/texture_importer.hpp"

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

glfw::PlatformFactory factory;

#else

#include "Windows/platform_factory.hpp"

windows::PlatformFactory factory;

#endif

int32_t main()
{
    int32_t width  = 1024;
    int32_t height = 768;

    auto platform = factory.create_platform();
    auto window   = factory.create_window("Playground", { width, height });
    auto input    = factory.create_input();

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

    ResourceManager resources;
    resources.init("../Assets/");

    auto diffuse_shader = resources.load<Shader>("diffuse_shader.asset");

    // ==================================================================================

    auto diffuse_vert_binary = File::read<std::byte>("../Assets/spv/diffuse_instance.vert.spv");
    auto diffuse_frag_binary = File::read<std::byte>("../Assets/spv/diffuse.frag.spv");

    ShaderStage diffuse_vert_instance_shader { "diffuse_instance.vert.glsl", GL_VERTEX_SHADER };
    diffuse_vert_instance_shader.create();
    diffuse_vert_instance_shader.source(diffuse_vert_binary);

    ShaderStage diffuse_frag_shader { "diffuse.frag.glsl", GL_FRAGMENT_SHADER };
    diffuse_frag_shader.create();
    diffuse_frag_shader.source(diffuse_frag_binary);

    Shader diffuse_instance_shader;
    diffuse_instance_shader.create();
    diffuse_instance_shader.attach(&diffuse_vert_instance_shader);
    diffuse_instance_shader.attach(&diffuse_frag_shader);
    diffuse_instance_shader.link();

    diffuse_instance_shader.detach(&diffuse_vert_instance_shader);
    diffuse_instance_shader.detach(&diffuse_frag_shader);

    diffuse_vert_instance_shader.destroy();
    diffuse_frag_shader.destroy();

    // ==================================================================================

    auto sprite_vert_source = File::read<char>("../Assets/glsl/sprite.vert.glsl");
    auto sprite_frag_source = File::read<char>("../Assets/glsl/sprite.frag.glsl");

    ShaderStage sprite_vert_shader {"sprite.vert.gsl", GL_VERTEX_SHADER };
    sprite_vert_shader.create();
    sprite_vert_shader.source(sprite_vert_source.data());

    ShaderStage sprite_frag_shader {"sprite.frag.glsl", GL_FRAGMENT_SHADER };
    sprite_frag_shader.create();
    sprite_frag_shader.source(sprite_frag_source.data());

    Shader sprite_shader;
    sprite_shader.create();
    sprite_shader.attach(&sprite_vert_shader);
    sprite_shader.attach(&sprite_frag_shader);
    sprite_shader.link();

    sprite_shader.detach(&sprite_vert_shader);
    sprite_shader.detach(&sprite_frag_shader);

    sprite_vert_shader.destroy();
    sprite_frag_shader.destroy();

    // ==================================================================================

    auto playground_geometries = MeshImporter::load("../Assets/playground.obj");

    CombineGeometry scene_geometry;
    scene_geometry.combine(playground_geometries);

    auto& cube_submesh     = scene_geometry[0];
    auto& cylinder_submesh = scene_geometry[1];
    auto& sphere_submesh   = scene_geometry[2];
    auto& cone_submesh     = scene_geometry[3];

    // ==================================================================================

    auto bricks_texture_data = TextureImporter::load("../Assets/bricks.jpeg");

    // ==================================================================================

    Texture bricks_texture { GL_TEXTURE_2D };
    bricks_texture.create();
    bricks_texture.source(bricks_texture_data);
    bricks_texture_data.release();

    // ==================================================================================

    Sampler default_sampler;
    default_sampler.create();

    default_sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    default_sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    default_sampler.bind_at_location(0);

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
    scene_vbo.bind();
    scene_vbo.data(BufferData::make_data(scene_geometry.vertices()));

    Buffer scene_ibo { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    scene_ibo.create();
    scene_ibo.bind();
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

    auto square_submesh = square_geometry.get_submesh();

    VertexArray square_vao;
    square_vao.create();
    square_vao.bind();

    Buffer square_vbo {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_vbo.create();
    square_vbo.bind();
    square_vbo.data(BufferData::make_data(square_geometry.vertices()));

    Buffer square_ibo {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_ibo.create();
    square_ibo.bind();
    square_ibo.data(BufferData::make_data(square_geometry.faces()));

    square_vao.init_attributes_of_type<mesh_vertex::sprite>(sprite_vertex_attributes);

    // ==================================================================================

    Buffer matrices_ubo {GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    matrices_ubo.create();
    matrices_ubo.bind_at_location(0);

    Buffer material_ubo {GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    material_ubo.create();
    material_ubo.bind_at_location(1);

    Buffer light_ubo {GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    light_ubo.create();
    light_ubo.bind_at_location(2);

    Buffer matrices_instance_buffer { GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    matrices_instance_buffer.create();
    matrices_instance_buffer.bind_at_location(3);

    // ==================================================================================

    RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    rgb clear_color { 0.45f, 0.55f, 0.60f };
    render_pass.clear_color(clear_color);

    // ==================================================================================

    Material cube_material     { { 1.0f, 1.0f, 0.0f } };
    Material cylinder_material { { 0.0f, 1.0f, 0.0f } };
    Material sphere_material   { { 1.0f, 0.0f, 0.0f } };
    Material cone_material     { { 0.0f, 0.0f, 1.0f } };

    // ==================================================================================

    Light directional_light { { 0.0f, 0.0f, 5.0f }, { 1.0f, 1.0f, 1.0f } };

    // ==================================================================================

    std::vector<glm::mat4> matrices          { 3 };
    std::vector<glm::mat4> matrices_instance { 9 };

    // ==================================================================================

    Camera ortho_camera;
    Camera scene_camera { 60.0f };

    Transform ortho_camera_transform;
    Transform scene_camera_transform;

    vec3 scene_camera_position {0.0f, 0.0f, -20.0f };
         scene_camera_transform.translate(scene_camera_position);

    // ==================================================================================

    Transform cube_transform;
    Transform cylinder_transform;
    Transform sphere_transform;
    Transform cone_transform;
    Transform square_transform;

    vec3 cube_position     { -3.0f, -1.5f, 0.0f };
    vec3 cylinder_position { 3.0f, -1.5f, 0.0f };
    vec3 cone_position     { 0.0f, 3.0f, 0.0f };

    cylinder_transform.translate(cylinder_position);
    cone_transform.translate(cone_position);
    square_transform.translate({ 128.0f, 128.0f, 0.0f });

    // ==================================================================================

    PhysicsWorld physics;
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

        scene_camera.resize((float)width, (float)height); // TODO this should take a size??
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

        sprite_shader.bind();
        bricks_texture.bind();

        square_vao.bind();
        glDrawElements(GL_TRIANGLES, square_submesh.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(square_submesh.index));

        // ==================================================================================

        float offset = 9.0f;

        for (int32_t i = 0; i < 9; i++)
        {
            float value  = total_time + ((float)i * 0.7f);

            vec3 position = { std::sinf(value) * offset, std::cosf(value) * offset, 0.0f };
            sphere_transform.translate(position);
            matrices_instance[i] = sphere_transform.matrix();
        }

        matrices_instance_buffer.data(BufferData::make_data(matrices_instance));

        // ==================================================================================

        matrices[1] = scene_camera_transform.matrix();
        matrices[2] = scene_camera.projection();

        matrices_ubo.sub_data(BufferData::make_data(matrices));
        material_ubo.data(BufferData::make_data(&sphere_material));
        light_ubo.data(BufferData::make_data(&directional_light));

        diffuse_instance_shader.bind();
        scene_vao.bind();

        glDrawElementsInstanced(GL_TRIANGLES, sphere_submesh.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(sphere_submesh.index), 9);

        // ==================================================================================

        cube_transform.translate(cube_position)
                      .rotate({ 0.0f, 1.0f, 0.0f }, total_time);

        matrices_ubo.sub_data(BufferData::make_data(&cube_transform.matrix()));
        material_ubo.sub_data(BufferData::make_data(&cube_material));

        diffuse_shader->bind();

        glDrawElements(GL_TRIANGLES, cube_submesh.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(cube_submesh.index));

        // ==================================================================================

        matrices_ubo.sub_data(BufferData::make_data(&cylinder_transform.matrix()));
        material_ubo.sub_data(BufferData::make_data(&cylinder_material));

        glDrawElements(GL_TRIANGLES, cylinder_submesh.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(cylinder_submesh.index));

        // ==================================================================================

        matrices_ubo.sub_data(BufferData::make_data(&cone_transform.matrix()));
        material_ubo.sub_data(BufferData::make_data(&cone_material));

        glDrawElements(GL_TRIANGLES, cone_submesh.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(cone_submesh.index));

        // ==================================================================================

        editor.draw(&matrices_ubo);

        // ==================================================================================

        window->update();
        platform->update();
    }

    physics.release();

    window->destroy();
    platform->release();

    return 0;
}