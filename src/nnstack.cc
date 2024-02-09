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
        _updDone = false;
}


NNstack::NNstack(const Board &board){
    _curr_size  = 0;
    _nnstack[_curr_size] = NNueEvaluation(board);
    _updDone = false;
}


void NNstack::performUpdate(){
    _updDone = true;
    _nnstack[_curr_size + 1] = _nnstack[_curr_size];

    std::cout << _curr_size  <<std::endl;

    switch (_updSchedule.type)
    {
    case NN_MOVE:
        _nnstack[_curr_size].movePiece(_updSchedule.color, _updSchedule.movingPiece, _updSchedule.from, _updSchedule.to);
        break;
    case NN_PROMO:
        _nnstack[_curr_size].promotePiece(_updSchedule.color, _updSchedule.promotedPiece, _updSchedule.from, _updSchedule.to);
        break;
    case NN_CAPTURE:
        _nnstack[_curr_size].capturePiece(_updSchedule.color, _updSchedule.movingPiece, _updSchedule.capturedPiece, _updSchedule.from, _updSchedule.to);
        break;
    case NN_CAPPROMO:
        _nnstack[_curr_size].cappromPiece(_updSchedule.color, _updSchedule.capturedPiece, _updSchedule.promotedPiece, _updSchedule.from, _updSchedule.to);
        break;
    case NN_CASTLE:
        _nnstack[_curr_size].castleMove(_updSchedule.color, _updSchedule.from, _updSchedule.to, _updSchedule.fromRook, _updSchedule.toRook);
        break;
    case NN_ENPASS:
        _nnstack[_curr_size].enpassMove(_updSchedule.color, _updSchedule.from, _updSchedule.to);
        break;
    default:
        break;
    }

}


void NNstack::scheduleUpdateMove(Color c, PieceType moving, unsigned int from, unsigned int to){
    _updSchedule.type = NN_MOVE;
    _updSchedule.color = c;
    _updSchedule.movingPiece = moving;
    _updSchedule.from = from;
    _updSchedule.to = to;

    _updDone = false;
}

void NNstack::scheduleUpdatePromote(Color c, PieceType promoted, unsigned int from, unsigned int to){
    _updSchedule.type = NN_PROMO;
    _updSchedule.color = c;
    _updSchedule.promotedPiece = promoted;
    _updSchedule.from = from;
    _updSchedule.to = to;

    _updDone = false;
}

void NNstack::scheduleUpdateCapprom(Color c, PieceType captured, PieceType promoted, unsigned int from, unsigned int to){
    _updSchedule.type = NN_CAPPROMO;
    _updSchedule.color = c;
    _updSchedule.capturedPiece = captured;
    _updSchedule.promotedPiece = promoted;
    _updSchedule.from = from;
    _updSchedule.to = to;

    _updDone = false;
}

void NNstack::scheduleUpdateCapture(Color c, PieceType moving, PieceType captured, unsigned int from, unsigned int to){
    _updSchedule.type = NN_CAPTURE;
    _updSchedule.color = c;
    _updSchedule.movingPiece = moving;
    _updSchedule.capturedPiece = captured;
    _updSchedule.from = from;
    _updSchedule.to = to;

    _updDone = false;
}

void NNstack::scheduleUpdateCastle(Color c, unsigned int from, unsigned int to, unsigned int fromR, unsigned int toR){
    _updSchedule.type = NN_CASTLE;
    _updSchedule.color = c;
    _updSchedule.from = from;
    _updSchedule.to = to;
    _updSchedule.fromRook = fromR;
    _updSchedule.toRook = toR;

    _updDone = false;
}

void NNstack::scheduleUpdateEnpass(Color c, unsigned int from, unsigned int to){
    _updSchedule.type = NN_ENPASS;
    _updSchedule.color = c;
    _updSchedule.from = from;
    _updSchedule.to = to;

    _updDone = false;
}

int NNstack::evaluate(Color color){
    return _nnstack[_curr_size].evaluate(color);

}

void NNstack::popOut(){
    if (_updDone){
        _curr_size--;
        _updDone = false;
    }

}