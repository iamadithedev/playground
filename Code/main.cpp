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
#include "vec2.hpp"
#include "light.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <stb_image.h>

struct diffuse_vertex
{
    vec3 position;
    vec3 normal;
};

struct texture_vertex
{
    vec3 position;
    vec2 uv;
};

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

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(((glfw::Window*)window.get())->handle(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImGuiIO& io = ImGui::GetIO();

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

    auto texture_vertex_source = File::read("../texture_vert.glsl");
    auto texture_fragment_source = File::read("../texture_frag.glsl");

    Shader texture_vertex_shader { "texture_vert.gsl", GL_VERTEX_SHADER };
    texture_vertex_shader.create();
    texture_vertex_shader.source(texture_vertex_source.data());
    texture_vertex_shader.compile();

    Shader texture_fragment_shader { "texture_frag.glsl", GL_FRAGMENT_SHADER };
    texture_fragment_shader.create();
    texture_fragment_shader.source(texture_fragment_source.data());
    texture_fragment_shader.compile();

    Program texture_program;
    texture_program.create();
    texture_program.attach(&texture_vertex_shader);
    texture_program.attach(&texture_fragment_shader);
    texture_program.link();

    texture_program.detach(&texture_vertex_shader);
    texture_program.detach(&texture_fragment_shader);

    // ==================================================================================

    Assimp::Importer importer;

    std::vector<diffuse_vertex> x_vertices;
    std::vector<uint32_t>       x_indices;

    const aiScene* scene = importer.ReadFile("../x.obj", 0);

    if (scene && scene->mRootNode)
    {
        const aiNode* node = scene->mRootNode->mChildren[0];

        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            for (uint32_t j = 0; j < mesh->mNumVertices; j++)
            {
                const aiVector3D& position = mesh->mVertices[j];
                const aiVector3D& normal   = mesh->mNormals[j];

                x_vertices.push_back({{ position.x, position.y, position.z },
                                      { normal.x, normal.y, normal.z }});
            }

            for (uint32_t j = 0; j < mesh->mNumFaces; j++)
            {
                const aiFace& face = mesh->mFaces[j];

                for (uint32_t f = 0; f < face.mNumIndices; f++)
                {
                    x_indices.push_back(face.mIndices[f]);
                }
            }
        }
    }

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

    std::vector<vertex_attribute> diffuse_vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(diffuse_vertex, position) },
        { 1, 3, (int32_t) offsetof(diffuse_vertex, normal) }
    };

    VertexArray x_vertex_array;
    x_vertex_array.create();
    x_vertex_array.bind();

    Buffer x_vertex_buffer {GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    x_vertex_buffer.create();
    x_vertex_buffer.data(BufferData::make_data(x_vertices));

    Buffer x_indices_buffer {GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    x_indices_buffer.create();
    x_indices_buffer.data(BufferData::make_data(x_indices));

    x_vertex_array.init_attributes_of_type<diffuse_vertex>(diffuse_vertex_attributes);

    // ==================================================================================

    std::vector<texture_vertex> square_vertices =
    {
        {{  128.0f,  128.0f, 0.0f }, { 1.0f, 1.0f } },
        {{  128.0f, -128.0f, 0.0f }, { 1.0f, 0.0f } },
        {{ -128.0f, -128.0f, 0.0f }, { 0.0f, 0.0f } },
        {{ -128.0f,  128.0f, 0.0f }, { 0.0f, 1.0f } },
    };

    std::vector<uint32_t> square_indices =
    {
        0, 1, 3,
        1, 2, 3
    };

    std::vector<vertex_attribute> texture_vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(texture_vertex, position) },
        { 1, 2, (int32_t)offsetof(texture_vertex, uv) }
    };

    VertexArray square_vertex_array;
    square_vertex_array.create();
    square_vertex_array.bind();

    Buffer square_vertex_buffer { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_vertex_buffer.create();
    square_vertex_buffer.data(BufferData::make_data(square_vertices));

    Buffer square_indices_buffer { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    square_indices_buffer.create();
    square_indices_buffer.data(BufferData::make_data(square_indices));

    square_vertex_array.init_attributes_of_type<texture_vertex>(texture_vertex_attributes);

    // ==================================================================================

    Material x_material;
    x_material.diffuse = { 1.0f, 1.0f, 0.0f };

    // ==================================================================================

    rgb   light_color { 1.0f, 1.0f, 1.0f };
    Light directional_light { { 0.0f, 0.0f, 5.0f }, light_color };

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

    const Time time;
    float fov = 60.0f;

    while (!window->closed())
    {
        int32_t width  = window->width();
        int32_t height = window->height();

        const float ratio = (float) width / (float) height;

        perspective_camera.perspective(fov, ratio);
        ortho_camera.ortho(0.0f, (float)width, 0.0f, (float)height);

        render_pass.viewport(0, 0, width, height);

        // ==================================================================================

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame(width, height, time.total_time());

        ImGui::NewFrame();

        ImGui::Begin("RenderPass");
        ImGui::ColorEdit3("Clear color", (float*) &clear_color, ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit3("Diffuse color", (float*) &x_material.diffuse, ImGuiColorEditFlags_NoOptions);
        ImGui::SliderFloat("Fov", &fov, 45, 120);

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Begin("Texture");
        ImGui::Image((void*)(intptr_t)test_texture.handle(), { 256, 256}); // TODO fix flipped image
        ImGui::End();

        ImGui::Begin("Light");
        ImGui::ColorEdit3("Color", (float*)&light_color, ImGuiColorEditFlags_NoOptions);
        ImGui::End();

        ImGui::Render();

        // ==================================================================================

        directional_light.color(light_color);
        light_buffer.data(BufferData::make_data(&directional_light));

        // ==================================================================================

        render_pass.clear_color(clear_color.x, clear_color.y, clear_color.z);
        render_pass.clear_buffers();

        // ==================================================================================

        x_transform.translate({ 0.0f, 0.0f, 0.0f })
                   .rotate({ 0.0f, 1.0f, 0.0f }, time.total_time())
                   .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = x_transform.matrix();
        matrices[1] = perspective_camera_transform.matrix();
        matrices[2] = perspective_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));
        material_buffer.data(BufferData::make_data(&x_material));

        diffuse_program.bind();

        x_vertex_array.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)x_indices.size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        matrices[0] = square_transform.matrix();
        matrices[1] = ortho_camera_transform.matrix();
        matrices[2] = ortho_camera.projection();

        matrices_buffer.data(BufferData::make_data(matrices));

        texture_program.bind();
        test_texture.bind();

        square_vertex_array.bind();
        glDrawElements(GL_TRIANGLES, (int32_t)square_indices.size(), GL_UNSIGNED_INT, 0);

        // ==================================================================================

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window->update();
        platform->update();
    }

    window->destroy();
    platform->release();

    return 0;
}