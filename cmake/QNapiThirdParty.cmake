# Windows and macOS packages bundle 7-Zip and the MediaInfo library;
# scripts/fetch_deps.sh downloads pinned versions into deps/prebuilt/.
# Linux uses the distribution's packages instead.

if(WIN32)
  set(_platform windows)
  set(_p7zip_name 7za.exe)
  set(_mediainfo_name MediaInfo.dll)
elseif(APPLE)
  set(_platform macos)
  set(_p7zip_name 7za)
  set(_mediainfo_name libmediainfo.0.dylib)
else()
  return()
endif()

set(QNAPI_PREBUILT_DIR "${PROJECT_SOURCE_DIR}/deps/prebuilt/${_platform}" CACHE PATH
    "Directory with the 7-Zip and MediaInfo binaries to bundle (see scripts/fetch_deps.sh)")

set(QNAPI_P7ZIP_BINARY "")
set(QNAPI_MEDIAINFO_BINARY "")
set(QNAPI_THIRDPARTY_LICENSES "")

if(EXISTS "${QNAPI_PREBUILT_DIR}/${_p7zip_name}")
  set(QNAPI_P7ZIP_BINARY "${QNAPI_PREBUILT_DIR}/${_p7zip_name}")
else()
  message(WARNING "${_p7zip_name} not found in ${QNAPI_PREBUILT_DIR}; run scripts/fetch_deps.sh. "
                  "Without it QNapi cannot unpack downloaded subtitles unless 7-Zip is set in its options.")
endif()

if(EXISTS "${QNAPI_PREBUILT_DIR}/${_mediainfo_name}")
  set(QNAPI_MEDIAINFO_BINARY "${QNAPI_PREBUILT_DIR}/${_mediainfo_name}")
else()
  message(WARNING "${_mediainfo_name} not found in ${QNAPI_PREBUILT_DIR}; run scripts/fetch_deps.sh. "
                  "Without it QNapi cannot read the frame rate needed to convert frame-based subtitles.")
endif()

foreach(_license IN ITEMS 7-Zip-License.txt MediaInfo-License.html)
  if(EXISTS "${QNAPI_PREBUILT_DIR}/${_license}")
    list(APPEND QNAPI_THIRDPARTY_LICENSES "${QNAPI_PREBUILT_DIR}/${_license}")
  endif()
endforeach()
