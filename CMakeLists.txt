# Cmake script based off several sources, including cleancodequake2.

# Specify minimum version of CMake required
cmake_minimum_required(VERSION 3.25)

include(FetchContent)

# Project name
project(VortexQuake2)

include(CMakeDependentOption)
option(VRX_ALLOW_ADMIN "Allow usage of admin commands via rcon/sv commands." TRUE)
option(VRX_LOCKDEFAULTS "Disallow changing exp/credits rates via cvar." FALSE)
option(VRX_REMOVERESPAWNS "Remove the usage of the respawns system." TRUE)
option(VRX_USEHASH "Use hashing on commands instead of iterating through the list." TRUE)
option(VRX_Q2PRO "Use map/gamemap differentiation (for q2pro)." TRUE)
option(VRX_OLD_NOLAG "Use old NOLAG style. Not recommended." FALSE)
option(VRX_OLD_VOTE "Use old vote system." FALSE)
option(VRX_REPRO "Support q2repro (remaster, EXPERIMENTAL)" TRUE)
option(BUILD_TESTS "Build test driver" TRUE)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# Source files
file(GLOB_RECURSE VRX_FILES CONFIGURE_DEPENDS ./src/*.c)
file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS ./tests/*.c)


if (NOT VRX_USE_MYSQL)
    list(FILTER VRX_FILES EXCLUDE REGEX ".*/src/characters/io/v_mysql_gds\\.c$")
endif ()

if (NOT VRX_REPRO)
    set(GAME_NAME game)
else ()
    # serraboof comments: game_ is the remaster's convention for game dlls
    set(GAME_NAME game_)
endif ()

# Set the platform-specific library name and use platform-specific files.
if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        if (VRX_REPRO)
            set(CC_LIB_NAME ${GAME_NAME}x64)
        else ()
            set(CC_LIB_NAME ${GAME_NAME}x86_64)
        endif ()
    else ()
        set(CC_LIB_NAME ${GAME_NAME}x86)
    endif ()

    add_definitions(-DWIN32)
    #file (GLOB CC_PLATFORM_FILES source/Platform/windows/*)
elseif (UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-return-mismatch -Wno-incompatible-pointer-types -Wno-implicit-function-declaration")
    # Get machine hardware name (arch), force 386 (if applicable), strip newlines, and store in ARCH
    execute_process(COMMAND uname -m COMMAND sed s/i.86/i386/ COMMAND tr -d \n OUTPUT_VARIABLE ARCH)
    set(CC_LIB_NAME ${GAME_NAME}${ARCH})
    #file (GLOB CC_PLATFORM_FILES source/Platform/unix/*)
    link_libraries(dl m)
else ()
    message(FATAL "Unknown platform")
endif ()

include_directories(./src/)

# Specify the source files for the game library
add_library(${CC_LIB_NAME} SHARED ${VRX_FILES})
set_target_properties(${CC_LIB_NAME} PROPERTIES PREFIX "")

if (BUILD_TESTS)
    add_executable(vrxtestdriver ${VRX_FILES} ${TEST_FILES})
    add_test(NAME vrxtestdriver COMMAND vrxtestdriver)
    target_compile_definitions(vrxtestdriver PRIVATE TEST_RUNNER)
endif()

if (WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${CC_LIB_NAME} PRIVATE
        -Wno-incompatible-function-pointer-types
        -Wno-incompatible-pointer-types
    )
endif ()

if (WIN32)
    set(CC_LINK_LIBS ${CC_LINK_LIBS} Wininet Winmm Ws2_32)

    if (MSVC)
        FetchContent_Declare(
                pthreads
                GIT_REPOSITORY https://github.com/GerHobbelt/pthread-win32
                GIT_TAG master
        )
        FetchContent_MakeAvailable(pthreads)
        target_include_directories(${CC_LIB_NAME} PRIVATE ${pthreads_SOURCE_DIR})
        set(CC_LINK_LIBS ${CC_LINK_LIBS} pthreadVC3)
    else ()
        if (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
            execute_process(
                COMMAND ${CMAKE_C_COMPILER} --print-file-name=libwinpthread.a
                OUTPUT_VARIABLE WINPTHREAD_STATIC
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            if (EXISTS "${WINPTHREAD_STATIC}")
                set(CC_LINK_LIBS ${CC_LINK_LIBS} "${WINPTHREAD_STATIC}")
            else ()
                set(CC_LINK_LIBS ${CC_LINK_LIBS} pthread)
            endif ()
        else ()
            set(CC_LINK_LIBS ${CC_LINK_LIBS} pthread)
        endif ()
    endif ()

    # add /fsanitize=address to the compiler flags
    # if (SANITIZE_ADDRESS)
    #     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
    # endif ()
endif ()

if (UNIX)
    find_package(Threads)
    set(CC_LINK_LIBS ${CC_LINK_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif ()

if (VRX_REMOVERESPAWNS)
    add_definitions(-DREMOVE_RESPAWNS)
endif ()

if (VRX_OLD_NOLAG)
    add_definitions(-DOLD_NOLAG_STYLE)
endif ()

if (VRX_OLD_VOTE)
    add_definitions(-DOLD_VOTE_SYSTEM)
endif ()

if (VRX_USEHASH)
    add_definitions(-DCMD_USEHASH)
endif ()

if (VRX_Q2PRO)
    add_definitions(-DQ2PRO_COMPATIBILITY)
endif ()

if (VRX_ALLOW_ADMIN)
    add_definitions(-DALLOW_ADMIN)
endif ()

if (VRX_LOCKDEFAULTS)
    add_definitions(-DLOCK_DEFAULTS)
endif ()

if (VRX_REPRO)
    add_definitions(-DVRX_REPRO)
endif ()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(MSGPACK_ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(MSGPACK_ENABLE_STATIC ON CACHE BOOL "" FORCE)
set(MSGPACK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MSGPACK_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        MsgPack
        GIT_REPOSITORY https://github.com/msgpack/msgpack-c/
        GIT_TAG c_master
)

FetchContent_MakeAvailable(MsgPack)

set(CC_LINK_LIBS ${CC_LINK_LIBS} msgpack-c)
target_include_directories(${CC_LIB_NAME} PRIVATE
    ${msgpack_SOURCE_DIR}/include
    ${msgpack_BINARY_DIR}/include/msgpack
    ${msgpack_BINARY_DIR}/include
)

target_link_libraries(${CC_LIB_NAME} ${CC_LINK_LIBS})

if (BUILD_TESTS)
    target_include_directories(vrxtestdriver PRIVATE
            ${msgpack_SOURCE_DIR}/include
            ${msgpack_BINARY_DIR}/include/msgpack
            ${msgpack_BINARY_DIR}/include
    )

    target_link_libraries(vrxtestdriver ${CC_LINK_LIBS})
endif()

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    target_compile_definitions(${CC_LIB_NAME} PRIVATE BIG_ENDIAN)
else ()
    target_compile_definitions(${CC_LIB_NAME} PRIVATE LITTLE_ENDIAN)
endif ()

if (BUILD_TESTS)
    target_compile_definitions(vrxtestdriver PRIVATE BIG_ENDIAN)
else ()
    target_compile_definitions(vrxtestdriver PRIVATE LITTLE_ENDIAN)
endif ()