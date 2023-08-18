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
 * Contains utility functions for working with bitboards.
 */

#ifndef BITUTILS_H
#define BITUTILS_H

/**
 * @brief Sets the LSB of the given bitboard to 0 and returns its index.
 *
 * @param  board Value to reset LSB of
 * @return Index of reset LSB
 */
inline int _popLsb(U64 &board) {
  int lsbIndex = __builtin_ffsll(board) - 1;
  board &= board - 1;
  return lsbIndex;
}

/**
 * @brief Returns the number of set bits in the given bitboard.
 *
 * @param  board Value to return number of set bits for
 * @return Number of set bits in value
 */
inline int _popCount(U64 board) {
  return __builtin_popcountll(board);
}

/**
 * @brief Returns the index of the LSB in the given bitboard or -1 if
 * the bitboard is empty.
 *
 * @param  board Bitboard to get LSB of
 * @return The index of the LSB in the given bitboard.
 */
inline int _bitscanForward(U64 board) {
  if (board == ZERO) {
    return -1;
  }
  return __builtin_ffsll(board) - 1;
}

/**
 * @brief Returns the index of the MSB in the given bitboard or -1 if
 * the bitboard is empty.
 *
 * @param  board Bitboard to get MSB of
 * @return The index of the MSB in the given bitboard.
 */
inline int _bitscanReverse(U64 board) {
  if (board == ZERO) {
    return -1;
  }
  return 63 - __builtin_clzll(board);
}

/**
* @brief Moves all set bits in the given bitboard n squares east and returns
* the new bitboard, discarding those that fall off the edge.
*
* @param board Board to move bits east on
* @param n Number of squares to move east
* @return A bitboard with all set bits moved one square east, bits falling off the edge discarded
*/
inline U64 _eastN(U64 board, int n) {
  U64 newBoard = board;
  for (int i = 0; i < n; i++) {
    newBoard = ((newBoard << 1) & (~FILE_A));
  }

  return newBoard;
}

/**
 * @brief Moves all set bits in the given bitboard n squares west and returns the new
 * bitboard, discarding those that fall off the edge.
 *
 * @param board Board to move bits west on
 * @param n Number of squares to move west
 * @return A bitboard with all set bits moved one square west, bits falling off the edge discarded
 */
inline U64 _westN(U64 board, int n) {
  U64 newBoard = board;
  for (int i = 0; i < n; i++) {
    newBoard = ((newBoard >> 1) & (~FILE_H));
  }

  return newBoard;
}

/**
 * @brief Returns the zero indexed row of the given square
 *
 * @param square A square in little endian rank file mapping form
 * @return The zero indexed row of the square
 */
inline int _row(int square) {
  return square / 8;
}

/**
 * @brief Returns the zero indexed column of the given square.
 *
 * @param square A square in little endian rank file mapping form
 * @return The zero indexed row of the squares
 */
inline int _col(int square) {
  return square % 8;
}

inline int _edgedist(int square){
    int cw = ((square % 8 < 4) ? (square % 8) : (7 - (square % 8)));
    int rw = ((square / 8 < 4) ? (square / 8) : (7 - (square / 8)));
  return std::min(cw, rw);
}

inline int _relrank(int square, Color color){
    return color == WHITE ? _row(square) : 7 - _row(square);
}

inline int _mir(int square){
  return square ^ 56;
}

inline U64 _sqBB(int square){
    return (ONE << square);
}

#endif
