# Aseprite FLIC Library
# Copyright (c) 2015 David Capello

add_library(flic-lib decoder.cpp encoder.cpp stdio.cpp)
set_property(TARGET flic-lib PROPERTY CXX_STANDARD 11)
set_property(TARGET flic-lib PROPERTY CXX_STANDARD_REQUIRED ON)
