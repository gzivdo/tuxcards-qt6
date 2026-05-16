cmake_minimum_required(VERSION 3.20)
project(tuxcards-tests LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

option(TUXCARDS_BACKEND_OPENSSL    "Link OpenSSL AES-GCM backend in tests"     ON)
option(TUXCARDS_BACKEND_MONOCYPHER "Link monocypher XChaCha backend in tests"  ON)
set(TUXCARDS_WRITE_BACKEND "openssl" CACHE STRING
    "Default crypto backend for writes in test builds (openssl|monocypher).")

find_package(Qt6 REQUIRED COMPONENTS Core Test Core5Compat Xml)
if(TUXCARDS_BACKEND_OPENSSL)
   find_package(OpenSSL REQUIRED)
endif()

include_directories(${CMAKE_SOURCE_DIR}/../src)

# Shared sources from the main project that the tests depend on.
set(CRYPTER_SRC
   ../src/utilities/crypt/StringCrypter.cpp
   ../src/utilities/crypt/BlowFish.cpp
   ../src/utilities/crypt/MD5.cpp
)
if(TUXCARDS_BACKEND_MONOCYPHER)
   list(APPEND CRYPTER_SRC
        ../src/utilities/crypt/monocypher/monocypher.c)
endif()

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
                                                Qt6::Core5Compat)
if(TUXCARDS_BACKEND_OPENSSL)
   target_link_libraries(tst_stringcrypter PRIVATE OpenSSL::Crypto)
   target_compile_definitions(tst_stringcrypter PRIVATE TUXCARDS_BACKEND_OPENSSL)
endif()
if(TUXCARDS_BACKEND_MONOCYPHER)
   target_compile_definitions(tst_stringcrypter PRIVATE TUXCARDS_BACKEND_MONOCYPHER)
endif()
if(TUXCARDS_WRITE_BACKEND STREQUAL "openssl")
   target_compile_definitions(tst_stringcrypter PRIVATE TUXCARDS_WRITE_BACKEND_AESGCM)
elseif(TUXCARDS_WRITE_BACKEND STREQUAL "monocypher")
   target_compile_definitions(tst_stringcrypter PRIVATE TUXCARDS_WRITE_BACKEND_XCHACHA)
endif()
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
