add_executable(qnapic
    src/main.cpp
    src/climain.cpp
    src/climain.h
    src/clisubtitlesdownloader.cpp
    src/clisubtitlesdownloader.h
)
target_include_directories(qnapic PRIVATE src)
target_link_libraries(qnapic PRIVATE libqnapi)
qnapi_embed_translations(qnapic)

if(WIN32)
  target_sources(qnapic PRIVATE "${QNAPI_RC_FILE}")
endif()

install(TARGETS qnapic RUNTIME DESTINATION ${QNAPI_INSTALL_BINDIR})
