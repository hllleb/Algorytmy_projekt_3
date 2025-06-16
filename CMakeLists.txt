cmake_minimum_required(VERSION 3.11)

project(projekt3)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra)

if (WIN32)
    set( SFML_STATIC_LIBRARIES TRUE )

    set ( SFML_DIR "C:/Users/glebl/SFML-2.5.1/lib/cmake/SFML/")
endif(WIN32)


find_package(SFML 2 COMPONENTS graphics audio REQUIRED)

add_executable(projekt3 src/main.cpp
        src/Move.h
        src/PieceType.h
        src/Color.h
        src/Piece.h
        src/Logger.h
        src/ChessGame.h
        src/GameState.h
        src/TranspositionEntry.h
)
target_link_libraries(projekt3 sfml-graphics sfml-audio)

