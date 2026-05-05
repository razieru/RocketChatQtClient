cmake_minimum_required(VERSION 3.21)

project(RocketChatQtClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Qml Quick)
option(RC_ENABLE_QTKEYCHAIN "Enable secure session storage via qtkeychain" ON)

if(RC_ENABLE_QTKEYCHAIN)
    find_package(Qt6Keychain QUIET)
    if(TARGET Qt6Keychain::Qt6Keychain)
        set(RC_QTKEYCHAIN_TARGET Qt6Keychain::Qt6Keychain)
        set(RC_HAS_QTKEYCHAIN TRUE)
    elseif(TARGET qt6keychain)
        set(RC_QTKEYCHAIN_TARGET qt6keychain)
        set(RC_HAS_QTKEYCHAIN TRUE)
    else()
        message(STATUS
            "qtkeychain was not found. Secure storage is disabled, "
            "falling back to QSettings session storage.")
    endif()
else()
    message(STATUS "RC_ENABLE_QTKEYCHAIN=OFF: using QSettings session storage.")
endif()

add_executable(rocketchat_qt_client
    src/main.cpp
    src/network/apitypes.h
    src/network/httptransport.h
    src/network/httptransport.cpp
    src/network/rocketchatclient.h
    src/network/rocketchatclient.cpp
    src/storage/isessionstore.h
    src/storage/keychainsessionstore.h
    src/storage/keychainsessionstore.cpp
    src/storage/sessionstorefactory.h
    src/storage/sessionstorefactory.cpp
    src/storage/settingssessionstore.h
    src/storage/settingssessionstore.cpp
    src/ui/authviewmodel.h
    src/ui/authviewmodel.cpp
    src/ui/chatlistmodel.h
    src/ui/chatlistmodel.cpp
    src/ui/messagelistmodel.h
    src/ui/messagelistmodel.cpp
)

target_link_libraries(rocketchat_qt_client PRIVATE Qt6::Core Qt6::Network Qt6::Qml Qt6::Quick)

qt_add_qml_module(rocketchat_qt_client
    URI RocketChat.Client
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/LoginForm.qml
        qml/components/ChatListItemDelegate.qml
        qml/components/MessageListItemDelegate.qml
        qml/components/ChatListPanel.qml
        qml/components/MessageListPanel.qml
)

if(RC_HAS_QTKEYCHAIN)
    target_link_libraries(rocketchat_qt_client PRIVATE ${RC_QTKEYCHAIN_TARGET})
    target_compile_definitions(rocketchat_qt_client PRIVATE RC_HAS_QTKEYCHAIN=1)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(rocketchat_qt_client_tests
        tests/sessionstore_tests.cpp
        src/storage/isessionstore.h
        src/storage/keychainsessionstore.h
        src/storage/keychainsessionstore.cpp
        src/storage/sessionstorefactory.h
        src/storage/sessionstorefactory.cpp
        src/storage/settingssessionstore.h
        src/storage/settingssessionstore.cpp
        src/network/apitypes.h
    )
    target_link_libraries(rocketchat_qt_client_tests PRIVATE Qt6::Core Qt6::Test)
    if(RC_HAS_QTKEYCHAIN)
        target_link_libraries(rocketchat_qt_client_tests PRIVATE ${RC_QTKEYCHAIN_TARGET})
        target_compile_definitions(rocketchat_qt_client_tests PRIVATE RC_HAS_QTKEYCHAIN=1)
    endif()
    add_test(NAME rocketchat_qt_client_tests COMMAND rocketchat_qt_client_tests)

    add_executable(rocketchat_qt_client_twofa_tests
        tests/twofa_tests.cpp
        src/network/apitypes.h
        src/network/httptransport.h
        src/network/httptransport.cpp
        src/network/rocketchatclient.h
        src/network/rocketchatclient.cpp
        src/storage/isessionstore.h
        src/storage/settingssessionstore.h
        src/storage/settingssessionstore.cpp
    )
    target_link_libraries(rocketchat_qt_client_twofa_tests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
    add_test(NAME rocketchat_qt_client_twofa_tests COMMAND rocketchat_qt_client_twofa_tests)
endif()
