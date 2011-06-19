cmake_minimum_required(VERSION 2.6)

project( sound )

# to debug, change the CMAKE_BUILD_TYPE in ccmake to Debug

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -DNOOGLFT -D_LINUX_")
MESSAGE( STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS} )

include_directories(
  ../include
  ../../motlab/motlab/trunk/include
  ../../motlab/timer/trunk/include
  ../../motlab/getopt_basic/trunk/include
  )

link_directories(
  /usr/lib
  /usr/local/lib
  ../../motlab/motlab/trunk/build/lib
  ../../motlab/timer/trunk/build/lib
  ../../motlab/getopt_basic/trunk/build/lib
  )

set ( sound_SRCS clientcontrol controller main ogg servercontrol windowgensdl windowgen graphicsopengl graphics outstd outverbose input inputSDL )

set ( sound_LIBS getopt_basic motlab openal timer vorbis vorbisfile GL SDL)

add_executable( sound ${sound_SRCS} )
target_link_libraries (sound ${sound_LIBS})

