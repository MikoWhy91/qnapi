add_library(libqnapi STATIC
    src/config/configreader.cpp
    src/config/configwriter.cpp
    src/config/engineconfig.cpp
    src/config/generalconfig.cpp
    src/config/postprocessingconfig.cpp
    src/config/qnapiconfig.cpp
    src/config/scanconfig.cpp
    src/config/staticconfig.cpp
    src/engines/napiprojektdownloadengine.cpp
    src/engines/napisy24downloadengine.cpp
    src/engines/opensubtitlesdownloadengine.cpp
    src/engines/subtitledownloadengine.cpp
    src/engines/subtitledownloadenginesregistry.cpp
    src/libqnapi.cpp
    src/movieinfo/libmediainfomovieinfoprovider.cpp
    src/parser/backuplangargparser.cpp
    src/parser/cliargparser.cpp
    src/parser/cliargparsersexecutor.cpp
    src/parser/dontshowlistargparser.cpp
    src/parser/downloadsubtitlesargparser.cpp
    src/parser/extensionargparser.cpp
    src/parser/formatargparser.cpp
    src/parser/langargparser.cpp
    src/parser/quietbatchargparser.cpp
    src/parser/runcliappargparser.cpp
    src/parser/scandirectoryargparser.cpp
    src/parser/showhelpargparser.cpp
    src/parser/showhelplanguagesargparser.cpp
    src/parser/showversionargparser.cpp
    src/parser/showlistargparser.cpp
    src/parser/showoptionsargparser.cpp
    src/qnapi.cpp
    src/subconvert/formats/microdvd.cpp
    src/subconvert/formats/mpl2.cpp
    src/subconvert/formats/subrip.cpp
    src/subconvert/formats/tmplayer.cpp
    src/subconvert/subtitleconverter.cpp
    src/subconvert/subtitleformat.cpp
    src/subconvert/subtitleformatsregistry.cpp
    src/subtitleinfo.cpp
    src/subtitlelanguage.cpp
    src/subtitlematcher.cpp
    src/subtitlepostprocessor.cpp
    src/utils/console.cpp
    src/utils/encodingutils.cpp
    src/utils/helphelper.cpp
    src/utils/p7zipdecoder.cpp
    src/utils/synchttp.cpp
)
set_target_properties(libqnapi PROPERTIES OUTPUT_NAME qnapi)

# PUBLIC so qnapi/qnapic see a release-tag override (e.g. 0.3.0-rc1).
target_compile_definitions(libqnapi PUBLIC
    "QNAPI_DISPLAYABLE_VERSION=\"${QNAPI_DISPLAYABLE_VERSION}\"")

target_include_directories(libqnapi PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/deps/qt-maybe")

target_link_libraries(libqnapi PUBLIC Qt::Core Qt::Network)
if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(libqnapi PUBLIC Qt::Core5Compat)
endif()

# Linux links libmediainfo directly; Windows and macOS load the MediaInfo
# library at runtime through MediaInfoDLL.h.
if(WIN32 OR APPLE)
  target_include_directories(libqnapi PRIVATE "${PROJECT_SOURCE_DIR}/deps/libmediainfo/include")
  target_link_libraries(libqnapi PRIVATE ${CMAKE_DL_LIBS})
  if(APPLE)
    # MediaInfoDLL.h uses CFBundle* to find the dylib next to the executable.
    # PUBLIC so both qnapi and qnapic pick it up when linking the static lib.
    find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
    target_link_libraries(libqnapi PUBLIC ${CORE_FOUNDATION})
  endif()
else()
  find_package(PkgConfig REQUIRED)
  # not an IMPORTED_TARGET: some distributions' libmediainfo.pc lists
  # include directories that do not exist, which CMake rejects
  pkg_check_modules(MEDIAINFO REQUIRED libmediainfo)
  target_include_directories(libqnapi PRIVATE ${MEDIAINFO_INCLUDE_DIRS})
  target_compile_options(libqnapi PRIVATE ${MEDIAINFO_CFLAGS_OTHER})
  target_link_libraries(libqnapi PRIVATE ${MEDIAINFO_LINK_LIBRARIES})
endif()
