find_package(Qt6 REQUIRED COMPONENTS Test Gui Qml)

function(quokka_add_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE quokka_core Qt6::Test)
    target_compile_definitions(${name} PRIVATE
        QUOKKA_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

quokka_add_test(tst_parsers tst_parsers.cpp)
quokka_add_test(tst_redditclient tst_redditclient.cpp)

quokka_add_test(tst_comments tst_comments.cpp
    ${PROJECT_SOURCE_DIR}/src/app/commenttreemodel.cpp
    ${PROJECT_SOURCE_DIR}/src/app/postmap.cpp
    ${PROJECT_SOURCE_DIR}/src/app/postlistmodel.cpp
    ${PROJECT_SOURCE_DIR}/tests/test_services.cpp)
target_include_directories(tst_comments PRIVATE ${PROJECT_SOURCE_DIR}/src/app)
target_link_libraries(tst_comments PRIVATE Qt6::Gui Qt6::Qml)

quokka_add_test(tst_format tst_format.cpp ${PROJECT_SOURCE_DIR}/src/app/format.cpp)
target_include_directories(tst_format PRIVATE ${PROJECT_SOURCE_DIR}/src/app)
target_link_libraries(tst_format PRIVATE Qt6::Gui Qt6::Qml)
