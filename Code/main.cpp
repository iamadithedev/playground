#include "vec3.hpp"
#include "transform.hpp"
#include "buffer.hpp"

#include <vector>
#include <iostream>

#include <imgui.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

const char* vertex_shader_text =
"#version 420\n"
"in vec3 in_position;\n"
"layout (binding = 0, std140) uniform matrices\n"
"{\n"
"   mat4 model;\n"
"   mat4 view;\n"
"   mat4 proj;\n"
"};\n"
"void main()\n"
"{\n"
"   gl_Position = proj * view * model * vec4(in_position, 1.0);\n"
"}\n";

const char* fragment_shader_text =
"#version 430\n"
"layout (location = 0) uniform vec3 u_color;\n"
"out vec4 out_color;\n"
"void main()\n"
"{\n"
"   out_color = vec4(u_color, 1.0);\n"
"}\n";

struct vertex
{
    vec3 position;
};

void key_callback(GLFWwindow* window, int key, int, int action, int)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
}

void shader_status(uint32_t handle)
{
    int  success;
    char info[512];

    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(handle, 512, nullptr, info);

        std::cerr << "shader compilation failed\n" << info << std::endl;
    }
}

void program_status(uint32_t handle)
{
    int  success;
    char info[512];

    glGetProgramiv(handle, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(handle, 512, nullptr, info);

        std::cerr << "program linking failed\n" << info << std::endl;
    }
}

int main()
{
    if (!glfwInit())
    {
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Playground", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    glfwSwapInterval(1);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // ==================================================================================

    uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    shader_status(vertex_shader);

    uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    shader_status(fragment_shader);

    uint32_t program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    program_status(program);

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

    uint32_t vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    Buffer vertex_buffer { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    vertex_buffer.create();
    vertex_buffer.bind();
    vertex_buffer.data(BufferData::make_data_of_type<vertex>(x_vertices));

    Buffer indices_buffer { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    indices_buffer.create();
    indices_buffer.bind();
    indices_buffer.data(BufferData::make_data_of_type<uint32_t>(x_indices));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*) offsetof(vertex, position));
    glEnableVertexAttribArray(0);

    // ==================================================================================

    Buffer matrices_buffer { GL_UNIFORM_BUFFER, GL_STATIC_DRAW };
    matrices_buffer.create();
    matrices_buffer.bind_at_location(0);

    // ==================================================================================

    glEnable(GL_MULTISAMPLE);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::vector<glm::mat4> matrices { 3 };
    glm::vec3 triangle_color { 1.0f, 0.0f, 0.0f };

    Transform x_transform;

    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        float ratio = (float) width / (float) height;

        glViewport(0, 0, width, height);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        ImGui::Begin("RenderPass");
        ImGui::ColorEdit3("Clear color", (float*) &clear_color, ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit3("Triangle color", (float*) &triangle_color, ImGuiColorEditFlags_NoOptions);
        ImGui::End();

        ImGui::Render();

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        x_transform.translate({ 0.0f, 0.0f, 0.0f })
                   .rotate({ 0.0f, 0.0f, 1.0f }, (float) glfwGetTime())
                   .scale({ 0.5f, 0.5f, 0.5f });

        glm::mat4 view = glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, -5.0f });
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), ratio, 0.1f, 100.0f);

        matrices[0] = x_transform.matrix();
        matrices[1] = view;
        matrices[2] = proj;

        matrices_buffer.data(BufferData::make_data_of_type<glm::mat4>(matrices));

        glUseProgram(program);
        glUniform3fv(0, 1, (const float*) &triangle_color);

        glBindVertexArray(vertex_array);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, (int32_t) x_indices.size(), GL_UNSIGNED_INT, 0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}