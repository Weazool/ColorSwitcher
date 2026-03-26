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
)
