cmake_minimum_required(VERSION 2.6)

project( sound )

# to debug, change the CMAKE_BUILD_TYPE in ccmake to Debug

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -DNOOGLFT -D_LINUX_")
MESSAGE( STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS} )

include_directories( ../include )

link_directories( /usr/lib /usr/local/lib )

set ( sound_SRCS buffer main ogg soundmanager )

set ( sound_LIBS openal vorbis vorbisfile)

add_executable( sound ${sound_SRCS} )
target_link_libraries (sound ${sound_LIBS})

