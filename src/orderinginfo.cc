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
#include "orderinginfo.h"
#include <cstring>

OrderingInfo::OrderingInfo() {
  clearAllHistory();
}

void OrderingInfo::clearAllHistory(){
  std::memset(_history, 0, sizeof(_history));
  std::memset(_captureHistory, 0, sizeof(_captureHistory));
  std::memset(_counterMove, 0, sizeof(_counterMove));
  std::memset(_counterMoveHistory, 0, sizeof(_counterMoveHistory));
  std::memset(_killer1, 0, sizeof(_killer1));
  std::memset(_killer2, 0, sizeof(_killer2));
}

void OrderingInfo::clearKillers(){
  std::memset(_killer1, 0, sizeof(_killer1));
  std::memset(_killer2, 0, sizeof(_killer2));
}

void OrderingInfo::clearChildrenKillers(int ply){
  _killer1[ply + 2] = 0;
  _killer2[ply + 2] = 0;
}

void OrderingInfo::updateCounterMove(Color color, int counteredMove, int counterMove){
  int pType = counteredMove & 0x7;
  int to = (counteredMove >> 15) & 0x3f;
  _counterMove[color][pType][to] = counterMove;
}

int OrderingInfo::getCounterMoveINT(Color color, int pMove) const{
  int type = pMove & 0x7;
  int to = (pMove >> 15) & 0x3f;
  return _counterMove[color][type][to];
}

// currently use formula clamps history between (-16384 and 16384)
void OrderingInfo::incrementHistory(Color color, int from, int to, int depth) {
  int16_t current = _history[color][from][to];
  int16_t bonus   = depth * depth;
  _history[color][from][to] += 32 * bonus - current * abs(bonus) / 512;
}

void OrderingInfo::decrementHistory(Color color, int from, int to, int depth) {
  int16_t current = _history[color][from][to];
  int16_t bonus   = -1 * depth * (depth - 1);
  _history[color][from][to] += 32 * bonus - current * abs(bonus) / 512;
}

void OrderingInfo::incrementCapHistory(PieceType capturingPiece, PieceType capturedPiece, int to, int depth){
  int16_t current = _captureHistory[capturingPiece][capturedPiece][to];
  int16_t bonus   = depth * depth;
  _captureHistory[capturingPiece][capturedPiece][to] += 32 * bonus - current * abs(bonus) / 512;
}

void OrderingInfo::decrementCapHistory(PieceType capturingPiece, PieceType capturedPiece, int to, int depth){
  int16_t current = _captureHistory[capturingPiece][capturedPiece][to];
  int16_t bonus   = -1 * depth * depth;
  _captureHistory[capturingPiece][capturedPiece][to] += 32 * bonus - current * abs(bonus) / 512;
}

void OrderingInfo::incrementCounterHistory(Color color, int pMove, PieceType pType, int to, int depth){
  int16_t current = _counterMoveHistory[color][cmhCalculateIndex(pMove)][pType][to];
  int16_t bonus   = 4 * depth * depth;
  _counterMoveHistory[color][cmhCalculateIndex(pMove)][pType][to] += 32 * bonus - current * abs(bonus) / 512;
}

void OrderingInfo::decrementCounterHistory(Color color, int pMoveIndx, PieceType pType, int to, int depth){
  int16_t current = _counterMoveHistory[color][pMoveIndx][pType][to];
  int16_t bonus   = -1 * depth * depth;
  _counterMoveHistory[color][pMoveIndx][pType][to] += 32 * bonus - current * abs(bonus) / 512;
}

int OrderingInfo::getHistory(Color color, int from, int to) const {
  return _history[color][from][to];
}

int OrderingInfo::getCaptureHistory(PieceType capturingPiece, PieceType capturedPiece, int to) const{
  return _captureHistory[capturingPiece][capturedPiece][to];
}

int OrderingInfo::getCountermoveHistory(Color color, int pMoveIndx, PieceType pType, int to) const{
  return _counterMoveHistory[color][pMoveIndx][pType][to];
}

void OrderingInfo::updateKillers(int ply, Move move) {
  int t = move.getMoveINT();
  if (!(_killer1[ply] == t)){
      _killer2[ply] = _killer1[ply];
      _killer1[ply] = t;
  }

}

  void OrderingInfo::incrementCorrectionHistory(Color color, U64 pawnkey, int depth, int diff){
    int current = _correctionHistory[color][pawnkey % 16384];
    int weight  = std::min(depth, 16);
    int scaled_diff = diff * CORRECTION_HISTORY_GRAIN;

    int update =  current * (CORRECTION_HISTORY_WEIGHT_SCALE - weight) + scaled_diff * weight;

    int newcorr = std::max(-CORRECTION_HISTORY_MAX, update / CORRECTION_HISTORY_WEIGHT_SCALE);
    newcorr = std::min(CORRECTION_HISTORY_MAX, update / CORRECTION_HISTORY_WEIGHT_SCALE);

    _correctionHistory[color][pawnkey % 16384] = newcorr;

  }

int OrderingInfo::getKiller1(int ply) const {
  return _killer1[ply];
}

int OrderingInfo::getKiller2(int ply) const {
  return _killer2[ply];
}

int OrderingInfo::getCorrHistory(Color color, U64 pawnkey) const{
    return _correctionHistory[color][pawnkey % 16384] / CORRECTION_HISTORY_WEIGHT_SCALE;
}