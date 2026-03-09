cmake_minimum_required(VERSION 3.16)

project(UpsNeyro2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia)

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
        QML_FILES components/SidebarButton.qml
        QML_FILES components/FilterSlider.qml
        QML_FILES components/StatBar.qml
        QML_FILES components/ResourceMonitorPage.qml
        QML_FILES components/SettingsPage.qml
        SOURCES systemmonitor.h systemmonitor.cpp
        RESOURCES icons/filters.svg icons/magic.svg icons/monitor.svg icons/pause.svg icons/play.svg icons/settings-2.svg icons/settings.svg
)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appUpsNeyro2 PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appUpsNeyro2
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    QT_QML_MODULE_URI "UpsNeyro2"
)

target_link_libraries(appUpsNeyro2 PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
)

include(GNUInstallDirs)
install(TARGETS appUpsNeyro2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
