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
#include "movepicker.h"
#include "eval.h"
#include "defs.h"


MovePicker::MovePicker(const OrderingInfo *orderingInfo, const Board *board, int hMove, Color color, int ply, int pMove){
  _orderingInfo = orderingInfo;
  _color = color;
  _ply = ply;
  _pMove = pMove;
  _currHead = 0;
  _board = board;
  _checkHashMove(hMove);
}

void MovePicker::_checkHashMove(int hMoveInt){
    Move m = Move(hMoveInt);
    if (_board->moveIsPseudoLegal(m)){
        _stage = MP_TT;
        m.setValue(INF);
        _hashMove = m;
    }else{
        _hashMove = Move(0);
        _stage = MP_GENERATE;
    }

}

void MovePicker::_scoreMoves() {
  bool isQsearch  = _ply == MAX_PLY;
  _moves = MoveList();
  _moves.reserve(isQsearch ? MOVELIST_RESERVE_SIZE_CAPS : MOVELIST_RESERVE_SIZE);
   MoveGen(_board,isQsearch, &_moves);

  int i = -1;
  int ttIndx = -1;

  int Killer1  = _orderingInfo->getKiller1(_ply);
  int Killer2  = _orderingInfo->getKiller2(_ply);
  int Counter  = _orderingInfo->getCounterMoveINT(_color, _pMove);
  int pMoveInx = (_pMove & 0x7) + ((_pMove >> 15) & 0x3f) * 6;

  for (auto &move : _moves) {
    i++;
    int moveINT = move.getMoveINT();
    if (_hashMove.getMoveINT() != 0 && moveINT == _hashMove.getMoveINT()) {
      move.setValue(INF);
      ttIndx = i;
      // set ttmove first so it can be skipped

    // Sort promotions first so that capture-promotions were here
    } else if (move.getFlags() & Move::PROMOTION) {
        // history
        int value = _orderingInfo->getCaptureHistory(move.getPieceType(),move.getCapturedPieceType(), move.getTo());
        // general values
        value += opS(Eval::MATERIAL_VALUES[move.getPromotionPieceType()])
               - opS(Eval::MATERIAL_VALUES[PAWN]);
        // for SEE+ Q promotions use good capture bonus, otherwise treat as bad captures
        if (move.getPromotionPieceType() == QUEEN && _board->SEE_GreaterOrEqual(move, 0)){
            value += CAPTURE_BONUS;
        }else{
            value += BAD_CAPTURE;
        }
        // for capture-promotions add victim value
        if (move.getFlags() & Move::CAPTURE){
            value += opS(Eval::MATERIAL_VALUES[move.getCapturedPieceType()]);
        }
      move.setValue(value);
    } else if (move.getFlags() & Move::CAPTURE) {
      int hist  = _orderingInfo->getCaptureHistory(move.getPieceType(),move.getCapturedPieceType(), move.getTo());
      int value = opS(Eval::MATERIAL_VALUES[move.getCapturedPieceType()]) + hist;
      int th = isQsearch ? 0 : -((hist / 8192) * 100);
      value +=  _board->SEE_GreaterOrEqual(move, th)  ? CAPTURE_BONUS : BAD_CAPTURE;
      move.setValue(value);
    } else if (moveINT == Killer1) {
      move.setValue(KILLER1_BONUS);
    } else if (moveINT == Killer2) {
      move.setValue(KILLER2_BONUS);
    } else if (moveINT == Counter){
      move.setValue(COUNTERMOVE_BONUS);
    } else { // Quiet
      move.setValue(_orderingInfo->getHistory(_color, move.getFrom(), move.getTo()) +
                    _orderingInfo->getCountermoveHistory(_color, pMoveInx, move.getPieceType(), move.getTo()));
    }
  }
  // swap ttMove first
  if (ttIndx >= 0){
    _currHead++;
    std::swap(_moves.at(0), _moves.at(ttIndx));
  }
}


bool MovePicker::hasNext(){
    if (_stage == MP_TT){
        return true;
    }else if (_stage == MP_GENERATE){
        _stage = MP_NORMAL;
        _scoreMoves();
    }

    return _currHead < _moves.size();
}

Move MovePicker::getNext() {
  size_t bestIndex;
  int bestScore = -INF;

  if (_stage == MP_TT){
    _stage = MP_GENERATE;
    return _hashMove;
  }

  for (size_t i = _currHead; i < _moves.size(); i++) {
    if (_moves.at(i).getValue() > bestScore) {
      bestScore = _moves.at(i).getValue();
      bestIndex = i;
    }
  }

  std::swap(_moves.at(_currHead), _moves.at(bestIndex));
  return _moves.at(_currHead++);
}

void MovePicker::refreshPicker(){
  _currHead = 0;
}

bool MovePicker::moveExists(int moveint){
    for(size_t j = 0; j < _moves.size(); j++){
        if (_moves[j].getMoveINT() == moveint){
            return true;
        }
    }
    return false;
}