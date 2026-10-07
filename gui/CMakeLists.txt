set(QNAPI_GUI_SOURCES
    src/main.cpp
    src/guimain.cpp
    src/guimain.h
    src/qnapiapp.cpp
    src/qnapiapp.h
    src/qnapiopendialog.cpp
    src/qnapiopendialog.h
    src/qnapithread.h
    src/qcheckedlistwidget.h
    src/forms/frmabout.cpp
    src/forms/frmabout.h
    src/forms/frmconvert.cpp
    src/forms/frmconvert.h
    src/forms/frmlistsubtitles.cpp
    src/forms/frmlistsubtitles.h
    src/forms/frmnapiprojektconfig.cpp
    src/forms/frmnapiprojektconfig.h
    src/forms/frmnapisy24config.cpp
    src/forms/frmnapisy24config.h
    src/forms/frmopensubtitlesconfig.cpp
    src/forms/frmopensubtitlesconfig.h
    src/forms/frmoptions.cpp
    src/forms/frmoptions.h
    src/forms/frmprogress.cpp
    src/forms/frmprogress.h
    src/forms/frmscan.cpp
    src/forms/frmscan.h
    src/forms/frmsummary.cpp
    src/forms/frmsummary.h
    src/forms/subdatawidget.cpp
    src/forms/subdatawidget.h
    src/qcumber/qcumber.h
    src/qcumber/qmanagedrequest.cpp
    src/qcumber/qmanagedrequest.h
    src/qcumber/qmanagedsocket.cpp
    src/qcumber/qmanagedsocket.h
    src/qcumber/qsingleapplication.cpp
    src/qcumber/qsingleapplication.h
    ui/frmabout.ui
    ui/frmconvert.ui
    ui/frmlistsubtitles.ui
    ui/frmoptions.ui
    ui/frmprogress.ui
    ui/frmscan.ui
    ui/frmsummary.ui
    ui/subDataWidget.ui
    ui/napiprojekt/frmnapiprojektconfig.ui
    ui/napisy24/frmnapisy24config.ui
    ui/opensubtitles/frmopensubtitlesconfig.ui
    res/resources.qrc
)

if(WIN32)
  list(APPEND QNAPI_GUI_SOURCES
      src/qcumber/qinterprocesschannel_win32.cpp
      src/qcumber/qinterprocesschannel_win32.h
      "${PROJECT_SOURCE_DIR}/win32/qnapi.rc")
  set_property(SOURCE "${PROJECT_SOURCE_DIR}/win32/qnapi.rc" APPEND PROPERTY
               INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/win32")
else()
  list(APPEND QNAPI_GUI_SOURCES
      src/qcumber/qinterprocesschannel.cpp
      src/qcumber/qinterprocesschannel.h)
endif()

if(APPLE)
  set(QNAPI_MACOS_BUNDLE_FILES
      "${PROJECT_SOURCE_DIR}/macx/qnapi.icns"
      "${PROJECT_SOURCE_DIR}/macx/content/7za")
  set_source_files_properties(${QNAPI_MACOS_BUNDLE_FILES} PROPERTIES
                              MACOSX_PACKAGE_LOCATION Resources)
  set(QNAPI_MACOS_MEDIAINFO "${PROJECT_SOURCE_DIR}/deps/libmediainfo/lib/libmediainfo.0.dylib")
  set_source_files_properties("${QNAPI_MACOS_MEDIAINFO}" PROPERTIES
                              MACOSX_PACKAGE_LOCATION MacOS)
  list(APPEND QNAPI_GUI_SOURCES
      src/utils/infoplistdockicon.cpp
      src/utils/infoplistdockicon.h
      ${QNAPI_MACOS_BUNDLE_FILES}
      "${QNAPI_MACOS_MEDIAINFO}")
endif()

add_executable(qnapi WIN32 MACOSX_BUNDLE ${QNAPI_GUI_SOURCES})
target_include_directories(qnapi PRIVATE src)
set_target_properties(qnapi PROPERTIES
    AUTOUIC_SEARCH_PATHS "ui;ui/napiprojekt;ui/napisy24;ui/opensubtitles")
target_link_libraries(qnapi PRIVATE libqnapi Qt::Widgets Qt::Network)
qnapi_embed_translations(qnapi)

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
  target_link_libraries(qnapi PRIVATE Qt::Xml ${CORE_FOUNDATION})
  set_target_properties(qnapi PROPERTIES
      OUTPUT_NAME QNapi
      MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/macx/Info.plist")
endif()

if(APPLE)
  install(TARGETS qnapi BUNDLE DESTINATION .)
else()
  install(TARGETS qnapi RUNTIME DESTINATION ${QNAPI_INSTALL_BINDIR})
endif()
