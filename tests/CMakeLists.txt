set(CMAKE_AUTOUIC OFF)

function(qnapi_add_test name)
  add_executable(${name} ${name}.cpp testutils.h ${ARGN})
  target_link_libraries(${name} PRIVATE libqnapi Qt::Test)
  add_test(NAME ${name} COMMAND ${name})
  # the tests never need a display, a network connection or the user's
  # QNapi settings
  set_tests_properties(${name} PROPERTIES
      ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QNAPI_TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/data"
      TIMEOUT 120)
endfunction()

qnapi_add_test(tst_checksums)
qnapi_add_test(tst_subconvert)
qnapi_add_test(tst_encoding)
qnapi_add_test(tst_config)
qnapi_add_test(tst_cliargs)
qnapi_add_test(tst_maybe)
qnapi_add_test(tst_p7zip)
qnapi_add_test(tst_opensubtitles mockhttpserver.cpp mockhttpserver.h)

qnapi_add_test(tst_qnapi
    "${PROJECT_SOURCE_DIR}/cli/src/clisubtitlesdownloader.cpp"
    "${PROJECT_SOURCE_DIR}/cli/src/clisubtitlesdownloader.h")
target_include_directories(tst_qnapi PRIVATE "${PROJECT_SOURCE_DIR}/cli/src")

# P7ZipDecoder is tested against a real 7-Zip binary when one is available:
# QNAPI_TEST_7ZIP, the bundled one, or 7z/7za/7zz in PATH
set(_test_7zip "$ENV{QNAPI_TEST_7ZIP}")
if(NOT _test_7zip AND QNAPI_P7ZIP_BINARY)
  set(_test_7zip "${QNAPI_P7ZIP_BINARY}")
endif()
if(NOT _test_7zip)
  find_program(_test_7zip_found NAMES 7z 7za 7zz)
  if(_test_7zip_found)
    set(_test_7zip "${_test_7zip_found}")
  endif()
endif()
if(_test_7zip)
  set_property(TEST tst_p7zip APPEND PROPERTY ENVIRONMENT "QNAPI_TEST_7ZIP=${_test_7zip}")
endif()
