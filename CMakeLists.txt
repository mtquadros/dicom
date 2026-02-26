cmake_minimum_required(VERSION 3.16)
project(read_dicom_gdcm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GDCM REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)


add_executable(read_dicom read_dicom.cpp)

target_link_libraries(read_dicom PRIVATE gdcmMSFF)
target_link_libraries(read_dicom PRIVATE Qt5::Widgets)
include_directories(include)
