cmake_minimum_required(VERSION 3.18)
project(UpsNeyro2 VERSION 0.1 LANGUAGES CXX)
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED) # Ищем пакет ДО создания таргетов
    set(CUDA_AVAILABLE ON)
else()
    set(CUDA_AVAILABLE OFF)
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# ── Пути к сторонним зависимостям ─────────────────────────────────────────────
set(FFMPEG_DIR     "${CMAKE_SOURCE_DIR}/third_party/ffmpeg")
set(REALESRGAN_DIR "${CMAKE_SOURCE_DIR}/third_party/realesrgan")
# Linux: vendor path for Python dependencies
if(NOT WIN32)
    set(PYTHON_DEPS_DIR "${CMAKE_BINARY_DIR}/python_deps")
    set(PYTHON_VENV_FOR_INSTALL ${CMAKE_BINARY_DIR}/python_venv_install) # Temporary venv for the pip installation process
endif()

# If we are on Linux and building a Release, try to use the system-installed Real‑ESRGAN package (e.g. from pacman)
if(NOT WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    if(EXISTS "/usr/share/realesrgan")
        set(REALESRGAN_DIR "/usr/share/realesrgan")
    elseif(EXISTS "/usr/bin/realesrgan-ncnn-vulkan")
        # Binary is directly in /usr/bin – use its containing directory as REALESRGAN_DIR
        get_filename_component(REALESRGAN_DIR "/usr/bin/realesrgan-ncnn-vulkan" DIRECTORY)
    elseif(EXISTS "/usr/local/share/realesrgan")
        set(REALESRGAN_DIR "/usr/local/share/realesrgan")
    endif()
endif()

# ── Qt ────────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Concurrent Network)

# ── Исполняемый файл ──────────────────────────────────────────────────────────
qt_add_executable(appUpsNeyro2
    main.cpp
)

set_source_files_properties(theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(appUpsNeyro2
    URI UpsNeyro2
    VERSION 1.0
    QML_FILES
        Main.qml
        components/BottomControls.qml
        components/FiltersPage.qml
        components/PrimaryButton.qml
        components/ResolutionButton.qml
        components/SettingsSection.qml
        components/Sidebar.qml
        components/UpscalePage.qml
        components/VideoPreview.qml
        theme/Theme.qml
        components/SidebarButton.qml
        components/FilterSlider.qml
        components/StatBar.qml
        components/ResourceMonitorPage.qml
        components/SettingsPage.qml
        components/Toast.qml
        components/PreviewPopup.qml
        components/JobQueuePage.qml
        components/TintedIcon.qml
    SOURCES
        systemmonitor.h systemmonitor.cpp
        filtermanager.h
        settingsmanager.h
        gpuupscaler.h gpuupscaler.cpp
        upscalemanager.h upscalemanager.cpp
        framecapture.h framecapture.cpp
        videodecoder.h videodecoder.cpp
        pythonupscaler.h pythonupscaler.cpp
        videoencoder.h videoencoder.cpp
        pipelinemanager.h pipelinemanager.cpp
        framefilter.h framefilter.cpp
        performancemonitor.h performancemonitor.cpp
        presetmanager.h presetmanager.cpp
        filterpreviewmanager.h filterpreviewmanager.cpp
        exportjob.h exportjob.cpp
        settingsmanager.cpp
        hardwareprofiler.h hardwareprofiler.cpp
        updatechecker.h updatechecker.cpp
        projectmanager.h projectmanager.cpp
        jobqueue.h jobqueue.cpp
        recentfilesmodel.h recentfilesmodel.cpp
        logger.h logger.cpp
    RESOURCES
        icons/filters.svg
        icons/magic.svg
        icons/monitor.svg
        icons/pause.svg
        icons/play.svg
        icons/settings-2.svg
        icons/settings.svg
        icons/queue.svg
        icons/chevron-down.svg
        icons/chevron-up.svg
        icons/chevrons-left.svg
        icons/circle-x.svg
        icons/info.svg
        icons/trash-2.svg
        icons/triangle-alert.svg
        icons/volume-2.svg
        icons/volume-off.svg
        icons/x.svg
)

if(CUDA_AVAILABLE)
    message(STATUS "CUDA found: ${CMAKE_CUDA_COMPILER}")

    target_compile_definitions(appUpsNeyro2 PRIVATE USE_CUDA)

    # Добавляем ТОЛЬКО .cu и .cuh файлы (остальные уже в qt_add_qml_module)
    target_sources(appUpsNeyro2 PRIVATE
        framefilter_cuda.cuh
        framefilter_cuda.cu
    )

    # Указываем архитектуры (sm_86 для вашей RTX 3060)
    set_target_properties(appUpsNeyro2 PROPERTIES
        CUDA_ARCHITECTURES "75;86;89"
    )

    # Линкуем библиотеки из CUDAToolkit, который нашли выше
    target_link_libraries(appUpsNeyro2 PRIVATE
        CUDA::cudart
        CUDA::curand
    )
else()
    message(STATUS "CUDA not found, filters will use CPU")
endif()

# ── Свойства таргета ──────────────────────────────────────────────────────────
set_target_properties(appUpsNeyro2 PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    QT_QML_MODULE_URI "UpsNeyro2"
)

# ── Подключаем Qt библиотеки ──────────────────────────────────────────────────
target_link_libraries(appUpsNeyro2 PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Concurrent
    Qt6::Network
)

# ── FFmpeg (только если папка существует) ─────────────────────────────────────
if(EXISTS "${FFMPEG_DIR}/include")
    message(STATUS "FFmpeg found at: ${FFMPEG_DIR}")
    target_include_directories(appUpsNeyro2 PRIVATE "${FFMPEG_DIR}/include")

    if(WIN32)
        target_link_libraries(appUpsNeyro2 PRIVATE
            "${FFMPEG_DIR}/lib/avcodec.lib"
            "${FFMPEG_DIR}/lib/avformat.lib"
            "${FFMPEG_DIR}/lib/avutil.lib"
            "${FFMPEG_DIR}/lib/swscale.lib"
        )
    else()
        target_link_directories(appUpsNeyro2 PRIVATE "${FFMPEG_DIR}/lib")
        target_link_libraries(appUpsNeyro2 PRIVATE
            avcodec
            avformat
            avutil
            swscale
        )
        # rpath — чтобы бинарь нашёл .so рядом с собой в рантайме
        target_link_options(appUpsNeyro2 PRIVATE
            "-Wl,-rpath,${FFMPEG_DIR}/lib"
        )
    endif()
else()
    message(WARNING "FFmpeg not found at ${FFMPEG_DIR} — захват кадров будет недоступен")
endif()

if(NOT DEFINED PYTHON_EXE)
    if(WIN32)
        set(PYTHON_EXE "python")
    else()
        set(PYTHON_EXE "python3")
    endif()
endif()

# Windows Release: распакованный embeddable Python (рядом с exe будет папка python/)
set(UPSNEYRO_PORTABLE_PYTHON "" CACHE PATH
    "Windows Release only: directory that already contains python.exe (embeddable layout). Copied to output as python/. Empty = do not copy; app may use python from PATH.")

# ── Real-ESRGAN путь → передаём в C++ как макрос ──────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(appUpsNeyro2 PRIVATE
        REALESRGAN_DIR="."
        FFMPEG_BIN_DIR="."
        AI_ENGINE_DIR="."
        PYTHON_EXE="python"
        # PYTHON_PACKAGES_DIR is not set for Release, relying on system Python to find packages
    )
else()
    # В debug — из папки проекта
    target_compile_definitions(appUpsNeyro2 PRIVATE
        REALESRGAN_DIR="${REALESRGAN_DIR}"
        FFMPEG_BIN_DIR="${FFMPEG_DIR}/bin"
        AI_ENGINE_DIR="${AI_ENGINE_DIR}"
        PYTHON_EXE="${PYTHON_EXE}"
        PYTHON_PACKAGES_DIR="${PYTHON_INSTALL_DIR}" # This should now point to ${CMAKE_BINARY_DIR}/python_deps
    )
endif()

# --- Python Dependency Management for Linux ---
message(STATUS "Configuring Python dependency installation for Linux...")

# Find Python executable
find_program(PYTHON_EXE_CMD NAMES python3 python)
if(NOT PYTHON_EXE_CMD)
    message(FATAL_ERROR "Python interpreter not found. Please install Python 3 and ensure it is in your PATH.")
endif()
message(STATUS "Using Python: ${PYTHON_EXE_CMD}")

# Find pip executable
# Pinning pip and setuptools to specific versions known to work
find_program(PIP_CMD NAMES pip pip3)
if(NOT PIP_CMD)
    message(FATAL_ERROR "pip command not found. Please install pip for your Python interpreter.")
endif()
message(STATUS "Using pip: ${PIP_CMD}")

# Define a temporary directory for installing Python packages during build
# This ensures a self-contained build environment.
set(PYTHON_INSTALL_DIR ${CMAKE_BINARY_DIR}/python_deps)
set(PYTHON_VENV_FOR_INSTALL ${CMAKE_BINARY_DIR}/python_venv_install) # Temporary venv for the pip installation process

# Copy requirements.txt into the build directory to ensure it's accessible from there
file(COPY ${CMAKE_SOURCE_DIR}/requirements.txt DESTINATION ${CMAKE_BINARY_DIR})

add_custom_target(install_python_requirements
    COMMAND ${PYTHON_EXE_CMD} -m venv ${PYTHON_VENV_FOR_INSTALL} # Create a virtual environment for installation
    COMMAND ${PYTHON_VENV_FOR_INSTALL}/bin/pip install --upgrade pip==23.3.1 setuptools==65.5.0 wheel==0.43.0 # Pin versions, using older setuptools
    # Explicitly provide the path to requirements.txt copied into the build directory
    COMMAND ${PYTHON_VENV_FOR_INSTALL}/bin/pip install -r ${CMAKE_BINARY_DIR}/requirements.txt --target ${PYTHON_INSTALL_DIR}
    COMMENT "Installing Python dependencies from requirements.txt to ${PYTHON_INSTALL_DIR}..."
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
    # Byproducts help CMake avoid re-running if the target directory exists
    BYPRODUCTS ${PYTHON_INSTALL_DIR} ${PYTHON_VENV_FOR_INSTALL}
)

# Ensure Python dependencies are installed before the main executable is built
add_dependencies(appUpsNeyro2 install_python_requirements)

# --- Adjusting file copying commands for Linux ---
# Condition modified to include Linux Debug and Linux Release
if((WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release") OR (NOT WIN32 AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "Release")))
    # ai_engine скрипты
    add_custom_command(TARGET appUpsNeyro2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/ai_engine"
            "$<TARGET_FILE_DIR:appUpsNeyro2>/ai_engine"
        COMMENT "Copying ai_engine..."
    )

    # realesrgan бинарник и модели
    add_custom_command(TARGET appUpsNeyro2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${REALESRGAN_DIR}"
            "$<TARGET_FILE_DIR:appUpsNeyro2>"
        COMMENT "Copying realesrgan..."
    )

    # Portable Python (embeddable): иначе в релизе не будет python/python.exe
    if(UPSNEYRO_PORTABLE_PYTHON AND EXISTS "${UPSNEYRO_PORTABLE_PYTHON}/python.exe")
        add_custom_command(TARGET appUpsNeyro2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${UPSNEYRO_PORTABLE_PYTHON}"
                "$<TARGET_FILE_DIR:appUpsNeyro2>/python"
            COMMENT "Copying portable Python from UPSNEYRO_PORTABLE_PYTHON..."
        )
    endif()
endif()

option(UPSNEYRO_BUILD_TESTS "Build unit tests (Qt Test)" OFF)
if(UPSNEYRO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Установка ─────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS appUpsNeyro2
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)