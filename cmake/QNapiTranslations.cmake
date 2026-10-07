# Compiles translations/*.ts to .qm at build time and embeds them in the
# executables under the :/translations resource prefix.

set(QNAPI_TS_FILES
    "${PROJECT_SOURCE_DIR}/translations/qnapi_it.ts"
    "${PROJECT_SOURCE_DIR}/translations/qnapi_pl.ts")

set(QNAPI_QM_DIR "${PROJECT_BINARY_DIR}/translations")
file(MAKE_DIRECTORY "${QNAPI_QM_DIR}")

set(QNAPI_QM_FILES "")
set(_qnapi_qrc_entries "")
foreach(_ts IN LISTS QNAPI_TS_FILES)
  get_filename_component(_name "${_ts}" NAME_WE)
  set(_qm "${QNAPI_QM_DIR}/${_name}.qm")
  add_custom_command(
    OUTPUT "${_qm}"
    COMMAND ${QT}::lrelease -silent "${_ts}" -qm "${_qm}"
    DEPENDS "${_ts}"
    COMMENT "Compiling translation ${_name}"
    VERBATIM)
  list(APPEND QNAPI_QM_FILES "${_qm}")
  string(APPEND _qnapi_qrc_entries "        <file>${_name}.qm</file>\n")
endforeach()

set(QNAPI_TRANSLATIONS_QRC "${QNAPI_QM_DIR}/translations.qrc")
file(WRITE "${QNAPI_TRANSLATIONS_QRC}.in"
     "<RCC>\n    <qresource prefix=\"/translations\">\n${_qnapi_qrc_entries}    </qresource>\n</RCC>\n")
configure_file("${QNAPI_TRANSLATIONS_QRC}.in" "${QNAPI_TRANSLATIONS_QRC}" COPYONLY)

add_custom_target(qnapi_translations DEPENDS ${QNAPI_QM_FILES})

# The resource is compiled into each executable instead of the static
# library, whose unreferenced resource initializer the linker would drop.
function(qnapi_embed_translations target)
  set(_cpp "${CMAKE_CURRENT_BINARY_DIR}/qrc_qnapi_translations.cpp")
  add_custom_command(
    OUTPUT "${_cpp}"
    COMMAND ${QT}::rcc --name qnapi_translations --output "${_cpp}" "${QNAPI_TRANSLATIONS_QRC}"
    DEPENDS "${QNAPI_TRANSLATIONS_QRC}" ${QNAPI_QM_FILES}
    COMMENT "Embedding translations in ${target}"
    VERBATIM)
  set_source_files_properties("${_cpp}" PROPERTIES SKIP_AUTOGEN ON)
  target_sources(${target} PRIVATE "${_cpp}")
  add_dependencies(${target} qnapi_translations)
endfunction()
