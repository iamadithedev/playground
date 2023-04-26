project(Playground)

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE ${BULLET_PHYSICS_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE Core Graphics Components Resources stb imgui BulletCollision LinearMath)
target_sources(${PROJECT_NAME}        PRIVATE main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build")