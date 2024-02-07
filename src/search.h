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
#ifndef SEARCH_H
#define SEARCH_H

#include "searchdata.h"
#include "defs.h"
#include "movegen.h"
#include "transptable.h"
#include "orderinginfo.h"
#include "timer.h"
#include <chrono>
#include <atomic>


struct SearchParms {

   int asp_window = 30;
   int asp_delta  = 48;
   int asp_depth  = 7;

   int nmp_base = 4;
   int nmp_maxreduct = 5;
   int nmp_depthdiv  = 4;
   int nmp_delta_div = 128;
   int nmp_cond_base = 118;
   int nmp_cond_depth = 21;

   int prcut_beta_base = 218;
   int prcut_depth = 4;



   int sing_search_start = 5;


   int futil_move_const = 232;

   int revf_move_const = 161;
   int revf_impr_const = 142;
   int revf_depth = 8;

   int razoring_margin = 945;


   int delta_move_const = 186;
   int see_q_base = 48;
   int see_q_depth = -68;


   int cmh_depth = -4096;
   int cmh_base  = 4096;
   int lmp_hist_limit = 0;
   int pm_hist_reduction_limit = -8192;
   int m_hist_lmr_div = 8192;
   int cm_hist_lmr_div = 8192;
   int pm_hist_malus_factor = -8192;


   double lmr_init_a = 0.57;
   double lmr_init_div = 2.49;
   double lmr_depth_pow = 0.10;
   double lmr_number_pow = 0.16;
   double lmp_start_base = 1.57;
   double lmp_start_impr = 3.51;
   double lmp_multipl_base = 1.71;
   double lmp_multipl_impr = 1.73;



};


/**
 * @brief Represents a search through a minmax tree.
 *
 * Searches are performed using recursive alpha-beta search (invoked initially
 * through a call to the _rootMax() method, which in turn calls _negaMax().
 * This search is followed by a quiescence search at leaf nodes.
 */
class Search {
 public:
  /**
   * @brief Constructs a new Search for the given board.
   *
   * @param board The board to search
   * @param limits limits imposed on this search
   * @param positionHistory Vector of ZKeys reprenting all positions that have
   * occurred in the game
   * @param logUci If logUci is set, UCI info commands about the search will be printed
   * to standard output in real time.k
   */
  Search(const Board &, Limits, Hist, OrderingInfo *, SearchParms, bool= true);

  /**
   * @brief Performs an iterative deepening search within the constraints of the given limits.
   */
  void iterDeep();

  /**
   * @brief Returns the best move obtained through the last search performed.
   * @return The best move obtained through the last search
   */
  Move getBestMove();

  /**
   * @brief Returns the score of the best move
   * @return Score of the best move from previous iteration
   */
  int getBestScore();

  /**
   * @brief Instructs this Search to stop as soon as possible.
   */
  void stop();

  /**
   * @brief get amount of nodes we spent searching
   */
  int getNodes();

  /**
   * @brief get selective depth of the search thread
   */
  int getSeldepth();

 private:

  /**
   * @brief Array of reductions applied to the branch during
   * LATE MOVE REDUCTION during AB-search
   */
  int _lmr_R_array[34][34];

  /**
   * @brief Array of the pre-calculated move-nums
   * used for LATE MOVE PRUNING during AB-search
   */
  int _lmp_Array[MAX_PLY][2];

  U64 _rootNodesSpent[6][64];

  /**
   * @brief that is showing maxDepth with extentions we reached in the search
   */
  int _selDepth = 0;

  //search_constants
  //

   int ASP_WINDOW = 30;
   int ASP_DELTA  = 48;
   int ASP_DEPTH  = 7;

   int NMP_BASE = 4;
   int NMP_MAXREDUCT = 5;
   int NMP_DEPTHDIV  = 4;
   int NMP_DELTA_DIV = 128;
   int NMP_COND_BASE = 118;
   int NMP_COND_DEPTH = 21;

   int PRCUT_BETA_BASE = 218;
   int PRCUT_DEPTH = 4;

   int SEE_Q_BASE = 48;
   int SEE_Q_DEPTH = -68;

   int SING_SEARCH_START = 5;

   int DELTA_MOVE_CONST = 186;
   int FUTIL_MOVE_CONST = 232;

   int REVF_MOVE_CONST = 161;
   int REVF_IMPR_CONST = 142;
   int REVF_DEPTH = 8;

   int RAZORING_MARGIN = 945;

   int CMH_DEPTH = -4096;
   int CMH_BASE  = 4096;
   int LMP_HIST_LIMIT = 0;
   int PM_HIST_REDUCTION_LIMIT = -8192;
   int M_HIST_LMR_DIV = 8192;
   int CM_HIST_LMR_DIV = 8192;
   int PM_HIST_MALUS_FACTOR = -8192;

   double LMR_INIT_A = 0.57;
   double LMR_INIT_DIV = 2.49;
   double LMR_DEPTH_POW = 0.10;
   double LMR_NUMBER_POW = 0.16;

   double LMR_INIT_A_CAP = 0.57;
   double LMR_INIT_DIV_CAP = 2.49;
   double LMR_DEPTH_POW_CAP = 0.10;
   double LMR_NUMBER_POW_CAP = 0.16;

   double LMP_START_BASE = 1.57;
   double LMP_START_IMPR = 3.51;
   double LMP_MULTIPL_BASE = 1.71;
   double LMP_MULTIPL_IMPR = 1.73;



  //

  /**
   * @brief Structure of ZKeys for each position that has occurred in the game
   *
   * This is used to detect threefold repetitions.
   */
  Hist  _posHist;

  SEARCH_Data _sStack;

  /**
   * @brief OrderingInfo object containing information about the current state
   * of this search
   */
  OrderingInfo & _orderingInfo;

  /**
   * @brief Timer that is controlling time management and testing to not over
   * overstep depth/nodes/etc bounds
   *
   */
  Timer _timer;

  /**
   * @brief Initial board being used in this search.
   */
  Board _initialBoard;

  /**
   * @brief True if UCI will be logged to standard output during the search.
   */
  bool _logUci;

  /**
   * @brief If this flag is set, calls to _negaMax() and _rootMax() will end as soon
   * as possible and calls to _rootMax will not set the best move and best score.
   */
  std::atomic<bool> _stop;


  /**
   * @brief Returns True if this search has exceeded its given limits
   *
   * Note that to avoid a needless amount of computation, limits are only
   * checked every 4096 calls to _checkLimits() (using the Search::_limitCheckCount property).
   * If Search::_limitCheckCount is not 0, false will be returned.
   *
   * @return True if this search has exceed its limits, true otherwise
   */
  bool _checkLimits();

  /**
   * @brief Number of nodes searched in the last search.
   */
  U64 _nodes;

  /**
   * @brief Best move found on last search.
   */
  Move _bestMove;

  /**
   * @brief Score corresponding to _bestMove
   */
  int _bestScore;

  /**
   * @brief principal variantion we calculated
   */
  pV _ourPV;

  /**
   * @brief updating heuristics when beta cut occured
   *
   * @param move Move that caused cut
   * @param color moving player
   * @param pMove previous move
   * @param ply   search ply
   * @param depth search depth
   */
  inline void _updateBeta(bool isQuiet, const Move move, Color color, int pMove, int ply, int depth);

  inline bool _isRepetitionDraw(U64, int);

  inline int _makeDrawScore();

  /**
   * @brief Root negamax function.
   *
   * Starts performing a search to the given depth using recursive minimax
   * with alpha-beta pruning.
   *
   * @param board Board to search through
   * @param depth Depth to search to
   */
  int _rootMax(const Board &, int, int, int);

  /**
   * @brief Non root negamax function, should only be called by _rootMax()
   *
   *
   * @param  board Board to search
   * @param  depth Plys remaining to search
   * @param  alpha Alpha value
   * @param  beta  Beta value
   * @param  isSing is this a singular re- search
   * @return The score of the given board
   */
  int _negaMax(const Board &, pV *myPV, int, int, int, bool, bool);

  /**
   * @brief Performs a quiescence search
   *
   * _qSearch only takes into account captures (checks, promotions are not
   * considered)
   *
   * @param  board Board to perform a quiescence search on
   * @param  alpha Alpha value
   * @param  beta  Beta value
   * @return The score of the given board
   */
  int _qSearch(const Board &, int= -INF, int= INF);

  /**
   * @brief Logs info about a search according to the UCI protocol.
   *
   * @param pv        MoveList representing the Principal Variation (first moves at index 0)
   * @param depth     Depth of search
   * @param bestScore Score corresponding to the best move
   * @param nodes     Number of nodes searched
   * @param elapsed   Time taken to complete the search in milliseconds
   */
  void _logUciInfo(const MoveList &, int, int, U64, int);

  /**
   * @brief Returns the principal variation for the last performed search.
   *
   * Internally, this method probes the transposition table for the PV of the last
   * performed search.
   *
   * @return MoveList The principal variation for the last performed search
   */
  MoveList _getPv();

  /**
   * @brief this function calculates reductions values and stores
   * it in the _lmr_R_array
   */
  void init_LMR_array(SearchParms);

};

#endif
