cmake_minimum_required(VERSION 3.16)

project(gutterTab VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

# Find XCB components
find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED xcb xcb-ewmh)

# Define sources
set(PROJECT_SOURCES
    src/main.cpp
    src/infrastructure/ConfigManager.cpp
    src/infrastructure/DatabaseManager.cpp
    src/domain/TabController.cpp
    src/presentation/OverlayWindow.cpp
    src/presentation/GutterStrip.cpp
    src/presentation/GutterTab.cpp
    src/presentation/DashboardButton.cpp
    src/presentation/BentoCard.cpp
    src/presentation/BentoDashboard.cpp
    src/presentation/SleekDialogs.cpp
    src/presentation/ProfilePickerWindow.cpp
    src/presentation/SleekSideChooserDialog.cpp
    src/presentation/NoteEditorOverlay.cpp
    src/presentation/MarkdownEditor.cpp
)

# Add executable
add_executable(gutterTab ${PROJECT_SOURCES})

# Link libraries
target_link_libraries(gutterTab PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    ${XCB_LIBRARIES}
)

# Include directories
target_include_directories(gutterTab PRIVATE
    src
    ${XCB_INCLUDE_DIRS}
)

# Documentation target (Doxygen)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

