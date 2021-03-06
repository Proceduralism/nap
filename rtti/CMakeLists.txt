include(${NAP_ROOT}/cmake/packaging_macros.cmake)
project(naprtti)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
filter_platform_specific_files(SOURCES)

find_rttr()

set(rapidjson_FIND_QUIETLY TRUE)
find_package(rapidjson REQUIRED)
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)

# Let find_python find our prepackaged Python in thirdparty
if (NAP_ENABLE_PYTHON)
  find_python_in_thirdparty()
  set(pybind11_DIR ${THIRDPARTY_DIR}/pybind11/install/share/cmake/pybind11)
  find_package(pybind11 REQUIRED)
endif()

add_platform_specific_files("${WIN32_SOURCES}" "${MACOS_SOURCES}" "${LINUX_SOURCES}" "${NATIVE_SOURCES}" "${ANDROID_SOURCES}")
add_library(${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Core)
# Remove lib prefix on Unix libraries
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if (MSVC)
    include(GenerateExportheader)
    GENERATE_EXPORT_HEADER(${PROJECT_NAME})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE MODULE_NAME=${PROJECT_NAME} NAP_SHARED_LIBRARY)
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_include_directories(${PROJECT_NAME} PUBLIC ${RAPIDJSON_INCLUDE_DIRS})

if (NAP_ENABLE_PYTHON)
    target_include_directories(${PROJECT_NAME} PUBLIC ${pybind11_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} pybind11::embed)
endif()

target_link_libraries(${PROJECT_NAME} naputility)
target_link_libraries(${PROJECT_NAME} RTTR::Core)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework CoreServices")
elseif(ANDROID)
    target_link_libraries(${PROJECT_NAME} dl)
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} dl pthread)
endif ()

# Windows thirdparty DLL copying
if(WIN32)
    # RTTR
    copy_files_to_bin($<TARGET_FILE:RTTR::Core>)

    # Python
    copy_windows_python_dlls_to_bin()
endif()

if(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME}
                       PRE_BUILD
                       COMMAND ${CMAKE_INSTALL_NAME_TOOL}
                               -id 
                               @rpath/librttr_core.0.9.6.dylib 
                               ${THIRDPARTY_DIR}/rttr/xcode/install/bin/librttr_core.0.9.6.dylib
                       COMMENT "Setting install name for rttr_core")

    add_custom_command(TARGET ${PROJECT_NAME}
                       PRE_BUILD
                       COMMAND ${CMAKE_INSTALL_NAME_TOOL}
                               -id 
                               @rpath/librttr_core_d.0.9.6.dylib 
                               ${THIRDPARTY_DIR}/rttr/xcode/install/bin/librttr_core_d.0.9.6.dylib
                       COMMENT "Setting install name for rttr_core_d")    
endif()

# Package into release build
if (WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION lib/$<CONFIG>
                                    ARCHIVE DESTINATION lib/$<CONFIG>)

    # Install for fbxconverter
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION tools/platform
            CONFIGURATIONS Release)

    if(PACKAGE_PDBS)
        install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION lib/$<CONFIG>)
    endif()
elseif(APPLE)
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/$<CONFIG>)  
elseif(ANDROID)
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/${CMAKE_BUILD_TYPE}/${ANDROID_ABI})
else()
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/${CMAKE_BUILD_TYPE})   
endif()

install(DIRECTORY src/rtti/ DESTINATION include/rtti/
        FILES_MATCHING PATTERN "*.h")

# Package dependent lib into platform release --

# Package pybind
if (NAP_ENABLE_PYTHON)
    install(DIRECTORY ${THIRDPARTY_DIR}/pybind11/install/ DESTINATION thirdparty/pybind11)
    install(FILES ${THIRDPARTY_DIR}/pybind11/LICENSE DESTINATION thirdparty/pybind11)
endif()

# Package RTTR
if (MSVC)
    install(DIRECTORY ${THIRDPARTY_DIR}/rttr/msvc64/install/ DESTINATION thirdparty/rttr)
elseif(APPLE)
    set(BUILT_RPATH "@loader_path/../../thirdparty/rttr/bin/")
    set(BUILT_RPATH "${BUILT_RPATH};@loader_path/../../thirdparty/python/")
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "${BUILT_RPATH}")
    install(DIRECTORY ${THIRDPARTY_DIR}/rttr/xcode/install/ DESTINATION thirdparty/rttr)
elseif(ANDROID)
    install(DIRECTORY ${THIRDPARTY_DIR}/rttr/android/install/ DESTINATION thirdparty/rttr)
elseif(UNIX)
    install(DIRECTORY ${THIRDPARTY_DIR}/rttr/linux/install/ DESTINATION thirdparty/rttr)
endif()

# Package rapidjson
install(FILES ${THIRDPARTY_DIR}/rapidjson/license.txt 
              ${THIRDPARTY_DIR}/rapidjson/readme.md
        DESTINATION thirdparty/rapidjson
        )
install(DIRECTORY ${THIRDPARTY_DIR}/rapidjson/include DESTINATION thirdparty/rapidjson)
