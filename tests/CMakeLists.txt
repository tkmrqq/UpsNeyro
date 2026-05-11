find_package(Qt6 REQUIRED COMPONENTS Core Test Gui)

add_executable(test_projectmanager
    test_projectmanager.cpp
    ${CMAKE_SOURCE_DIR}/projectmanager.cpp
)
target_include_directories(test_projectmanager PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(test_projectmanager PRIVATE Qt6::Core Qt6::Test)

add_test(NAME projectmanager COMMAND $<TARGET_FILE:test_projectmanager>)

add_executable(test_upscalemanager
    test_upscalemanager.cpp
    ${CMAKE_SOURCE_DIR}/filtermanager.h
    ${CMAKE_SOURCE_DIR}/upscalemanager.cpp
    ${CMAKE_SOURCE_DIR}/pipelinemanager.cpp
    ${CMAKE_SOURCE_DIR}/framecapture.cpp
    ${CMAKE_SOURCE_DIR}/videodecoder.cpp
    ${CMAKE_SOURCE_DIR}/pythonupscaler.cpp
    ${CMAKE_SOURCE_DIR}/videoencoder.cpp
    ${CMAKE_SOURCE_DIR}/framefilter.cpp
    ${CMAKE_SOURCE_DIR}/performancemonitor.cpp
    ${CMAKE_SOURCE_DIR}/logger.cpp
    ${CMAKE_SOURCE_DIR}/updatechecker.cpp
)
target_include_directories(test_upscalemanager PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(test_upscalemanager PRIVATE Qt6::Core Qt6::Test Qt6::Gui Qt6::Concurrent Qt6::Network)

target_compile_definitions(test_upscalemanager PRIVATE
    REALESRGAN_DIR="${CMAKE_SOURCE_DIR}/third_party/realesrgan"
    FFMPEG_BIN_DIR="${CMAKE_SOURCE_DIR}/third_party/ffmpeg/bin"
    AI_ENGINE_DIR="${CMAKE_SOURCE_DIR}/ai_engine"
    PYTHON_EXE="python"
)

if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/include")
    target_include_directories(test_upscalemanager PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/include"
    )

    if(WIN32)
        target_link_libraries(test_upscalemanager PRIVATE
            "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/avcodec.lib"
            "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/avformat.lib"
            "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/avutil.lib"
            "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/swscale.lib"
        )
    else()
        target_link_directories(test_upscalemanager PRIVATE
            "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib"
        )
        target_link_libraries(test_upscalemanager PRIVATE avcodec avformat avutil swscale)
    endif()
endif()

add_test(NAME upscalemanager COMMAND $<TARGET_FILE:test_upscalemanager>)

if(WIN32)
    # Ensure Qt runtime DLLs are visible for ctest on Windows.
    set_tests_properties(projectmanager PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:test_projectmanager>\;$<TARGET_FILE_DIR:Qt6::Core>\;${CMAKE_BINARY_DIR}\;$ENV{PATH}"
    )
    set_tests_properties(upscalemanager PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:test_upscalemanager>\;$<TARGET_FILE_DIR:Qt6::Core>\;${CMAKE_BINARY_DIR}\;$ENV{PATH}"
    )
endif()
