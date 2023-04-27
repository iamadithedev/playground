project(Playground)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE Core Graphics Components Resources Physics Editor stb)
target_sources(${PROJECT_NAME}        PRIVATE main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build")