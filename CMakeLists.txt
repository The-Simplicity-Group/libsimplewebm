cmake_minimum_required(VERSION 4.0)

project(libsimplewebm VERSION 0.0.2 LANGUAGES CXX)

add_library(libsimplewebm
    demuxer/WebMDemuxer.cpp
    decoder/VPXDecoder.cpp
    decoder/OpusVorbisDecoder.cpp

    libwebm/mkvparser/mkvparser.cc
)

target_include_directories(libsimplewebm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libwebm)

target_compile_features(libsimplewebm PUBLIC cxx_std_11)

find_package(PkgConfig REQUIRED)

pkg_check_modules(VPX REQUIRED IMPORTED_TARGET vpx)
pkg_check_modules(OPUS REQUIRED IMPORTED_TARGET opus)
pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis)

target_link_libraries(libsimplewebm PRIVATE PkgConfig::VPX PkgConfig::OPUS PkgConfig::VORBIS)

add_executable(libsimplewebm_example example/example.cpp)

target_include_directories(libsimplewebm_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libwebm)

target_link_libraries(libsimplewebm_example PRIVATE libsimplewebm)
