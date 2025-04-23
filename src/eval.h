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
#ifndef EVAL_H
#define EVAL_H

#include "defs.h"
#include "movegen.h"
#include "bitutils.h"
#include "endgame.h"

#define gS(opS, egS) (int)((unsigned int)(opS) << 16) + (egS)
#define opS(gS) (int16_t)((uint16_t)((unsigned)((gS) + 0x8000) >> 16))
#define egS(gS) (int16_t)((uint16_t)((unsigned)((gS))))

struct evalBits{
    U64 EnemyPawnAttackMap[2];
    U64 PossibleProtOutposts[2];
    U64 PossibleGenOutposts[2];
    U64 EnemyKingZone[2];
    U64 Passers[2];
    U64 AttackedSquares[2];
    U64 AttackedByKing[2];
    int RammedCount;
    int KingAttackers[2];
    int KingAttackPower[2];
};

enum CastleSide {
    KingSide,
    QueenSide,
    NoCastle
};

/**
 * @brief Namespace containing board evaluation functions
 */
namespace Eval {
namespace detail {
/**
 * @brief Array of files A-H as bitboards
 */
extern U64 FILES[8];
extern U64 OUTPOST_PROTECTION[2][64];
extern U64 DISTANCE[64][64];
extern U64 FORWARD_BITS[2][64];
extern U64 IN_BETWEEN[64][64];
extern U64 PASSED_PAWN_MASKS[2][64];


};


/**
 * @brief Piece - Square - Tables
 * Bonuses for each piece being on the square on the board. From Black point of view
 */
const int KING_PSQT_BLACK[64] = {
           gS(0,-85), gS(0,-25), gS(0,-8), gS(0,-5), gS(0,-17), gS(0,-20), gS(0,-10), gS(0,-93),
           gS(0,-14), gS(0,31), gS(4,31), gS(0,32), gS(1,18), gS(0,36), gS(0,32), gS(-2,-14),
           gS(0,-3), gS(27,25), gS(8,36), gS(0,39), gS(0,42), gS(17,38), gS(11,31), gS(0,0),
           gS(-1,-17), gS(9,12), gS(-2,25), gS(-9,34), gS(-11,31), gS(-2,26), gS(5,14), gS(-30,-12),
           gS(-11,-28), gS(-2,0), gS(-6,15), gS(-18,28), gS(-19,25), gS(-25,14), gS(-7,-1), gS(-41,-19),
           gS(-6,-30), gS(11,-7), gS(-13,6), gS(-30,19), gS(-24,16), gS(-24,6), gS(0,-7), gS(-9,-25),
           gS(9,-31), gS(0,-7), gS(7,0), gS(-14,6), gS(-15,8), gS(4,-1), gS(7,-11), gS(1,-32),
           gS(-13,-59), gS(15,-43), gS(-11,-16), gS(-36,-16), gS(14,-36), gS(-14,-21), gS(8,-40), gS(-1,-70),
};

const int PAWN_PSQT_BLACK[64] = {
           gS(0,0), gS(0,0), gS(0,0), gS(0,0), gS(0,0), gS(0,0), gS(0,0), gS(0,0),
           gS(47,69), gS(37,73), gS(35,71), gS(65,50), gS(56,54), gS(39,59), gS(-1,77), gS(-3,68),
           gS(2,33), gS(8,34), gS(27,13), gS(33,-3), gS(33,-5), gS(73,0), gS(30,25), gS(26,19),
           gS(-17,21), gS(0,10), gS(1,1), gS(3,-8), gS(9,-8), gS(15,-4), gS(15,0), gS(-3,0),
           gS(-28,8), gS(-20,10), gS(-13,0), gS(0,-6), gS(3,-6), gS(0,-5), gS(-8,0), gS(-18,-7),
           gS(-31,2), gS(-19,2), gS(-11,-2), gS(-7,0), gS(1,-2), gS(-4,-5), gS(-6,-7), gS(-23,-11),
           gS(-36,10), gS(-17,10), gS(-12,8), gS(1,4), gS(0,11), gS(3,3), gS(-6,0), gS(-34,-5),
           gS(0,0), gS(0,0), gS(0,0), gS(0,0), gS(0,0), gS(0,0), gS(0,0), gS(0,0),
};

const int ROOK_PSQT_BLACK[64] = {
           gS(22,10), gS(11,16), gS(-1,25), gS(-3,18), gS(2,15), gS(8,17), gS(22,16), gS(34,13),
           gS(-2,9), gS(-4,16), gS(12,16), gS(30,3), gS(20,2), gS(23,3), gS(-2,14), gS(11,7),
           gS(-6,9), gS(15,5), gS(11,6), gS(15,0), gS(34,-5), gS(28,-7), gS(29,0), gS(0,3),
           gS(-4,11), gS(2,9), gS(13,8), gS(14,2), gS(15,-2), gS(20,-7), gS(13,3), gS(2,-1),
           gS(-14,7), gS(-12,9), gS(-2,6), gS(1,6), gS(2,2), gS(-5,0), gS(1,3), gS(-15,1),
           gS(-15,1), gS(-9,0), gS(-3,-1), gS(-1,0), gS(3,-2), gS(5,-11), gS(18,-11), gS(-3,-13),
           gS(-17,-6), gS(-7,-7), gS(2,-3), gS(4,-5), gS(11,-13), gS(6,-18), gS(16,-20), gS(-26,-9),
           gS(-5,-5), gS(-2,-8), gS(-1,-1), gS(5,-7), gS(13,-14), gS(0,-8), gS(4,-14), gS(-6,-20),
};

const int BISHOP_PSQT_BLACK[64] = {
           gS(-36,-1), gS(-43,-2), gS(-74,14), gS(-82,13), gS(-56,11), gS(-91,10), gS(-26,-6), gS(-31,-18),
           gS(-38,-5), gS(-23,-5), gS(-15,-2), gS(-13,7), gS(-13,2), gS(-18,-1), gS(-31,-7), gS(-30,-12),
           gS(-4,3), gS(-1,0), gS(-6,0), gS(12,-4), gS(7,-1), gS(20,0), gS(4,-2), gS(11,0),
           gS(-10,0), gS(15,6), gS(4,4), gS(17,15), gS(19,8), gS(17,4), gS(21,0), gS(-7,-5),
           gS(-3,-5), gS(-7,5), gS(3,7), gS(20,7), gS(20,3), gS(7,3), gS(4,1), gS(11,-15),
           gS(-2,-13), gS(16,-2), gS(9,-1), gS(8,7), gS(10,11), gS(16,0), gS(19,-6), gS(13,-11),
           gS(7,-21), gS(6,-25), gS(14,-13), gS(1,2), gS(10,1), gS(14,-4), gS(21,-17), gS(8,-28),
           gS(-3,-31), gS(14,-11), gS(1,0), gS(0,0), gS(10,-2), gS(-11,8), gS(0,-9), gS(0,-27),
};

const int KNIGHT_PSQT_BLACK[64] = {
           gS(-163,-53), gS(-33,-6), gS(-81,7), gS(-14,-6), gS(-1,-3), gS(-82,0), gS(-30,-6), gS(-92,-65),
           gS(-36,-11), gS(-23,-1), gS(-6,-1), gS(14,1), gS(4,-1), gS(26,-13), gS(-12,-3), gS(-21,-18),
           gS(-11,-3), gS(0,0), gS(12,20), gS(22,7), gS(38,0), gS(46,0), gS(9,-4), gS(9,-16),
           gS(2,-2), gS(15,3), gS(28,16), gS(43,19), gS(31,18), gS(57,14), gS(22,4), gS(24,-10),
           gS(-6,0), gS(10,3), gS(20,20), gS(26,18), gS(30,23), gS(32,12), gS(27,2), gS(8,0),
           gS(-17,-18), gS(1,-4), gS(9,7), gS(12,15), gS(23,14), gS(22,2), gS(23,-8), gS(6,-8),
           gS(-18,-26), gS(-16,-2), gS(-3,-2), gS(8,4), gS(10,2), gS(8,-2), gS(-2,-3), gS(-3,-8),
           gS(-41,-29), gS(-18,-8), gS(-10,-6), gS(-2,1), gS(7,4), gS(-3,-2), gS(-14,-4), gS(-20,-28),
};

const int QUEEN_PSQT_BLACK[64] = {
           gS(-21,-5), gS(-6,4), gS(-1,8), gS(0,18), gS(-9,25), gS(-1,9), gS(-11,14), gS(-16,11),
           gS(-17,-4), gS(-31,16), gS(-18,22), gS(-29,43), gS(-43,63), gS(-24,28), gS(-15,25), gS(8,11),
           gS(-9,-14), gS(-6,-4), gS(-6,22), gS(-5,34), gS(-6,41), gS(4,31), gS(8,10), gS(3,7),
           gS(-7,-16), gS(0,7), gS(-4,18), gS(-10,37), gS(-8,50), gS(2,32), gS(9,25), gS(8,-3),
           gS(1,-27), gS(-6,0), gS(-4,8), gS(-3,36), gS(0,32), gS(-1,11), gS(12,3), gS(1,-1),
           gS(-4,-33), gS(8,-24), gS(2,-5), gS(0,2), gS(4,3), gS(9,-4), gS(22,-12), gS(7,-25),
           gS(2,-52), gS(8,-55), gS(13,-47), gS(13,-29), gS(14,-27), gS(17,-58), gS(27,-76), gS(1,-45),
           gS(0,-62), gS(0,-65), gS(5,-67), gS(6,-49), gS(8,-57), gS(0,-70), gS(1,-61), gS(12,-69),
};


/**
 * @brief Array indexed by [PieceType] of material values (in centipawns)
 */
const int MATERIAL_VALUES[5] = {
           gS(82,110), gS(484,685), gS(339,373), gS(366,385), gS(980,1254),
};

/**
 * @brief Initializes all inner constants used by functions in the Eval namespace
 */
void init();

/**
 * @brief Returns the evaluated advantage of the given color in centipawns
 *
 * @param board Board to evaluate
 * @param color Color to evaluate advantage of
 * @return Advantage of the given color in centipawns
 */
inline int evaluateMain(const Board &, Color);

int evaluate(const Board &, Color);


void initEG();

inline void egHashAdd(std::string , egEvalFunction, egEntryType);

int evaluateDraw(const Board &, Color);
int evaluateRookMinor_Rook(const Board &, Color);
int evaluateQueen_vs_X(const Board &, Color);
int evaluateQueen_vs_Pawn(const Board &, Color);
int evaluateRook_vs_Bishop(const Board &, Color);
int evaluateRook_vs_Knight(const Board &, Color);
int evaluateRook_vs_Pawn(const Board &, Color);
int evaluateBishopPawn_vs_Bishop(const Board &, Color);
int evaluateBishopPawn_vs_Knight(const Board &, Color);
int evaluateQueen_vs_RookPawn(const Board &, Color);
int evaluateRookPawn_vs_Bishop(const Board &, Color);
int evaluateHugeAdvantage(const Board &, Color);
int evaluateBN_Mating(const Board &, Color);
int evaluateKnights_vs_Pawn(const Board &, Color);
int evaluateRookPawn_vs_Rook(const Board &, Color);
int evaluateKingPawn_vs_King(const Board &, Color);
int evaluateBishopPawn_vs_KP(const Board &, Color);

/**
 * @brief Returns the value of the given PieceType used for evaluation
 * purposes in centipawns
 *
 * This method returns the material value assuming the game is in the opening
 * and can be used to quickly determine relative capture values for move
 * ordering purposes.
 *
 * @param pieceType PieceType to get value of
 * @return The value of the given PieceType used for evaluation purposes
 */
int getMaterialValue(int, PieceType);

  /**
    * @name Functions used for the evaluating positions of the Major Pieces
    * @brief Returns structure that contain opening and enggame scores
    * @{
    */
   inline int evaluateQUEEN(const Board &, Color, evalBits *);
   inline int evaluateROOK(const Board &, Color, evalBits *);
   inline int evaluateBISHOP(const Board &, Color, evalBits *);
   inline int evaluateKNIGHT(const Board &, Color, evalBits *);
   inline int evaluatePAWNS(const Board &, Color, evalBits *);
   inline int evaluateKING(const Board &, Color, evalBits *);
   inline int probePawnStructure(const Board &, Color, evalBits *);
   inline int winnableEndgame(const Board &, Color, evalBits *, int);
   inline int evaluatePNN(const Board &);

  /**@}*/

evalBits Setupbits(const Board &);

/**
 * @brief This function analyses king shield safety.
 * it returns simple overall score gS() and
 * adjust base safety value for some types of shields
 */
inline int kingShieldSafety(const Board &, Color, evalBits *);

/**
 * @brief Function evaluate piece-pawns interactions for given color
 * Includes:
 * 1. Blocked Pawns
 * 2. Minors shielded by pawns
 * 3. Threats by pawn push
 */
inline int PiecePawnInteraction(const Board &, Color, evalBits *);

/**
 * @brief Taper evaluation between Opening and Endgame and scale it
 * if there is some specific endgame position
 */
inline int TaperAndScale(const Board &, Color, int);

/**
 * @brief Transform danger score accumulated in other functions in
 *        a score used for an evaluation
 */
inline int kingDanger(Color, const evalBits *);

/**
 * @brief Set value for a MATERIAL_VALUES_TUNABLE array
 * which is used for optuna tuning
 */
void SetupTuning(PieceType piece, int value);

};

#endif
