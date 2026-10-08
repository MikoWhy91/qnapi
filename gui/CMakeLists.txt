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
      "${QNAPI_RC_FILE}")
else()
  list(APPEND QNAPI_GUI_SOURCES
      src/qcumber/qinterprocesschannel.cpp
      src/qcumber/qinterprocesschannel.h)
endif()

if(APPLE)
  set(QNAPI_MACOS_RESOURCES
      "${PROJECT_SOURCE_DIR}/macx/qnapi.icns"
      ${QNAPI_P7ZIP_BINARY}
      ${QNAPI_THIRDPARTY_LICENSES})
  set_source_files_properties(${QNAPI_MACOS_RESOURCES} PROPERTIES
                              MACOSX_PACKAGE_LOCATION Resources)
  list(APPEND QNAPI_GUI_SOURCES
      src/utils/infoplistdockicon.cpp
      src/utils/infoplistdockicon.h
      ${QNAPI_MACOS_RESOURCES})
  # MediaInfoDLL.h loads it from @executable_path
  if(QNAPI_MEDIAINFO_BINARY)
    set_source_files_properties("${QNAPI_MEDIAINFO_BINARY}" PROPERTIES
                                MACOSX_PACKAGE_LOCATION MacOS)
    list(APPEND QNAPI_GUI_SOURCES "${QNAPI_MEDIAINFO_BINARY}")
  endif()
endif()

add_executable(qnapi WIN32 MACOSX_BUNDLE ${QNAPI_GUI_SOURCES})
target_include_directories(qnapi PRIVATE src)
set_target_properties(qnapi PROPERTIES
    AUTOUIC_SEARCH_PATHS "ui;ui/napiprojekt;ui/napisy24;ui/opensubtitles")
target_link_libraries(qnapi PRIVATE libqnapi Qt::Widgets Qt::Network)
qnapi_embed_translations(qnapi)

if(APPLE)
  target_link_libraries(qnapi PRIVATE Qt::Xml)
  set_target_properties(qnapi PROPERTIES
      OUTPUT_NAME QNapi
      MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/macx/Info.plist"
      MACOSX_BUNDLE_BUNDLE_NAME QNapi
      MACOSX_BUNDLE_GUI_IDENTIFIER "org.qnapi.QNapi"
      MACOSX_BUNDLE_ICON_FILE qnapi.icns
      MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${QNAPI_DISPLAYABLE_VERSION}"
      MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2008-2017 Piotr Krzeminski; 2026 Mikolaj Stanczak"
      MACOSX_BUNDLE_INFO_STRING "QNapi ${QNAPI_DISPLAYABLE_VERSION}")
endif()

if(APPLE)
  install(TARGETS qnapi BUNDLE DESTINATION .)
else()
  install(TARGETS qnapi RUNTIME DESTINATION ${QNAPI_INSTALL_BINDIR})
endif()
