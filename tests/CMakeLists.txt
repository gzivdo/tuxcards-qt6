cmake_minimum_required(VERSION 3.20)
project(tuxcards-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test Core5Compat Xml)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/../src)

# Shared sources from the main project that the tests depend on.
set(CRYPTER_SRC
   ../src/utilities/crypt/StringCrypter.cpp
   ../src/utilities/crypt/BlowFish.cpp
   ../src/utilities/crypt/MD5.cpp
)

set(CONFIG_SRC
   ../src/utilities/iniparser/configparser.cpp
   ../src/utilities/iniparser/configgroup.cpp
)

set(PATH_SRC
   ../src/information/Path.cpp
)

enable_testing()

# ---------- tst_stringcrypter ----------
qt_add_executable(tst_stringcrypter tst_stringcrypter.cpp ${CRYPTER_SRC})
target_link_libraries(tst_stringcrypter PRIVATE Qt6::Core Qt6::Test
                                                Qt6::Core5Compat
                                                OpenSSL::Crypto)
add_test(NAME stringcrypter COMMAND tst_stringcrypter)

# ---------- tst_configparser ----------
qt_add_executable(tst_configparser tst_configparser.cpp ${CONFIG_SRC})
target_link_libraries(tst_configparser PRIVATE Qt6::Core Qt6::Test
                                               Qt6::Core5Compat)
add_test(NAME configparser COMMAND tst_configparser)

# ---------- tst_path ----------
qt_add_executable(tst_path tst_path.cpp ${PATH_SRC})
target_link_libraries(tst_path PRIVATE Qt6::Core Qt6::Test
                                       Qt6::Core5Compat)
add_test(NAME path COMMAND tst_path)
