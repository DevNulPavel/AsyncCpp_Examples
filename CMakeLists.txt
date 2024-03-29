set(PROJECT_NAME "AsyncCPP_Examples")
set(EXECUTABLE_NAME "AsyncCPP_Examples")

# Пути к исходникам
set(SRC_FOLDERS
    src
)

# Конфиги используемых библиотек
set(LIBS_FOLDER 
    "libs/"
)
set(LIBS_FOLDERS
    "asyncplusplus"
    "cpp-channel"
)
set(LIBS_LINK_NAMES
    "Async++"
    "cpp-channel"
)
set(LIBS_DEPENCY_NAMES
    "Async++"
    "cpp-channel"
)
set(LIBS_INCLUDE_FOLDERS
    "asyncplusplus/include"
    "concurrentqueue"
    "cpp-channel/include"
)


####################################################
# Настройка CMake сборки
####################################################
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)
if (NOT CMAKE_VERSION VERSION_LESS "3.1")
    cmake_policy(SET CMP0054 NEW)
endif()

# цвета
option(CMAKE_COLOR_MAKEFILE "Color makefile" ON)

# Инклюдим файлик с разными хелперами для сборки
include("CMake/Helpers.cmake")

# Если у нас есть компилятор clang на юникс - используем его
if (UNIX)
    message("********* Unix compilation check *********")
     # Используем ccache (если есть) для ускорения сборки
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        message("-> ССache will used for compilation")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif(CCACHE_FOUND)

    # Вариант с путями
    # execute_process(COMMAND which clang OUTPUT_VARIABLE CLANG_PATH)
    # execute_process(COMMAND which clang++ OUTPUT_VARIABLE CLANGXX_PATH)
    # if(NOT ("${CLANG_PATH}" STREQUAL ""))
    find_program(CLANG_FOUND clang)
    if(CLANG_FOUND)
        message("-> Clang will used for compilation")
        set(CMAKE_C_COMPILER "clang")
        set(CMAKE_CXX_COMPILER "clang++")
        # Скрываем ворнинги на clang + цвета
        set(CUSTOM_COMPILER_C_FLAGS "-Wno-error=unused-command-line-argument -Qunused-arguments -fcolor-diagnostics")
        set(CUSTOM_COMPILER_CXX_FLAGS "-Wno-error=unused-command-line-argument -Qunused-arguments -fcolor-diagnostics")
    endif(CLANG_FOUND)
    message("**********************************")
endif()

# Инициализация проекта
project(${PROJECT_NAME} C CXX)

enable_testing()



####################################################
# Библиотеки в системе
####################################################
message("******** System libs search ***********")



####################################################
# Библиотеки собираемые из исходников
####################################################

# Запускаем конфигурацию подпроектов
foreach(LIBRARY_DIR ${LIBS_FOLDERS})
    message("******** Subproject start ***********")
    message("Project with path = ${LIBS_FOLDER}${LIBRARY_DIR}")
    # Кастомные флаги сборки библиотек
    set(CMAKE_C_FLAGS "${CUSTOM_COMPILER_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CUSTOM_COMPILER_CXX_FLAGS}")
    add_subdirectory(${LIBS_FOLDER}${LIBRARY_DIR} ${CMAKE_BINARY_DIR}/libs/${LIBRARY_DIR})
    # message("LIBS = ${${LIBRARY_DIR}_LIBRARIES}")
    # message("includes = ${GLFW_INCLUDE_DIRS}")
    # message("libs = ${GLFW_PKG_DEPS}")
    # message("pkg = ${GLFW_PKG_LIBS}")
endforeach()



####################################################
# Обрабатываем инклюды
####################################################
foreach(LIBRARY_INCLUDE_DIR ${LIBS_INCLUDE_FOLDERS})
    include_directories(${LIBS_FOLDER}${LIBRARY_INCLUDE_DIR})
endforeach()



####################################################
# Системные библиотеки
####################################################
if (APPLE)
    set (SYSTEM_LIBS "pthread" "m" "dl" "c")
endif ()
if (UNIX AND NOT WIN32 AND NOT APPLE)
    set (SYSTEM_LIBS "pthread" "m" "dl" "c")
endif()
if (WIN32)
    set (SYSTEM_LIBS "pthread" "m" "dl" "c")
endif ()
if (MSYS)
    set (SYSTEM_LIBS )
endif ()
message("******** System libs list ***********")
message("${SYSTEM_LIBS}")



####################################################
# Настройка сборки текущего проекта
####################################################
# Сразу отключаем ненужные предупреждения на Windows
if (MSYS)
    add_definitions ( -D_CRT_SECURE_NO_WARNINGS )
    add_definitions ( -D_SCL_SECURE_NO_WARNINGS )
endif ()

# Не кастомные флаги компиляции
if(APPLE)
    # TODO: пробросить линковку
    set (PROJECT_LINK_FLAGS "-stdlib=libc++")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -std=c11 -O0 -Wall -g")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -std=c11 -O2 -Wall")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -Wall -g -stdlib=libc++")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -Wall -stdlib=libc++")
endif ()
if (UNIX)
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} ${CUSTOM_COMPILER_C_FLAGS} -std=c11 -O0 -Wall -g")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} ${CUSTOM_COMPILER_C_FLAGS} -std=c11 -O2 -Wall")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CUSTOM_COMPILER_CXX_FLAGS} -std=c++11 -O0 -Wall -g")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CUSTOM_COMPILER_CXX_FLAGS} -std=c++11 -O2 -Wall")
endif ()
if (MSYS)
    # микрософту не нужен флаг -std=c++11?
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /std:c11 /O0 /g /Wall")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /std:c11 /O2 /Wall")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /std:c++11 /O0 /g /Wall")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /std:c++11 /O2 /Wall")
endif ()
if (WIN32)
    # микрософту не нужен флаг -std=c++11?
    set (PROJECT_LINK_FLAGS "-stdlib=libc++")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /std:c11 -O0 -Wall -g")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /std:c11 -O2 -Wall -g")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /std:c++11 -O0 -Wall -g -std=c++11")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /std:c++11 -O0 -Wall -g -std=c++11")
endif ()

# Вывод информации о настройке копиляции (Функция в CMake/Helpers.cmake)
print_current_cmake_config()



####################################################
# Исходники
####################################################
# Запускаем поиск иходников
foreach(DIR ${SRC_FOLDERS})
    # собираем список файлов в папке (не рекурсивно, не GLOB_RECURSE !)
    # можно еще обобщить, задавая список нужных расширений
    file(GLOB HEADERS "${DIR}/*.h")
    file(GLOB SOURCES "${DIR}/*.cpp" "${DIR}/*.c" "${DIR}/*.cc")
    # Формирование групп исходников для красивого отображения в проекте
    # для имен групп в xcode нужен обратный слэш
    string(REGEX REPLACE "/" "\\\\" GROUP_NAME ${DIR})
    # source_group("Sources\\${GROUP_NAME}" FILES ${HEADERS} ${SOURCES})
    source_group("${GROUP_NAME}" FILES ${HEADERS} ${SOURCES})
    # сцепляем пути
    list(APPEND ALL_HEADERS ${HEADERS})
    list(APPEND ALL_SOURCES ${SOURCES})
endforeach(DIR)
message("******** Sources list ***********")
message("${ALL_HEADERS}")
message("${ALL_SOURCES}")

# тип сборки
if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE Debug CACHE STRING "Debug" FORCE )
endif()



####################################################
# Генерация
####################################################
message("******** Generating started ***********")
# Из найденных исходников выставляем генерацию бинарника
add_executable(${EXECUTABLE_NAME} ${ALL_HEADERS} ${ALL_SOURCES})

# Конкретные пути к бинарнику
set_target_properties( ${EXECUTABLE_NAME}
    PROPERTIES
    # ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib"
    # LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib"
    # RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
)
# set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
# set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

# Создаем символьную ссылку на ресурсы (Для исполняемго файла, после выбора)
if (WIN32)
    string(REGEX REPLACE "/" "\\\\" SOURCE_RES_PATH "${CMAKE_SOURCE_DIR}/res")
    string(REGEX REPLACE "/" "\\\\" RESULT_RES_PATH "${CMAKE_BINARY_DIR}/res/")
    message("COPY RES COMMAND = xcopy ${SOURCE_RES_PATH} ${RESULT_RES_PATH} /y")
    add_custom_command (
        TARGET ${EXECUTABLE_NAME}
        POST_BUILD
        COMMAND xcopy ${SOURCE_RES_PATH} ${RESULT_RES_PATH} /y
    )
else()
    add_custom_command (
        TARGET ${EXECUTABLE_NAME}
        POST_BUILD
        COMMAND rm -f ${CMAKE_BINARY_DIR}/res
        COMMAND ln -s ../../res ${CMAKE_BINARY_DIR}/res
    )
endif()

# Зависимости сборки (в данном проекте нету)
add_dependencies(${EXECUTABLE_NAME} ${LIBS_DEPENCY_NAMES})

# Библиотеки, с которыми будем ликовать
# link_directories(${LIBS_LINK_FOLDERS})
target_link_libraries(${EXECUTABLE_NAME} ${LIBS_LINK_NAMES} ${SYSTEM_LIBS})
