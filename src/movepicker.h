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
#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include "defs.h"
#include "board.h"
#include "orderinginfo.h"

/**
 * @brief Abstract base class for an object that picks moves from a move list in an optimal order.
 *
 * Implementations must define hasNext(), which returns true if there are remaining moves
 * to be processed and getNext(), which gets the next move from the provided MoveList.
 *
 * Note that instances of a MovePicker modify their provided MoveList instances in place.
 */
class MovePicker {
 public:
  /**
   * @brief Constructs a new GeneralMovePicker for the given OrderingInfo, Board and MoveList.
   *
   * @param orderingInfo OrderingInfo object containing information about the current state of the search
   * @param board Current board state for all moves in the provided MoveList
   * @param moveList Pointer to the MoveList to pick moves from
   */
  MovePicker(const OrderingInfo *, const Board *, int, Color, int, int, int);

  /**
   * @brief Returns the next best move from this MovePicker's MoveList.
   *
   * @return Move the next best move from this MovePicker's MoveList
   */
  Move getNext();

  /**
   * @brief Returns true if there are more moves to be picked from this MovePicker's MoveList
   *
   * @return true if there are more moves to be picked from this MovePicker's MoveList, false otherwise.
   */
  bool hasNext();

  /**
   * @brief Refresh movepicker to the first entry to start picking moves from the beginning
   *
   * Used in the probcut implementation to avoid double-move-generation
   */
  void refreshPicker();

  bool moveExists(int);
   private:
  /**
   * @brief List of moves this MovePicker picks from
   */
  MoveList _moves;

  /**
   * @brief Size of _moves to pre-reserve before generating moves.
   *
   * At the current time 218 seems to be an upper bound on the maximum number
   * of moves from any one position.
   */
  static const int MOVELIST_RESERVE_SIZE = 64;
  static const int MOVELIST_RESERVE_SIZE_CAPS = 24;

  const Board * _board;

  /**
   * @brief Bonuses applied to specific move types.
   * @{
   */
  static const int CAPTURE_BONUS      = 400000;
  static const int KILLER1_BONUS      = 200000;
  static const int KILLER2_BONUS      = 150000;
  static const int COUNTERMOVE_BONUS  = 50000;
  static const int QUIET_BONUS        = 0;
  static const int BAD_CAPTURE        =-16384;
  /**@}*/

  /**
   * @brief Assigns a value to each move in this GeneralMovePicker's MoveList representing desirability
   * in a negamax search.
   */
  void _scoreMoves();

  void _checkHashMove(int);

  MpStage _stage;

  /**
   * @brief Position of the first unpicked move in this GeneralMovePicker's MoveList
   */
  size_t _currHead;

  /**
   * @brief OrderingInfo object containing search related information used by this GeneralMovePicker
   */
  const OrderingInfo *_orderingInfo;

  /**
   * @brief posKey of the current position
   */
  Move _hashMove;

  /**
   * @brief Color of the current sideToMove
   */
  Color _color;

  /**
   * @brief Current ply of the search
   */
  int _ply;

  /**
   * @brief Previous move made during the search
   * ie one that lead to the position we are currently in.
   *
   * Used to determine CounterMoves
   */
  int _pMove;
  int _ppMove;
};

#endif