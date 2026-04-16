cmake_minimum_required(VERSION 3.20)
project(ColorSwitcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static link MinGW runtime so the exe has no DLL dependencies
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

add_executable(ColorSwitcher WIN32
    src/main.cpp
    src/gamma_engine.cpp
    src/config_manager.cpp
    src/preset_editor.cpp
    src/autostart.cpp
    src/nvapi_controller.cpp
    src/foreground_watcher.cpp
    resources/app.rc
)

target_include_directories(ColorSwitcher PRIVATE
    src
    third_party
)

target_link_libraries(ColorSwitcher PRIVATE
    user32
    gdi32
    shell32
    advapi32
    comctl32
    comdlg32
)

# Code signing with osslsigncode (self-signed certificate)
set(PFX_FILE "${CMAKE_SOURCE_DIR}/ColorSwitcher.pfx" CACHE FILEPATH "Path to PFX certificate")
set(PFX_PASSWORD "colorswitcher" CACHE STRING "PFX password")

find_program(OSSLSIGNCODE osslsigncode)
if(OSSLSIGNCODE AND EXISTS "${PFX_FILE}")
    add_custom_command(TARGET ColorSwitcher POST_BUILD
        COMMAND ${OSSLSIGNCODE} sign
            -pkcs12 "${PFX_FILE}"
            -pass "${PFX_PASSWORD}"
            -n "ColorSwitcher"
            -t http://timestamp.digicert.com
            -in "$<TARGET_FILE:ColorSwitcher>"
            -out "$<TARGET_FILE:ColorSwitcher>.signed"
        COMMAND ${CMAKE_COMMAND} -E rename
            "$<TARGET_FILE:ColorSwitcher>.signed"
            "$<TARGET_FILE:ColorSwitcher>"
        COMMENT "Signing ColorSwitcher.exe with osslsigncode..."
    )
endif()
