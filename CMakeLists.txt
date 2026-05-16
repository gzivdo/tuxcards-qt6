cmake_minimum_required(VERSION 3.20)
project(tuxcards LANGUAGES CXX VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Honor INCLUDE_CURRENT_DIR so AUTOUIC-generated ui_*.h are picked up
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS
   Core
   Gui
   Widgets
   Xml
   PrintSupport
   Core5Compat
)

find_package(OpenSSL REQUIRED)

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
   translations/translations.qrc
)

# Compile .ts -> .qm at configure time using Qt's lrelease.
find_package(Qt6 COMPONENTS LinguistTools QUIET)
if(Qt6LinguistTools_FOUND)
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
   qt_add_lrelease(tuxcards_translations TS_FILES ${TS_FILES}
                   QM_FILES_OUTPUT_VARIABLE qm_files)
   # `cmake --build build --target tuxcards_lupdate` refreshes .ts files
   # from the C++/UI sources, no need for the old `lupdate6 tuxcards.pro`.
   qt_add_lupdate(tuxcards_lupdate
                  SOURCES ${TUXCARDS_SOURCES} ${TUXCARDS_HEADERS} ${TUXCARDS_FORMS}
                  TS_FILES ${TS_FILES}
                  NO_GLOBAL_TARGET)
endif()

qt_add_executable(tuxcards
   ${TUXCARDS_SOURCES}
   ${TUXCARDS_HEADERS}
   ${TUXCARDS_FORMS}
   ${TUXCARDS_RESOURCES}
)

target_link_libraries(tuxcards PRIVATE
   Qt6::Core
   Qt6::Gui
   Qt6::Widgets
   Qt6::Xml
   Qt6::PrintSupport
   Qt6::Core5Compat
   OpenSSL::Crypto
)

install(TARGETS tuxcards RUNTIME DESTINATION bin)
install(FILES packaging/desktop/tuxcards.desktop
        DESTINATION share/applications)
install(FILES src/icons/lo16-app-tuxcards.png
        DESTINATION share/icons/hicolor/16x16/apps RENAME tuxcards.png)
install(FILES src/icons/lo32-app-tuxcards.png
        DESTINATION share/icons/hicolor/32x32/apps RENAME tuxcards.png)
