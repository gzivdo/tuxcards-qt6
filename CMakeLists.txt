cmake_minimum_required(VERSION 3.20)
project(tuxcards LANGUAGES CXX C VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Honor INCLUDE_CURRENT_DIR so AUTOUIC-generated ui_*.h are picked up
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(TUXCARDS_BACKEND_OPENSSL
       "Link the OpenSSL AES-256-GCM + PBKDF2 crypto backend (read+write)." ON)
option(TUXCARDS_BACKEND_MONOCYPHER
       "Link the vendored monocypher XChaCha20-Poly1305 + Argon2i backend\
 (read+write). No external dependency — bundled in tree." ON)
set(TUXCARDS_WRITE_BACKEND "openssl" CACHE STRING
       "Default crypto backend for writing new encrypted blobs.\
 Allowed values: openssl, monocypher.")
set_property(CACHE TUXCARDS_WRITE_BACKEND PROPERTY STRINGS openssl monocypher)

# Backward-compat: the old TUXCARDS_WITH_OPENSSL knob still toggles the
# OpenSSL backend the same way it used to.
if(DEFINED TUXCARDS_WITH_OPENSSL AND NOT TUXCARDS_WITH_OPENSSL)
   set(TUXCARDS_BACKEND_OPENSSL OFF)
endif()

# Validate: at least one backend must be on, and the chosen write backend
# must actually be linked.
if(NOT TUXCARDS_BACKEND_OPENSSL AND NOT TUXCARDS_BACKEND_MONOCYPHER)
   message(FATAL_ERROR
      "At least one of TUXCARDS_BACKEND_OPENSSL or TUXCARDS_BACKEND_MONOCYPHER\
 must be ON.")
endif()
# If OpenSSL was disabled (often via legacy TUXCARDS_WITH_OPENSSL=OFF) but the
# write backend is still at the default 'openssl', silently fall back to
# monocypher instead of failing — it's the only remaining backend.
if(NOT TUXCARDS_BACKEND_OPENSSL AND TUXCARDS_WRITE_BACKEND STREQUAL "openssl"
   AND TUXCARDS_BACKEND_MONOCYPHER)
   message(STATUS
      "TUXCARDS_BACKEND_OPENSSL=OFF → switching TUXCARDS_WRITE_BACKEND to monocypher.")
   set(TUXCARDS_WRITE_BACKEND "monocypher" CACHE STRING
       "Default crypto backend for writing new encrypted blobs." FORCE)
endif()
if(TUXCARDS_WRITE_BACKEND STREQUAL "openssl" AND NOT TUXCARDS_BACKEND_OPENSSL)
   message(FATAL_ERROR
      "TUXCARDS_WRITE_BACKEND=openssl requires TUXCARDS_BACKEND_OPENSSL=ON.")
endif()
if(TUXCARDS_WRITE_BACKEND STREQUAL "monocypher" AND NOT TUXCARDS_BACKEND_MONOCYPHER)
   message(FATAL_ERROR
      "TUXCARDS_WRITE_BACKEND=monocypher requires TUXCARDS_BACKEND_MONOCYPHER=ON.")
endif()

find_package(Qt6 REQUIRED COMPONENTS
   Core
   Gui
   Widgets
   Xml
   PrintSupport
   Core5Compat
)

if(TUXCARDS_BACKEND_OPENSSL)
   find_package(OpenSSL REQUIRED)
endif()

set(TUXCARDS_SOURCES
   src/main.cpp
   src/persister.cpp
   src/fontsettings.cpp
   src/CTuxCardsConfiguration.cpp
   src/commandlineoptions.cpp

   src/gui/mainwindow.cpp
   src/gui/CTree.cpp
   src/gui/CTreeElement.cpp
   src/gui/CSingleEntryView.cpp
   src/gui/editor.cpp
   src/gui/editorfindbar.cpp
   src/gui/BookmarkButton.cpp
   src/gui/RecentFileList.cpp
   src/gui/colorbar/CColorBar.cpp

   src/gui/dialogs/CFileEncryptionPasswordDialog.cpp
   src/gui/dialogs/CPasswdDialog.cpp
   src/gui/dialogs/CPropertyDialog.cpp
   src/gui/dialogs/iconselector.cpp
   src/gui/dialogs/iconselectordialog.cpp
   src/gui/dialogs/optionsdialog.cpp
   src/gui/dialogs/searchdialog.cpp
   src/gui/dialogs/searchlistitem.cpp
   src/gui/dialogs/searchposition.cpp
   src/gui/dialogs/searchhighlightdelegate.cpp

   src/information/CInformationCollection.cpp
   src/information/CInformationElement.cpp
   src/information/CInformationElementHistory.cpp
   src/information/CTreeInformationElement.cpp
   src/information/Path.cpp
   src/information/converter.cpp
   src/information/htmlwriter.cpp
   src/information/informationformat.cpp
   src/information/xmlpersister.cpp

   src/utilities/CIconManager.cpp
   src/utilities/integers.cpp
   src/utilities/strings.cpp
   src/utilities/crypt/BlowFish.cpp
   src/utilities/crypt/MD5.cpp
   src/utilities/crypt/StringCrypter.cpp
   src/utilities/iniparser/configgroup.cpp
   src/utilities/iniparser/configparser.cpp
)

set(TUXCARDS_HEADERS
   src/global.h
   src/version.h
   src/Greetings.h
   src/persister.h
   src/fontsettings.h
   src/CTuxCardsConfiguration.h
   src/commandlineoptions.h

   src/gui/mainwindow.h
   src/gui/CTree.h
   src/gui/CTreeElement.h
   src/gui/CSingleEntryView.h
   src/gui/editor.h
   src/gui/editorfindbar.h
   src/gui/BookmarkButton.h
   src/gui/RecentFileList.h
   src/gui/colorbar/CColorBar.h

   src/gui/dialogs/CFileEncryptionPasswordDialog.h
   src/gui/dialogs/CPasswdDialog.h
   src/gui/dialogs/CPropertyDialog.h
   src/gui/dialogs/iconselector.h
   src/gui/dialogs/iconselectordialog.h
   src/gui/dialogs/optionsdialog.h
   src/gui/dialogs/searchdialog.h
   src/gui/dialogs/searchlistitem.h
   src/gui/dialogs/searchposition.h
   src/gui/dialogs/searchhighlightdelegate.h

   src/information/CInformationCollection.h
   src/information/CInformationElement.h
   src/information/CInformationElementHistory.h
   src/information/CTreeInformationElement.h
   src/information/IHistoryListener.h
   src/information/IParent.h
   src/information/IRemoveElementListener.h
   src/information/IView.h
   src/information/Path.h
   src/information/converter.h
   src/information/htmlwriter.h
   src/information/informationformat.h
   src/information/xmlpersister.h

   src/utilities/CIconManager.h
   src/utilities/integers.h
   src/utilities/strings.h
   src/utilities/crypt/BlowFish.h
   src/utilities/crypt/BlowFish2.h
   src/utilities/crypt/MD5.h
   src/utilities/crypt/StringCrypter.h
   src/utilities/iniparser/configgroup.h
   src/utilities/iniparser/configparser.h
)

set(TUXCARDS_FORMS
   src/gui/dialogs/IFileEncryptionPasswordDialog.ui
   src/gui/dialogs/IPropertyDialog.ui
   src/gui/dialogs/IPasswdDialog.ui
   src/gui/dialogs/IOptionsDialog.ui
)

set(TUXCARDS_RESOURCES
   src/icons/icons.qrc
)

# Compile .ts -> .qm at configure time using Qt's lrelease.
# The free-standing form of qt_add_lrelease / qt_add_lupdate that
# creates its own target (`qt_add_lrelease(name TS_FILES ...)`) only
# appeared in Qt 6.7. On older distro Qt (Ubuntu 24.04 ships 6.4,
# Debian 12 ships 6.2) the older signature expects a pre-existing
# target and would fail. On those older versions we fall back to the
# pre-built .qm files checked into translations/ via the static
# translations.qrc — same end-user experience, just no automated refresh.
set(TS_FILES
   translations/tuxcards_en.ts
   translations/tuxcards_ru.ts
   translations/tuxcards_de.ts
   translations/tuxcards_zh_CN.ts
   translations/tuxcards_zh_TW.ts
   translations/tuxcards_es.ts
   translations/tuxcards_fr.ts
   translations/tuxcards_pt_BR.ts
   translations/tuxcards_ja.ts
   translations/tuxcards_ko.ts
)
find_package(Qt6 COMPONENTS LinguistTools QUIET)
if(Qt6LinguistTools_FOUND AND Qt6_VERSION VERSION_GREATER_EQUAL "6.7")
   # Qt 6.7+ — generate .qm into the build dir and synthesize a .qrc
   # that points at *those* build-dir .qm files. This makes the
   # translations refresh fully automatic: `lupdate` + rebuild is
   # enough, no manual copy of build/*.qm back into the source tree
   # and no `touch translations.qrc` afterwards.
   qt_add_lrelease(tuxcards_translations TS_FILES ${TS_FILES}
                   QM_FILES_OUTPUT_VARIABLE qm_files)
   qt_add_lupdate(tuxcards_lupdate
                  SOURCES ${TUXCARDS_SOURCES} ${TUXCARDS_HEADERS} ${TUXCARDS_FORMS}
                  TS_FILES ${TS_FILES}
                  NO_GLOBAL_TARGET)
   set(_generated_qrc_body
       "<!DOCTYPE RCC><RCC version=\"1.0\">\n  <qresource prefix=\"/translations\">\n")
   foreach(_qm ${qm_files})
      get_filename_component(_qm_name "${_qm}" NAME)
      string(APPEND _generated_qrc_body
             "    <file alias=\"${_qm_name}\">${_qm}</file>\n")
   endforeach()
   string(APPEND _generated_qrc_body "  </qresource>\n</RCC>\n")
   set(_generated_qrc "${CMAKE_CURRENT_BINARY_DIR}/translations_generated.qrc")
   file(GENERATE OUTPUT "${_generated_qrc}" CONTENT "${_generated_qrc_body}")
   list(APPEND TUXCARDS_RESOURCES "${_generated_qrc}")
else()
   # Pre-Qt-6.7 — use the .qm files committed under translations/.
   list(APPEND TUXCARDS_RESOURCES translations/translations.qrc)
endif()

qt_add_executable(tuxcards
   ${TUXCARDS_SOURCES}
   ${TUXCARDS_HEADERS}
   ${TUXCARDS_FORMS}
   ${TUXCARDS_RESOURCES}
)

# qt_add_executable enables MACOSX_BUNDLE/WIN32_EXECUTABLE by default
# in Qt 6.4+, but older Qt or single-file generators sometimes don't.
# Force it on so macdeployqt has a real .app bundle to operate on.
set_target_properties(tuxcards PROPERTIES
   MACOSX_BUNDLE TRUE
   WIN32_EXECUTABLE TRUE)

target_link_libraries(tuxcards PRIVATE
   Qt6::Core
   Qt6::Gui
   Qt6::Widgets
   Qt6::Xml
   Qt6::PrintSupport
   Qt6::Core5Compat
)

if(TUXCARDS_BACKEND_OPENSSL)
   target_link_libraries(tuxcards PRIVATE OpenSSL::Crypto)
   target_compile_definitions(tuxcards PRIVATE TUXCARDS_BACKEND_OPENSSL)
endif()

if(TUXCARDS_BACKEND_MONOCYPHER)
   target_sources(tuxcards PRIVATE
      src/utilities/crypt/monocypher/monocypher.c
      src/utilities/crypt/monocypher/monocypher.h)
   target_compile_definitions(tuxcards PRIVATE TUXCARDS_BACKEND_MONOCYPHER)
endif()

if(TUXCARDS_WRITE_BACKEND STREQUAL "openssl")
   target_compile_definitions(tuxcards PRIVATE TUXCARDS_WRITE_BACKEND_AESGCM)
elseif(TUXCARDS_WRITE_BACKEND STREQUAL "monocypher")
   target_compile_definitions(tuxcards PRIVATE TUXCARDS_WRITE_BACKEND_XCHACHA)
endif()

message(STATUS "TuxCards crypto backends: "
        "OpenSSL=${TUXCARDS_BACKEND_OPENSSL} "
        "monocypher=${TUXCARDS_BACKEND_MONOCYPHER} "
        "write=${TUXCARDS_WRITE_BACKEND}")

# MSVC needs /utf-8 because the source files contain UTF-8 byte
# sequences in strings (German umlauts in comments, fallback strings,
# etc.). The XPM icon arrays used to be declared `static char *` —
# MSVC errors C2440 on the const char[] -> char* conversion — but we
# patched the few offenders to `static const char *`, no /permissive
# needed.
if(MSVC)
   target_compile_options(tuxcards PRIVATE /utf-8 /wd4996)
endif()

install(TARGETS tuxcards
        RUNTIME DESTINATION bin
        BUNDLE  DESTINATION .)
install(FILES packaging/desktop/tuxcards.desktop
        DESTINATION share/applications)
install(FILES src/icons/lo16-app-tuxcards.png
        DESTINATION share/icons/hicolor/16x16/apps RENAME tuxcards.png)
install(FILES src/icons/lo32-app-tuxcards.png
        DESTINATION share/icons/hicolor/32x32/apps RENAME tuxcards.png)
