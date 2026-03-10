find_package(OpenGL QUIET)
find_package(GLUT QUIET)

if (OpenGL_FOUND AND GLUT_FOUND)
    add_executable(segy_data_viewer
        main.cpp
    )

    target_include_directories(segy_data_viewer
        PRIVATE
            ${CMAKE_SOURCE_DIR}/System/InputOutput
    )

    target_link_libraries(segy_data_viewer
        PRIVATE
            OpenGL::GL
            OpenGL::GLU
            GLUT::GLUT
    )
else()
    message(WARNING "Skipping segy_data_viewer: OpenGL/GLUT not found")
endif()
