cmake_minimum_required(VERSION 3.8)
project(biorbd VERSION 1.1.3)
set (CMAKE_CXX_STANDARD 11)

# Set some variables
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/modules)

# Because of Eigen, it is not possible to compile biorbd as a dynamic library
if (WIN32)
    SET(BUILD_SHARED_LIBS FALSE CACHE BOOL "Choose if the dynamic library should be build" FORCE)
else()
    SET(BUILD_SHARED_LIBS TRUE CACHE BOOL "Choose if the dynamic library should be build")
endif()
# Set a default build type to 'Release' if none was specified
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	MESSAGE(STATUS "Setting build type to 'Release' as none was specified.")
	SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build.")
  # Set the possible values of build type for cmake-gui
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()

# Choose the math's library backend to use
set(MATH_LIBRARY_BACKEND Eigen3 CACHE STRING "Choose the backend library for the linear algebra.")
# Set the possible values of build type for cmake-gui
set_property(CACHE MATH_LIBRARY_BACKEND PROPERTY STRINGS "Eigen3" "Casadi")

# Prepare linkings
find_package(RBDL REQUIRED)
set(BOOST_ROOT ${CMAKE_INSTALL_PREFIX})
find_package(Boost REQUIRED)
if(${MATH_LIBRARY_BACKEND} STREQUAL "Eigen3")
  find_package(Eigen3 REQUIRED)
  set(MATH_BACKEND_INCLUDE_DIR ${EIGEN3_INCLUDE_DIR})
  set(MATH_BACKEND_LIBRARIES "")
  set(BIORBD_USE_EIGEN3_MATH true)
  set(BIORBD_USE_CASADI_MATH false)
elseif(${MATH_LIBRARY_BACKEND} STREQUAL "Casadi")
  find_package(Casadi REQUIRED)
  set(MATH_BACKEND_INCLUDE_DIR ${Casadi_INCLUDE_DIR} ${Casadi_INCLUDE_DIR}/..)
  set(MATH_BACKEND_LIBRARIES ${Casadi_LIBRARY})
  set(BIORBD_USE_EIGEN3_MATH false)
  set(BIORBD_USE_CASADI_MATH true)
endif()
find_package(Dlib REQUIRED)
find_package(IPOPT)
find_package(TinyXML)

# Manage options
option(SKIP_KALMAN "If Kalman filter should be compiled" OFF)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    option(SKIP_ASSERT "Some checks slow the code down at run-time, but provide more robust code.
        By default, it is ON when compiling in debug, but OFF in release." ON)
else()
    option(SKIP_ASSERT "Some checks slow the code down at run-time, but provide more robust code.
        By default, it is ON when compiling in debug, but OFF in release." OFF)
endif()

if (IPOPT_FOUND)
    option(SKIP_STATIC_OPTIM "If Static optimization should be compiled" OFF)
    if (BIORBD_USE_CASADI_MATH)
        set(SKIP_STATIC_OPTIM ON CACHE BOOL "Static optimization can't be compiled along with Casadi" FORCE)
    endif()
else()
    set(IPOPT_INCLUDE_DIR "")
    set(IPOPT_LIBRARY "")
    set(SKIP_STATIC_OPTIM ON CACHE BOOL "Static optimization can't be compiled since Ipopt was not found" FORCE)
endif()

if (TinyXML_FOUND)
    option(MODULE_VTP_FILES_READER "If reader for geometry vtp files from opensim should be compiled" ON)
else()
    set(TinyXML_INCLUDE_DIR "")
    set(TinyXML_LIBRARY "")
    set(MODULE_VTP_FILES_READER OFF CACHE BOOL "VTP cannot be used since TinyXML was not found" FORCE)
endif()


# Prepare add library
set(SRC_LIST
    src/BiorbdModel.cpp
    src/ModelReader.cpp
    src/ModelWriter.cpp
)
if (BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
else()
    add_library(${PROJECT_NAME} STATIC ${SRC_LIST})
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_debug")

# Create the configuration header file associated with the asked options
configure_file(${CMAKE_SOURCE_DIR}/include/biorbdConfig.h.in ${CMAKE_BINARY_DIR}/include/biorbdConfig.h)

# Add headers
set(ALL_INCLUDE_DIR 
  ${RBDL_INCLUDE_DIR}
  ${RBDL_INCLUDE_DIR}/..
  ${Boost_INCLUDE_DIRS}
  ${MATH_BACKEND_INCLUDE_DIR}
  ${DLIB_INCLUDE_DIR}
  ${IPOPT_INCLUDE_DIR}
  ${TinyXML_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${ALL_INCLUDE_DIR}
)

# Installation directories
if(WIN32)
    set(${PROJECT_NAME}_LIB_FOLDER Lib)
    set(${PROJECT_NAME}_BIN_FOLDER bin)
    set(${PROJECT_NAME}_INCLUDE_FOLDER include/${PROJECT_NAME})
else()
    set(${PROJECT_NAME}_LIB_FOLDER ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME})
    set(${PROJECT_NAME}_BIN_FOLDER ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME})
    set(${PROJECT_NAME}_INCLUDE_FOLDER ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH ${${PROJECT_NAME}_BIN_FOLDER})
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)


# Add the different modules of biorbd
set(BIORBD_MODULE_NAMES)

# Utils
add_subdirectory(src/Utils)
list(APPEND BIORBD_MODULE_NAMES ${UTILS_MODULE_NAME})

# Utils
add_subdirectory(src/RigidBody)
list(APPEND BIORBD_MODULE_NAMES ${RIGIDBODY_MODULE_NAME})

# Actuators
option(MODULE_ACTUATORS "Choose if the module actuators is to be added to biorbd" ON)
if (MODULE_ACTUATORS)
    add_subdirectory(src/Actuators)
    list(APPEND BIORBD_MODULE_NAMES ${ACTUATOR_MODULE_NAME})
endif()

# Muscles
option(MODULE_MUSCLES "Choose if the module actuators is to be added to biorbd" ON)
if (MODULE_MUSCLES)
    add_subdirectory(src/Muscles)
    list(APPEND BIORBD_MODULE_NAMES ${MUSCLE_MODULE_NAME})
endif()

# Add linker
target_link_libraries(${PROJECT_NAME}
    ${BIORBD_MODULE_NAMES}
    ${MATH_BACKEND_LIBRARIES}
    ${RBDL_LIBRARY}
    ${DLIB_LIBRARY}
    ${IPOPT_LIBRARY}
    ${TinyXML_LIBRARY}
)

# install target
install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${${PROJECT_NAME}_LIB_FOLDER}
        RUNTIME DESTINATION ${${PROJECT_NAME}_BIN_FOLDER}
        LIBRARY DESTINATION ${${PROJECT_NAME}_LIB_FOLDER}
        )
install(DIRECTORY include/ DESTINATION ${${PROJECT_NAME}_INCLUDE_FOLDER})
install(FILES ${CMAKE_BINARY_DIR}/include/biorbdConfig.h DESTINATION ${${PROJECT_NAME}_INCLUDE_FOLDER})
        
# uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

        
# Add binding subdirectory
add_subdirectory(binding)

# Add the example if asked
option(BUILD_EXAMPLE "Build a C++ example" ON)
if (BUILD_EXAMPLE)
    add_subdirectory(example)
endif()

# Doc
option(BUILD_DOC "Build documentation" OFF)
if (BUILD_DOC)
    add_subdirectory(doc)
endif (BUILD_DOC)

# Testing
option(BUILD_TESTS "Build all tests." OFF) # Makes boolean 'test' available.
if (BUILD_TESTS)
    add_subdirectory(test)
endif()





