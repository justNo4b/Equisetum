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
  _goodCapCount = 0;
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
        _stage = MP_GENERATE_CAPTURES;
    }

}


void MovePicker::_scoreCaptures() {
    _moves.reserve(MOVELIST_RESERVE_SIZE);
     MoveGen(_board, true, &_moves);

    int i = -1;
    int ttIndx = -1;

    for (auto &move : _moves) {
      i++;
      int moveINT = move.getMoveINT();
      if (_hashMove.getMoveINT() != 0 && moveINT == _hashMove.getMoveINT()) {
        move.setValue(INF);
        ttIndx = i;
        // set ttmove first so it can be skipped
        _goodCapCount++;
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
              _goodCapCount++;
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
        int th = -((hist / 8192) * 100);
        if (_board->SEE_GreaterOrEqual(move, th)){
            value += CAPTURE_BONUS;
            _goodCapCount++;
        }else{
            value += BAD_CAPTURE;
        }
        move.setValue(value);
      }
    }
    // swap ttMove first
    if (ttIndx >= 0){
      _currHead++;
      std::swap(_moves.at(0), _moves.at(ttIndx));
    }
  }

void MovePicker::_scoreQuiets() {
   MoveGen(_board, false, &_moves);

  int i = -1;
  int ttIndx = -1;
  int k1Indx = -1;
  int k2Indx = -1;
  int coIndx = -1;

  int Killer1  = _orderingInfo->getKiller1(_ply);
  int Killer2  = _orderingInfo->getKiller2(_ply);
  int Counter  = _orderingInfo->getCounterMoveINT(_color, _pMove);
  int pMoveInx = (_pMove & 0x7) + ((_pMove >> 15) & 0x3f) * 6;

  for (auto &move : _moves) {
    i++;
    if (move.getValue() != 0){
        continue;
    }
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
    } else if (moveINT == Killer1) {
      k1Indx = i;
      move.setValue(KILLER1_BONUS);
    } else if (moveINT == Killer2) {
      k2Indx = i;
      move.setValue(KILLER2_BONUS);
    } else if (moveINT == Counter){
        coIndx = i;
      move.setValue(COUNTERMOVE_BONUS);
    } else { // Quiet
      move.setValue(_orderingInfo->getHistory(_color, move.getFrom(), move.getTo()) +
                    _orderingInfo->getCountermoveHistory(_color, pMoveInx, move.getPieceType(), move.getTo()));
    }
  }
  // swap ttMove first
  if (ttIndx >= 0){
    std::swap(_moves.at(_currHead), _moves.at(ttIndx));
    _currHead++;
  }

  if (k1Indx >= 0){
    std::swap(_moves.at(_currHead), _moves.at(k1Indx));
    _currHead++;
  }

  if (k2Indx >= 0){
    std::swap(_moves.at(_currHead), _moves.at(k2Indx));
    _currHead++;
  }

  if (coIndx >= 0){
        std::swap(_moves.at(_currHead), _moves.at(coIndx));
    _currHead++;
  }
}


bool MovePicker::hasNext(){
    if (_stage == MP_TT){
        return true;
    }

    if (_stage == MP_GENERATE_CAPTURES){
        _stage = MP_CAPTURES;
        _scoreCaptures();
    }

    if (_stage == MP_CAPTURES){
        if (_currHead < _goodCapCount && _currHead < _moves.size()){
            return true;
        }else if (_ply != MAX_PLY){
            _stage = MP_KILLER1;
        }else{
            return false;
        }
    }

    if (_stage == MP_KILLER1){
        _killer1 = Move(_orderingInfo->getKiller1(_ply));
        if (_board->moveIsPseudoLegal(_killer1)){
            return true;
        }else{
            _stage = MP_KILLER2;
        }
    }

    if (_stage == MP_KILLER2){
        _killer2 = Move(_orderingInfo->getKiller2(_ply));
        if (_board->moveIsPseudoLegal(_killer2)){
            return true;
        }else{
            _stage = MP_COUNTER;
        }
    }

    if (_stage == MP_COUNTER){
        _counter = Move(_orderingInfo->getCounterMoveINT(_color, _pMove));
        if (_board->moveIsPseudoLegal(_counter)){
            return true;
        }else{
            _stage = MP_GENERATE_QUIET;
        }
    }

    if (_stage == MP_GENERATE_QUIET){
        _stage = MP_QUIETS;
        _scoreQuiets();
    }

    return _currHead < _moves.size();
}

Move MovePicker::getNext() {
  size_t bestIndex;
  int bestScore = -INF;

  if (_stage == MP_TT){
    _stage = MP_GENERATE_CAPTURES;
    return _hashMove;
  }

  if (_stage == MP_KILLER1){
    _stage = MP_KILLER2;
    _killer1.setValue(KILLER1_BONUS);
    return _killer1;
  }

  if (_stage == MP_KILLER2){
    _stage = MP_COUNTER;
    _killer2.setValue(KILLER2_BONUS);
    return _killer2;
  }

  if (_stage == MP_COUNTER){
    _stage = MP_GENERATE_QUIET;
    _counter.setValue(COUNTERMOVE_BONUS);
    return _counter;
  }

  // else quiets
  for (size_t i = _currHead; i < _moves.size(); i++) {
    if (_moves.at(i).getValue() > bestScore) {
      bestScore = _moves.at(i).getValue();
      bestIndex = i;
    }
  }

  std::swap(_moves.at(_currHead), _moves.at(bestIndex));
  return _moves.at(_currHead++);
}

MpStage MovePicker::getStage(){
    return _stage;
}