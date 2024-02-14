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
#include <sstream>

// Enable incbin
#ifdef _INCBIN_
#include "incbin/incbin.h"
INCBIN(network, EVALFILE);
#endif

    int16_t NNueEvaluation::NNUE_HIDDEN_BIAS[NNUE_HIDDEN];
    int16_t NNueEvaluation::NNUE_HIDDEN_WEIGHT[NNUE_INPUT * NNUE_BUCKETS][NNUE_HIDDEN];
    int16_t NNueEvaluation::NNUE_OUTPUT_WEIGHT[NNUE_HIDDEN];
    int16_t NNueEvaluation::NNUE_OUTPUT_WEIGHT2[NNUE_HIDDEN];
    int32_t NNueEvaluation::NNUE_OUTPUT_BIAS[NNUE_OUTPUT];


void NNueEvaluation::init(){

#ifdef _INCBIN_
    // read embedded NNUE file
    auto file = std::stringstream(std::ios::in | std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<const char*>(gnetworkData), gnetworkSize);

#else
    // normal compilation, read NNUE from harddrive

    std::ifstream file(EVAL_FILE, std::ios::binary | std::ios::in);
    // Exit the program if file is not found or cannot be opened
    if (!file){
        std::cout << "Failed to open NNUE file. Exit"<< std::endl;
        exit(0);
    }
    // File exist, proceed
#endif


    for (int i = 0; i < NNUE_INPUT * NNUE_BUCKETS; i++){
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

    int indexesToUpdate [2][64] = {};
    int indexesTotal = 0;

    int wKing = _bitscanForward(board.getPieces(WHITE, KING));
    int bKing = _bitscanForward(board.getPieces(BLACK, KING));

    // Calculate all needed indexes for weights
    for (auto pt : {PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING}){
        for (auto pieceColor : {WHITE, BLACK}){
            U64 tmpBB = board.getPieces(pieceColor, pt);
            while (tmpBB){
                int sq = _popLsb(tmpBB);
                indexesToUpdate[WHITE][indexesTotal] = _getPieceIndex(sq, pt, pieceColor, WHITE, wKing);
                indexesToUpdate[BLACK][indexesTotal] = _getPieceIndex(sq, pt, pieceColor, BLACK, bKing);
                indexesTotal++;
            }
        }
    }

    // Load biases
    // Here we simply set values
    for (int k = 0; k < NNUE_HIDDEN; k++){
        _hiddenScore[WHITE][k] =  NNUE_HIDDEN_BIAS[k];
        _hiddenScore[BLACK][k] =  NNUE_HIDDEN_BIAS[k];
    }


    for (int i = 0; i < NNUE_HIDDEN; i++){
        for (int k = 0; k < indexesTotal; k++){
            _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[indexesToUpdate[WHITE][k]][i];
            _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[indexesToUpdate[BLACK][k]][i];
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

void NNueEvaluation::movePiece(Color color, PieceType pieceType, unsigned int fromSquare, unsigned int toSquare, int wKingSquare, int bKingSquare){
    int remove_indexWV  = _getPieceIndex(fromSquare, pieceType, color, WHITE, wKingSquare);
    int remove_indexBV  = _getPieceIndex(fromSquare, pieceType, color, BLACK, bKingSquare);
    int add_indexWV     = _getPieceIndex(toSquare, pieceType, color, WHITE, wKingSquare);
    int add_indexBV     = _getPieceIndex(toSquare, pieceType, color, BLACK, bKingSquare);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i];
    }

}

void NNueEvaluation::promotePiece(Color color, PieceType promotedTo, unsigned int fromSquare, unsigned int toSquare, int wKingSquare, int bKingSquare){
    // remove pawn
    int remove_indexWV  = _getPieceIndex(fromSquare, PAWN, color, WHITE, wKingSquare);
    int remove_indexBV  = _getPieceIndex(fromSquare, PAWN, color, BLACK, bKingSquare);
    // add promoted piece
    int add_indexWV     = _getPieceIndex(toSquare, promotedTo, color, WHITE, wKingSquare);
    int add_indexBV     = _getPieceIndex(toSquare, promotedTo, color, BLACK, bKingSquare);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i];
    }

}


void NNueEvaluation::cappromPiece(Color color, PieceType capturedPiece, PieceType promotedTo, unsigned int fromSquare, unsigned int toSquare, int wKingSquare, int bKingSquare){
    // Remove pawn
    int remove_indexWV      = _getPieceIndex(fromSquare, PAWN, color, WHITE, wKingSquare);
    int remove_indexBV      = _getPieceIndex(fromSquare, PAWN, color, BLACK, bKingSquare);
    // Add promoted piece
    int add_indexWV         = _getPieceIndex(toSquare, promotedTo, color, WHITE, wKingSquare);
    int add_indexBV         = _getPieceIndex(toSquare, promotedTo, color, BLACK, bKingSquare);
    // Remove captured
    int captured_indexWV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), WHITE, wKingSquare);
    int captured_indexBV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), BLACK, bKingSquare);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i];
    }

}

void NNueEvaluation::capturePiece(Color color, PieceType pieceType, PieceType capturedPiece, unsigned int fromSquare, unsigned int toSquare, int wKingSquare, int bKingSquare){
    // MovePiece
    int remove_indexWV  = _getPieceIndex(fromSquare, pieceType, color, WHITE, wKingSquare);
    int remove_indexBV  = _getPieceIndex(fromSquare, pieceType, color, BLACK, bKingSquare);
    int add_indexWV     = _getPieceIndex(toSquare, pieceType, color, WHITE, wKingSquare);
    int add_indexBV     = _getPieceIndex(toSquare, pieceType, color, BLACK, bKingSquare);
    // RemoveCaptured
    int captured_indexWV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), WHITE, wKingSquare);
    int captured_indexBV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), BLACK, bKingSquare);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i];
    }

}

void NNueEvaluation::castleMove(Color color, unsigned int fromSquareKing, unsigned int toSquareKing,unsigned int fromSquareRook, unsigned int toSquareRook, int wKingSquare, int bKingSquare){
    // King
    int removeK_indexWV  = _getPieceIndex(fromSquareKing, KING, color, WHITE, wKingSquare);
    int removeK_indexBV  = _getPieceIndex(fromSquareKing, KING, color, BLACK, bKingSquare);
    int addK_indexWV     = _getPieceIndex(toSquareKing, KING, color, WHITE, wKingSquare);
    int addK_indexBV     = _getPieceIndex(toSquareKing, KING, color, BLACK, bKingSquare);
    //Rook
    int removeR_indexWV  = _getPieceIndex(fromSquareRook, ROOK, color, WHITE, wKingSquare);
    int removeR_indexBV  = _getPieceIndex(fromSquareRook, ROOK, color, BLACK, bKingSquare);
    int addR_indexWV     = _getPieceIndex(toSquareRook, ROOK, color, WHITE, wKingSquare);
    int addR_indexBV     = _getPieceIndex(toSquareRook, ROOK, color, BLACK, bKingSquare);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[addK_indexWV][i] - NNUE_HIDDEN_WEIGHT[removeK_indexWV][i]
                                + NNUE_HIDDEN_WEIGHT[addR_indexWV][i] - NNUE_HIDDEN_WEIGHT[removeR_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[addK_indexBV][i] - NNUE_HIDDEN_WEIGHT[removeK_indexBV][i]
                                + NNUE_HIDDEN_WEIGHT[addR_indexBV][i] - NNUE_HIDDEN_WEIGHT[removeR_indexBV][i];
    }

}

void NNueEvaluation::enpassMove(Color color, unsigned int fromSquare, unsigned int toSquare, int wKingSquare, int bKingSquare){
    unsigned int epPawn = color == WHITE ? toSquare - 8 : toSquare + 8;
    // MovePiece
    int remove_indexWV  = _getPieceIndex(fromSquare, PAWN, color, WHITE, wKingSquare);
    int remove_indexBV  = _getPieceIndex(fromSquare, PAWN, color, BLACK, bKingSquare);
    int add_indexWV     = _getPieceIndex(toSquare, PAWN, color, WHITE, wKingSquare);
    int add_indexBV     = _getPieceIndex(toSquare, PAWN, color, BLACK, bKingSquare);
    // RemoveCaptured
    int captured_indexWV    = _getPieceIndex(epPawn, PAWN, getOppositeColor(color), WHITE, wKingSquare);
    int captured_indexBV    = _getPieceIndex(epPawn, PAWN, getOppositeColor(color), BLACK, bKingSquare);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i];
    }

}