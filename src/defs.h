/*
    Equisetum - UCI compatable chess engine
        Copyright (C) 2017 - 2019  Rhys Rustad-Elliott
                      2020 - 2023  Litov Alexander
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/**
 * @file
 *
 * Contains global constants and functions used in many classes / functions.
 *
 */

#ifndef DEFS_H
#define DEFS_H

#include <limits>
#include <iostream>
#include <cstdlib>

/**
 * An unsigned 64 bit integer (A bitboard).
 */
typedef unsigned long long U64;


#define MIN_HASH    (    8)
#define MAX_HASH    (65536)
#define MIN_THREADS (    1)
#define MAX_THREADS (  256)
#define MAX_PLY     (  127)
#define MAX_INT_PLY (  128)

/**
 * @name Rank bitboards
 *
 * @{
 */
const U64 RANK_1 = 0xffull;
const U64 RANK_2 = 0xff00ull;
const U64 RANK_3 = 0xff0000ull;
const U64 RANK_4 = 0xff000000ull;
const U64 RANK_5 = 0xff00000000ull;
const U64 RANK_6 = 0xff0000000000ull;
const U64 RANK_7 = 0xff000000000000ull;
const U64 RANK_8 = 0xff00000000000000ull;

/**@}*/


/**
 * Global search constants here
 *
*/
const int LOST_SCORE = -30000;
const int NOSCORE = 32666;
const int WON_IN_X = 30000 - MAX_INT_PLY;
const int MAX_GAME_PLY = 2048;

/**
 * @enum SquareIndex
 * @brief Little endian rank file mapping of each square.
 */
enum SquareIndex {
  a1, b1, c1, d1, e1, f1, g1, h1,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a8, b8, c8, d8, e8, f8, g8, h8
};

const int REFLECTED_SQUARE[64] = {
   0,  1,  2,  3,  3,  2,  1,  0,
   4,  5,  6,  7,  7,  6,  5,  4,
   8,  9, 10, 11, 11, 10,  9,  8,
  12, 13, 14, 15, 15, 14, 13, 12,
  16, 17, 18, 19, 19, 18, 17, 16,
  20, 21, 22, 23, 23, 22, 21, 20,
  24, 25, 26, 27, 27, 26, 25, 24,
  28, 29, 30, 31, 31, 30, 29, 28
};

/**
 * @brief An empty bitboard. (ie. the number 0)
 */
const U64 ZERO = U64(0);

/**
 * @brief A bitboard containing only the square a1. (ie. the number 1)
 */
const U64 ONE = U64(1);

/**
 * @name File bitboards
 *
 * @{
 */
const U64 FILE_H = 0x8080808080808080ull;
const U64 FILE_G = 0x4040404040404040ull;
const U64 FILE_F = 0x2020202020202020ull;
const U64 FILE_E = 0x1010101010101010ull;
const U64 FILE_D = 0x808080808080808ull;
const U64 FILE_C = 0x404040404040404ull;
const U64 FILE_B = 0x202020202020202ull;
const U64 FILE_A = 0x101010101010101ull;
/**@}*/

/**
 * @name Black and white squares
 */
const U64 BLACK_SQUARES = 0xAA55AA55AA55AA55;
const U64 WHITE_SQUARES = 0x55AA55AA55AA55AA;
/**@}*/

/**
 * @name Other helpfull bitboards
 */
const U64 CENTER            = (ONE << e4) | (ONE << e5) | (ONE << d4) | (ONE << d5);
const U64 KING_SIDE         = FILE_E | FILE_F | FILE_G | FILE_H;
const U64 QUEEN_SIDE        = FILE_A | FILE_B | FILE_C | FILE_D;
const U64 FIGHTING_AREA     = RANK_3 | RANK_4 | RANK_5 | RANK_6;
const U64 SIDE_FILES        = FILE_A | FILE_H;
const U64 EXT_MIDDLE_FILES  = FILE_C | FILE_D | FILE_E | FILE_F;
const U64 PROMOTION_RANK[2] = {RANK_8, RANK_1};
const U64 DOUBLE_PUSH_RANK[2] = {RANK_4, RANK_5};
const U64 PASSER_ZONE [2] = { (RANK_5 | RANK_6),
                              (RANK_3 | RANK_4) };
const U64 ENEMY_SIDE [2]  = { (RANK_5 | RANK_6 | RANK_7 | RANK_8),
                              (RANK_1 | RANK_2 | RANK_3 | RANK_4) };
const U64 KSIDE_CASTLE [2] = { ((ONE << g1) | (ONE << h1) | (ONE << g2) | (ONE << h2)),
                               ((ONE << g8) | (ONE << h8) | (ONE << g7) | (ONE << h7)) };
const U64 QSIDE_CASTLE [2] = { ((ONE << a1) | (ONE << b1) | (ONE << c1) | (ONE << a2) | (ONE << b2)),
                               ((ONE << a8) | (ONE << b8) | (ONE << c8) | (ONE << a7) | (ONE << b7)) };

const U64 DECENT_BISHOP_PROT_OUTPOST[2] = { (RANK_3 | RANK_4 | RANK_5 | RANK_6 | RANK_7) & ~SIDE_FILES,
                                            (RANK_2 | RANK_3 | RANK_4 | RANK_5 | RANK_6) & ~SIDE_FILES };

const U64 DECENT_KNIGHT_PROT_OUTPOST[2] = {(RANK_4 | RANK_5 | RANK_6),
                                           (RANK_3 | RANK_4 | RANK_5) };

const U64 DECENT_BISHOP_GEN_OUTPOST  = { RANK_3 | RANK_4 | RANK_5 | RANK_6 };

const U64 DECENT_KNIGHT_GEN_OUTPOST[2] = { (((RANK_4 | RANK_5) & EXT_MIDDLE_FILES) | (ONE << b5) | (ONE << g5)),
                                           (((RANK_4 | RANK_5) & EXT_MIDDLE_FILES) | (ONE << b4) | (ONE << g4)) };
/**@}*/

/** @brief Positive infinity to be used during search (eg. as a return value for winning) */
const int INF = std::numeric_limits<int>::max();

/**
 * @enum Color
 * @brief Represents a color.
 */
enum Color {
  WHITE,
  BLACK
};

/**
 * @enum PieceType
 * @brief Represents a piece type.
 */
enum PieceType {
  PAWN,
  ROOK,
  KNIGHT,
  BISHOP,
  QUEEN,
  KING
};

const int DEFAULT_SEARCH_DEPTH = 15;
const int MAX_SEARCH_DEPTH = 64;
const int PHASE_WEIGHT_SUM = 24;
const int MAX_PHASE = 256;

const int PHASE_WEIGHTS[6] = {
    [PAWN] = 0,
    [ROOK] = 2,
    [KNIGHT] = 1,
    [BISHOP] = 1,
    [QUEEN] = 4,
    [KING] = 0
};

    // mapping our pieces onto weigth
    // Initialize index coefficients
const int NNUE_PIECE_TO_INDEX[2][6] = {
        [WHITE] = {
            [PAWN] = 0,
            [ROOK] = 3,
            [KNIGHT] = 1,
            [BISHOP] = 2,
            [QUEEN] = 4,
            [KING] = 5
        },
        [BLACK] = {
            [PAWN] = 6,
            [ROOK] = 9,
            [KNIGHT] = 7,
            [BISHOP] = 8,
            [QUEEN] = 10,
            [KING] = 11
        }
    };


enum UpdateType{
    NN_MOVE,
    NN_PROMO,
    NN_CAPTURE,
    NN_CAPPROMO,
    NN_CASTLE,
    NN_ENPASS
};

enum MpStage{
    MP_TT,
    MP_GENERATE,
    MP_NORMAL
};

struct UpdData{
    UpdateType type;

    Color color;
    PieceType movingPiece;
    PieceType capturedPiece;
    PieceType promotedPiece;

    unsigned int from;
    unsigned int to;

    unsigned int fromRook;
    unsigned int toRook;

    unsigned int wKing;
    unsigned int bKing;
    // init
};

/**
 * @brief Returns the opposite of the given color
 *
 * @param  color Color to get the opposite of
 * @return WHITE if color == BLACK, BLACK otherwise
 */
inline Color getOppositeColor(Color color) {
  return color == WHITE ? BLACK : WHITE;
}

/**
 * @brief Print the given message to stderr and exit with code 1. Should be used in
 * unrecoverable situations.
 *
 * @param msg Message to print to stderr before exiting
 */
[[ noreturn ]]
inline void fatal(std::string msg) {
  std::cerr << msg << std::endl;
  std::exit(1);
}

/**
 * @enum GamePhase
 * @brief Enum representing the game phase (opening/endgame)
 */
enum GamePhase {
  OPENING,
  ENDGAME
};
#endif
