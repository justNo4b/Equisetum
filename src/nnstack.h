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

#ifndef NNSTACK_H
#define NNSTACK_H

#include "defs.h"
#include "nnue.h"


enum UpdateType{
    NN_MOVE,
    NN_PROMO,
    NN_CAPTURE,
    NN_CAPPROMO,
    NN_CASTLE,
    NN_ENPASS
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



    // init
};

class NNstack
{

private:

    bool           _updDone [MAX_INT_PLY * 2];
    NNueEvaluation _nnstack [MAX_INT_PLY * 2];
    UpdData        _updSchedule [MAX_INT_PLY * 2];
    int            _curr_size = 0;





    /* data */
public:

    NNstack();
    NNstack(const Board &board);

    void performUpdate();
    void scheduleUpdateMove(Color, PieceType, unsigned int, unsigned int);
    void scheduleUpdatePromote(Color, PieceType, unsigned int, unsigned int);
    void scheduleUpdateCapprom(Color, PieceType, PieceType, unsigned int, unsigned int);
    void scheduleUpdateCapture(Color, PieceType, PieceType, unsigned int, unsigned int);
    void scheduleUpdateCastle(Color, unsigned int, unsigned int, unsigned int, unsigned int);
    void scheduleUpdateEnpass(Color, unsigned int, unsigned int);
    void scheduleUpdateEmpty();

    bool popOut();

    int getSize();

    int evaluate(Color);



};





#endif