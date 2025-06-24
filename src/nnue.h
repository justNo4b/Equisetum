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
#include <cstdint>

#define NNUE_BUCKETS (15)
#define NNUE_INPUT   (2 * 6 * 64)
#define NNUE_HIDDEN  (1024)
#define NNUE_OUTPUT  (1)

const int NNUE_SCALE = 16 * 512;

const std::string EVAL_FILE = "equi_7b_1024x2F_7Bv8_430.nnue";

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

    // incremental update functions - full updates
    // here we assume that castle requires reset in any case
    void movePiece(UpdData);
    void promotePiece(UpdData);
    void cappromPiece(UpdData);
    void capturePiece(UpdData);

    void enpassMove(UpdData);

    // incremental update - half reset
    void movePieceHalf(UpdData, Color);
    void promotePieceHalf(UpdData, Color);
    void cappromPieceHalf(UpdData, Color);
    void capturePieceHalf(UpdData, Color);
    void castleMoveHalf(UpdData, Color);
    void enpassMoveHalf(UpdData, Color);

    void fullReset(const Board &board);
    void halfReset(const Board &, Color);
    void addSubDifference(const Board &, Color, U64 (*)[2][6]);

    bool resetNeeded(PieceType, int, int, Color);
    int  getCurrentBucket(int, Color);

    int16_t * getHalfAccumulatorPtr(Color);


private:


        int BUCKETS[64] {
            0,  1,  2,  3,  3,  2,  1,  0,
            4,  5,  5,  6,  6,  5,  5,  4,    
            7,  8,  9, 10, 10,  9,  8,  7,
            7,  8,  9, 10, 10,  9,  8,  7,
            7, 14, 13, 12, 12, 13, 14,  7,
           11, 14, 13, 12, 12, 13, 14, 11,
           11, 14, 13, 12, 12, 13, 14, 11,
           11, 11, 11, 11, 11, 11, 11, 11,
        };

    // our main holder of pre-calculated hidden layers for both colors
    // two holders, for white and black perspectives
    int16_t _hiddenScore[2][NNUE_HIDDEN] = {0};

    // Weights etc
    static int16_t NNUE_HIDDEN_BIAS[NNUE_HIDDEN];
    static int16_t NNUE_HIDDEN_WEIGHT[NNUE_INPUT * NNUE_BUCKETS][NNUE_HIDDEN];
    static int16_t NNUE_OUTPUT_WEIGHT[NNUE_HIDDEN];
    static int16_t NNUE_OUTPUT_WEIGHT2[NNUE_HIDDEN];
    static int32_t NNUE_OUTPUT_BIAS[NNUE_OUTPUT];

    inline int _getPieceIndex(int sq, PieceType pt, Color c, Color view, int ksq){
        int r_king = view == WHITE ? ksq : _mir(ksq);
        int r_sq   = view == WHITE ? sq  : _mir(sq);
        int k_index = BUCKETS[r_king];

        if (_col(ksq) > 3){
            r_sq = _horizontal_mir(r_sq);
        }

        int index = r_sq
                  + NNUE_PIECE_TO_INDEX[view == c][pt] * 64
                  + k_index * 64 * 6 * 2;

        return index;
    }



    // Activation function
    inline int relu(int v){
        return std::max(0, v);
    }
};



#endif
