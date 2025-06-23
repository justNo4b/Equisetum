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
    // on init just do a full reset
    fullReset(board);
}

int16_t * NNueEvaluation::getHalfAccumulatorPtr(Color color){
    return _hiddenScore[color];
}

void NNueEvaluation::fullReset(const Board &board){
    // Load a net given board
    // Pre-calculate hidden layer scores for further incremental updates

    int wKing = _bitscanForward(board.getPieces(WHITE, KING));
    int bKing = _bitscanForward(board.getPieces(BLACK, KING));

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
                    _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[_getPieceIndex(sq, pt, color, WHITE, wKing)][i];
                    _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[_getPieceIndex(sq, pt, color, BLACK, bKing)][i];
                }
            }
        }
    }
}

void NNueEvaluation::halfReset(const Board &board, Color half){
    // Load a net given board
    // Pre-calculate hidden layer scores for further incremental updates

    int hKing = half == WHITE ?  _bitscanForward(board.getPieces(WHITE, KING)) : _bitscanForward(board.getPieces(BLACK, KING));

    // Load biases
    // Here we simply set values
        for (int k = 0; k < NNUE_HIDDEN; k++){
            _hiddenScore[half][k] =  NNUE_HIDDEN_BIAS[k];
        }


    // Load pieces
    // Here we do ++
    for (auto pt : {PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING}){
        for (auto color : {WHITE, BLACK}){
            U64 tmpBB = board.getPieces(color, pt);
            while (tmpBB){
                int sq = _popLsb(tmpBB);
                for (int i = 0; i < NNUE_HIDDEN; i++){
                    _hiddenScore[half][i] += NNUE_HIDDEN_WEIGHT[_getPieceIndex(sq, pt, color, half, hKing)][i];
                }
            }
        }
    }
}

void NNueEvaluation::addSubDifference(const Board &board, Color half, U64 (* otherPieces)[2][6]){

    int hKing = half == WHITE ? _bitscanForward(board.getPieces(WHITE, KING)) : _bitscanForward(board.getPieces(BLACK, KING));
    int maxSubs = _popCount(board.getOccupied());

    for (auto color : { WHITE, BLACK }){
        for (auto piece :{PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING}){

            U64 toremove = (*otherPieces)[color][piece] & ~board.getPieces(color, piece);
            U64 toadd = board.getPieces(color, piece) & ~(*otherPieces)[color][piece];

            // exists in current board, absent in past -> add index
            while (toadd){
                int square = _popLsb(toadd);
                int index = _getPieceIndex(square, piece, color, half, hKing);
                for (int i = 0; i < NNUE_HIDDEN; i++){
                        _hiddenScore[half][i] += NNUE_HIDDEN_WEIGHT[index][i];
                    }
            }

            // absent in current, thus present in the past -> remove index
            while(toremove){
                int square = _popLsb(toremove);
                int index = _getPieceIndex(square, piece, color, half, hKing);
                for (int i = 0; i < NNUE_HIDDEN; i++){
                    _hiddenScore[half][i] -= NNUE_HIDDEN_WEIGHT[index][i];
                }
            }
        }
    }





}

void NNueEvaluation::addSubDifferenceExternal(int16_t (*external)[NNUE_HIDDEN], int (* add)[32], int addCount, int (* sub)[32], int subCount){

    for (int i = 0; i < NNUE_HIDDEN; i++){
        int16_t val =  (*external)[i];
        for (int j = 0; j < addCount; j++){
            val += NNUE_HIDDEN_WEIGHT[(*add)[j]][i];
        }

        for (int k = 0; k < subCount; k++){
            val -= NNUE_HIDDEN_WEIGHT[(*sub)[k]][i];
        }
         (*external)[i] = val;
    }

}

int NNueEvaluation::getIndex(int sq, PieceType pt, Color c, Color view, int ksq){
    return _getPieceIndex(sq, pt, c, view, ksq);
}

bool NNueEvaluation::resetNeeded(PieceType pt, int from, int to, Color view){
    int bucket_to   = BUCKETS[view == WHITE ? to    : _mir(to)];
    int bucket_from = BUCKETS[view == WHITE ? from  : _mir(from)];

    // if king is moved, do full reset if
    // side is changed or bucket is changed
    if (pt == KING &&
        (((_col(from) > 3) != (_col(to) > 3))  ||  (bucket_to != bucket_from))){
            return true;
        }

    return false;
}

int NNueEvaluation::getCurrentBucket(int to, Color view){
    return BUCKETS[view == WHITE ? to    : _mir(to)];
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

void NNueEvaluation::movePiece(UpdData ud){
    Color color = ud.color;
    PieceType pieceType = ud.movingPiece;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int wKing = ud.wKing;
    int bKing = ud.bKing;

    int remove_indexWV  = _getPieceIndex(fromSquare, pieceType, color, WHITE, wKing);
    int remove_indexBV  = _getPieceIndex(fromSquare, pieceType, color, BLACK, bKing);
    int add_indexWV     = _getPieceIndex(toSquare, pieceType, color, WHITE, wKing);
    int add_indexBV     = _getPieceIndex(toSquare, pieceType, color, BLACK, bKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i];
    }

}

void NNueEvaluation::movePieceHalf(UpdData ud, Color half){
    Color color = ud.color;
    PieceType pieceType = ud.movingPiece;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int hKing = half == WHITE ? ud.wKing : ud.bKing;


    int remove_index  = _getPieceIndex(fromSquare, pieceType, color, half, hKing);
    int add_index     = _getPieceIndex(toSquare, pieceType, color, half, hKing);


    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[half][i] += NNUE_HIDDEN_WEIGHT[add_index][i] - NNUE_HIDDEN_WEIGHT[remove_index][i];
    }

}

void NNueEvaluation::promotePiece(UpdData ud){
    Color color = ud.color;
    PieceType promotedTo = ud.promotedPiece;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int wKing = ud.wKing;
    int bKing = ud.bKing;

    // remove pawn
    int remove_indexWV  = _getPieceIndex(fromSquare, PAWN, color, WHITE, wKing);
    int remove_indexBV  = _getPieceIndex(fromSquare, PAWN, color, BLACK, bKing);
    // add promoted piece
    int add_indexWV     = _getPieceIndex(toSquare, promotedTo, color, WHITE, wKing);
    int add_indexBV     = _getPieceIndex(toSquare, promotedTo, color, BLACK, bKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i];
    }

}

void NNueEvaluation::promotePieceHalf(UpdData ud, Color half){
    Color color = ud.color;
    PieceType promotedTo = ud.promotedPiece;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int hKing = half == WHITE ? ud.wKing : ud.bKing;

    // remove pawn
    int remove_index  = _getPieceIndex(fromSquare, PAWN, color, half, hKing);
    // add promoted piece
    int add_index     = _getPieceIndex(toSquare, promotedTo, color, half, hKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[half][i] += NNUE_HIDDEN_WEIGHT[add_index][i] - NNUE_HIDDEN_WEIGHT[remove_index][i];
    }

}

void NNueEvaluation::cappromPiece(UpdData ud){
    Color color = ud.color;
    PieceType capturedPiece = ud.capturedPiece;
    PieceType promotedTo = ud.promotedPiece;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int wKing = ud.wKing;
    int bKing = ud.bKing;

    // Remove pawn
    int remove_indexWV      = _getPieceIndex(fromSquare, PAWN, color, WHITE, wKing);
    int remove_indexBV      = _getPieceIndex(fromSquare, PAWN, color, BLACK, bKing);
    // Add promoted piece
    int add_indexWV         = _getPieceIndex(toSquare, promotedTo, color, WHITE, wKing);
    int add_indexBV         = _getPieceIndex(toSquare, promotedTo, color, BLACK, bKing);
    // Remove captured
    int captured_indexWV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), WHITE, wKing);
    int captured_indexBV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), BLACK, bKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i];
    }

}

void NNueEvaluation::cappromPieceHalf(UpdData ud, Color half){
    Color color = ud.color;
    PieceType capturedPiece = ud.capturedPiece;
    PieceType promotedTo = ud.promotedPiece;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int hKing = half == WHITE ? ud.wKing : ud.bKing;

    // Remove pawn
    int remove_index      = _getPieceIndex(fromSquare, PAWN, color, half, hKing);
    // Add promoted piece
    int add_index         = _getPieceIndex(toSquare, promotedTo, color, half, hKing);
    // Remove captured
    int captured_index    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), half, hKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[half][i] += NNUE_HIDDEN_WEIGHT[add_index][i] - NNUE_HIDDEN_WEIGHT[remove_index][i] - NNUE_HIDDEN_WEIGHT[captured_index][i];
    }

}

void NNueEvaluation::capturePiece(UpdData ud){
    Color color = ud.color;
    PieceType capturedPiece = ud.capturedPiece;
    PieceType pieceType = ud.movingPiece;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int wKing = ud.wKing;
    int bKing = ud.bKing;

    // MovePiece
    int remove_indexWV  = _getPieceIndex(fromSquare, pieceType, color, WHITE, wKing);
    int remove_indexBV  = _getPieceIndex(fromSquare, pieceType, color, BLACK, bKing);
    int add_indexWV     = _getPieceIndex(toSquare, pieceType, color, WHITE, wKing);
    int add_indexBV     = _getPieceIndex(toSquare, pieceType, color, BLACK, bKing);
    // RemoveCaptured
    int captured_indexWV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), WHITE, wKing);
    int captured_indexBV    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), BLACK, bKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i];
    }

}

void NNueEvaluation::capturePieceHalf(UpdData ud, Color half){
    Color color = ud.color;
    PieceType capturedPiece = ud.capturedPiece;
    PieceType pieceType = ud.movingPiece;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int hKing = half == WHITE ? ud.wKing : ud.bKing;

    // MovePiece
    int remove_index  = _getPieceIndex(fromSquare, pieceType, color, half, hKing);
    int add_index     = _getPieceIndex(toSquare, pieceType, color, half, hKing);

    // RemoveCaptured
    int captured_index    = _getPieceIndex(toSquare, capturedPiece, getOppositeColor(color), half, hKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[half][i] += NNUE_HIDDEN_WEIGHT[add_index][i] - NNUE_HIDDEN_WEIGHT[remove_index][i] - NNUE_HIDDEN_WEIGHT[captured_index][i];
    }

}

void NNueEvaluation::castleMoveHalf(UpdData ud, Color half){
    Color color = ud.color;
    int fromSquareKing = ud.from;
    int toSquareKing = ud.to;
    int fromSquareRook = ud.fromRook;
    int toSquareRook = ud.toRook;
    int hKing = half == WHITE ? ud.wKing : ud.bKing;

    // King
    int removeK_index  = _getPieceIndex(fromSquareKing, KING, color, half, hKing);
    int addK_index     = _getPieceIndex(toSquareKing, KING, color, half, hKing);
    //Rook
    int removeR_index  = _getPieceIndex(fromSquareRook, ROOK, color, half, hKing);
    int addR_index     = _getPieceIndex(toSquareRook, ROOK, color, half, hKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[half][i] += NNUE_HIDDEN_WEIGHT[addK_index][i] - NNUE_HIDDEN_WEIGHT[removeK_index][i]
                                + NNUE_HIDDEN_WEIGHT[addR_index][i] - NNUE_HIDDEN_WEIGHT[removeR_index][i];
    }

}

void NNueEvaluation::enpassMove(UpdData ud){
    Color color = ud.color;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int wKing = ud.wKing;
    int bKing = ud.bKing;

    unsigned int epPawn = color == WHITE ? toSquare - 8 : toSquare + 8;
    // MovePiece
    int remove_indexWV  = _getPieceIndex(fromSquare, PAWN, color, WHITE, wKing);
    int remove_indexBV  = _getPieceIndex(fromSquare, PAWN, color, BLACK, bKing);
    int add_indexWV     = _getPieceIndex(toSquare, PAWN, color, WHITE, wKing);
    int add_indexBV     = _getPieceIndex(toSquare, PAWN, color, BLACK, bKing);
    // RemoveCaptured
    int captured_indexWV    = _getPieceIndex(epPawn, PAWN, getOppositeColor(color), WHITE, wKing);
    int captured_indexBV    = _getPieceIndex(epPawn, PAWN, getOppositeColor(color), BLACK, bKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[WHITE][i] += NNUE_HIDDEN_WEIGHT[add_indexWV][i] - NNUE_HIDDEN_WEIGHT[remove_indexWV][i] - NNUE_HIDDEN_WEIGHT[captured_indexWV][i];
        _hiddenScore[BLACK][i] += NNUE_HIDDEN_WEIGHT[add_indexBV][i] - NNUE_HIDDEN_WEIGHT[remove_indexBV][i] - NNUE_HIDDEN_WEIGHT[captured_indexBV][i];
    }

}

void NNueEvaluation::enpassMoveHalf(UpdData ud, Color half){
    Color color = ud.color;
    int fromSquare = ud.from;
    int toSquare = ud.to;
    int hKing = half == WHITE ? ud.wKing : ud.bKing;

    unsigned int epPawn = color == WHITE ? toSquare - 8 : toSquare + 8;
    // MovePiece
    int remove_index  = _getPieceIndex(fromSquare, PAWN, color, half, hKing);
    int add_index     = _getPieceIndex(toSquare, PAWN, color, half, hKing);
    // RemoveCaptured
    int captured_index    = _getPieceIndex(epPawn, PAWN, getOppositeColor(color), half, hKing);

    for (int i = 0; i < NNUE_HIDDEN; i++){
        _hiddenScore[half][i] += NNUE_HIDDEN_WEIGHT[add_index][i] - NNUE_HIDDEN_WEIGHT[remove_index][i] - NNUE_HIDDEN_WEIGHT[captured_index][i];
    }

}