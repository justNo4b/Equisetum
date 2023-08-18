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
#include "nnue.h"
#include "board.h"
#include "bitutils.h"
#include <fstream>
#include <fstream>


    int16_t NNueEvaluation::NNUE_HIDDEN_BIAS[NNUE_HIDDEN];
    int16_t NNueEvaluation::NNUE_HIDDEN_WEIGHT[NNUE_INPUT][NNUE_HIDDEN];
    int16_t NNueEvaluation::NNUE_OUTPUT_WEIGHT[NNUE_HIDDEN];
    int16_t NNueEvaluation::NNUE_OUTPUT_WEIGHT2[NNUE_HIDDEN];
    int32_t NNueEvaluation::NNUE_OUTPUT_BIAS[NNUE_OUTPUT];


void NNueEvaluation::init(){

    std::ifstream file(EVAL_FILE, std::ios::binary | std::ios::in);

    // Exit the program if file is not found or cannot be opened
    if (!file){
        std::cout << "Failed to open NNUE file. Exit"<< std::endl;
        exit(0);
    }
    // File exist, proceed

    for (int i = 0; i < NNUE_INPUT; i++){
        for (int h = 0; h < NNUE_HIDDEN; h++){
            int16_t tmp_int = 0;
            file.read(reinterpret_cast<char *>(&tmp_int), sizeof(tmp_int));
            NNUE_HIDDEN_WEIGHT[i][h] = tmp_int;
        }
    }

    for (int h = 0; h < NNUE_HIDDEN; h++){
        int16_t tmp_int = 0;
        file.read(reinterpret_cast<char *>(&tmp_int), sizeof(tmp_int));
        NNUE_HIDDEN_BIAS[h] = tmp_int;
    }

    for (int h = 0; h < NNUE_HIDDEN; h++){
        int16_t tmp_int = 0;
        file.read(reinterpret_cast<char *>(&tmp_int), sizeof(tmp_int));
        NNUE_OUTPUT_WEIGHT[h] = tmp_int;
    }

    for (int h = 0; h < NNUE_HIDDEN; h++){
        int16_t tmp_int = 0;
        file.read(reinterpret_cast<char *>(&tmp_int), sizeof(tmp_int));
        NNUE_OUTPUT_WEIGHT2[h] = tmp_int;
    }

    // Read final bias
    int32_t tmp_int = 0;
    file.read(reinterpret_cast<char *>(&tmp_int), sizeof(tmp_int));
    NNUE_OUTPUT_BIAS[0] = tmp_int;

}

NNueEvaluation::NNueEvaluation() = default;

NNueEvaluation::NNueEvaluation(const Board &board) {

    // Load a net given board
    // Pre-calculate hidden layer scores for further incremental updates

    // Load biases
    // Here we simply set values
    for (auto color : {WHITE, BLACK}){
        for (int k = 0; k < NNUE_HIDDEN; k++){
            _hiddenScore[color][k] =  NNUE_HIDDEN_BIAS[k];
        }
    }


    // Load pieces
    // Here we do ++
    for (auto pt : {PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING}){
        for (auto color : {WHITE, BLACK}){
            U64 tmpBB = board.getPieces(color, pt);
            while (tmpBB){
                int sq = _popLsb(tmpBB);
                for (int i = 0; i < NNUE_HIDDEN; i++){
                    _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[_getPieceIndex(sq, pt, color, WHITE)][i];
                    _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[_getPieceIndex(sq, pt, color, BLACK)][i];
                }
            }
        }
    }

}

int NNueEvaluation::evaluate(const Color color){
    int32_t s = NNUE_OUTPUT_BIAS[0];
    Color oppColor = getOppositeColor(color);

    // We expect hidden layer to be up-to-date, simply calculate rest of NN
    for (int k = 0; k < NNUE_HIDDEN; k++){
        // apply relu and multyply by weight
        s += relu(_hiddenScore[color][k]) * NNUE_OUTPUT_WEIGHT[k];
    }
    for (int k = 0; k < NNUE_HIDDEN; k++){
        // apply relu and multyply by weight
        s += relu(_hiddenScore[oppColor][k]) * NNUE_OUTPUT_WEIGHT2[k];
    }

    s = s / NNUE_SCALE;

    return s;
}


void NNueEvaluation::addPiece(Color color, PieceType pieceType, unsigned int square){
    int indexWV = _getPieceIndex(square, pieceType, color, WHITE);
    int indexBV = _getPieceIndex(square, pieceType, color, BLACK);
    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[indexBV][i];
    }
}

void NNueEvaluation::removePiece(Color color, PieceType pieceType, unsigned int square){
    int indexWV = _getPieceIndex(square, pieceType, color, WHITE);
    int indexBV = _getPieceIndex(square, pieceType, color, BLACK);
    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] -= NNUE_HIDDEN_WEIGHT[indexWV][i];
        _hiddenScore[BLACK][i] -= NNUE_HIDDEN_WEIGHT[indexBV][i];
    }
}

void NNueEvaluation::movePiece(Color color, PieceType pieceType, unsigned int fromSquare, unsigned int toSquare){
    removePiece(color, pieceType, fromSquare);
    addPiece(color, pieceType, toSquare);
}