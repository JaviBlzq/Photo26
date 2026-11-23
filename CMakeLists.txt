cmake_minimum_required(VERSION 3.25)

project(Photo26
    VERSION 1.0.0
    DESCRIPTION "Photo26 - Caja de herramientas nativa de procesamiento, conversión y optimización de imágenes para Windows"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Concurrent
    Svg
    PrintSupport
    Test
)

# ==============================================================================
# 1. photo26-core (STATIC library — ZERO dependency on Qt6::Widgets)
# ==============================================================================
add_library(photo26-core STATIC
    src/core/common/safe_file_writer.cpp
    src/core/io/format_registry.cpp
    src/core/io/image_io.cpp
    src/core/io/ico_codec.cpp
    src/core/io/gif_codec.cpp
    src/core/io/pdf_exporter.cpp
    src/core/io/zip_writer.cpp
    src/core/metadata/metadata_engine.cpp
    src/core/color/icc_manager.cpp
    src/core/ops/crop_transform.cpp
    src/core/ops/resize_op.cpp
    src/core/ops/combine_op.cpp
    src/core/ops/color_adjust_op.cpp
    src/core/ops/compress_web_op.cpp
    src/core/ops/watermark_text_op.cpp
    src/core/ops/spritesheet_op.cpp
    src/core/ops/palette_compare_bg_op.cpp
    src/core/ops/batch_rename_op.cpp
)

target_include_directories(photo26-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(photo26-core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Concurrent
    Qt6::Svg
    Qt6::PrintSupport
)

# ==============================================================================
# 2. photo26-desktop-lib & photo26 (WIN32 GUI Application)
# ==============================================================================
add_library(photo26-desktop-lib STATIC
    src/desktop/widgets/theme_manager.hpp
    src/desktop/widgets/theme_manager.cpp
    src/desktop/widgets/interactive_canvas.hpp
    src/desktop/widgets/interactive_canvas.cpp
    src/desktop/widgets/tool_card_grid.hpp
    src/desktop/widgets/tool_card_grid.cpp
    src/desktop/widgets/export_bar.hpp
    src/desktop/widgets/export_bar.cpp
    src/desktop/main_window.hpp
    src/desktop/main_window.cpp
)

target_include_directories(photo26-desktop-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(photo26-desktop-lib PUBLIC
    photo26-core
    Qt6::Widgets
)

add_executable(photo26 WIN32
    src/desktop/main.cpp
)

target_link_libraries(photo26 PRIVATE
    photo26-desktop-lib
)
if(WIN32)
    target_link_libraries(photo26 PRIVATE psapi)
endif()

set_target_properties(photo26 PROPERTIES
    OUTPUT_NAME "photo26"
)

# ==============================================================================
# 3. photo26-tests (Automated Unit, Integration, Format, Metadata & GUI Tests)
# ==============================================================================
enable_testing()

add_executable(photo26-tests
    tests/test_photo26_all.cpp
)

target_link_libraries(photo26-tests PRIVATE
    photo26-desktop-lib
    photo26-core
    Qt6::Test
    Qt6::Widgets
)

add_test(
    NAME photo26-tests
    COMMAND photo26-tests -platform offscreen
)
