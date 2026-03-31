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
set(AI_ENGINE_DIR "${CMAKE_SOURCE_DIR}/ai_engine")

# ── Qt ────────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Concurrent)

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
        exportjob.h
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

# ── Real-ESRGAN путь → передаём в C++ как макрос ──────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(appUpsNeyro2 PRIVATE
        REALESRGAN_DIR="."
        FFMPEG_BIN_DIR="."
        AI_ENGINE_DIR="."
        PYTHON_EXE="python"
    )
else()
    # В debug — из папки проекта
    target_compile_definitions(appUpsNeyro2 PRIVATE
        REALESRGAN_DIR="${REALESRGAN_DIR}"
        FFMPEG_BIN_DIR="${FFMPEG_DIR}/bin"
        AI_ENGINE_DIR="${AI_ENGINE_DIR}"
        PYTHON_EXE="${PYTHON_EXE}"
    )
endif()

# Копируем FFmpeg DLL в папку с exe после сборки
if(WIN32)
    file(GLOB FFMPEG_DLLS "${FFMPEG_DIR}/bin/*.dll")
    foreach(DLL ${FFMPEG_DLLS})
        add_custom_command(TARGET appUpsNeyro2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "$<TARGET_FILE_DIR:appUpsNeyro2>"
            COMMENT "Copying ${DLL}"
        )
    endforeach()
endif()

if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
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
endif()

# ── Установка ─────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS appUpsNeyro2
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
