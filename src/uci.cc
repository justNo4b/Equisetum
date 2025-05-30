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
#include <memory>
#include "uci.h"
#include "version.h"
#include "eval.h"
#include "searchdata.h"
#include "timer.h"
#include "board.h"
#include <iostream>
#include <thread>

extern HASH         * myHASH;
extern OrderingInfo * myOrdering;

int  myTHREADSCOUNT = 1;

OrderingInfo  * cOrdering[MAX_THREADS];
Search        * cSearch[MAX_THREADS];
std::thread     cThread[MAX_THREADS];

SearchParms ourSearchParams;
BoardParams ourBoardParams;
Limits uci_timer;

namespace {
Book book;
std::shared_ptr<Search> search;
Board board;
Hist positionHistory = Hist();

void loadBook() {
  std::ifstream bookFile(optionsMap["BookPath"].getValue());
  bool bookOk = bookFile.good();
  bookFile.close();

  if (bookOk) {
    book = Book(optionsMap["BookPath"].getValue());
  } else {
    std::cerr << optionsMap["BookPath"].getValue() << " is inaccessible or doesn't exist" << std::endl;
  }
}

void changeTTsize(){
  int size = atoi(optionsMap["Hash"].getValue().c_str());
  // make sure we do not overstep bounds
  size = std::min(size, MAX_HASH);
  size = std::max(size, MIN_HASH);
  // call TT
  myHASH->HASH_Initalize_MB(size);
}

void changeThreadsNumber(){
  int tNum = atoi(optionsMap["Threads"].getValue().c_str());
  // Make sure threads are within min/max bounds
  tNum = std::min(tNum, MAX_THREADS);
  tNum = std::max(tNum, MIN_THREADS);

  // Change number
  myTHREADSCOUNT = tNum;

  // Create ordering for them
  if (myTHREADSCOUNT > 1){
    for (int i = 1; i < myTHREADSCOUNT; i++){
      cOrdering[i] = new OrderingInfo();
    }
  }
}


void loadCosts(){

    uci_timer.width_a  = atoi(optionsMap["width_a"].getValue().c_str());
    uci_timer.width    = atoi(optionsMap["width"].getValue().c_str());
    uci_timer.t_move   = atoi(optionsMap["t_move"].getValue().c_str());
    uci_timer.c_move   = atoi(optionsMap["crit_move"].getValue().c_str());

    uci_timer.mtg_incr = atoi(optionsMap["mtg_cycl_incr"].getValue().c_str());
    uci_timer.nodes_max = atoi(optionsMap["nodes_max"].getValue().c_str());
    uci_timer.nodes_min = atoi(optionsMap["nodes_min"].getValue().c_str());
    uci_timer.nodes_div = atoi(optionsMap["nodes_div"].getValue().c_str());
    uci_timer.nodes_fact = atoi(optionsMap["nodes_fact"].getValue().c_str());
    uci_timer.nodes_base = atoi(optionsMap["nodes_base"].getValue().c_str());
    uci_timer.lpt_multiplier = atoi(optionsMap["lpt_multiplier"].getValue().c_str());


    ourSearchParams.asp_window = atoi(optionsMap["asp_window"].getValue().c_str());
    ourSearchParams.asp_delta  = atoi(optionsMap["asp_delta"].getValue().c_str());
    ourSearchParams.asp_incr  = atoi(optionsMap["asp_incr"].getValue().c_str());

    ourSearchParams.nmp_base = atoi(optionsMap["nmp_base"].getValue().c_str());
    ourSearchParams.nmp_maxreduct = atoi(optionsMap["nmp_maxreduct"].getValue().c_str());
    ourSearchParams.nmp_depthdiv  = atoi(optionsMap["nmp_depthdiv"].getValue().c_str());
    ourSearchParams.nmp_delta_div = atoi(optionsMap["nmp_delta_div"].getValue().c_str());
    ourSearchParams.nmp_cond_base = atoi(optionsMap["nmp_cond_base"].getValue().c_str());
    ourSearchParams.nmp_cond_depth = atoi(optionsMap["nmp_cond_depth"].getValue().c_str());

    ourSearchParams.prcut_beta_base = atoi(optionsMap["prcut_beta_base"].getValue().c_str());
    ourSearchParams.prcut_improving = atoi(optionsMap["prcut_improving"].getValue().c_str());
    ourSearchParams.prcut_depth = atoi(optionsMap["prcut_depth"].getValue().c_str());

    ourSearchParams.sing_search_start = atoi(optionsMap["sing_search_start"].getValue().c_str());

    ourSearchParams.revf_move_const = atoi(optionsMap["revf_move_const"].getValue().c_str());
    ourSearchParams.revf_impr_const = atoi(optionsMap["revf_impr_const"].getValue().c_str());
    ourSearchParams.revf_depth = atoi(optionsMap["revf_depth"].getValue().c_str());

    ourSearchParams.lmr_init_a = atoi(optionsMap["lmr_init_a"].getValue().c_str()) / 100;
    ourSearchParams.lmr_init_div = atoi(optionsMap["lmr_init_div"].getValue().c_str()) / 100;
    ourSearchParams.lmr_depth_pow = atoi(optionsMap["lmr_depth_pow"].getValue().c_str()) / 100;
    ourSearchParams.lmr_number_pow = atoi(optionsMap["lmr_number_pow"].getValue().c_str()) / 100;

    ourSearchParams.lmr_init_a_cap = atoi(optionsMap["lmr_init_a_cap"].getValue().c_str()) / 100;
    ourSearchParams.lmr_init_div_cap = atoi(optionsMap["lmr_init_div_cap"].getValue().c_str()) / 100;
    ourSearchParams.lmr_depth_pow_cap = atoi(optionsMap["lmr_depth_pow_cap"].getValue().c_str()) / 100;
    ourSearchParams.lmr_number_pow_cap = atoi(optionsMap["lmr_number_pow_cap"].getValue().c_str()) / 100;

    ourSearchParams.delta_move_const = atoi(optionsMap["delta_move_const"].getValue().c_str());
    ourSearchParams.see_q_base = atoi(optionsMap["see_q_base"].getValue().c_str());
    ourSearchParams.see_q_depth = -1 * atoi(optionsMap["see_q_depth"].getValue().c_str());

    ourSearchParams.see_c_base = atoi(optionsMap["see_c_base"].getValue().c_str());
    ourSearchParams.see_c_depth = -1 * atoi(optionsMap["see_c_depth"].getValue().c_str());

/*
    ourBoardParams.pSee = atoi(optionsMap["pawnSee"].getValue().c_str());
    ourBoardParams.kSee = atoi(optionsMap["knightSee"].getValue().c_str());
    ourBoardParams.bSee = atoi(optionsMap["bishopSee"].getValue().c_str());
    ourBoardParams.rSee = atoi(optionsMap["rookSee"].getValue().c_str());
    ourBoardParams.qSee = atoi(optionsMap["queenSee"].getValue().c_str());
*/

    ourSearchParams.ppe_depth_limit = atoi(optionsMap["ppe_depth_limit"].getValue().c_str());

   ourSearchParams.hb_multp = atoi(optionsMap["hb_multp"].getValue().c_str());
   ourSearchParams.hb_base_a  = atoi(optionsMap["hb_base_a"].getValue().c_str()) - 1000;
   ourSearchParams.hb_base_b  = atoi(optionsMap["hb_base_b"].getValue().c_str()) - 1000;

   ourSearchParams.hp_multp = atoi(optionsMap["hp_multp"].getValue().c_str());
   ourSearchParams.hp_base_a  = atoi(optionsMap["hp_base_a"].getValue().c_str()) - 1000;
   ourSearchParams.hp_base_b  = atoi(optionsMap["hp_base_b"].getValue().c_str()) - 1000;

   ourSearchParams.cmhp_mt_2 = atoi(optionsMap["cmhp_mt_2"].getValue().c_str());


    ourSearchParams.lmp_start_base = atoi(optionsMap["lmp_start_base"].getValue().c_str()) / 100;
    ourSearchParams.lmp_start_impr = atoi(optionsMap["lmp_start_impr"].getValue().c_str()) / 100;
    ourSearchParams.lmp_multipl_base = atoi(optionsMap["lmp_multipl_base"].getValue().c_str()) / 100;
    ourSearchParams.lmp_multipl_impr = atoi(optionsMap["lmp_multipl_impr"].getValue().c_str()) / 100;

   ourSearchParams.cmh_depth = -1 * atoi(optionsMap["cmh_depth"].getValue().c_str());
   ourSearchParams.cmh_base  = atoi(optionsMap["cmh_base"].getValue().c_str());
   ourSearchParams.lmp_hist_limit = atoi(optionsMap["lmp_hist_limit"].getValue().c_str()) - 4096;
   ourSearchParams.pm_hist_reduction_limit = -1 * atoi(optionsMap["pm_hist_reduction_limit"].getValue().c_str());
   ourSearchParams.m_hist_lmr_div = atoi(optionsMap["m_hist_lmr_div"].getValue().c_str());
   ourSearchParams.cm_hist_lmr_div = atoi(optionsMap["cm_hist_lmr_div"].getValue().c_str());
   ourSearchParams.pm_hist_malus_factor = -1 * atoi(optionsMap["pm_hist_malus_factor"].getValue().c_str());
}


void initOptions() {
  optionsMap["OwnBook"] = Option(false);
  optionsMap["BookPath"] = Option("book.bin", &loadBook);
  optionsMap["Hash"] = Option(MIN_HASH, MIN_HASH, MAX_HASH, &changeTTsize);
  optionsMap["Threads"] = Option(MIN_THREADS, MIN_THREADS, MAX_THREADS, &changeThreadsNumber);
  optionsMap["UCI_Chess960"] = Option(false);


  // Options for tuning is defined here.
  // They are used only if programm is build with "make tune"
  // Tuning versionshould not be the one playing regular games
  // but having this options here allows tuner
  // to change different parameters via communocation
  // with the engine.


  optionsMap["asp_window"] =   Option(30, 10, 75, &loadCosts);
  optionsMap["asp_delta"] =   Option(48, 10, 100, &loadCosts);
  optionsMap["asp_incr"] =   Option(200, 50, 400, &loadCosts);

  optionsMap["nmp_base"] =     Option(4, 2, 8, &loadCosts);
  optionsMap["nmp_maxreduct"] = Option(5, 2, 10, &loadCosts);
  optionsMap["nmp_depthdiv"] =   Option(4, 2, 8, &loadCosts);
  optionsMap["nmp_delta_div"] =   Option(128, 64, 512, &loadCosts);
  optionsMap["nmp_cond_base"] =     Option(118, 60, 250, &loadCosts);
  optionsMap["nmp_cond_depth"] =    Option(21, 10, 40, &loadCosts);
  optionsMap["prcut_beta_base"] =   Option(218, 100, 350, &loadCosts);
  optionsMap["prcut_improving"] =   Option(100, 50, 150, &loadCosts);
  optionsMap["prcut_depth"] =   Option(4, 2, 8, &loadCosts);

  optionsMap["sing_search_start"] =   Option(5, 2, 8, &loadCosts);

  optionsMap["revf_move_const"] =    Option(161, 25, 300, &loadCosts);
  optionsMap["revf_impr_const"] =   Option(142, 25, 300, &loadCosts);
  optionsMap["revf_depth"] =   Option(8, 4, 12, &loadCosts);

  optionsMap["see_q_base"] =     Option(48, 0, 120, &loadCosts);
  optionsMap["see_q_depth"] =    Option(68, 0, 150, &loadCosts); // member to go negative
  optionsMap["delta_move_const"] =   Option(186, 50, 500, &loadCosts);

  optionsMap["see_c_base"] =     Option(100, 0, 120, &loadCosts);
  optionsMap["see_c_depth"] =    Option(150, 0, 150, &loadCosts); // member to go negative

/*
  optionsMap["pawnSee"] =    Option(100, 50, 250, &loadCosts);
  optionsMap["knightSee"] =   Option(300, 150, 450, &loadCosts);
  optionsMap["bishopSee"] =   Option(300, 150, 450, &loadCosts);
  optionsMap["rookSee"] =     Option(500, 400, 700, &loadCosts);
  optionsMap["queenSee"] =     Option(1000, 800, 1200, &loadCosts);
*/


  optionsMap["cmh_depth"] =     Option(4096, 0, 10000, &loadCosts); // member to go negative
  optionsMap["cmh_base"] =     Option(4096, 0, 10000, &loadCosts);
  optionsMap["lmp_hist_limit"] =     Option(4096, 0, 16000, &loadCosts); // ????
  optionsMap["pm_hist_reduction_limit"] =     Option(8192, 4000, 16000, &loadCosts);// member to go negative
  optionsMap["m_hist_lmr_div"] =     Option(8192, 4000, 16000, &loadCosts);
  optionsMap["cm_hist_lmr_div"] =     Option(8192, 4000, 16000, &loadCosts);
  optionsMap["pm_hist_malus_factor"] =     Option(8192, 4000, 16000, &loadCosts); // member to go negative
  optionsMap["cmh_pruning_depth"] =     Option(3, 1, 8, &loadCosts);

  // member divide by 100
  optionsMap["lmr_init_a"] =    Option(57, 0, 400, &loadCosts);
  optionsMap["lmr_init_div"] =   Option(249, 0, 500, &loadCosts);
  optionsMap["lmr_depth_pow"] =   Option(10, 5, 25, &loadCosts);
  optionsMap["lmr_number_pow"] =     Option(16, 5, 25, &loadCosts);

  optionsMap["lmr_init_a_cap"] =    Option(57, 0, 400, &loadCosts);
  optionsMap["lmr_init_div_cap"] =   Option(249, 0, 500, &loadCosts);
  optionsMap["lmr_depth_pow_cap"] =   Option(10, 5, 25, &loadCosts);
  optionsMap["lmr_number_pow_cap"] =     Option(16, 5, 25, &loadCosts);

  optionsMap["lmp_start_base"] =    Option(157, 0, 300, &loadCosts);
  optionsMap["lmp_start_impr"] =   Option(351, 0, 600, &loadCosts);
  optionsMap["lmp_multipl_base"] =   Option(171, 0, 300, &loadCosts);
  optionsMap["lmp_multipl_impr"] =     Option(173, 0, 600, &loadCosts);

  optionsMap["width_a"] =       Option(322, 100, 500, &loadCosts);
  optionsMap["width"] =         Option(188, 50, 300, &loadCosts);
  optionsMap["t_move"] =        Option(22, 8, 40, &loadCosts);
  optionsMap["crit_move"] =     Option(26, 8, 40, &loadCosts);
  optionsMap["mtg_cycl_incr"] = Option(5, 2, 10, &loadCosts);
  optionsMap["nodes_max"] =     Option(85, 50, 100, &loadCosts);
  optionsMap["nodes_min"] =     Option(25, 1, 50, &loadCosts);
  optionsMap["nodes_fact"] =    Option(50, 25, 75, &loadCosts);
  optionsMap["nodes_div"] =     Option(50, 25, 75, &loadCosts);
  optionsMap["nodes_base"] =     Option(100, 1, 200, &loadCosts);
  optionsMap["lpt_multiplier"] =     Option(20, 5, 50, &loadCosts);

  optionsMap["ppe_depth_limit"] =     Option(8, 4, 12, &loadCosts);

  optionsMap["hb_multp"] =     Option(32, 1, 64, &loadCosts);
  optionsMap["hb_base_a"] =     Option(1000, 900, 1100, &loadCosts);
  optionsMap["hb_base_b"] =     Option(1000, 0, 2000, &loadCosts);

  optionsMap["hp_multp"] =     Option(32, 1, 64, &loadCosts);
  optionsMap["hp_base_a"] =     Option(1032, 900, 1100, &loadCosts);
  optionsMap["hp_base_b"] =     Option(1000, 0, 2000, &loadCosts);

  optionsMap["cmhp_mt_2"] =     Option(512, 1, 2048, &loadCosts);
}

void uciNewGame() {
  board.setToStartPos();
  positionHistory = Hist();
}

void setPosition(std::istringstream &is) {
  std::string token;
  is >> token;

  if (token == "startpos") {
    board.setToStartPos();
  } else {
    std::string fen;

    while (is >> token && token != "moves") {
      fen += token + " ";
    }

    board.setToFen(fen, (optionsMap["UCI_Chess960"].getValue() == "true"));
  }

  while (is >> token) {
    if (token == "moves") {
      continue;
    }

    MoveList moves = MoveList();
    MoveGen movegen(&board, false, &moves);
    for (auto &move : moves) {
      if (move.getNotation((optionsMap["UCI_Chess960"].getValue() == "true")) == token) {
        board.doMove(move);
        if ((move.getPieceType() == PAWN) || (move.getFlags() & Move::CAPTURE) ){
          positionHistory = Hist();
        }
        positionHistory.Add(board.getZKey().getValue());
        break;
      }
    }
  }
}

void pickBestMove() {
  if (optionsMap["OwnBook"].getValue() == "true" && book.inBook(board)) {
    std::cout << "bestmove " << book.getMove(board).getNotation(board.getFrcMode()) << std::endl;
  } else {
    search->iterDeep();
  }
}

void go(std::istringstream &is) {
  std::string token;

  while (is >> token) {
    if (token == "depth") is >> uci_timer.depth;
    else if (token == "infinite") uci_timer.infinite = true;
    else if (token == "movetime") is >> uci_timer.moveTime;
    else if (token == "nodes") is >> uci_timer.nodes;
    else if (token == "wtime") is >> uci_timer.time[WHITE];
    else if (token == "btime") is >> uci_timer.time[BLACK];
    else if (token == "winc") is >> uci_timer.increment[WHITE];
    else if (token == "binc") is >> uci_timer.increment[BLACK];
    else if (token == "movestogo") is >> uci_timer.movesToGo;
  }

    board.setSeeValues(ourBoardParams);
// if we have > 1 threads, run some additional threads
  if (myTHREADSCOUNT > 1){
    for (int i = 1; i < myTHREADSCOUNT; i++){
      // copy board stuff
      Board b = board;
      Limits l = uci_timer;
      Hist h = positionHistory;

      // clear killers for every ordering
      cOrdering[i]->clearKillers();
      // create search and assign to the thread
      cSearch[i] = new Search(b, l, h, cOrdering[i], ourSearchParams, false);
      cThread[i] = std::thread(&Search::iterDeep, cSearch[i]);
    }
  }

  myOrdering->clearKillers();
  search = std::make_shared<Search>(board, uci_timer, positionHistory, myOrdering, ourSearchParams);

  std::thread searchThread(&pickBestMove);
  searchThread.detach();


}


void printEngineInfo() {
  std::cout << "id name Equisetum " << VER_MAJ << "." << VER_MIN << "." << VER_PATCH << std::endl;
  std::cout << "id author Rhys Rustad-Elliott and Alexander Litov" << std::endl;
#ifdef _TUNE_
  std::cout << "This is _TUNE_ build, it can be slower" << std::endl;
#endif

  std::cout << std::endl;

  for (auto optionPair : optionsMap) {
    std::cout << "option ";
    std::cout << "name " << optionPair.first << " ";
    std::cout << "type " << optionPair.second.getType() << " ";
    std::cout << "default " << optionPair.second.getDefaultValue();

    if (optionPair.second.getType() == "spin") {
      std::cout << " ";
      std::cout << "min " << optionPair.second.getMin() << " ";
      std::cout << "max " << optionPair.second.getMax();
    }
    std::cout << std::endl;
  }
  std::cout << "uciok" << std::endl;
}

void setOption(std::istringstream &is) {
  std::string token;
  std::string optionName;

  is >> token >> optionName; // Advance past "name"

  if (optionsMap.find(optionName) != optionsMap.end()) {
    is >> token >> token; // Advance past "value"
    optionsMap[optionName].setValue(token);
  } else {
    std::cout << "Invalid option" << std::endl;
  }
}

void loop() {
  std::cout << "Equisetum " << VER_MAJ << "." << VER_MIN << "." << VER_PATCH;
  std::cout << " by Rhys Rustad-Elliott and Litov Alexander";
  std::cout << " (built " << __DATE__ << " " << __TIME__ << ")" << std::endl;

  board.setToStartPos();

  std::string line;
  std::string token;

  while (std::getline(std::cin, line)) {
    std::istringstream is(line);
    is >> token;

    if (token == "uci") {
      printEngineInfo();
    } else if (token == "ucinewgame") {
      uciNewGame();
    } else if (token == "isready") {
      std::cout << "readyok" << std::endl;
    } else if (token == "stop") {
      if (search) search->stop();
    } else if (token == "go") {
      go(is);
    } else if (token == "quit") {
      if (search) search->stop();
      return;
    } else if (token == "position") {
      setPosition(is);
    } else if (token == "setoption") {
      setOption(is);
    }

      // Non UCI commands
    else if (token == "printboard") {
      std::cout << std::endl << board.getStringRep() << std::endl;
    } else if (token == "printmoves") {
      MoveList moves = MoveList();
      MoveGen movegen(&board, false, &moves);
      for (auto &move : moves) {
        std::cout << move.getNotation(board.getFrcMode()) << " ";
      }
      std::cout << std::endl;
    } else {
      std::cout << "what?" << std::endl;
    }
  }
}
}

void Uci::init() {
  initOptions();

/* Shit for SPSA

*/
        for(auto it = optionsMap.cbegin(); it != optionsMap.cend(); ++it)
        {
            auto step = (int)((it->second.getMax() - it->second.getMin()) / 20);
            step = std::max(1, step);
            std::cout << it->first << ", int, " << it->second.getDefaultValue() << ", " << it->second.getMin() << ", " << it->second.getMax() << ", " <<  step << ", 0.002" << std::endl;
        }
}

void Uci::start() {
  loop();
}
