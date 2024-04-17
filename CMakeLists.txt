cmake_minimum_required(VERSION 3.5.0)
project(DiveintoVulkan VERSION 0.1.0)

enable_testing()

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)


file(GLOB SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_PATH})

target_include_directories (${PROJECT_NAME} PUBLIC
  ${PROJECT_BINARY_DIR}
  ${GLFW_INCLUDE_DIRS}
  ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)