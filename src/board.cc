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
#include "board.h"
#include "nnue.h"
#include "bitutils.h"
#include "attacks.h"
#include "eval.h"
#include <sstream>
#include <cstring>

Board::Board() {
    setToFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false);
}

Board::Board(std::string fen, bool isFrc) {
  setToFen(fen, isFrc);
}

U64 Board::getPieces(Color color, PieceType pieceType) const {
  return _pieces[color][pieceType];
}

U64 Board::getAllPieces(Color color) const {
  return _allPieces[color];
}

U64 Board::getAttackable(Color color) const {
  return _allPieces[color] & ~_pieces[color][KING];
}

U64 Board::getOccupied() const {
  return _occupied;
}

U64 Board::getNotOccupied() const {
  return ~_occupied;
}

U64 Board::getEnPassant() const {
  return _enPassant;
}

Color Board::getActivePlayer() const {
  return _activePlayer;
}

U64 Board::getAttacksForSquare(PieceType pieceType, Color color, int square) const {
  // Special case for pawns
  if (pieceType == PAWN) {
    switch (color) {
      case WHITE: return _getWhitePawnAttacksForSquare(square);
      case BLACK: return _getBlackPawnAttacksForSquare(square);
    }
  }

  U64 own = getAllPieces(color);
  U64 attacks;
  switch (pieceType) {
    case ROOK: attacks = _getRookAttacksForSquare(square, own);
      break;
    case KNIGHT: attacks = _getKnightAttacksForSquare(square, own);
      break;
    case BISHOP: attacks = _getBishopAttacksForSquare(square, own);
      break;
    case QUEEN: attacks = _getQueenAttacksForSquare(square, own);
      break;
    case KING: attacks = _getKingAttacksForSquare(square, own);
      break;
    default: fatal("Invalid piece type");
  }

  return attacks;
}

U64 Board::getMobilityForSquare(PieceType pieceType, Color color, int square, U64 pBB) const {

  U64 scan;
  U64 own = getAllPieces(color);
  U64 attacks;
  switch (pieceType) {
    case PAWN:
      // pawn mobility isnt used
      return 0;
    case ROOK:
      own = own ^  getPieces(color, ROOK) ^ getPieces (color, QUEEN);
      scan =  getPieces(color, ROOK) | getPieces (color, QUEEN);
      attacks = _getRookMobilityForSquare(square, own, scan);
      break;
    case KNIGHT:
      attacks = _getKnightMobilityForSquare(square, own);
      break;
    case BISHOP:
      own = own ^ getPieces (color, QUEEN) ^ getPieces (color, BISHOP);
      scan = getPieces (color, QUEEN) | getPieces (color, BISHOP);
      attacks = _getBishopMobilityForSquare(square, own, scan);
      break;
    case QUEEN:
      // Queen is a special case
      // We sont want it to scan (i guess it plays out
      // with Safety (ie when Q behind B it isnt good in attack)
      // Maybe later test scan thorough R only test?
      scan = ZERO;
      attacks = _getRookMobilityForSquare(square, own, scan) | _getBishopMobilityForSquare(square, own, scan);
      break;
    case KING:
      attacks = _getKingAttacksForSquare(square, own);
      break;
  }
  attacks = attacks & (~pBB);
  return attacks;
}

Color Board::getInactivePlayer() const {
  return _activePlayer == WHITE ? BLACK : WHITE;
}

ZKey Board::getZKey() const {
  return _zKey;
}

ZKey Board::getPawnStructureZKey() const {
  return _pawnStructureZkey;
}

ZKey Board::getpCountKey() const {
    return _pCountKey;
}

PSquareTable Board::getPSquareTable() const {
  return _pst;
}

int Board::getNNueEval() const {
  return _nnue->evaluate(_activePlayer);
}

void Board::setNnuePtr(NNueEvaluation * nn){
    _updDone = true;
    _nnue = nn;
}

bool Board::colorIsInCheck(Color color) const {
  int kingSquare = _bitscanForward(getPieces(color, KING));
  return squareUnderAttack(getOppositeColor(color), kingSquare);
}

int Board::getHalfmoveClock() const {
  return _halfmoveClock;
}

std::string Board::getStringRep() const {
  std::string stringRep = "8  ";
  int rank = 8;

  U64 boardPos = 56; // Starts at a8, goes down rank by rank
  int squaresProcessed = 0;

  while (squaresProcessed < 64) {
    U64 square = ONE << boardPos;
    bool squareOccupied = (square & _occupied) != 0;

    if (squareOccupied) {
      if (square & _pieces[WHITE][PAWN]) stringRep += " P ";
      else if (square & _pieces[BLACK][PAWN]) stringRep += " p ";

      else if (square & _pieces[WHITE][ROOK]) stringRep += " R ";
      else if (square & _pieces[BLACK][ROOK]) stringRep += " r ";

      else if (square & _pieces[WHITE][KNIGHT]) stringRep += " N ";
      else if (square & _pieces[BLACK][KNIGHT]) stringRep += " n ";

      else if (square & _pieces[WHITE][BISHOP]) stringRep += " B ";
      else if (square & _pieces[BLACK][BISHOP]) stringRep += " b ";

      else if (square & _pieces[WHITE][QUEEN]) stringRep += " Q ";
      else if (square & _pieces[BLACK][QUEEN]) stringRep += " q ";

      else if (square & _pieces[WHITE][KING]) stringRep += " K ";
      else if (square & _pieces[BLACK][KING]) stringRep += " k ";
    } else {
      stringRep += " . ";
    }
    squaresProcessed++;

    if ((squaresProcessed % 8 == 0) && (squaresProcessed != 64)) {
      switch (squaresProcessed / 8) {
        case 1:
          stringRep += "        ";
          stringRep += getActivePlayer() == WHITE ? "White" : "Black";
          stringRep += " to Move";
          break;
        case 2:
          stringRep += "        Halfmove Clock: ";
          stringRep += std::to_string(_halfmoveClock);
          break;
        case 3:
          stringRep += "        Castling Rights: ";
          stringRep += _castlingRights & 1 ? "K" : "";
          stringRep += _castlingRights & 2 ? "Q" : "";
          stringRep += _castlingRights & 4 ? "k" : "";
          stringRep += _castlingRights & 8 ? "q" : "";
          break;
        case 4:
          stringRep += "        En Passant Square: ";
          stringRep += _enPassant == ZERO ? "-" : Move::indexToNotation(_bitscanForward(_enPassant));
          break;
      }
      stringRep += "\n" + std::to_string(--rank) + "  ";
      boardPos -= 16;
    }

    boardPos++;
  }

  stringRep += "\n\n    A  B  C  D  E  F  G  H";
  return stringRep;
}

void Board::_clearBitBoards() {
  for (Color color: {WHITE, BLACK}) {
    for (PieceType pieceType : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
      _pieces[color][pieceType] = ZERO;
    }

    _allPieces[WHITE] = ZERO;
    _allPieces[BLACK] = ZERO;
  }

  _enPassant = ZERO;

  _occupied = ZERO;
}

void Board::setToFen(std::string fenString, bool isFrc) {
  std::istringstream fenStream(fenString);
  std::string token;
  _gameClock = 0;
  _frc = isFrc;

  _clearBitBoards();

  U64 boardPos = 56; // Fen string starts at a8 = index 56
  fenStream >> token;
  for (auto currChar : token) {
    switch (currChar) {
      case 'p': _pieces[BLACK][PAWN] |= (ONE << boardPos++);
        break;
      case 'r': _pieces[BLACK][ROOK] |= (ONE << boardPos++);
        break;
      case 'n': _pieces[BLACK][KNIGHT] |= (ONE << boardPos++);
        break;
      case 'b': _pieces[BLACK][BISHOP] |= (ONE << boardPos++);
        break;
      case 'q': _pieces[BLACK][QUEEN] |= (ONE << boardPos++);
        break;
      case 'k': _pieces[BLACK][KING] |= (ONE << boardPos++);
        break;
      case 'P': _pieces[WHITE][PAWN] |= (ONE << boardPos++);
        break;
      case 'R': _pieces[WHITE][ROOK] |= (ONE << boardPos++);
        break;
      case 'N': _pieces[WHITE][KNIGHT] |= (ONE << boardPos++);
        break;
      case 'B': _pieces[WHITE][BISHOP] |= (ONE << boardPos++);
        break;
      case 'Q': _pieces[WHITE][QUEEN] |= (ONE << boardPos++);
        break;
      case 'K': _pieces[WHITE][KING] |= (ONE << boardPos++);
        break;
      case '/': boardPos -= 16; // Go down one rank
        break;
      default:boardPos += static_cast<U64>(currChar - '0');
    }
  }

  // Next to move
  fenStream >> token;
  _activePlayer = token == "w" ? WHITE : BLACK;

  // Castling availability
  fenStream >> token;
  _castlingRights = 0;
  for (auto currChar : token) {
    U64 rook = 0;
    int king = 0;
    switch (currChar) {
      case 'K':
        rook = _pieces[WHITE][ROOK];
        king = _bitscanForward(_pieces[WHITE][KING]);
        while (rook)
        {
            int sq = _popLsb(rook);
            if (sq > king) _castlingRights |= (ONE << sq);
        }
        break;
      case 'Q':
        rook = _pieces[WHITE][ROOK];
        king = _bitscanForward(_pieces[WHITE][KING]);
        while (rook)
        {
            int sq = _popLsb(rook);
            if (sq < king) _castlingRights |= (ONE << sq);
        }
        break;
      case 'k':
        rook = _pieces[BLACK][ROOK];
        king = _bitscanForward(_pieces[BLACK][KING]);
        while (rook)
        {
            int sq = _popLsb(rook);
            if (sq > king) _castlingRights |= (ONE << sq);
        }
        break;
      case 'q':
        rook = _pieces[BLACK][ROOK];
        king = _bitscanForward(_pieces[BLACK][KING]);
        while (rook)
        {
            int sq = _popLsb(rook);
            if (sq < king) _castlingRights |= (ONE << sq);
        }
        break;
      case 'A': _castlingRights |= (ONE << a1);
        break;
      case 'a': _castlingRights |= (ONE << a8);
        break;
      case 'B': _castlingRights |= (ONE << b1);
        break;
      case 'b': _castlingRights |= (ONE << b8);
        break;
      case 'C': _castlingRights |= (ONE << c1);
        break;
      case 'c': _castlingRights |= (ONE << c8);
        break;
      case 'D': _castlingRights |= (ONE << d1);
        break;
      case 'd': _castlingRights |= (ONE << d8);
        break;
      case 'E': _castlingRights |= (ONE << e1);
        break;
      case 'e': _castlingRights |= (ONE << e8);
        break;
      case 'F': _castlingRights |= (ONE << f1);
        break;
      case 'f': _castlingRights |= (ONE << f8);
        break;
      case 'G': _castlingRights |= (ONE << g1);
        break;
      case 'g': _castlingRights |= (ONE << g8);
        break;
      case 'H': _castlingRights |= (ONE << h1);
        break;
      case 'h': _castlingRights |= (ONE << h8);
        break;
    }
  }

  // En passant target square
  fenStream >> token;
  _enPassant = token == "-" ? ZERO : ONE << Move::notationToIndex(token);

  // Halfmove clock
  fenStream >> _halfmoveClock;
  // Our gameClock is number of plys, so multyply movenum by 2
  fenStream >> _gameClock;
  _gameClock = _gameClock * 2;


  // set up phase
  _phase = PHASE_WEIGHT_SUM;

  for (auto pieceType : {ROOK, KNIGHT, BISHOP, QUEEN}) {
    _phase -= _popCount(getPieces(WHITE, pieceType)) * PHASE_WEIGHTS[pieceType];
    _phase -= _popCount(getPieces(BLACK, pieceType)) * PHASE_WEIGHTS[pieceType];
  }

  // Make sure phase is not negative
  _phase = std::max(0, _phase);

  _updateNonPieceBitBoards();
  _zKey = ZKey(*this);
  _pawnStructureZkey.setFromPawnStructure(*this);
  _pCountKey.setFromPieceCounts(*this);

  _pst = PSquareTable(*this);
  _nnue = nullptr;

}

void Board::_updateNonPieceBitBoards() {
  _allPieces[WHITE] = _pieces[WHITE][PAWN] | \
    _pieces[WHITE][ROOK] | \
    _pieces[WHITE][KNIGHT] | \
    _pieces[WHITE][BISHOP] | \
    _pieces[WHITE][QUEEN] | \
    _pieces[WHITE][KING];

  _allPieces[BLACK] = _pieces[BLACK][PAWN] | \
    _pieces[BLACK][ROOK] | \
    _pieces[BLACK][KNIGHT] | \
    _pieces[BLACK][BISHOP] | \
    _pieces[BLACK][QUEEN] | \
    _pieces[BLACK][KING];

  _occupied = _allPieces[WHITE] | _allPieces[BLACK];
}

PieceType Board::getPieceAtSquare(Color color, int squareIndex) const {
  U64 square = ONE << squareIndex;

  PieceType piece;

  if (square & _pieces[color][PAWN]) piece = PAWN;
  else if (square & _pieces[color][ROOK]) piece = ROOK;
  else if (square & _pieces[color][KNIGHT]) piece = KNIGHT;
  else if (square & _pieces[color][BISHOP]) piece = BISHOP;
  else if (square & _pieces[color][KING]) piece = KING;
  else if (square & _pieces[color][QUEEN]) piece = QUEEN;
  else
    fatal((color == WHITE ? std::string("White") : std::string("Black")) +
        " piece at square " + std::to_string(squareIndex) + " does not exist");

  return piece;
}

void Board::_movePiece(Color color, PieceType pieceType, int from, int to) {
  U64 squareMask = to != from ? (ONE << to) | (ONE << from) : 0;

  _pieces[color][pieceType] ^= squareMask;
  _allPieces[color] ^= squareMask;

  _occupied ^= squareMask;

  // Update pawn structure ZKey if this is a pawn move
  if (pieceType == PAWN || pieceType == KING) {
    _pawnStructureZkey.movePiece(color, pieceType, from, to);
  }

  _zKey.movePiece(color, pieceType, from, to);
  _pst.movePiece(color, pieceType, from, to);
}

void Board::_removePiece(Color color, PieceType pieceType, int squareIndex) {
  U64 square = ONE << squareIndex;
  _phase += PHASE_WEIGHTS[pieceType];

  _pieces[color][pieceType] ^= square;
  _allPieces[color] ^= square;

  _occupied ^= square;

  if (pieceType == PAWN){
    _pawnStructureZkey.flipPiece(color, PAWN, squareIndex);
  }

  _pCountKey.flipPieceCount(color, pieceType, _popCount(getPieces(color, pieceType)) + 1);
  _zKey.flipPiece(color, pieceType, squareIndex);
  _pst.removePiece(color, pieceType, squareIndex);
}

void Board::_addPiece(Color color, PieceType pieceType, int squareIndex) {
  U64 square = ONE << squareIndex;
  _phase -= PHASE_WEIGHTS[pieceType];

  _pieces[color][pieceType] |= square;
  _allPieces[color] |= square;

  _occupied |= square;

  _pCountKey.flipPieceCount(color, pieceType, _popCount(getPieces(color, pieceType)));
  _zKey.flipPiece(color, pieceType, squareIndex);
  _pst.addPiece(color, pieceType, squareIndex);
}

bool Board:: isThereMajorPiece() const {
  Color active = getActivePlayer();
  return (_popCount(_allPieces[active] ^ _pieces[active][PAWN] ^ _pieces[active][KING]) > 0);
}

bool Board:: isEndGamePosition() const {
  return (_popCount(_allPieces[WHITE] ^ _pieces[WHITE][PAWN]) +
          _popCount(_allPieces[BLACK] ^ _pieces[BLACK][PAWN])) < 5;
}

U64 Board::_getLeastValuableAttacker(Color color, U64 attackers, PieceType &piece) const{

  U64 tmp = ZERO;
  //check pawns
  tmp = attackers & getPieces(color, PAWN);
  if (tmp){
    piece = PAWN;
    return ONE << _popLsb(tmp);
  }
  //check knight
  tmp = attackers & getPieces(color, KNIGHT);
  if (tmp){
    piece = KNIGHT;
    return ONE << _popLsb(tmp);
  }
  //check bishop
  tmp = attackers & getPieces(color, BISHOP);
  if (tmp){
    piece = BISHOP;
    return ONE << _popLsb(tmp);
  }
  // check ROOK
   tmp = attackers & getPieces(color, ROOK);
  if (tmp){
    piece = ROOK;
    return ONE << _popLsb(tmp);
  }
  // Check QUEEN
  tmp = attackers & getPieces(color, QUEEN);
    if (tmp){
    piece = QUEEN;
    return ONE << _popLsb(tmp);
  }
  // King
  tmp = attackers & getPieces(color, KING);
    if (tmp){
    piece = KING;
    return ONE << _popLsb(tmp);
  }
  piece = KING;
  return 0;
}

bool Board::SEE_GreaterOrEqual(const Move move, int threshold) const{

  // 0. Early exits
  // If move is special case (promotion, enpass, castle)
  // its SEE is at least 0 (well, not exactly, Prom could be -100, but still)
  // so just return true

  unsigned int flags = move.getFlags();
  if ((flags & Move::EN_PASSANT) || (flags & Move::KSIDE_CASTLE) || (flags & Move::QSIDE_CASTLE)){
       return 1024;
     }

  // 1. Set variables
  int from = move.getFrom();
  int to = move.getTo();
  Color side = getActivePlayer();
  PieceType movingPt = move.getPieceType();

  // 2. Early exits 2.
  // If we capture stuff and dont beat limit, we are done
  int value = (flags & Move::CAPTURE) ? _SEE_cost[getPieceAtSquare(getOppositeColor(side), to)] : 0;
  value -= threshold;
  if (value < 0) return false;

  // if we capture, lose a capturing piece and still beat limit,
  // we are good
  value -= _SEE_cost[movingPt];
  if (value >= 0) return true;

  // 3. Prepare variables for negamax
  // Get pieces that attack target sqv
  U64 aBoard[2];
  aBoard[WHITE] = _squareAttackedBy(WHITE, to);
  aBoard[BLACK] = _squareAttackedBy(BLACK, to);

  // get occupied
  U64 occupied = _occupied;

  U64 horiXray = getPieces(WHITE, ROOK) | getPieces(WHITE, QUEEN) |  getPieces(BLACK, ROOK) | getPieces(BLACK, QUEEN);
  U64 diagXray = getPieces(WHITE, PAWN) | getPieces(WHITE, BISHOP) | getPieces(WHITE, QUEEN) |
                 getPieces(BLACK, PAWN) | getPieces(BLACK, BISHOP) | getPieces(BLACK, QUEEN);
  U64 attBit = (ONE << from);



  occupied = occupied ^ attBit;
  if (horiXray & attBit) aBoard[side] |= (_squareAttackedByRook(side, to, occupied) & occupied);
  if (diagXray & attBit) aBoard[side] |= (_squareAttackedByBishop(side, to, occupied)  & occupied);

  side = getOppositeColor(side);

  while(true)
  {
    aBoard[getOppositeColor(side)] = aBoard[getOppositeColor(side)] & ~attBit;
    attBit = _getLeastValuableAttacker(side, aBoard[side], movingPt);
    if (!attBit) break;

    occupied = occupied ^ attBit;
    if (horiXray & attBit) aBoard[side] |= (_squareAttackedByRook(side, to, occupied) & occupied);
    if (diagXray & attBit) aBoard[side] |= (_squareAttackedByBishop(side, to, occupied)  & occupied);

    side = getOppositeColor(side);

    value = -value - 1 - _SEE_cost[movingPt];
    if (value >= 0){
       break;
    }
  }



  return side != getActivePlayer();

}

int  Board:: Calculate_SEE(const Move move) const{

  // 0. Early exits
  // If move is special case (promotion, enpass, castle)
  // its SEE is at least 0 (well, not exactly, Prom could be -100, but still)
  // so just return true

  unsigned int flags = move.getFlags();
  if ((flags & Move::EN_PASSANT) || (flags & Move::KSIDE_CASTLE) || (flags & Move::QSIDE_CASTLE)){
       return 1024;
     }


  // 1. Set variables
  int gain[32] = {0};
  int d = 0;
  int from = move.getFrom();
  int to = move.getTo();
  Color side = getActivePlayer();
  PieceType aPiece = move.getPieceType();

  // Get pieces that attack target sqv
  U64 aBoard[2];
  aBoard[WHITE] = _squareAttackedBy(WHITE, to);
  aBoard[BLACK] = _squareAttackedBy(BLACK, to);

  // get occupied
  U64 occupied = _occupied;

  U64 horiXray = getPieces(WHITE, ROOK) | getPieces(WHITE, QUEEN) |  getPieces(BLACK, ROOK) | getPieces(BLACK, QUEEN);
  U64 diagXray = getPieces(WHITE, PAWN) | getPieces(WHITE, BISHOP) | getPieces(WHITE, QUEEN) |
                 getPieces(BLACK, PAWN) | getPieces(BLACK, BISHOP) | getPieces(BLACK, QUEEN);
  U64 attBit = (ONE << from);


    gain[0] = (flags & Move::CAPTURE) ? _SEE_cost[getPieceAtSquare(getOppositeColor(side), to)] : 0;
    //std::cout <<"d"<< d << " gain[d] " << gain [d] <<std::endl;
  // 3.SEE Negamax Cycle
  do
  {
    d++;
    gain[d]  = _SEE_cost[aPiece] - gain[d-1];
    //std::cout <<"d"<< d << " gain[d] " << gain [d] << "  " << aPiece <<std::endl;
    if ( std::max(-gain[d-1], gain[d]) < 0){
      break;
    }
    aBoard[side] = aBoard[side] & ~attBit;
    occupied = occupied ^ attBit;
    if (horiXray & attBit){
      aBoard[side] |= (_squareAttackedByRook(side, to, occupied) & occupied);
    }
    if (diagXray & attBit){
      aBoard[side] |= (_squareAttackedByBishop(side, to, occupied)  & occupied);
    }
    // switch side and get next attacker
    side = getOppositeColor(side);
    attBit = _getLeastValuableAttacker(side, aBoard[side], aPiece);
    //std::cout << d << " atta " << _bitscanForward(attBit)<<std::endl;

  } while (attBit);

  // 4.Calculate value
  while (--d){
    gain[d-1] = - std::max(-gain[d-1], gain[d]);
  }

  return gain[0];
}

bool Board::doMove(Move move) {
  // Clear En passant info after each move if it exists
  if (_enPassant) {
    _zKey.clearEnPassant();
    _enPassant = ZERO;
  }
  _gameClock++;
  int from = move.getFrom();
  int to = move.getTo();
  // Handle move depending on what type of move it is
  unsigned int flags = move.getFlags();
  if (!flags) {
    // No flags set, not a special move
    _movePiece(_activePlayer, move.getPieceType(), from, to);
    // Check if we are in check after moving
    if (colorIsInCheck(_activePlayer)) return false;
    _scheduleUpdateMove(*this, _activePlayer, move.getPieceType(), from, to);
  } else if ((flags & Move::CAPTURE) && (flags & Move::PROMOTION)) { // Capture promotion special case
    // Remove captured Piece
    PieceType capturedPieceType = move.getCapturedPieceType();
    _removePiece(getInactivePlayer(), capturedPieceType, to);

    // Remove promoting pawn
    _removePiece(_activePlayer, PAWN, from);

    // Add promoted piece
    PieceType promotionPieceType = move.getPromotionPieceType();
    _addPiece(_activePlayer, promotionPieceType, to);
    if (colorIsInCheck(_activePlayer)) return false;
    _scheduleUpdateCapprom(*this, getActivePlayer(), capturedPieceType, promotionPieceType, from, to);
  } else if (flags & Move::CAPTURE) {
    // Remove captured Piece
    PieceType capturedPieceType = move.getCapturedPieceType();
    _removePiece(getInactivePlayer(), capturedPieceType, to);

    // Move capturing piece
    _movePiece(_activePlayer, move.getPieceType(), from, to);
    if (colorIsInCheck(_activePlayer)) return false;
    _scheduleUpdateCapture(*this,_activePlayer, move.getPieceType(), move.getCapturedPieceType(), from, to);
  } else if (flags & Move::KSIDE_CASTLE) {
    // Move the correct rook
    if (_activePlayer == WHITE) {
      _movePiece(_activePlayer, KING, from, g1);
      _movePiece(WHITE, ROOK, to, f1);
      if (colorIsInCheck(_activePlayer)) return false;
        _scheduleUpdateCastle(*this, _activePlayer, from, g1, to, f1);
    } else {
      _movePiece(_activePlayer, KING, from, g8);
      _movePiece(BLACK, ROOK, to, f8);
      if (colorIsInCheck(_activePlayer)) return false;
       _scheduleUpdateCastle(*this, _activePlayer, from, g8, to, f8);
    }
  } else if (flags & Move::QSIDE_CASTLE) {
    // Move the correct rook
    if (_activePlayer == WHITE) {
      _movePiece(_activePlayer, KING, from, c1);
      _movePiece(WHITE, ROOK, to, d1);
      if (colorIsInCheck(_activePlayer)) return false;
       _scheduleUpdateCastle(*this, _activePlayer, from, c1, to, d1);
    } else {
      _movePiece(_activePlayer, KING, from, c8);
      _movePiece(BLACK, ROOK, to, d8);
      if (colorIsInCheck(_activePlayer)) return false;
       _scheduleUpdateCastle(*this, _activePlayer, from, c8, to, d8);
    }
  } else if (flags & Move::EN_PASSANT) {
    // Remove the correct pawn
    if (_activePlayer == WHITE) {
      _removePiece(BLACK, PAWN, to - 8);
    } else {
      _removePiece(WHITE, PAWN, to + 8);
    }

    // Move the capturing pawn
    _movePiece(_activePlayer, move.getPieceType(), from, to);
    if (colorIsInCheck(_activePlayer)) return false;
    _scheduleUpdateEnpass(*this, _activePlayer, from, to);
  } else if (flags & Move::PROMOTION) {
    // Remove promoted pawn
    _removePiece(_activePlayer, PAWN, from);

    // Add promoted piece
    _addPiece(_activePlayer, move.getPromotionPieceType(), to);
    if (colorIsInCheck(_activePlayer)) return false;
    _scheduleUpdatePromote(*this, getActivePlayer(), move.getPromotionPieceType(), from, to);
  } else if (flags & Move::DOUBLE_PAWN_PUSH) {
    _movePiece(_activePlayer, move.getPieceType(), from, to);
    if (colorIsInCheck(_activePlayer)) return false;
    _scheduleUpdateMove(*this, _activePlayer, move.getPieceType(), from, to);

    // Set square behind pawn as _enPassant
    unsigned int enPasIndex = _activePlayer == WHITE ? to - 8 : to + 8;
    _enPassant = ONE << enPasIndex;
    _zKey.setEnPassantFile(enPasIndex % 8);
  }

  // Halfmove clock reset on pawn moves or captures, incremented otherwise
  if (move.getPieceType() == PAWN || move.getFlags() & Move::CAPTURE) {
    _halfmoveClock = 0;
  } else {
    _halfmoveClock++;
  }

  if (_castlingRights) {
    _updateCastlingRightsForMove(move);
  }

  _zKey.flipActivePlayer();
  _activePlayer = getInactivePlayer();

  // everything is fine, return
  return true;
}

void Board:: doNool(){
  // Clear En passant info after each move if it exists
  if (_enPassant) {
    _zKey.clearEnPassant();
    _enPassant = ZERO;
  }

  _zKey.flipActivePlayer();
  _activePlayer = getInactivePlayer();
}

bool Board::squareUnderAttack(Color color, int squareIndex) const {
  // Check for pawn, knight and king attacks
  if (Attacks::getNonSlidingAttacks(PAWN, squareIndex, getOppositeColor(color)) & getPieces(color, PAWN)) return true;
  if (Attacks::getNonSlidingAttacks(KNIGHT, squareIndex) & getPieces(color, KNIGHT)) return true;
  if (Attacks::getNonSlidingAttacks(KING, squareIndex) & getPieces(color, KING)) return true;

  // Check for bishop/queen attacks
  U64 bishopsQueens = getPieces(color, BISHOP) | getPieces(color, QUEEN);
  if (_getBishopAttacksForSquare(squareIndex, ZERO) & bishopsQueens) return true;

  // Check for rook/queen attacks
  U64 rooksQueens = getPieces(color, ROOK) | getPieces(color, QUEEN);
  if (_getRookAttacksForSquare(squareIndex, ZERO) & rooksQueens) return true;

  return false;
}

U64 Board::getCastlingRightsColored(Color color) const {
    return color == WHITE ? _castlingRights & RANK_1 : _castlingRights & RANK_8;
}

U64 Board::getCastlingRights() const{
    return _castlingRights;
}

U64 Board::_squareAttackedBy(Color color, int squareIndex) const {
  // Check for pawn, knight and king attacks
  U64 Attackers;

  Attackers  = Attacks::getNonSlidingAttacks(PAWN, squareIndex, getOppositeColor(color)) & getPieces(color, PAWN);
  Attackers |= Attacks::getNonSlidingAttacks(KNIGHT, squareIndex) & getPieces(color, KNIGHT);
  Attackers |= Attacks::getNonSlidingAttacks(KING, squareIndex) & getPieces(color, KING);

  // Check for bishop/queen attacks
  U64 bishopsQueens = getPieces(color, BISHOP) | getPieces(color, QUEEN);
  Attackers |= (_getBishopAttacksForSquare(squareIndex, ZERO) & bishopsQueens);

  // Check for rook/queen attacks
  U64 rooksQueens = getPieces(color, ROOK) | getPieces(color, QUEEN);
  Attackers |= (_getRookAttacksForSquare(squareIndex, ZERO) & rooksQueens);

  return Attackers;
}

U64 Board::_squareAttackedByRook(Color color, int square, U64 occupied) const{
  U64 rooksQueens = getPieces(color, ROOK) | getPieces(color, QUEEN);
  U64 Attackers = Attacks::getSlidingAttacks(ROOK, square, occupied) & rooksQueens;
  return Attackers;
}

U64 Board::_squareAttackedByBishop(Color color, int square, U64 occupied) const{
  U64 bishopsQueens = getPieces(color, BISHOP) | getPieces(color, QUEEN);
  U64 Attackers = Attacks::getSlidingAttacks(BISHOP, square, occupied) & bishopsQueens;
  return Attackers;
}

void Board::_updateCastlingRightsForMove(Move move) {
  unsigned int flags = move.getFlags();
  U64 oldCastlingRights = _castlingRights;

  // Update castling flags if rooks have been captured
  if (flags & Move::CAPTURE) {
    // Update castling rights if a rook was captured
    _castlingRights &= ~(ONE << move.getTo());
  }

  // Update castling flags if king have moved
  if (move.getPieceType() == KING){
    if(_row(move.getFrom()) == 0) _castlingRights &= ~RANK_1;
    if(_row(move.getFrom()) == 7) _castlingRights &= ~RANK_8;
  }

  // Update flasgs if rook have moved
  _castlingRights &= ~ (ONE << move.getFrom());

  _zKey.updateCastlingRights(oldCastlingRights ,_castlingRights);
}

bool Board::moveIsPseudoLegal(Move m) const{
    Color color = getActivePlayer();
    Color other = getOppositeColor(color);
    int from = m.getFrom();
    int to = m.getTo();
    PieceType pt = m.getPieceType();
    PieceType captured = m.getCapturedPieceType();
    PieceType promo = m.getPromotionPieceType();

    U64 attackable = getAttackable(other);
    U64 fromBit = (ONE << from);
    U64 toBit = (ONE << to);

    // Handle move depending on what type of move it is
    unsigned int flags = m.getFlags();

    // First bits of the move should be 0
    if (m.getMoveINT() & 0xF0000000){
        return false;
    }


    // basci test - piece actually exists
    // and moving type == type at square
    PieceType moving;
    if (fromBit & _allPieces[color]){
        moving = getPieceAtSquare(color, from);
        if (moving != pt) return false;
    }else{
        return false;
    }

  // basic checks are done, now check if the move is okay in details
  // we start with castling stuff
    if (flags == Move::KSIDE_CASTLE || flags == Move::QSIDE_CASTLE) {
        // reproduce movegen and see if move is ok
        // return if we under check
        if (colorIsInCheck(color)) return false;

        Move trueCastling;
        int kingIndex = _bitscanForward(getPieces(color, KING));

        // Add Castlings
        U64 castlingRights = getCastlingRightsColored(color);

        while(castlingRights){
            int rookSquare  = _popLsb(castlingRights);
            // no suitable castling found
            if ((rookSquare <= kingIndex && flags == Move::KSIDE_CASTLE) || (rookSquare > kingIndex && flags == Move::QSIDE_CASTLE)) continue;

            int toCastle    = color == WHITE ? rookSquare > kingIndex ? g1 : c1
                                            : rookSquare > kingIndex ? g8 : c8;
            int toRook      = color == WHITE ? rookSquare > kingIndex ? f1 : d1
                                            : rookSquare > kingIndex ? f8 : d8;
            U64 rookToKing  = Eval::detail::IN_BETWEEN[kingIndex][rookSquare];
            U64 kingJumpSq  = Eval::detail::IN_BETWEEN[kingIndex][toCastle] | (ONE << toCastle);

            // both rookToKing AND king Jump AND Rook landing must be free !!!!
            U64 toBeFree = rookToKing | kingJumpSq | (ONE << toRook);
            toBeFree = toBeFree & ~(ONE << kingIndex);
            toBeFree = toBeFree & ~(ONE << rookSquare);
            if (toBeFree & getOccupied()) return false;
            bool pathAttacked = false;

            while (kingJumpSq)
            {
                int sq = _popLsb(kingJumpSq);
                if (squareUnderAttack(getOppositeColor(color), sq)){
                    pathAttacked = true;
                    break;
                    }
            }
            Move::Flag flag = rookSquare > kingIndex ? Move::KSIDE_CASTLE : Move::QSIDE_CASTLE;
            if (!pathAttacked) trueCastling = Move(kingIndex, rookSquare, KING, flag);
        }
        if (m.getMoveINT() == trueCastling.getMoveINT()) return true; else return false;

    }
  else if (moving == PAWN){
    // single pawn moves
    // not promotions
    if ((!flags || flags == Move::DOUBLE_PAWN_PUSH) && captured == 0 && promo == 0){
          U64 movedPawn = color == WHITE ? fromBit << 8 : fromBit >> 8;
          movedPawn &= getNotOccupied();
          movedPawn &= ~PROMOTION_RANK[color];

          if (flags == Move::DOUBLE_PAWN_PUSH){
            movedPawn = color == WHITE ? movedPawn << 8 : movedPawn >> 8;
            movedPawn &= getNotOccupied();
            movedPawn &= DOUBLE_PUSH_RANK[color];
          }

          if (movedPawn & toBit){
                return true;
            }

    }else if(flags == Move::PROMOTION && captured == 0){
         if ((promo < 1 || promo > 4)) return false;

          U64 movedPawn = color == WHITE ? fromBit << 8 : fromBit >> 8;
          movedPawn &= getNotOccupied();
          movedPawn &= PROMOTION_RANK[color];

          if (movedPawn & toBit){
                return true;
            }

    }else if (flags == Move::CAPTURE || flags == (Move::CAPTURE | Move::PROMOTION)){
        // this handles both captures and capture-promotions
        U64 moves = 0;
        moves |= color == WHITE ? (fromBit << 7) & ~FILE_H : (fromBit >> 9) & ~FILE_H;
        moves |= color == WHITE ? (fromBit << 9) & ~FILE_A : (fromBit >> 7) & ~FILE_A;

        // captured piece exists
        moves &= attackable;

        // not a promotion
        if (flags & Move::PROMOTION){
          if ((promo < 1 || promo > 4)) return false;
          moves &= PROMOTION_RANK[color];
        }else{
          if (promo != 0) return false;
          moves &= ~PROMOTION_RANK[color];
        }

        // see if captured exists and proper
        if (toBit & _allPieces[other]){
            if (captured != getPieceAtSquare(other, to)){
                return false;
            }
        }

        if (moves & toBit){
            return true;
        }

    }else if (flags == Move::EN_PASSANT && promo == 0 && captured == PAWN){
        U64 moves = 0;
        moves |= color == WHITE ? (fromBit << 7) & ~FILE_H : (fromBit >> 9) & ~FILE_H;
        moves |= color == WHITE ? (fromBit << 9) & ~FILE_A : (fromBit >> 7) & ~FILE_A;

        moves &= getEnPassant();

        // see if captured exists and proper
        if (toBit & _allPieces[other]){
            if (captured != getPieceAtSquare(other, to)){
                return false;
            }
        }

        if (moves & toBit){
            return true;
        }

    }
  }
  else if (moving != PAWN){

    // normal move by non-pawn
    if (!flags && captured == 0 && promo == 0) {
        // No flags set, not a special move
        // additionally no capture or promotion piece type is set
        // NOTE - potential bug - since by default 0 is a PAWN
            U64 moves = getAttacksForSquare(moving, color, from);
            // to square is empty

            // not a capture
            moves =  moves & ~attackable;

            if (moves & toBit){
                return true;
            }

    // normal moves - captures
    }else
    if(flags == Move::CAPTURE && promo == 0){
        U64 moves = getAttacksForSquare(moving, color, from);

        // see if captured exists and proper
        if (toBit & _allPieces[other]){
            if (captured != getPieceAtSquare(other, to)){
                return false;
            }
        }

        moves = moves & attackable;
        if (moves & toBit){
            return true;
        }
    }
  }

  return false;
}

void Board::setToStartPos() {
  setToFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false);
}

U64 Board::_getWhitePawnAttacksForSquare(int square) const {
  return Attacks::getNonSlidingAttacks(PAWN, square, WHITE);
}

U64 Board::_getBlackPawnAttacksForSquare(int square) const {
  return Attacks::getNonSlidingAttacks(PAWN, square, BLACK);
}

U64 Board::_getKnightAttacksForSquare(int square, U64 own) const {
  return Attacks::getNonSlidingAttacks(KNIGHT, square) & ~own;
}

U64 Board::_getKingAttacksForSquare(int square, U64 own) const {
  return Attacks::getNonSlidingAttacks(KING, square) & ~own;
}

U64 Board::_getBishopAttacksForSquare(int square, U64 own) const {
  return Attacks::getSlidingAttacks(BISHOP, square, _occupied) & ~own;
}

U64 Board::_getRookAttacksForSquare(int square, U64 own) const {
  return Attacks::getSlidingAttacks(ROOK, square, _occupied) & ~own;
}

U64 Board::_getQueenAttacksForSquare(int square, U64 own) const {
  return Attacks::getSlidingAttacks(QUEEN, square, _occupied) & ~own;
}

U64 Board::_getBishopMobilityForSquare(int square, U64 own, U64 scanthrough) const {
  return Attacks::getSlidingAttacks(BISHOP, square, _occupied ^ scanthrough)& ~own;
}

U64 Board::_getRookMobilityForSquare(int square, U64 own, U64 scanthrough) const {
  return Attacks::getSlidingAttacks(ROOK, square, _occupied ^ scanthrough) & ~own;
}

U64 Board::_getQueenMobilityForSquare(int square, U64 own, U64 scanthrough) const {
  return Attacks::getSlidingAttacks(QUEEN, square, _occupied ^ scanthrough) & ~own;
}

U64 Board::_getKnightMobilityForSquare(int square, U64 own) const {
  return Attacks::getNonSlidingAttacks(KNIGHT, square) & ~own;
}

int Board::_getGameClock() const{
  return _gameClock;
}

int Board::getPhase() const{
  return ((std::max(0, _phase) * MAX_PHASE) + (PHASE_WEIGHT_SUM / 2)) / PHASE_WEIGHT_SUM;
}

 bool Board::getFrcMode() const{
    return _frc;
 }

 bool Board::calculateBoardDifference(U64 (* otherPieces)[2][6], int (* add)[32], int * addCount, int (* sub)[32], int * subCount){
    return false;
 }


 void Board::performUpdate(FinnyEntry (*entry)[2][NNUE_BUCKETS]){

    // already updated
    if (_updDone) return;

    // mark as updated, copy nnue and perform an update
    _updDone = true;

    // Check if full reset is needed - when we change bucket or a side.
    bool isResetNeeded = _nnue->resetNeeded(_updSchedule.movingPiece, _updSchedule.from, _updSchedule.to, _updSchedule.color);

    // Reset is needed
    if(isResetNeeded){
        int curbucket = _nnue->getCurrentBucket(_updSchedule.to, _updSchedule.color);
        // good color - > color of the accumulator that does not need to be updated
        Color goodcolor = getOppositeColor(_updSchedule.color);

        // Copy "good" part of accumulator to a new shit
        int16_t * goodhalf = _nnue->getHalfAccumulatorPtr(goodcolor);
        _nnue = _nnue + 1;
        int16_t * newhalf = _nnue->getHalfAccumulatorPtr(goodcolor);
        std::memcpy(newhalf, goodhalf, sizeof(int16_t) * NNUE_HIDDEN);

        // incrementally update good part by using half update functions
        switch (_updSchedule.type)
        {
        case NN_MOVE:
            _nnue->movePieceHalf(_updSchedule, goodcolor);
            break;
        case NN_PROMO:
            _nnue->promotePieceHalf(_updSchedule, goodcolor);
            break;
        case NN_CAPTURE:
            _nnue->capturePieceHalf(_updSchedule, goodcolor);
            break;
        case NN_CAPPROMO:
            _nnue->cappromPieceHalf(_updSchedule, goodcolor);
            break;
        case NN_CASTLE:
            //_nnue->fullReset(*this);
            _nnue->castleMoveHalf(_updSchedule, goodcolor);
            break;
        case NN_ENPASS:
            _nnue->enpassMoveHalf(_updSchedule, goodcolor);
            break;
        default:
            break;
        }

        int add[32];
        int sub[32];
        int addCount = 0;
        int subCount = 0;
        // if finny acc is ready and have reasonable amount of changes, copy and refresh
        // otherwise do half reset
        if ((*entry)[_updSchedule.color][curbucket].isReady == true &&
            calculateBoardDifference(&(* entry)[_updSchedule.color][curbucket]._pieces, &add, &addCount, &sub, &subCount)){
            memcpy(_nnue->getHalfAccumulatorPtr(_updSchedule.color), (*entry)[_updSchedule.color][curbucket]._halfHidden, sizeof(int16_t) * NNUE_HIDDEN);
            _nnue->addSubDifference(_updSchedule.color, &add, addCount, &sub, subCount);
        }else{
            // half reset "bad" part
            _nnue->halfReset(*this, _updSchedule.color);
        }


        // save to finny table
        (*entry)[_updSchedule.color][curbucket].isReady = true;
        memcpy((*entry)[_updSchedule.color][curbucket]._halfHidden, _nnue->getHalfAccumulatorPtr(_updSchedule.color), sizeof(int16_t) * NNUE_HIDDEN);
        memcpy((*entry)[_updSchedule.color][curbucket]._pieces, this->_pieces, sizeof(this->_pieces));

      return;
    }

    // Full reset is not needed
    // copy accumulator and proceed
    *(_nnue + 1) = *_nnue;
    _nnue = _nnue + 1;

    switch (_updSchedule.type)
    {
    case NN_MOVE:
        _nnue->movePiece(_updSchedule);
        break;
    case NN_PROMO:
        _nnue->promotePiece(_updSchedule);
        break;
    case NN_CAPTURE:
        _nnue->capturePiece(_updSchedule);
        break;
    case NN_CAPPROMO:
        _nnue->cappromPiece(_updSchedule);
        break;
    case NN_CASTLE:
        // this should never happens, but do a full reset just in case
        _nnue->fullReset(*this);
        //_nnue->castleMove(_updSchedule.color, _updSchedule.from, _updSchedule.to, _updSchedule.fromRook, _updSchedule.toRook);
        break;
    case NN_ENPASS:
        _nnue->enpassMove(_updSchedule);
        break;
    default:
        break;
    }

}


void Board::_scheduleUpdateMove(const Board &board, Color c, PieceType moving, unsigned int from, unsigned int to){

    _updSchedule.type = NN_MOVE;
    _updSchedule.color = c;
    _updSchedule.movingPiece = moving;
    _updSchedule.from = from;
    _updSchedule.to = to;
    _updSchedule.wKing = _bitscanForward(board.getPieces(WHITE, KING));
    _updSchedule.bKing = _bitscanForward(board.getPieces(BLACK, KING));

    _updDone = false;
}

void Board::_scheduleUpdatePromote(const Board &board, Color c, PieceType promoted, unsigned int from, unsigned int to){

    _updSchedule.type = NN_PROMO;
    _updSchedule.movingPiece = PAWN;
    _updSchedule.color = c;
    _updSchedule.promotedPiece = promoted;
    _updSchedule.from = from;
    _updSchedule.to = to;
    _updSchedule.wKing = _bitscanForward(board.getPieces(WHITE, KING));
    _updSchedule.bKing = _bitscanForward(board.getPieces(BLACK, KING));

    _updDone = false;
}

void Board::_scheduleUpdateCapprom(const Board &board, Color c, PieceType captured, PieceType promoted, unsigned int from, unsigned int to){

    _updSchedule.type = NN_CAPPROMO;
    _updSchedule.movingPiece = PAWN;
    _updSchedule.color = c;
    _updSchedule.capturedPiece = captured;
    _updSchedule.promotedPiece = promoted;
    _updSchedule.from = from;
    _updSchedule.to = to;
    _updSchedule.wKing = _bitscanForward(board.getPieces(WHITE, KING));
    _updSchedule.bKing = _bitscanForward(board.getPieces(BLACK, KING));

    _updDone = false;
}

void Board::_scheduleUpdateCapture(const Board &board, Color c, PieceType moving, PieceType captured, unsigned int from, unsigned int to){

    _updSchedule.type = NN_CAPTURE;
    _updSchedule.color = c;
    _updSchedule.movingPiece = moving;
    _updSchedule.capturedPiece = captured;
    _updSchedule.from = from;
    _updSchedule.to = to;
    _updSchedule.wKing = _bitscanForward(board.getPieces(WHITE, KING));
    _updSchedule.bKing = _bitscanForward(board.getPieces(BLACK, KING));

    _updDone = false;
}

void Board::_scheduleUpdateCastle(const Board &board, Color c, unsigned int from, unsigned int to, unsigned int fromR, unsigned int toR){

    _updSchedule.type = NN_CASTLE;
    _updSchedule.movingPiece = KING;
    _updSchedule.color = c;
    _updSchedule.from = from;
    _updSchedule.to = to;
    _updSchedule.fromRook = fromR;
    _updSchedule.toRook = toR;

    _updSchedule.wKing = _bitscanForward(board.getPieces(WHITE, KING));
    _updSchedule.bKing = _bitscanForward(board.getPieces(BLACK, KING));

    _updDone = false;
}

void Board::_scheduleUpdateEnpass(const Board &board, Color c, unsigned int from, unsigned int to){

    _updSchedule.type = NN_ENPASS;
    _updSchedule.movingPiece = PAWN;
    _updSchedule.color = c;
    _updSchedule.from = from;
    _updSchedule.to = to;
    _updSchedule.wKing = _bitscanForward(board.getPieces(WHITE, KING));
    _updSchedule.bKing = _bitscanForward(board.getPieces(BLACK, KING));

    _updDone = false;
}

void Board::_scheduleUpdateEmpty(){
    _updDone = false;
}

