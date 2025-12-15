add_library(caqtdm_android_functions STATIC
    src/androidtimeb.c)

target_include_directories(caqtdm_android_functions
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(caqtdm::android_functions ALIAS caqtdm_android_functions)
