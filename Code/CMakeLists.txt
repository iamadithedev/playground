project(Playground)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm imgui)
target_sources(${PROJECT_NAME}        PRIVATE main.cpp)