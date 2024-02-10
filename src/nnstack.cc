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

#include "defs.h"
#include "nnstack.h"
#include "nnue.h"
#include "board.h"

NNstack::NNstack(){
    Board b = Board();
    _curr_size  = 0;
    _nnstack[_curr_size] = NNueEvaluation(b);
    _updDone[_curr_size] = true;
}


NNstack::NNstack(const Board &board){
    _curr_size  = 0;
    _nnstack[_curr_size] = NNueEvaluation(board);
    _updDone[_curr_size] = true;
}


void NNstack::performUpdate(){

    // already updated
    if (_updDone[_curr_size]) return;

    // mark as updated, copy nnue and perform an update
    _updDone[_curr_size] = true;
    _nnstack[_curr_size] = _nnstack[_curr_size - 1];

    switch (_updSchedule[_curr_size].type)
    {
    case NN_MOVE:
        _nnstack[_curr_size].movePiece(_updSchedule[_curr_size].color, _updSchedule[_curr_size].movingPiece, _updSchedule[_curr_size].from, _updSchedule[_curr_size].to);
        break;
    case NN_PROMO:
        _nnstack[_curr_size].promotePiece(_updSchedule[_curr_size].color, _updSchedule[_curr_size].promotedPiece, _updSchedule[_curr_size].from, _updSchedule[_curr_size].to);
        break;
    case NN_CAPTURE:
        _nnstack[_curr_size].capturePiece(_updSchedule[_curr_size].color, _updSchedule[_curr_size].movingPiece, _updSchedule[_curr_size].capturedPiece, _updSchedule[_curr_size].from, _updSchedule[_curr_size].to);
        break;
    case NN_CAPPROMO:
        _nnstack[_curr_size].cappromPiece(_updSchedule[_curr_size].color, _updSchedule[_curr_size].capturedPiece, _updSchedule[_curr_size].promotedPiece, _updSchedule[_curr_size].from, _updSchedule[_curr_size].to);
        break;
    case NN_CASTLE:
        _nnstack[_curr_size].castleMove(_updSchedule[_curr_size].color, _updSchedule[_curr_size].from, _updSchedule[_curr_size].to, _updSchedule[_curr_size].fromRook, _updSchedule[_curr_size].toRook);
        break;
    case NN_ENPASS:
        _nnstack[_curr_size].enpassMove(_updSchedule[_curr_size].color, _updSchedule[_curr_size].from, _updSchedule[_curr_size].to);
        break;
    default:
        break;
    }

}


void NNstack::scheduleUpdateMove(Color c, PieceType moving, unsigned int from, unsigned int to){
    _curr_size++;

    _updSchedule[_curr_size].type = NN_MOVE;
    _updSchedule[_curr_size].color = c;
    _updSchedule[_curr_size].movingPiece = moving;
    _updSchedule[_curr_size].from = from;
    _updSchedule[_curr_size].to = to;

    _updDone[_curr_size] = false;
}

void NNstack::scheduleUpdatePromote(Color c, PieceType promoted, unsigned int from, unsigned int to){
    _curr_size++;

    _updSchedule[_curr_size].type = NN_PROMO;
    _updSchedule[_curr_size].color = c;
    _updSchedule[_curr_size].promotedPiece = promoted;
    _updSchedule[_curr_size].from = from;
    _updSchedule[_curr_size].to = to;

    _updDone[_curr_size] = false;
}

void NNstack::scheduleUpdateCapprom(Color c, PieceType captured, PieceType promoted, unsigned int from, unsigned int to){
    _curr_size++;

    _updSchedule[_curr_size].type = NN_CAPPROMO;
    _updSchedule[_curr_size].color = c;
    _updSchedule[_curr_size].capturedPiece = captured;
    _updSchedule[_curr_size].promotedPiece = promoted;
    _updSchedule[_curr_size].from = from;
    _updSchedule[_curr_size].to = to;

    _updDone[_curr_size] = false;
}

void NNstack::scheduleUpdateCapture(Color c, PieceType moving, PieceType captured, unsigned int from, unsigned int to){
    _curr_size++;

    _updSchedule[_curr_size].type = NN_CAPTURE;
    _updSchedule[_curr_size].color = c;
    _updSchedule[_curr_size].movingPiece = moving;
    _updSchedule[_curr_size].capturedPiece = captured;
    _updSchedule[_curr_size].from = from;
    _updSchedule[_curr_size].to = to;

    _updDone[_curr_size] = false;
}

void NNstack::scheduleUpdateCastle(Color c, unsigned int from, unsigned int to, unsigned int fromR, unsigned int toR){
    _curr_size++;

    _updSchedule[_curr_size].type = NN_CASTLE;
    _updSchedule[_curr_size].color = c;
    _updSchedule[_curr_size].from = from;
    _updSchedule[_curr_size].to = to;
    _updSchedule[_curr_size].fromRook = fromR;
    _updSchedule[_curr_size].toRook = toR;

    _updDone[_curr_size] = false;
}

void NNstack::scheduleUpdateEnpass(Color c, unsigned int from, unsigned int to){
    _curr_size++;

    _updSchedule[_curr_size].type = NN_ENPASS;
    _updSchedule[_curr_size].color = c;
    _updSchedule[_curr_size].from = from;
    _updSchedule[_curr_size].to = to;

    _updDone[_curr_size] = false;
}

void NNstack::scheduleUpdateEmpty(){
    _curr_size++;
    _updDone[_curr_size] = false;
}

int NNstack::evaluate(Color color){
    return _nnstack[_curr_size].evaluate(color);
}

bool NNstack::popOut(){
    _curr_size--;
    return true;
}

int NNstack::getSize(){
    return _curr_size;
}