add_library(seismic_io
    segy_data_interface.cpp
)

target_include_directories(seismic_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(segy_to_sep
    segy_to_sep.cpp
)

target_link_libraries(segy_to_sep
    PRIVATE
        seismic_io
)

add_executable(segy_sep_validate
    segy_sep_validate.cpp
)

target_link_libraries(segy_sep_validate
    PRIVATE
        seismic_io
)
