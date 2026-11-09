find_package(Qt6 REQUIRED COMPONENTS Test Gui Qml Sql)

function(quokka_add_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE quokka_core Qt6::Test)
    target_compile_definitions(${name} PRIVATE
        QUOKKA_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

quokka_add_test(tst_parsers tst_parsers.cpp)
quokka_add_test(tst_apiclient tst_apiclient.cpp)

quokka_add_test(tst_comments tst_comments.cpp
    ${PROJECT_SOURCE_DIR}/src/app/commenttreemodel.cpp
    ${PROJECT_SOURCE_DIR}/src/app/postmap.cpp
    ${PROJECT_SOURCE_DIR}/src/app/postlistmodel.cpp
    ${PROJECT_SOURCE_DIR}/src/app/services.cpp)
target_include_directories(tst_comments PRIVATE ${PROJECT_SOURCE_DIR}/src/app)
target_link_libraries(tst_comments PRIVATE Qt6::Gui Qt6::Qml)

quokka_add_test(tst_library tst_library.cpp
    ${PROJECT_SOURCE_DIR}/src/app/library.cpp
    ${PROJECT_SOURCE_DIR}/src/app/networkfactory.cpp
    ${PROJECT_SOURCE_DIR}/src/app/postmap.cpp
    ${PROJECT_SOURCE_DIR}/src/app/postlistmodel.cpp
    ${PROJECT_SOURCE_DIR}/src/app/services.cpp)
target_include_directories(tst_library PRIVATE ${PROJECT_SOURCE_DIR}/src/app)
target_link_libraries(tst_library PRIVATE Qt6::Gui Qt6::Qml Qt6::Sql)

quokka_add_test(tst_lemmy tst_lemmy.cpp
    ${PROJECT_SOURCE_DIR}/src/app/commenttreemodel.cpp
    ${PROJECT_SOURCE_DIR}/src/app/postmap.cpp
    ${PROJECT_SOURCE_DIR}/src/app/postlistmodel.cpp
    ${PROJECT_SOURCE_DIR}/src/app/services.cpp)
target_include_directories(tst_lemmy PRIVATE ${PROJECT_SOURCE_DIR}/src/app)
target_link_libraries(tst_lemmy PRIVATE Qt6::Gui Qt6::Qml)

quokka_add_test(tst_account tst_account.cpp
    ${PROJECT_SOURCE_DIR}/src/app/account.cpp
    ${PROJECT_SOURCE_DIR}/src/app/apiinfo.cpp
    ${PROJECT_SOURCE_DIR}/src/app/networkfactory.cpp
    ${PROJECT_SOURCE_DIR}/src/app/services.cpp)
target_include_directories(tst_account PRIVATE ${PROJECT_SOURCE_DIR}/src/app)
target_link_libraries(tst_account PRIVATE Qt6::Gui Qt6::Qml)

quokka_add_test(tst_format tst_format.cpp ${PROJECT_SOURCE_DIR}/src/app/format.cpp)
target_include_directories(tst_format PRIVATE ${PROJECT_SOURCE_DIR}/src/app)
target_link_libraries(tst_format PRIVATE Qt6::Gui Qt6::Qml)
