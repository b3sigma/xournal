#Command line options
#-DUSE_TBB=ON/OFF     (default off)
#-DUSE_OMP=ON/OFF     (default off)
#-DUSE_SSE=ON/OFF     (default on)
#-DUSE_AVX=ON/OFF     (default on)

# ----------------------------------------------------------------------------
#   Basic Configuration
# ----------------------------------------------------------------------------
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(scribble)
set(PROJECT_VERSION "0.0.1")
string(REGEX MATCHALL "[0-9]" PROJECT_VERSION_PARTS "${PROJECT_VERSION}")
list(GET PROJECT_VERSION_PARTS 0 PROJECT_VERSION_MAJOR)
list(GET PROJECT_VERSION_PARTS 1 PROJECT_VERSION_MINOR)
list(GET PROJECT_VERSION_PARTS 2 PROJECT_VERSION_PATCH)
set(PROJECT_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

#------------------------------------------------------
# Build type
#------------------------------------------------------

IF(NOT CMAKE_BUILD_TYPE )
   SET( CMAKE_BUILD_TYPE "Release" )
ENDIF()

#------------------------------------------------------
# Lib Names and Dirs
#------------------------------------------------------

if(WIN32)
    # Postfix of DLLs:
    SET(PROJECT_DLLVERSION "${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}${PROJECT_VERSION_PATCH}")
    SET(RUNTIME_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Directory for dlls and binaries")
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Directory for binaries")
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Directory for dlls")
else()
    # Postfix of so's:
    set(PROJECT_DLLVERSION)
    SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_INSTALL_PREFIX}/lib/cmake/ /usr/lib/cmake)
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

OPTION(USE_TBB        "Set to ON to use TBB"             OFF)
OPTION(USE_OMP        "Set to ON to use OMP"             OFF)
OPTION(USE_SSE        "Set to ON to use sse"             ON)
OPTION(USE_AVX        "Set to ON to use avx"             ON)
OPTION(BUILD_TESTS    "Set to ON to build tests"         OFF)
OPTION(BUILD_EXAMPLES "Set to ON to build examples"      ON)
OPTION(BUILD_DOCS     "Set to ON to build documentation" OFF)

# ----------------------------------------------------------------------------
#   Find Dependencies
# ----------------------------------------------------------------------------

# Todo do a package find thing? Maybe not as it's already a submodule so this should just work currently
set(TINY_DNN_INCLUDE_DIRS ../tiny-dnn/)
include_directories(${TINY_DNN_INCLUDE_DIRS})

# Todo do a package find thing? Maybe not as it's already a submodule so this should just work currently
set(STB_INCLUDE_DIRS ../stb/)
include_directories(${STB_INCLUDE_DIRS})

IF(USE_TBB)
    find_package(TBB REQUIRED)
    add_definitions(-DCNN_USE_TBB)
    include_directories(${TBB_INCLUDE_DIRS})
    link_directories(${TBB_LIBRARY_DIRS})
    SET(REQUIRED_LIBRARIES  ${REQUIRED_LIBRARIES} ${TBB_LIBRARIES}  )
ELSEIF(USE_OMP)
    find_package(OpenMP  REQUIRED)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    add_definitions( -DCNN_USE_OMP)
ENDIF()
# ----------------------------------------------------------------------------
# COMPILER OPTIONS
# ----------------------------------------------------------------------------

IF(CMAKE_COMPILER_IS_GNUCXX OR MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    IF(USE_SSE)
        add_definitions(-DCNN_USE_SSE)
        SET(EXTRA_C_FLAGS  " ${EXTRA_C_FLAGS} -msse3 ")
    ENDIF()
    IF(USE_AVX)
        add_definitions(-DCNN_USE_AVX)
        SET(EXTRA_C_FLAGS  "${EXTRA_C_FLAGS} -mavx ")
    ENDIF()
    IF((NOT USE_TBB) AND (NOT USE_OMP))
        SET(EXTRA_C_FLAGS " ${EXTRA_C_FLAGS} -pthread ")
    ENDIF()
    SET(EXTRA_C_FLAGS " ${EXTRA_C_FLAGS} -std=c++11 -Wall -Wpedantic")
    SET(EXTRA_C_FLAGS_RELEASE " -O3 ")
    SET(EXTRA_C_FLAGS_DEBUG " -g3 ")
ELSE()#MSVC
    IF(USE_SSE )
        add_definitions(-DCNN_USE_SSE)
        SET(EXTRA_C_FLAGS  " ${EXTRA_C_FLAGS} /arch:SSE2 ")
    ENDIF()
    IF(USE_AVX  )
        add_definitions(-DCNN_USE_AVX)
        SET(EXTRA_C_FLAGS  "${EXTRA_C_FLAGS}  /arch:AVX ")
    ENDIF()
    SET(EXTRA_C_FLAGS_RELEASE " /Ox /bigobj ")
    add_definitions( -D _CRT_SECURE_NO_WARNINGS)
ENDIF()


    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_C_FLAGS}")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${EXTRA_C_FLAGS_RELEASE}")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${EXTRA_C_FLAGS_DEBUG}")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EXTRA_EXE_LINKER_FLAGS}")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${EXTRA_EXE_LINKER_FLAGS_RELEASE}")
    SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${EXTRA_EXE_LINKER_FLAGS_DEBUG}")


#------------------------------------------------
# TARGETS
#------------------------------------------------
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} )
LINK_LIBRARIES(${REQUIRED_LIBRARIES} )

SET( scribble_hdrs scribble.h)

# SET(scribble_test_headers test/test.h)

ADD_EXECUTABLE(scribbletest scribble.cpp ${scribble_hdrs})

#IF (BUILD_EXAMPLES)
#    ADD_EXECUTABLE(example_mnist_train examples/mnist/train.cpp ${scribble_hdrs})
#    ADD_EXECUTABLE(example_deconv_train examples/deconv/train.cpp ${scribble_hdrs})
#    IF (USE_OPENCV)
#        ADD_EXECUTABLE(example_mnist_test examples/mnist/test.cpp ${scribble_hdrs})
#    ELSE()
#        FILE(GLOB stb_image_hdrs examples/mnist/stb_image/*.h)
#        ADD_EXECUTABLE(example_mnist_test examples/mnist/test_with_stb_image.cpp ${scribble_hdrs} ${stb_image_hdrs})
#    ENDIF()
#    ADD_EXECUTABLE(example_cifar_train examples/cifar10/train.cpp ${scribble_hdrs})
#    IF (USE_OPENCV)
#        ADD_EXECUTABLE(example_cifar_test examples/cifar10/test.cpp ${scribble_hdrs})
#    ENDIF()
#    IF (USE_OPENCV)
#        target_link_libraries( example_mnist_test ${OpenCV_LIBS} )
#        target_link_libraries( example_cifar_test ${OpenCV_LIBS} )
#        include_directories(${OpenCV_INCLUDE_DIRS})
#    ENDIF()
#
#    find_package(Protobuf)
#    IF (PROTOBUF_FOUND)
#        set(proto_file "${CMAKE_CURRENT_SOURCE_DIR}/scribble/io/caffe/caffe.pb.cc")
#        IF (EXISTS ${proto_file})
#            add_executable(example_caffe_converter
#                           examples/caffe_converter/caffe_converter.cpp
#                           ${scribble_hdrs} ${proto_file})
#            target_link_libraries(example_caffe_converter
#                                  ${PROTOBUF_LIBRARIES} ${OpenCV_LIBS})
#        ELSE()
#            MESSAGE(STATUS "Cannot generate Caffe Importer. Please generate Protobuf file.")
#        ENDIF()
#    ELSE()
#        MESSAGE(STATUS "Cannot generate Caffe Importer. Please install Protobuf.")
#    ENDIF()
#ENDIF()


#IF(BUILD_TESTS)
#    ADD_EXECUTABLE(scribble_test test/test.cpp ${scribble_hdrs} ${scribble_test_headers})
#ENDIF()

#------------------------------------------------
# DOCUMENTATION
#------------------------------------------------

# ========= Doxygen docs =========

#find_package(Doxygen)
#
#if(BUILD_DOCS)
#    if(NOT DOXYGEN_FOUND)
#        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
#    endif()
#
#    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
#    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
#    set(doxy_main_page doc/readme.md)
#
#    configure_file(${doxyfile_in} ${doxyfile} @ONLY)
#
#    add_custom_target(doc
#        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc
#        COMMENT "Generating API documentation with Doxygen"
#        VERBATIM)
#
#    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
#endif()


# ----------------------------------------------------------------------------
# display status message for important variables
# ----------------------------------------------------------------------------
message( STATUS )
MESSAGE( STATUS "-------------------------------------------------------------------------------" )
message( STATUS "General configuration for ${PROJECT_NAME} ${PROJECT_VERSION}")
MESSAGE( STATUS "-------------------------------------------------------------------------------" )


message( STATUS "BUILD_EXAMPLES=${BUILD_EXAMPLES} BUILD_TESTS=${BUILD_TESTS} USE_TBB=${USE_TBB} USE_OMP=${USE_OMP} USE_SSE=${USE_SSE} USE_AVX=${USE_AVX}")


MESSAGE( STATUS "-------------------------------------------------------------------------------" )

message( STATUS )
message("    Built as dynamic libs?:" ${BUILD_SHARED_LIBS})
message("    Compiler:"                   "${CMAKE_COMPILER}"   "${CMAKE_CXX_COMPILER}")
message( STATUS "C++ flags (Release):      ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
message( STATUS "C++ flags (Debug):        ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
message( STATUS "CMAKE_CXX_FLAGS:          ${CMAKE_CXX_FLAGS}")
message( STATUS "CMAKE_BINARY_DIR:         ${CMAKE_BINARY_DIR}")

MESSAGE( STATUS )
MESSAGE( STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}" )
MESSAGE( STATUS "REQUIRED_LIBRARIES = ${REQUIRED_LIBRARIES}" )
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )
MESSAGE( STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}" )
MESSAGE( STATUS "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}" )
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}")
MESSAGE( STATUS )
MESSAGE( STATUS )
MESSAGE( STATUS "Change a value with: cmake -D<Variable>=<Value>" )
MESSAGE( STATUS )
