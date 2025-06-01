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
#include "search.h"
#include "eval.h"
#include "movepicker.h"
#include "searchdata.h"
#include <cstring>
#include <thread>
#include <algorithm>
#include <iostream>
#include <math.h>


extern int myTHREADSCOUNT;
extern Search         * cSearch[MAX_THREADS];
extern std::thread      cThread[MAX_THREADS];
extern HASH           * myHASH;


void Search::init_LMR_array(){

  // 1. Initialization of the LMR_array.
  // Original formula, came up after plotting  Weiss formula and trying to came up with
  // something similar, but based on the pow (x,y) function for easier tuning later.

  for (int depth = 0; depth < 34; depth++){
    for (int movenum = 0; movenum < 34; movenum++){
      _lmr_R_array[depth][movenum] = (int) (0.57 + (pow(depth, 0.10) * pow(movenum, 0.16))/2.49);
    }
  }
  // 2. Initialization of the LMP array.
  // Current formula is completely based on the Weiss chess engine.
  for (int depth = 0; depth < MAX_PLY; depth++){
    _lmp_Array[depth][0] = (int)  (1.57 + pow((depth - 1), 2) * 1.71);
    _lmp_Array[depth][1] = (int)  (3.51 + pow((depth - 1), 2) * 1.73);
  }

}

Search::Search(const Board &board, Limits limits, Hist positionHistory, OrderingInfo *info, bool logUci) :
    _orderingInfo(*info),
    _timer(limits, board.getActivePlayer(), board._getGameClock() / 2),
    _initialBoard(board),
    _logUci(logUci),
    _stop(false),
    _nodes(0),
    _bestScore(0)
     {

  _sStack = SEARCH_Data();
  _posHist = positionHistory;
  _nnStack[0] = NNueEvaluation(_initialBoard);
  _initialBoard.setNnuePtr(&_nnStack[0]);
  init_LMR_array();
}

void Search::iterDeep() {

  _nodes = 0;
  _selDepth = 0;
  std::memset(_rootNodesSpent, 0, sizeof(_rootNodesSpent));
  _timer.startIteration();
  int maxDepthSearched = 0;

  int targetDepth = _timer.getSearchDepth();
  int aspWindow = 30;
  int aspDelta  = 48;

    for (int currDepth = 1; currDepth <= targetDepth; currDepth++) {
        maxDepthSearched = std::max(maxDepthSearched, currDepth);


        int aspAlpha = LOST_SCORE;
        int aspBeta  =-LOST_SCORE;
        if (currDepth > 6){
            aspAlpha = _bestScore - aspWindow;
            aspBeta  = _bestScore + aspWindow;
        }

        while (true){

            int score = _rootMax(_initialBoard, aspAlpha, aspBeta, currDepth);

            if (_stop) break;

            if (score <= aspAlpha){
                aspAlpha = std::max(aspAlpha - aspDelta, LOST_SCORE);
            }else if( score >= aspBeta){
                aspBeta  = std::min(aspBeta + aspDelta, -LOST_SCORE);
            }else{
                break;
            }

            aspDelta += aspDelta * 2 / 3;
        }

        // Iteration finished normally
        // Check and adjust time we should spend, and print UCI info

        if (_stop) break;

        int elapsed = 0;
        bool shouldStop = _timer.finishOnThisDepth(&elapsed, _nodes, _rootNodesSpent[_bestMove.getPieceType()][_bestMove.getTo()]);
        if (_logUci) {
            _logUciInfo(_getPv(), currDepth, _bestScore, _nodes, elapsed);
        }

        if (shouldStop) break;

    }

  // It can be the case where we exited before finishing iteration, and our PV etc can changed.
  // So update search info one more time
  if (_logUci) _logUciInfo(_getPv(), maxDepthSearched, _bestScore, _nodes, _timer.getElapsed());

  if (_logUci) std::cout << "bestmove " << getBestMove().getNotation(_initialBoard.getFrcMode()) << std::endl;

  if (_logUci){

    //send all other thread stop signal
    for (int i = 1; i < myTHREADSCOUNT; i++){
      if ( cSearch[i] != nullptr){
        cSearch[i]->stop();
      }
    }

    // wait for extra threads
    for (int i = 1; i < myTHREADSCOUNT; i++){
      if (cThread[i].joinable()){
        cThread[i].join();
      }
    }

    // threads finished, delete extensive Searches
    for (int i = 1; i < myTHREADSCOUNT; i++){
      delete cSearch[i];
      cSearch[i] = nullptr;
    }
}

}

MoveList Search::_getPv() {
  MoveList pv;
  for (int i = 0; i < _ourPV.length; i++){
    pv.push_back(Move(_ourPV.pVmoves[i]));
  }

  return pv;
}

void Search::_logUciInfo(const MoveList &pv, int depth, int bestScore, U64 nodes, int elapsed) {
  std::string pvString;

  for (auto move : pv) {
    pvString += move.getNotation(_initialBoard.getFrcMode()) + " ";
  }

  std::string scoreString;
  if (abs(bestScore) >= WON_IN_X) {
    int dist = (-LOST_SCORE - abs(bestScore) + 1) / 2;
    scoreString = "mate " + std::to_string(bestScore > 0 ? dist : -dist);
  }else if (abs(bestScore) <= 7){
   scoreString = "cp " + std::to_string(0);
  }else{
    scoreString = "cp " + std::to_string(bestScore);
  }

  // Avoid divide by zero errors with nps
  elapsed++;
  // Avoid _selDepth being smaller than depth when entire path to score is in TT
  _selDepth = std::max(depth, _selDepth);

  //collect info about nodes and seldepth from all Threads
  for (int i = 1; i < myTHREADSCOUNT; i++){
    if (cSearch[i] != nullptr){
      nodes += cSearch[i]->getNodes();
      _selDepth = std::max(cSearch[i]->getSeldepth(), _selDepth);
    }
  }

  std::cout << "info depth " + std::to_string(depth) + " ";
  std::cout << "seldepth " + std::to_string(_selDepth) + " ";
  std::cout << "nodes " + std::to_string(nodes) + " ";
  std::cout << "score " + scoreString + " ";
  std::cout << "nps " + std::to_string((nodes / elapsed) * 1000)  + " ";
  std::cout << "time " + std::to_string(elapsed) + " ";
  std::cout << "pv " + pvString;
  std::cout << std::endl;
}

void Search::stop() {
  _stop = true;
}

Move Search::getBestMove() {
  return _bestMove;
}

int Search::getNodes(){
  return _nodes;
}

int Search::getSeldepth(){
  return _selDepth;
}

int Search::getBestScore(){
  return _bestScore;
}

bool Search::_checkLimits() {

  if (_nodes % 2048 != 0) {
    return false;
  }

  return _timer.checkLimits(_nodes);
}

inline int Search::_makeCmhBonus(int bonus){
    return std::min(MAX_HISTORY_SCORE, bonus * 4);
}

inline int Search::_getHistoryBonus(int depth, int eval, int alpha){
    // initial bonus is depth
    int bonus = depth;

    //modify
    bonus += 2 * (eval < alpha);
    return std::min(MAX_HISTORY_SCORE, 32 * bonus * bonus);
}

int Search::_getHistoryPenalty(int depth, int eval, int alpha, int pmScore, bool ttNode, bool cutNode, CutOffState ttCut){
    // initial penalty is depth
    int penalty = depth;

    // modify
    penalty -= (eval < alpha);
    penalty -= (!ttNode && depth >= 4);
    penalty -= ttCut == ALPHA;
    penalty += (pmScore < -HALFMAX_HISTORY_SCORE);
    penalty += cutNode;

    penalty = std::max(0, penalty);
    return std::max(-MAX_HISTORY_SCORE, -32 * penalty * (penalty - 1));
}

inline void Search::_updateBeta(bool isQuiet, const Move move, Color color, int pMove, int ply, int bonus){
	if (isQuiet) {
    _orderingInfo.updateKillers(ply, move);
    _orderingInfo.incrementHistory(color, move.getFrom(), move.getTo(), bonus);
    _orderingInfo.updateCounterMove(color, pMove, move.getMoveINT());
    _orderingInfo.incrementCounterHistory(color, pMove, move.getPieceType(), move.getTo(), _makeCmhBonus(bonus));
  }else{
    _orderingInfo.incrementCapHistory(move.getPieceType(), move.getCapturedPieceType(), move.getTo(), bonus);
  }
}

inline bool Search::_isRepetitionDraw(U64 currKey, int untillFifty){
  for (int i = _posHist.head - 2; (i >= 0 || i > _posHist.head - 2 - untillFifty); i-=2){
    if (_posHist.hisKey[i] == currKey){
      return true;
    }
  }
  return false;
}

inline int Search::_makeDrawScore(){
    return (_nodes & 0x7);
}

int Search::_rootMax(const Board &board, int alpha, int beta, int depth) {
  _nodes++;
  int nodeEval = Eval::evaluate(board, board.getActivePlayer());
  int hashMove = 0;
  int currScore;
  pV rootPV = pV();
  Move bestMove;
  bool fullWindow = true;

  // Load TT
  const HASH_Entry ttEntry = myHASH->HASH_Get(board.getZKey().getValue());
  hashMove = ttEntry.Flag != NONE ? ttEntry.move : 0;

  _sStack.AddEval(nodeEval);

  MovePicker movePicker(&_orderingInfo, &board, hashMove, board.getActivePlayer(), 0, 0);

  while (movePicker.hasNext()) {
    Move move = movePicker.getNext();

    Board movedBoard = board;
    bool isLegal = movedBoard.doMove(move);

    if (isLegal){
        myHASH->HASH_Prefetch(movedBoard.getZKey().getValue());
        _sStack.AddMove(move);
        U64 nodesStart = _nodes;

        if (fullWindow) {
          currScore = -_negaMax(movedBoard, &rootPV, depth - 1, -beta, -alpha, false, false);
        } else {
          currScore = -_negaMax(movedBoard, &rootPV, depth - 1, -alpha - 1, -alpha,  false, true);
          if (currScore > alpha) currScore = -_negaMax(movedBoard, &rootPV, depth - 1, -beta, -alpha, false, false);
        }

        if (_stop || _checkLimits()) {
          _stop = true;
          break;
        }

        // If the current score is better than alpha, or this is the first move in the loop
        if (currScore > alpha) {
          fullWindow = false;
          bestMove = move;
          alpha = currScore;
          _ourPV.length = rootPV.length + 1;
          _ourPV.pVmoves[0] = move.getMoveINT();
          // memcpy - (куда, откуда, длина)
          std::memcpy(_ourPV.pVmoves + 1, rootPV.pVmoves, sizeof(int) * rootPV.length);
          // Break if we've found a checkmate
        }
        _rootNodesSpent[move.getPieceType()][move.getTo()] += _nodes - nodesStart;
        _sStack.Remove();
    }

  }

  if (!_stop && !(bestMove.getFlags() & Move::NULL_MOVE)) {
    myHASH->HASH_Store(board.getZKey().getValue(), bestMove.getMoveINT(), EXACT, alpha, depth, 0);
    _bestMove = bestMove;
    _bestScore = alpha;
  }

  return alpha;
}

int Search::_negaMax(Board &board, pV *up_pV, int depth, int alpha, int beta, bool singSearch, bool cutNode) {
  bool incheckNode;
  bool ttNode = false;
  bool qttNode = false;
  bool fnNode = false;
  bool improving = false;
  bool singNode = false;
  bool nmpTree = _sStack.nmpTree;
  bool pvNode = alpha != beta - 1;
  bool endgameNode = board.isEndGamePosition();
  int score;
  int ply = _sStack.ply;
  int pMove = _sStack.moves[ply - 1].getMoveINT();
  int pMoveScore = _sStack.moves[ply - 1].getValue();
  int pMoveIndx = cmhCalculateIndex(pMove);
  int alphaOrig = alpha;
  int nodeEval = NOSCORE;
  int  legalCount = 0;
  int  qCount = 0;
  Move ttMove = Move(0);
  Move bestMove;
  pV   thisPV = pV();
  up_pV->length = 0;
  Color behindColor = _sStack.sideBehind;

  bool isPmQuietCounter = (pMoveScore >= 50000 && pMoveScore <= 200000);

  _nodes++;
  // Check if we are out of time
  if (_stop || _checkLimits()) {
    _stop = true;
    return 0;
  }

  // Check for threefold repetition draws and 50 - move rule draw
  // cut pV out if we found draw
  if (board.getHalfmoveClock() >= 100 || _isRepetitionDraw(board.getZKey().getValue(), board.getHalfmoveClock())) {
    return _makeDrawScore();
  }

  // Check our InCheck status
  incheckNode = board.colorIsInCheck(board.getActivePlayer());

  // Go into the QSearch if depth is 0 and we are not in check
  // Cut out pV and update our seldepth before dropping into qSearch
  if ((depth <= 0 && !incheckNode) || ply >= MAX_PLY) {
    _selDepth = std::max(ply, _selDepth);
    return _qSearch(board, alpha, beta);
  }

    // Check transposition table cache
  // If TT is causing a cuttoff, we update move ordering stuff
  const HASH_Entry ttEntry = myHASH->HASH_Get(board.getZKey().getValue());
  if (ttEntry.Flag != NONE){
    ttNode = true;
    ttMove = Move(ttEntry.move);
    qttNode = ttMove.isQuiet();
    if (ttEntry.depth >= depth && !pvNode && !singSearch){
      int hashScore = ttEntry.score;

      if (abs(hashScore) > WON_IN_X){
        hashScore = (hashScore > 0) ? (hashScore - ply) :  (hashScore + ply);
      }

      if (ttEntry.Flag == EXACT){
        return hashScore;
      }
      if (ttEntry.Flag == BETA && hashScore >= beta){
        int bonus = _getHistoryBonus(depth, 0, 0);
        _updateBeta(qttNode, ttMove, board.getActivePlayer(), pMove, ply, bonus);
        return beta;
      }

      if (ttEntry.Flag == ALPHA && hashScore <= alpha){
        return alpha;
      }
    }
  }

  // Statically evaluate our position
  // Do the Evaluation, unless we are in check or prev move was NULL
  // If last Move was Null, just negate prev eval and add 2x tempo bonus (10)

  board.performUpdate();
  nodeEval = Eval::evaluate(board, board.getActivePlayer());
  _sStack.AddEval(nodeEval);



    // Use static evaluation difference to improve quiet move ordering
    // Stolen from SF (hello Viz)
    if (pMove != 0 && _sStack.moves[ply - 1].isQuiet())
    {
        int bonus = -10 * (_sStack.statEval[ply - 1] + nodeEval);
        bonus = std::max(-2250, bonus);
        bonus = std::min(bonus, 1500);
        _orderingInfo.incrementHistory(getOppositeColor(board.getActivePlayer()), _sStack.moves[ply - 1].getFrom(), _sStack.moves[ply - 1].getTo(), bonus);

    }


  // Check if we are improving
  // The idea is if we are not improving in this line we probably can prune a bit more

  if (ply >= 2){
    improving = nodeEval > _sStack.statEval[ply - 2];
  }

  // Clear Killers for the children node
  _orderingInfo.clearChildrenKillers(ply);

  // Check if we are doing pre-move pruning techniques
  // We do not do them InCheck, in pvNodes and when proving singularity
  bool isPrune = !pvNode && !incheckNode && !singSearch;

  // 2. REVERSE FUTILITY
  // The idea is so if we are very far ahead of beta at low
  // depth, we can just return estimated eval (eval - margin),
  // because beta probably will be beaten
  if (isPrune && depth <= 8 && ((nodeEval - 161 * depth + 142 * improving) >= beta)){
      return beta;
  }

  // 3. NULL MOVE
  // If we are doing so well, that giving opponent 2 moves wont improve his position we can safely prune this position.
  // No nmp in pvNode, InCheck, when doing singular, or just after Null move was made
  // Use SF-like conditional of requsting Eval being higher than beta at low depth
  // Equisetum track NMP_failure to use for extending decisions
  if (isPrune && pMove != 0 && nodeEval >= beta + std::max(0, 118 - 21 * depth) && board.isThereMajorPiece()){
          Board movedBoard = board;
          _posHist.Add(board.getZKey().getValue());
          _sStack.AddNullMove(getOppositeColor(board.getActivePlayer()));
          movedBoard.doNool();

          int fDepth = depth - NULL_MOVE_REDUCTION - depth / 4 - std::min((nodeEval - beta) / 128, 5);
          int score = -_negaMax(movedBoard, &thisPV, fDepth , -beta, -beta + 1, false, false);

          _posHist.Remove();
          _sStack.RemoveNull(behindColor, nmpTree);

          if (score >= beta){
            return beta;
          }
          fnNode = true;
  }

  // 4. UN_HASHED REDUCTION
  // We reduce depth by 1 if the position we currently analysing isnt hashed.
  // Based on talkchess discussion, replaces Internal iterative deepening.
  // The justification is if our hashing is decent, if the
  // position at high depth isnt here, its probably position not worth searching
  //
  // Equisetum dont do this reduction after NullMove, because we already reduced a lot,
  // and reducing further may reduce quality of the NM_Search
  if (depth >= 5 && !ttNode && pMove != 0 && !singSearch)
    depth--;


  // Probcut
  if (!pvNode &&
       depth >= 4 &&
       alpha < WON_IN_X){
        int pcBeta = beta + 218 - 100 * improving;

        MovePicker pcPicker(&_orderingInfo, &board, 0, board.getActivePlayer(), MAX_PLY, 0);
        while (pcPicker.hasNext()){
            Move move = pcPicker.getNext();

            // when proving singularity, dont try ttmove
            if (move == ttEntry.move && singSearch){
                continue;
            }

            // exit when there is no more good captures
            if (move.getValue() <= 300000){
                break;
            }

            // make a move
            Board movedBoard = board;
            bool isLegal = movedBoard.doMove(move);
            if (isLegal){
                // see if qSearch holds
                int qScore = - _qSearch(movedBoard, -pcBeta, -pcBeta + 1);

                // if it holds, do proper reduced search
                if(qScore >= pcBeta){
                    _posHist.Add(board.getZKey().getValue());
                    _sStack.AddMove(move);

                    int sScore = -_negaMax(movedBoard, &thisPV, depth - 4, -pcBeta, -pcBeta + 1, false, !cutNode);

                    _posHist.Remove();
                    _sStack.Remove();

                    if (sScore >= pcBeta){
                        return beta;
                    }
                }
            }
        }
    }

  // Initiate normal picker and proceed
  MovePicker movePicker(&_orderingInfo, &board, ttMove.getMoveINT(), board.getActivePlayer(), ply, pMove);

  while (movePicker.hasNext()) {
    Move move = movePicker.getNext();
    if (move == ttEntry.move && singSearch){
      continue;
    }
    bool isQuiet = move.isQuiet();
    qCount += isQuiet;

    int  moveHistory  = isQuiet ?
                        _orderingInfo.getHistory(board.getActivePlayer(), move.getFrom(), move.getTo()) :
                        _orderingInfo.getCaptureHistory(move.getPieceType(), move.getCapturedPieceType(), move.getTo());
    int cmHistory     = isQuiet ? _orderingInfo.getCountermoveHistory(board.getActivePlayer(), pMoveIndx, move.getPieceType(), move.getTo()) : 0;

    // 5. PRE-MOVELOOP PRUNING

    if (alpha < WON_IN_X
        && legalCount >= 1){

      // 5.1 LATE MOVE PRUNING
      // If we made many quiet moves in the position already
      // we suppose other moves wont improve our situation
      if ((qCount > _lmp_Array[depth][(improving || pvNode)]) && (moveHistory + cmHistory <= 0)) break;

      // 5.2. SEE pruning of quiet moves
      // At shallow depth prune highlyish -negative SEE-moves
      if (depth <= 10
          && isQuiet
          && !board.SEE_GreaterOrEqual(move, (-68 * depth + 48))) continue;

      if (depth <= 6
          && !isQuiet
          && !board.SEE_GreaterOrEqual(move, (-150 * depth + 100))) continue;

      // 5.3. COUNTER-MOVE HISTORY PRUNING
      // Prune quiet moves with poor CMH on the tips of the tree
      if (depth <= 3 && isQuiet && cmHistory <= (-4096 * depth + 4096)) continue;
    }


        int tDepth = depth;
        // 6. EXTENTIONS
        //

        // 6.1 Singular move extention
        // At high depth if we have the TT move, and we are certain
        // that non other moves are even close to it, extend this move
        // At low depth use statEval instead of search (Kimmys idea)
        if (ttEntry.Flag != ALPHA &&
            ttEntry.depth >= depth - 3 &&
            ttEntry.move == move.getMoveINT() &&
            abs(ttEntry.score) < WON_IN_X / 4){
              int sDepth = depth / 2;
              int sBeta = ttEntry.score - depth;
              int score = depth > 5 ? _negaMax(board, &thisPV, sDepth, sBeta - 1, sBeta, true, cutNode) : nodeEval;
              if (sBeta > score){
                tDepth += 1 + (!pvNode && depth > 5);
                singNode = true;
              }else if(!incheckNode && depth > 5 && ttEntry.score >= beta){
                tDepth -= 2;
              }else if (!incheckNode && depth > 5 && cutNode){
                tDepth -= 1;
              }
            }

        // 6.2. Passed pawn push extention
        // In the late game  we fear that we may miss
        // some pawn promotions near the leafs of the search tree
        // Thus we extend in the endgame pushes of the non-blocked
        // passers that are near the middle of the board
        // Extend more if null move failed
        if (depth <= 8 &&
            endgameNode &&
            move.isItPasserPush(board) &&
            ttEntry.move != move.getMoveINT()){
              tDepth++;
            }

        // 6.3 Last capture extention
        // In the endgame positions we extend any non-pawn captures
        // It seems benefitial as we calculate resulting endgame more accurately
        if (!isQuiet &&
            endgameNode &&
            move.getCapturedPieceType() != PAWN &&
            ttEntry.move != move.getMoveINT()){
              tDepth++;
            }

    Board movedBoard = board;
    bool isLegal = movedBoard.doMove(move);
    if (isLegal){
        myHASH->HASH_Prefetch(movedBoard.getZKey().getValue());
        bool doLMR = false;
        legalCount++;
        int score;

        bool giveCheck = movedBoard.colorIsInCheck(movedBoard.getActivePlayer());


        _posHist.Add(board.getZKey().getValue());
        _sStack.AddMove(move);

        // 8. LATE MOVE REDUCTIONS
        // mix of ideas from Weiss code, own ones and what is written in the chessprogramming wiki
        doLMR = tDepth > 2 && legalCount > 2 + pvNode;
        if (doLMR){

          //Basic reduction is done according to the array
          int reduction = _lmr_R_array[std::min(33, tDepth)][std::min(33, legalCount)];

          // Reduction tweaks
          // We generally want to guess if the move will not improve alpha and guess right to do no re-searches

          // if move is quiet, reduce a bit more (from Weiss)
          reduction += isQuiet;

          //reduce more when side to move is in check
          reduction += incheckNode;

          // Reduce more for late quiets if ttNode exists and it is non-Quiet move
          reduction += isQuiet && !qttNode && ttNode;

          // Reduce more when side-to-move was behind prior to NMP on the previous NMP try
          // Basically copy-pasted Koivisto idea
          reduction += isQuiet && nmpTree && board.getActivePlayer() == behindColor;

          // Reduce more in the cut-nodes - used by SF/Komodo/etc
          reduction += cutNode;

          // Reduce less if move on the previous ply was bad
          // Ie hystorycally bad quiet, see- capture or underpromotion
          reduction -= pMoveScore < -HALFMAX_HISTORY_SCORE;

          // if we are improving, reduce a bit less (from Weiss)
          reduction -= improving;

          // reduce less when a move is giving check
          reduction -= giveCheck;

          // reduce less for a position where singular move exists
          reduction -= singNode;

          // reduce more/less based on the hitory
          reduction -= moveHistory / HALFMAX_HISTORY_SCORE;
          reduction -= cmHistory  / HALFMAX_HISTORY_SCORE;

          // reduce less when move is a Queen promotion
          reduction -= (move.getFlags() & Move::PROMOTION) && (move.getPromotionPieceType() == QUEEN);

          // Reduce less for CounterMove and both Killers
          reduction -= 2 * (move.getMoveINT() == _orderingInfo.getCounterMoveINT(board.getActivePlayer(), pMove) ||
                            move == _orderingInfo.getKiller1(ply) ||  move == _orderingInfo.getKiller2(ply));

          // We finished reduction tweaking, calculate final depth and search
          // Idea from SF - > allow extending if our reductions are very negative
          int minReduction = (!isQuiet && legalCount <= 6) ? -2 :
                             (cutNode || pvNode) ? -1 : 0;

          reduction = std::max(minReduction, reduction);
          //Avoid to reduce so much that we go to QSearch right away
          int fDepth = std::max(1, tDepth - 1 - reduction);

          //Search with reduced depth around alpha in assumtion
          // that alpha would not be beaten here
          score = -_negaMax(movedBoard, &thisPV, fDepth, -alpha - 1 , -alpha, false, true);
        }

        // Code here is restructured based on Weiss
        // First part is clear here: if we did LMR and score beats alpha
        // We need to do a re-search.
        //
        // If we did not do LMR: if we are in a non-PV our we already have alpha == beta - 1,
        // and if we are searching 2nd move and so on we already did full window search -
        // So for both of this cases we do limited window search.
        if (doLMR){
          if (score > alpha){
            score = -_negaMax(movedBoard, &thisPV, tDepth - 1, -alpha - 1, -alpha, false, !cutNode);
          }
        } else if (!pvNode || legalCount > 1){
          score = -_negaMax(movedBoard, &thisPV, tDepth - 1, -alpha - 1, -alpha, false, !cutNode);
        }

        // If we are in the PV
        // Search with a full window the first move to calculate bounds
        // or if score improved alpha during the current round of search.
        if  (pvNode) {
          if ((legalCount == 1) || (score > alpha && score < beta)){
            score = -_negaMax(movedBoard, &thisPV, tDepth - 1, -beta, -alpha, false, false);
          }
        }

        _posHist.Remove();
        _sStack.Remove();
        // Beta cutoff
        if (score >= beta) {
          // Add this move as a new killer move and update history if move is quiet
          int bonus = _getHistoryBonus(depth, nodeEval, alpha);
          _updateBeta(isQuiet, move, board.getActivePlayer(), pMove, ply, bonus);
          // Award counter-move history additionally if we refuted special quite previous move
          if (isPmQuietCounter) _orderingInfo.incrementCounterHistory(board.getActivePlayer(), pMove, move.getPieceType(), move.getTo(), _makeCmhBonus(bonus));
          // Add a new tt entry for this node
          if (!_stop && !singSearch){
            myHASH->HASH_Store(board.getZKey().getValue(), move.getMoveINT(), BETA, score, depth, ply);
          }
          // we updated beta and in the pVNode so we should update our pV
          if (pvNode && !_stop){
            up_pV->length = thisPV.length + 1;
            up_pV->pVmoves[0] = move.getMoveINT();
            // memcpy - (куда, откуда, длина)
            std::memcpy(up_pV->pVmoves + 1, thisPV.pVmoves, sizeof(int) * thisPV.length);
          }

          return beta;
        }

        // Check if alpha raised (new best move)
        if (score > alpha) {
          alpha = score;
          bestMove = move;
          // we updated alpha and in the pVNode so we should update our pV
          if (pvNode && !_stop){
            up_pV->length = thisPV.length + 1;
            up_pV->pVmoves[0] = move.getMoveINT();
            // memcpy - (куда, откуда, длина)
            std::memcpy(up_pV->pVmoves + 1, thisPV.pVmoves, sizeof(int) * thisPV.length);
          }

        }else{
          // Beta was not beaten and we dont improve alpha in this case we lower our search history values
          int penalty = _getHistoryPenalty(depth, nodeEval, alpha, pMoveScore, ttNode, cutNode, (CutOffState)ttEntry.Flag);
          if (isQuiet){
            _orderingInfo.incrementHistory(board.getActivePlayer(), move.getFrom(), move.getTo(), penalty);
            _orderingInfo.incrementCounterHistory(board.getActivePlayer(), pMove, move.getPieceType(), move.getTo(), penalty);
          }else{
            _orderingInfo.incrementCapHistory(move.getPieceType(), move.getCapturedPieceType(), move.getTo(), penalty);
          }
        }
      }

  }

  // Check for checkmate and stalemate
  if (legalCount == 0) {
    if (singSearch) return alpha;
    score = incheckNode ? LOST_SCORE + ply : 0; // LOST_SCORE = checkmate, 0 = stalemate (draw)
    return score;
  }

  // Store bestScore in transposition table
  if (!_stop && !singSearch){
      if (alpha <= alphaOrig) {
        int saveMove = ttMove.getMoveINT() != 0 ? ttMove.getMoveINT() : 0;
        myHASH->HASH_Store(board.getZKey().getValue(),  saveMove, ALPHA, alpha, depth, ply);
      } else {
        myHASH->HASH_Store(board.getZKey().getValue(), bestMove.getMoveINT(), EXACT, alpha, depth, ply);
      }
  }

  return alpha;
}

int Search::_qSearch(Board &board, int alpha, int beta) {
   _nodes++;
   bool pvNode = alpha != beta - 1;
   int nodeEval = NOSCORE;
   int standPat = NOSCORE;

  if (_stop || _checkLimits()) {
    _stop = true;
    return 0;
  }

  board.performUpdate();
  nodeEval = Eval::evaluate(board, board.getActivePlayer());
  standPat = nodeEval;

  if (standPat >= beta) {
    if (!pvNode) return beta;

    standPat = std::min((alpha + beta) / 2, beta - 1);
  }

  if (alpha < standPat) {
    alpha = standPat;
  }

  // Check transposition table cache
  // If TT is causing a cuttoff, we update move ordering stuff
  const HASH_Entry ttEntry = myHASH->HASH_Get(board.getZKey().getValue());
  if (ttEntry.Flag != NONE){
    if (!pvNode){
      int hashScore = ttEntry.score;

      if (abs(hashScore) > WON_IN_X){
        hashScore = (hashScore > 0) ? (hashScore - MAX_PLY) :  (hashScore + MAX_PLY);
      }
      if (ttEntry.Flag == EXACT){
        return hashScore;
      }
      if (ttEntry.Flag == BETA && hashScore >= beta){
        return beta;
      }
      if (ttEntry.Flag == ALPHA && hashScore <= alpha){
        return alpha;
      }
    }
  }

  MovePicker movePicker(&_orderingInfo, &board, 0, board.getActivePlayer(), MAX_PLY, 0);

  while (movePicker.hasNext()) {
    Move move = movePicker.getNext();

    // in qSearch if Value < 0 it means it is a bad capture
    // and we should prune it
    if (move.getValue() < 0){
      break;
    }

    // Use Halogen futility variation
    if (!(move.getFlags() & Move::PROMOTION) && !board.SEE_GreaterOrEqual(move, (alpha - standPat - DELTA_MOVE_CONST)))
      continue;

    Board movedBoard = board;
    bool isLegal = movedBoard.doMove(move);

    if (isLegal){
          myHASH->HASH_Prefetch(movedBoard.getZKey().getValue());

          int score = -_qSearch(movedBoard, -beta, -alpha);
          if (score >= beta) {
            // Add a new tt entry for this node
            if (!_stop){
                myHASH->HASH_Store(board.getZKey().getValue(), move.getMoveINT(), BETA, score, 0, MAX_PLY);
            }
            return beta;
          }
          if (score > alpha) {
            alpha = score;
          }
    }


  }
  return alpha;
}
