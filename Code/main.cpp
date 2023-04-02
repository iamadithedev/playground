#include <imgui.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const char* vertex_shader_text =
"#version 330\n"
"in vec3 in_position;\n"
"uniform mat4 u_mvp;\n"
"void main()\n"
"{\n"
"   gl_Position = u_mvp * vec4(in_position, 1.0);\n"
"}\n";

const char* fragment_shader_text =
"#version 330\n"
"out vec4 out_color;\n"
"void main()\n"
"{\n"
"   out_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
"}\n";

struct vertex
{
    float x, y, z;
};

vertex vertices[3] =
{
{ -0.6f, -0.4f, 0.0f },
{ 0.6f, -0.4f, 0.0f },
{ 0.0f, 0.6f, 0.0f }
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

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);

    uint32_t program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    uint32_t vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    uint32_t vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*) 0);
    glEnableVertexAttribArray(0);

    glEnable(GL_MULTISAMPLE);

    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        float ratio = (float) width / (float) height;

        glViewport(0, 0, width, height);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        ImGui::Begin("Test");
        ImGui::Text("Text");
        ImGui::ColorEdit3("Clear color", (float*) &clear_color, ImGuiColorEditFlags_NoOptions);
        ImGui::End();

        ImGui::Render();

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, 0.0f });
                  model = glm::rotate(model, (float) glfwGetTime(), { 0.0f, 0.0f, 1.0f });

        glm::mat4 view = glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, -3.0f });
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), ratio, 0.1f, 100.0f);

        glm::mat4 mvp = proj * view * model;

        glUseProgram(program);
        glUniformMatrix4fv(0, 1, GL_FALSE, (const float*) &mvp);

        glBindVertexArray(vertex_array);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}