cmake_minimum_required(VERSION 3.1)

# Because we are using std::make_unique<>
set(CMAKE_CXX_STANDARD 14)

project(bwm)

# Find Xlib
find_package(X11 REQUIRED)

# Create the Executable and link libraries
set(SOURCE_FILES bwm.hpp bwm.cpp main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})

# Install the binary to /usr/local/bin
install(TARGETS ${PROJECT_NAME} DESTINATION /usr/local/bin)

# Create a uninstall target to remove the installed files
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -E remove /usr/local/bin/${PROJECT_NAME})

# Define an alias to run `make uninstall`
add_custom_target(uninstall_target COMMENT "Uninstalling the program" COMMAND make uninstall)