#include "transform.hpp"
#include "buffer.hpp"
#include "vertex_array.hpp"
#include "program.hpp"
#include "time.hpp"
#include "render_pass.hpp"
#include "camera.hpp"
#include "file.hpp"
#include "material.hpp"

#include <vector>

#include <imgui.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

struct vertex
{
    vec3 position;
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

    Shader vertex_shader { "diffuse_vert.glsl", GL_VERTEX_SHADER };
    vertex_shader.create();
    vertex_shader.source(diffuse_vertex_source.data());
    vertex_shader.compile();

    Shader fragment_shader { "diffuse_frag.glsl", GL_FRAGMENT_SHADER };
    fragment_shader.create();
    fragment_shader.source(diffuse_fragment_source.data());
    fragment_shader.compile();

    Program program;
    program.create();
    program.attach(&vertex_shader);
    program.attach(&fragment_shader);
    program.link();

    program.detach(&vertex_shader);
    program.detach(&fragment_shader);

    // ==================================================================================

    Assimp::Importer importer;

    std::vector<vertex> x_vertices;
    std::vector<uint32_t> x_indices;

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

                x_vertices.push_back({ position.x, position.y, position.z });
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

    std::vector<vertex_attribute> vertex_attributes =
    {
        { 0, 3, (int32_t)offsetof(vertex, position) }
    };

    VertexArray vertex_array;
    vertex_array.create();
    vertex_array.bind();

    Buffer vertex_buffer { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    vertex_buffer.create();
    vertex_buffer.bind();
    vertex_buffer.data(BufferData::make_data_of_type(x_vertices));

    Buffer indices_buffer { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    indices_buffer.create();
    indices_buffer.bind();
    indices_buffer.data(BufferData::make_data_of_type(x_indices));

    vertex_array.init_attributes_of_type<vertex>(vertex_attributes);

    // ==================================================================================

    auto* x_material = new Material();
    x_material->diffuse = { 1.0f, 0.0f, 0.0f };

    // ==================================================================================

    Buffer matrices_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    matrices_buffer.create();
    matrices_buffer.bind_at_location(0);

    Buffer material_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    material_buffer.create();
    material_buffer.bind_at_location(1);

    // ==================================================================================

    const RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    // ==================================================================================

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::vector<glm::mat4> matrices { 3 };

    Camera camera;

    Transform x_transform;
    Transform camera_transform;

    camera_transform.translate({ 0.0f, 0.0f, -5.0f });

    const Time time;
    float fov = 60.0f;

    while (!window->closed())
    {
        int32_t width  = window->width();
        int32_t height = window->height();

        const float ratio = (float) width / (float) height;

        render_pass.viewport(0, 0, width, height);
        camera.perspective(fov, ratio);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame(width, height, time.total_time());

        ImGui::NewFrame();

        ImGui::Begin("RenderPass");
        ImGui::ColorEdit3("Clear color", (float*) &clear_color, ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit3("Diffuse color", (float*) &x_material->diffuse, ImGuiColorEditFlags_NoOptions);
        ImGui::SliderFloat("Fov", &fov, 45, 120);

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Render();

        render_pass.clear_color(clear_color.x, clear_color.y, clear_color.z);
        render_pass.clear_buffers();

        x_transform.translate({ 0.0f, 0.0f, 0.0f })
                   .rotate({ 0.0f, 0.0f, 1.0f }, time.total_time())
                   .scale({ 0.5f, 0.5f, 0.5f });

        matrices[0] = x_transform.matrix();
        matrices[1] = camera_transform.matrix();
        matrices[2] = camera.projection();

        matrices_buffer.data(BufferData::make_data_of_type(matrices));
        material_buffer.data(BufferData::make_data_of_type(x_material));

        program.bind();

        vertex_array.bind();
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, (int32_t) x_indices.size(), GL_UNSIGNED_INT, 0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window->update();
        platform->update();
    }

    window->destroy();
    platform->release();

    return 0;
}