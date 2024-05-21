######################################################################
#
# According to "the internet", cmake and protobuf don't play well
# when you want to use directory hierarchies. While I only use the
# "steam" directory from the steamdatabase protofiles so far,
# I can't be sure that I'll never need any of the others.
#
# This hierarchy of CMakeLists.txt is entirely designed so we
# get paths like
#    protobufs/steam/xxx.pb.h
# when building the framework. It should be just one CMakeLists.txt,
# but then we don't get such pathnames.

cmake_minimum_required(VERSION 3.25)

project("steamdatabase-protobufs" LANGUAGES CXX)

foreach(protobufs-directory steam)
  add_subdirectory(protobufs/${protobufs-directory})
  target_link_libraries(${PROJECT_NAME} PUBLIC steamdatabase-protobufs-${protobufs-directory})
endforeach()

target_include_directories("${PROJECT_NAME}" INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/..")
