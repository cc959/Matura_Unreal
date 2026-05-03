cmake_minimum_required(VERSION 3.20)
project( Matura )

find_package( OpenCV REQUIRED )
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Boost REQUIRED COMPONENTS system serialization)
link_directories( /usr/local/lib )

# Source files for AprilTag and Serial
file(GLOB SerialFiles CONFIGURE_DEPENDS "src/serial/*.cc")
file(GLOB ApriltagFiles CONFIGURE_DEPENDS "src/apriltag/*.c")
file(GLOB ApriltagCommonFiles CONFIGURE_DEPENDS "src/apriltag/common/*.c")

# Include directories
include_directories( ${OpenCV_INCLUDE_DIRS} ./ ./include/ ./src/)
include_directories( ${Boost_INCLUDE_DIR} )

# Set C standard for C files
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set C++ standard for C++ files
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common compile options for both C and C++
add_compile_options(-O3 -march=native)

# Server executable (main.cpp)
add_executable(ServerApp 
    src/main.cpp
    src/TrackingCamera.cpp
    src/CameraManager.cpp
    ${SerialFiles}
    ${ApriltagFiles}
    ${ApriltagCommonFiles}
)
target_link_libraries(ServerApp PRIVATE ${OpenCV_LIBS} ${Boost_LIBRARIES} Eigen3::Eigen pthread)

# Client executable (client_example.cpp)
add_executable(ClientApp 
    src/client_example.cpp
    src/TrackingCamera.cpp
    src/CameraManager.cpp
    ${SerialFiles}
    ${ApriltagFiles}
    ${ApriltagCommonFiles}
)
target_link_libraries(ClientApp PRIVATE ${OpenCV_LIBS} ${Boost_LIBRARIES} Eigen3::Eigen pthread)
