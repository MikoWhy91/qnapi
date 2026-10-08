if(UNIX AND NOT APPLE)
  install(FILES
      doc/ChangeLog
      doc/LICENSE
      doc/LICENSE-pl
      doc/COPYRIGHT
      doc/qnapi-download.desktop
      doc/qnapi-scan.desktop
      doc/qnapi-download.schemas
      doc/qnapi-scan.schemas
      DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/qnapi)

  find_program(GZIP_EXECUTABLE gzip REQUIRED)
  set(_man_pages "")
  foreach(_lang IN ITEMS "" it pl)
    if(_lang)
      set(_src "${PROJECT_SOURCE_DIR}/doc/man/${_lang}/qnapi.1")
      set(_out_dir "${PROJECT_BINARY_DIR}/man/${_lang}")
      set(_dest "${CMAKE_INSTALL_MANDIR}/${_lang}/man1")
    else()
      set(_src "${PROJECT_SOURCE_DIR}/doc/man/qnapi.1")
      set(_out_dir "${PROJECT_BINARY_DIR}/man")
      set(_dest "${CMAKE_INSTALL_MANDIR}/man1")
    endif()
    set(_gz "${_out_dir}/qnapi.1.gz")
    add_custom_command(
      OUTPUT "${_gz}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
      COMMAND ${GZIP_EXECUTABLE} -9 -n -c "${_src}" > "${_gz}"
      DEPENDS "${_src}"
      COMMENT "Compressing man page ${_lang} qnapi.1")
    list(APPEND _man_pages "${_gz}")
    install(FILES "${_gz}" DESTINATION "${_dest}")
  endforeach()
  add_custom_target(qnapi_man_pages ALL DEPENDS ${_man_pages})

  if(QNAPI_BUILD_GUI)
    install(FILES doc/qnapi.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    foreach(_size IN ITEMS 16 32 48 128 512)
      install(FILES gui/res/icons/${_size}x${_size}/apps/qnapi.png
              DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps)
    endforeach()
  endif()
endif()

if(WIN32)
  install(FILES doc/ChangeLog doc/LICENSE doc/LICENSE-pl DESTINATION .)
  if(QNAPI_P7ZIP_BINARY)
    install(PROGRAMS "${QNAPI_P7ZIP_BINARY}" DESTINATION .)
  endif()
  if(QNAPI_MEDIAINFO_BINARY)
    install(FILES "${QNAPI_MEDIAINFO_BINARY}" DESTINATION .)
  endif()
  if(QNAPI_THIRDPARTY_LICENSES)
    install(FILES ${QNAPI_THIRDPARTY_LICENSES} DESTINATION .)
  endif()

  # OpenSubtitles is HTTPS-only and Qt loads OpenSSL at runtime without
  # shipping it. Qt >= 6.5 needs OpenSSL 3, Qt 5.15 - 6.4 OpenSSL 1.1.
  set(OPENSSL_BIN_DIR "$ENV{OPENSSL_BIN_DIR}" CACHE PATH
      "Directory with the OpenSSL DLLs to install next to the executables")
  if(${QT}_VERSION VERSION_GREATER_EQUAL 6.5)
    set(_openssl_suffix "-3")
  else()
    set(_openssl_suffix "-1_1")
  endif()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    string(APPEND _openssl_suffix "-x64")
  endif()
  set(QNAPI_OPENSSL_DLLS libssl${_openssl_suffix}.dll libcrypto${_openssl_suffix}.dll)
  set(_openssl_found "")
  foreach(_dll IN LISTS QNAPI_OPENSSL_DLLS)
    if(OPENSSL_BIN_DIR AND EXISTS "${OPENSSL_BIN_DIR}/${_dll}")
      list(APPEND _openssl_found "${OPENSSL_BIN_DIR}/${_dll}")
    endif()
  endforeach()
  list(LENGTH _openssl_found _openssl_found_count)
  list(LENGTH QNAPI_OPENSSL_DLLS _openssl_wanted_count)
  set(_openssl_shipped FALSE)
  if(_openssl_found_count EQUAL _openssl_wanted_count)
    install(FILES ${_openssl_found} DESTINATION .)
    set(_openssl_shipped TRUE)
  else()
    message(WARNING "OpenSSL DLLs (${QNAPI_OPENSSL_DLLS}) not found; set OPENSSL_BIN_DIR. "
                    "Without them the OpenSubtitles engine cannot connect (HTTPS).")
  endif()

  if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    include(InstallRequiredSystemLibraries)
  endif()

  get_target_property(_qmake ${QT}::qmake IMPORTED_LOCATION)
  get_filename_component(_qt_bin_dir "${_qmake}" DIRECTORY)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
  if(WINDEPLOYQT_EXECUTABLE)
    set(_deploy_flags --no-translations --no-system-d3d-compiler --no-opengl-sw --no-quick-import)
    if(QT_VERSION_MAJOR EQUAL 5)
      list(APPEND _deploy_flags --no-angle)
    endif()
    # since Qt 6.5 windeployqt skips the OpenSSL TLS backend by default
    # ("Skipping plugin qopensslbackend.dll. Use -force-openssl")
    if(_openssl_shipped AND ${QT}_VERSION VERSION_GREATER_EQUAL 6.5)
      list(APPEND _deploy_flags --force-openssl)
    endif()
    set(_deploy_targets "")
    if(QNAPI_BUILD_CLI)
      list(APPEND _deploy_targets "\${CMAKE_INSTALL_PREFIX}/qnapic.exe")
    endif()
    if(QNAPI_BUILD_GUI)
      list(APPEND _deploy_targets "\${CMAKE_INSTALL_PREFIX}/qnapi.exe")
    endif()
    string(REPLACE ";" "\" \"" _deploy_targets_str "${_deploy_targets}")
    string(REPLACE ";" " " _deploy_flags_str "${_deploy_flags}")
    install(CODE "
      execute_process(
        COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" ${_deploy_flags_str} \"${_deploy_targets_str}\"
        RESULT_VARIABLE _rc)
      if(NOT _rc EQUAL 0)
        message(FATAL_ERROR \"windeployqt failed: \${_rc}\")
      endif()")
  else()
    message(WARNING "windeployqt not found; the installed executables will lack Qt libraries")
  endif()

  # belt and suspenders: copy the plugin even if windeployqt still skips it
  if(_openssl_shipped AND ${QT}_VERSION VERSION_GREATER_EQUAL 6.5)
    get_filename_component(_qt_root "${_qt_bin_dir}" DIRECTORY)
    set(_openssl_plugin "${_qt_root}/plugins/tls/qopensslbackend.dll")
    if(EXISTS "${_openssl_plugin}")
      install(FILES "${_openssl_plugin}" DESTINATION tls)
    endif()
  endif()
endif()

if(APPLE AND QNAPI_BUILD_GUI)
  get_target_property(_qmake ${QT}::qmake IMPORTED_LOCATION)
  get_filename_component(_qt_bin_dir "${_qmake}" DIRECTORY)
  find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
  configure_file(macx/appdmg.json.in "${PROJECT_BINARY_DIR}/appdmg.json.in" @ONLY)
  file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/appdmg.json"
       INPUT "${PROJECT_BINARY_DIR}/appdmg.json.in")
  add_custom_target(macdeploy
    COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_BUNDLE_DIR:qnapi>"
    DEPENDS qnapi
    COMMENT "Deploying Qt into QNapi.app"
    VERBATIM)
  add_custom_target(appdmg
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/QNapi.dmg"
    COMMAND appdmg "${PROJECT_BINARY_DIR}/appdmg.json" "${PROJECT_BINARY_DIR}/QNapi.dmg"
    DEPENDS macdeploy
    COMMENT "Creating QNapi.dmg"
    VERBATIM)
endif()
