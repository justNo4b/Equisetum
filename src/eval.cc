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
#include "rays.h"
#include "movegen.h"
#include "attacks.h"
#include "outposts.h"
#include "eval.h"
#include "transptable.h"

extern egEvalEntry myEvalHash[];

U64 Eval::detail::FILES[8] = {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
U64 Eval::detail::DISTANCE[64][64];

U64 Eval::detail::PASSED_PAWN_MASKS[2][64];
U64 Eval::detail::OUTPOST_PROTECTION[2][64];
U64 Eval::detail::FORWARD_BITS[2][64];
U64 Eval::detail::IN_BETWEEN[64][64];



void Eval::init() {
  initEG();
  // Initialize passed pawn masks
  for (int square = 0; square < 64; square++) {

    for (int i = 0; i < 64; i++){
      detail::DISTANCE[square][i] = std::max(abs(_col(square) - _col(i)), abs(_row(square) - _row(i)));

      if (Attacks::getSlidingAttacks(BISHOP, square, 0) & (ONE << i)){
        detail::IN_BETWEEN[square][i] = (Attacks::getSlidingAttacks(BISHOP, square, ONE << i) & Attacks::getSlidingAttacks(BISHOP, i, ONE << square));
      }else if (Attacks::getSlidingAttacks(ROOK, square, 0) & (ONE << i)){
        detail::IN_BETWEEN[square][i] =  (Attacks::getSlidingAttacks(ROOK, square, ONE << i) & Attacks::getSlidingAttacks(ROOK, i, ONE << square));
      }else{
        detail::IN_BETWEEN[square][i] = 0;
      }
    }

    for (auto color : { WHITE, BLACK }) {

      U64 forwardRay = Rays::getRay(color == WHITE ? Rays::NORTH : Rays::SOUTH, square);

      detail::FORWARD_BITS[color][square] = forwardRay;

      detail::PASSED_PAWN_MASKS[color][square] = forwardRay | _eastN(forwardRay, 1) | _westN(forwardRay, 1);

      U64 sqv = ONE << square;
      detail::OUTPOST_PROTECTION[color][square] = color == WHITE ? ((sqv >> 9) & ~FILE_H) | ((sqv >> 7) & ~FILE_A)
                                                                 : ((sqv << 9) & ~FILE_A) | ((sqv << 7) & ~FILE_H);

      U64 kingAttack = Attacks::getNonSlidingAttacks(KING, square, WHITE);

    }
  }

  // Init KPK bitbase
  // It should be initialized after rest of the Eval as some const are used in the
  // bitbase creation
  Bitbase::init_kpk();
}

int Eval::evaluate(const Board &board, Color color){

    // Probe eval hash
    U64 index = board.getpCountKey().getValue() & (EG_HASH_SIZE - 1);
    egEvalFunction spEval   = myEvalHash[index].eFunction;
    egEntryType    spevType = myEvalHash[index].evalType;
    int            egResult = 1;

    if (myEvalHash[index].key == board.getpCountKey().getValue() && spEval != nullptr){
        egResult = spEval(board, color);
        if (spevType == RETURN_SCORE) return egResult;
    }


/*
    // for debug purposes
    NNueEvaluation nn = NNueEvaluation(board);
    int nnueEval =  nn.evaluate(board.getActivePlayer());
*/


    int nnueEval = board.getNNueEval();

    // phase 0 (max) -> 256 (min)
    // scale from 1.5 to 1
    nnueEval = (((384 - (board.getPhase() / 2) ) * nnueEval) / 256);

    return nnueEval / egResult;
}
