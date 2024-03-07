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
#ifndef NNUE_H
#define NNUE_H

#include "defs.h"
#include "bitutils.h"

#define NNUE_INPUT   (2 * 6 * 64)
#define NNUE_HIDDEN  (1024)
#define NNUE_OUTPUT  (1)

const int NNUE_SCALE = 16 * 512;

const std::string EVAL_FILE = "equi_1024x2_4Bv5_430.nnue";

class Board;


// NNUE update holder.
// Follows architecture set for PSQT updater class
class NNueEvaluation
{
public:
    NNueEvaluation();
    // Set up pre-calculation of hidden layers from board
    NNueEvaluation(const Board &board);

    // This function initializes weigths from evalfile
    static void init();

    // obvious
    int evaluate(const Color);

    // incremental update functions
    void movePiece(Color, PieceType, unsigned int, unsigned int);
    void promotePiece(Color, PieceType, unsigned int, unsigned int);
    void cappromPiece(Color, PieceType, PieceType, unsigned int, unsigned int);
    void capturePiece(Color, PieceType, PieceType, unsigned int, unsigned int);
    void castleMove(Color, unsigned int, unsigned int, unsigned int, unsigned int);
    void enpassMove(Color, unsigned int, unsigned int);

private:

    // our main holder of pre-calculated hidden layers for both colors
    // two holders, for white and black perspectives
    int16_t _hiddenScore[2][NNUE_HIDDEN] = {0};

    // Weights etc
    static int16_t NNUE_HIDDEN_BIAS[NNUE_HIDDEN];
    static int16_t NNUE_HIDDEN_WEIGHT[NNUE_INPUT][NNUE_HIDDEN];
    static int16_t NNUE_OUTPUT_WEIGHT[NNUE_HIDDEN];
    static int16_t NNUE_OUTPUT_WEIGHT2[NNUE_HIDDEN];
    static int32_t NNUE_OUTPUT_BIAS[NNUE_OUTPUT];

    inline int _getPieceIndex(int sq, PieceType pt, Color c, Color view){
        return view == WHITE
                    ?  (sq + NNUE_PIECE_TO_INDEX[view == c][pt] * 64)
                    :  (_mir(sq) + NNUE_PIECE_TO_INDEX[view == c][pt] * 64);
    }

    // Activation function
    inline int relu(int v){
        return std::max(0, v);
    }
};



#endif
