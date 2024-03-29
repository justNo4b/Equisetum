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
#include <cstdint>
#include <fstream>
#include <sstream>

// Enable incbin
#ifdef _INCBIN_
#include "incbin/incbin.h"
INCBIN(network, EVALFILE);
#endif

    int16_t NNueEvaluation::NNUE_HIDDEN_BIAS[NNUE_HIDDEN];
    int16_t NNueEvaluation::NNUE_HIDDEN_WEIGHT[NNUE_INPUT][NNUE_HIDDEN];
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

    // Load HCE feature
    // See if features are activated
    bool twoBishopsWhite = _popCount(board.getPieces(WHITE, BISHOP)) == 2;
    bool twoBishopsBlack = _popCount(board.getPieces(BLACK, BISHOP)) == 2;
    bool twoRooksWhite   = _popCount(board.getPieces(WHITE, ROOK)) == 2;
    bool twoRooksBlack   = _popCount(board.getPieces(BLACK, ROOK)) == 2;
    bool twoKnightsWhite = _popCount(board.getPieces(WHITE, KNIGHT)) == 2;
    bool twoKnightsBlack = _popCount(board.getPieces(BLACK, KNIGHT)) == 2;

    // activate features
    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[_getPieceIndex(a1, PAWN, WHITE, WHITE)][i] * twoBishopsWhite
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(a8, PAWN, BLACK, WHITE)][i] * twoBishopsBlack
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(b1, PAWN, WHITE, WHITE)][i] * twoRooksWhite
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(b8, PAWN, BLACK, WHITE)][i] * twoRooksBlack
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(c1, PAWN, WHITE, WHITE)][i] * twoKnightsWhite
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(c8, PAWN, BLACK, WHITE)][i] * twoKnightsBlack;

        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[_getPieceIndex(a1, PAWN, WHITE, BLACK)][i] * twoBishopsWhite
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(a8, PAWN, BLACK, BLACK)][i] * twoBishopsBlack
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(b1, PAWN, WHITE, BLACK)][i] * twoRooksWhite
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(b8, PAWN, BLACK, BLACK)][i] * twoRooksBlack
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(c1, PAWN, WHITE, BLACK)][i] * twoKnightsWhite
                                + NNUE_HIDDEN_WEIGHT[_getPieceIndex(c8, PAWN, BLACK, BLACK)][i] * twoKnightsBlack;
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

void NNueEvaluation::movePiece(Color color, PieceType pieceType, unsigned int fromSquare, unsigned int toSquare){
    int remove_indexWV  = _getPieceIndex(fromSquare, pieceType, color, WHITE);
    int remove_indexBV  = _getPieceIndex(fromSquare, pieceType, color, BLACK);
    int add_indexWV     = _getPieceIndex(toSquare, pieceType, color, WHITE);
    int add_indexBV     = _getPieceIndex(toSquare, pieceType, color, BLACK);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i];
    }

}

void NNueEvaluation::promotePiece(Color color, PieceType promotedTo, unsigned int fromSquare, unsigned int toSquare, bool doubleChanged){
    // remove pawn
    int remove_indexWV  = _getPieceIndex(fromSquare, PAWN, color, WHITE);
    int remove_indexBV  = _getPieceIndex(fromSquare, PAWN, color, BLACK);
    // add promoted piece
    int add_indexWV     = _getPieceIndex(toSquare, promotedTo, color, WHITE);
    int add_indexBV     = _getPieceIndex(toSquare, promotedTo, color, BLACK);

    //Remove double feature if nessesary
    int double_indexWV = _getPieceIndex(DF_SQUARE[color][promotedTo], PAWN, color, WHITE);
    int double_indexBV = _getPieceIndex(DF_SQUARE[color][promotedTo], PAWN, color, BLACK);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] + NNUE_HIDDEN_WEIGHT[double_indexWV][i] * doubleChanged;;
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] + NNUE_HIDDEN_WEIGHT[double_indexBV][i] * doubleChanged;;
    }

}


void NNueEvaluation::cappromPiece(Color color, PieceType capturedPiece, PieceType promotedTo, unsigned int fromSquare, unsigned int toSquare, bool doubleChanged){
    // Remove pawn
    int remove_indexWV      = _getPieceIndex(fromSquare, PAWN, color, WHITE);
    int remove_indexBV      = _getPieceIndex(fromSquare, PAWN, color, BLACK);
    // Add promoted piece
    int add_indexWV         = _getPieceIndex(toSquare, promotedTo, color, WHITE);
    int add_indexBV         = _getPieceIndex(toSquare, promotedTo, color, BLACK);
    // Remove captured
    int captured_indexWV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), WHITE);
    int captured_indexBV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), BLACK);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i];
    }

}

void NNueEvaluation::capturePiece(Color color, PieceType pieceType, PieceType capturedPiece, unsigned int fromSquare, unsigned int toSquare, bool doubleChanged){
    // MovePiece
    int remove_indexWV  = _getPieceIndex(fromSquare, pieceType, color, WHITE);
    int remove_indexBV  = _getPieceIndex(fromSquare, pieceType, color, BLACK);
    int add_indexWV     = _getPieceIndex(toSquare, pieceType, color, WHITE);
    int add_indexBV     = _getPieceIndex(toSquare, pieceType, color, BLACK);
    // RemoveCaptured
    int captured_indexWV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), WHITE);
    int captured_indexBV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), BLACK);

    //Remove double feature if nessesary
    int double_indexWV = _getPieceIndex(DF_SQUARE[getOppositeColor(color)][capturedPiece], PAWN, getOppositeColor(color), WHITE);
    int double_indexBV = _getPieceIndex(DF_SQUARE[getOppositeColor(color)][capturedPiece], PAWN, getOppositeColor(color), BLACK);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i] - NNUE_HIDDEN_WEIGHT[double_indexWV][i] * doubleChanged;
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i] - NNUE_HIDDEN_WEIGHT[double_indexBV][i] * doubleChanged;
    }

}

void NNueEvaluation::castleMove(Color color, unsigned int fromSquareKing, unsigned int toSquareKing,unsigned int fromSquareRook, unsigned int toSquareRook){
    // King
    int removeK_indexWV  = _getPieceIndex(fromSquareKing, KING, color, WHITE);
    int removeK_indexBV  = _getPieceIndex(fromSquareKing, KING, color, BLACK);
    int addK_indexWV     = _getPieceIndex(toSquareKing, KING, color, WHITE);
    int addK_indexBV     = _getPieceIndex(toSquareKing, KING, color, BLACK);
    //Rook
    int removeR_indexWV  = _getPieceIndex(fromSquareRook, ROOK, color, WHITE);
    int removeR_indexBV  = _getPieceIndex(fromSquareRook, ROOK, color, BLACK);
    int addR_indexWV     = _getPieceIndex(toSquareRook, ROOK, color, WHITE);
    int addR_indexBV     = _getPieceIndex(toSquareRook, ROOK, color, BLACK);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[addK_indexWV][i] - NNUE_HIDDEN_WEIGHT[removeK_indexWV][i]
                                + NNUE_HIDDEN_WEIGHT[addR_indexWV][i] - NNUE_HIDDEN_WEIGHT[removeR_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[addK_indexBV][i] - NNUE_HIDDEN_WEIGHT[removeK_indexBV][i]
                                + NNUE_HIDDEN_WEIGHT[addR_indexBV][i] - NNUE_HIDDEN_WEIGHT[removeR_indexBV][i];
    }

}

void NNueEvaluation::enpassMove(Color color, unsigned int fromSquare, unsigned int toSquare){
    unsigned int epPawn = color == WHITE ? toSquare - 8 : toSquare + 8;
    // MovePiece
    int remove_indexWV  = _getPieceIndex(fromSquare, PAWN, color, WHITE);
    int remove_indexBV  = _getPieceIndex(fromSquare, PAWN, color, BLACK);
    int add_indexWV     = _getPieceIndex(toSquare, PAWN, color, WHITE);
    int add_indexBV     = _getPieceIndex(toSquare, PAWN, color, BLACK);
    // RemoveCaptured
    int captured_indexWV    = _getPieceIndex(epPawn, PAWN, getOppositeColor(color), WHITE);
    int captured_indexBV    = _getPieceIndex(epPawn, PAWN, getOppositeColor(color), BLACK);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i];
    }

}