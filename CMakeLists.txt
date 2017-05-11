# Aseprite FLIC Library
# Copyright (c) 2015 David Capello

# check cmake version                                                           
CMAKE_MINIMUM_REQUIRED(VERSION 3.1) 

find_package( Boost COMPONENTS program_options REQUIRED )

add_library(flic-lib decoder.cpp encoder.cpp stdio.cpp)

add_executable(png2flic png2flic.cpp lodepng/lodepng.cpp)
target_link_libraries(png2flic flic-lib)
target_include_directories(png2flic PRIVATE ${Boost_INCLUDE_DIR} )
target_link_libraries(png2flic ${Boost_LIBRARIES} )
