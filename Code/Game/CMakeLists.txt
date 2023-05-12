project(Playground)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE Editor)
target_link_libraries(${PROJECT_NAME} PRIVATE Common Platform Assets Graphics Components Resources Physics Math)
target_sources(${PROJECT_NAME}        PRIVATE main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build")