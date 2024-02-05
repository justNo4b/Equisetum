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
#include <iostream>
#include <thread>

extern HASH         * myHASH;
extern OrderingInfo * myOrdering;

int  myTHREADSCOUNT = 1;

OrderingInfo  * cOrdering[MAX_THREADS];
Search        * cSearch[MAX_THREADS];
std::thread     cThread[MAX_THREADS];

SearchParms ourSearchParams;

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
/*
    ourSearchParams.asp_window = atoi(optionsMap["asp_window"].getValue().c_str());
    ourSearchParams.asp_delta  = atoi(optionsMap["asp_delta"].getValue().c_str());

    ourSearchParams.nmp_base = atoi(optionsMap["nmp_base"].getValue().c_str());
    ourSearchParams.nmp_maxreduct = atoi(optionsMap["nmp_maxreduct"].getValue().c_str());
    ourSearchParams.nmp_depthdiv  = atoi(optionsMap["nmp_depthdiv"].getValue().c_str());
    ourSearchParams.nmp_delta_div = atoi(optionsMap["nmp_delta_div"].getValue().c_str());
    ourSearchParams.nmp_cond_base = atoi(optionsMap["nmp_cond_base"].getValue().c_str());
    ourSearchParams.nmp_cond_depth = atoi(optionsMap["nmp_cond_depth"].getValue().c_str());

    ourSearchParams.prcut_beta_base = atoi(optionsMap["prcut_beta_base"].getValue().c_str());
    ourSearchParams.prcut_depth = atoi(optionsMap["prcut_depth"].getValue().c_str());

    ourSearchParams.see_q_base = atoi(optionsMap["see_q_base"].getValue().c_str());
    ourSearchParams.see_q_depth = -1 * atoi(optionsMap["see_q_depth"].getValue().c_str());

    ourSearchParams.sing_search_start = atoi(optionsMap["sing_search_start"].getValue().c_str());

    ourSearchParams.delta_move_const = atoi(optionsMap["delta_move_const"].getValue().c_str());
    ourSearchParams.futil_move_const = atoi(optionsMap["futil_move_const"].getValue().c_str());

    ourSearchParams.revf_move_const = atoi(optionsMap["revf_move_const"].getValue().c_str());
    ourSearchParams.revf_impr_const = atoi(optionsMap["revf_impr_const"].getValue().c_str());
    ourSearchParams.revf_depth = atoi(optionsMap["revf_depth"].getValue().c_str());

    ourSearchParams.razoring_margin = atoi(optionsMap["razoring_margin"].getValue().c_str());
*/


    ourSearchParams.lmr_init_a = atoi(optionsMap["lmr_init_a"].getValue().c_str()) / 100;
    ourSearchParams.lmr_init_div = atoi(optionsMap["lmr_init_div"].getValue().c_str()) / 100;
    ourSearchParams.lmr_depth_pow = atoi(optionsMap["lmr_depth_pow"].getValue().c_str()) / 100;
    ourSearchParams.lmr_number_pow = atoi(optionsMap["lmr_number_pow"].getValue().c_str()) / 100;

    ourSearchParams.lmr_init_a_cap = atoi(optionsMap["lmr_init_a_cap"].getValue().c_str()) / 100;
    ourSearchParams.lmr_init_div_cap = atoi(optionsMap["lmr_init_div_cap"].getValue().c_str()) / 100;
    ourSearchParams.lmr_depth_pow_cap = atoi(optionsMap["lmr_depth_pow_cap"].getValue().c_str()) / 100;
    ourSearchParams.lmr_number_pow_cap = atoi(optionsMap["lmr_number_pow_cap"].getValue().c_str()) / 100;

/*
    ourSearchParams.lmp_start_base = atoi(optionsMap["lmp_start_base"].getValue().c_str()) / 100;
    ourSearchParams.lmp_start_impr = atoi(optionsMap["lmp_start_impr"].getValue().c_str()) / 100;
    ourSearchParams.lmp_multipl_base = atoi(optionsMap["lmp_multipl_base"].getValue().c_str()) / 100;
    ourSearchParams.lmp_multipl_impr = atoi(optionsMap["lmp_multipl_impr"].getValue().c_str()) / 100;
*/


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

/*
  optionsMap["asp_window"] =   Option(30, 10, 75, &loadCosts);
  optionsMap["asp_delta"] =   Option(48, 10, 100, &loadCosts);
  optionsMap["nmp_base"] =     Option(4, 2, 7, &loadCosts);
  optionsMap["nmp_maxreduct"] = Option(5, 2, 7, &loadCosts);
  optionsMap["nmp_depthdiv"] =   Option(4, 2, 8, &loadCosts);
  optionsMap["nmp_delta_div"] =   Option(128, 64, 256, &loadCosts);
  optionsMap["nmp_cond_base"] =     Option(118, 60, 200, &loadCosts);
  optionsMap["nmp_cond_depth"] =    Option(21, 10, 30, &loadCosts);
  optionsMap["prcut_beta_base"] =   Option(218, 50, 300, &loadCosts);
  optionsMap["prcut_depth"] =   Option(4, 2, 8, &loadCosts);
  optionsMap["see_q_base"] =     Option(48, 0, 120, &loadCosts);
  optionsMap["see_q_depth"] =    Option(68, 0, 150, &loadCosts); // member to go negative
  optionsMap["sing_search_start"] =   Option(5, 2, 8, &loadCosts);
  optionsMap["delta_move_const"] =   Option(186, 50, 500, &loadCosts);
  optionsMap["futil_move_const"] =     Option(232, 50, 500, &loadCosts);
  optionsMap["revf_move_const"] =    Option(161, 25, 300, &loadCosts);
  optionsMap["revf_impr_const"] =   Option(142, 25, 175, &loadCosts);
  optionsMap["revf_depth"] =   Option(8, 3, 10, &loadCosts);
  optionsMap["razoring_margin"] =     Option(650, 100, 1000, &loadCosts);
*/



  // member divide by 100
  optionsMap["lmr_init_a"] =    Option(57, 0, 400, &loadCosts);
  optionsMap["lmr_init_div"] =   Option(249, 0, 500, &loadCosts);
  optionsMap["lmr_depth_pow"] =   Option(10, 5, 25, &loadCosts);
  optionsMap["lmr_number_pow"] =     Option(16, 5, 25, &loadCosts);

  optionsMap["lmr_init_a_cap"] =    Option(57, 0, 400, &loadCosts);
  optionsMap["lmr_init_div_cap"] =   Option(249, 0, 500, &loadCosts);
  optionsMap["lmr_depth_pow_cap"] =   Option(10, 5, 25, &loadCosts);
  optionsMap["lmr_number_pow_cap"] =     Option(16, 5, 25, &loadCosts);

/*
  optionsMap["lmp_start_base"] =    Option(157, 0, 300, &loadCosts);
  optionsMap["lmp_start_impr"] =   Option(351, 0, 600, &loadCosts);
  optionsMap["lmp_multipl_base"] =   Option(171, 0, 300, &loadCosts);
  optionsMap["lmp_multipl_impr"] =     Option(173, 0, 600, &loadCosts);
*/





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

    MoveGen movegen(board, false);
    MoveList * moves = movegen.getMoves();
    for (auto &move : *moves) {
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
  Limits limits;

  while (is >> token) {
    if (token == "depth") is >> limits.depth;
    else if (token == "infinite") limits.infinite = true;
    else if (token == "movetime") is >> limits.moveTime;
    else if (token == "nodes") is >> limits.nodes;
    else if (token == "wtime") is >> limits.time[WHITE];
    else if (token == "btime") is >> limits.time[BLACK];
    else if (token == "winc") is >> limits.increment[WHITE];
    else if (token == "binc") is >> limits.increment[BLACK];
    else if (token == "movestogo") is >> limits.movesToGo;
  }

// if we have > 1 threads, run some additional threads
  if (myTHREADSCOUNT > 1){
    for (int i = 1; i < myTHREADSCOUNT; i++){
      // copy board stuff
      Board b = board;
      Limits l = limits;
      Hist h = positionHistory;

      // clear killers for every ordering
      cOrdering[i]->clearKillers();
      // create search and assign to the thread
      cSearch[i] = new Search(b, l, h, cOrdering[i], ourSearchParams, false);
      cThread[i] = std::thread(&Search::iterDeep, cSearch[i]);
    }
  }

  myOrdering->clearKillers();
  search = std::make_shared<Search>(board, limits, positionHistory, myOrdering, ourSearchParams);

  std::thread searchThread(&pickBestMove);
  searchThread.detach();


}

unsigned long long perft(const Board &board, int depth) {
  if (depth <= 0) {
    return 1;
  } else if (depth == 1) {
    return MoveGen(board, false).getMoves()->size();
  }

  MoveGen movegen(board, false);
  MoveList * moves = movegen.getMoves();
  unsigned long long nodes = 0;
  for (auto &move : *moves) {
    Board movedBoard = board;
    movedBoard.doMove(move);

    nodes += perft(movedBoard, depth - 1);
  }

  return nodes;
}

void perftDivide(int depth) {
  unsigned long long total = 0;

  MoveGen movegen(board, false);

  std::cout << std::endl;
  auto start = std::chrono::steady_clock::now();
  MoveList * moves = movegen.getMoves();
  for (auto &move : *moves) {
    Board movedBoard = board;
    movedBoard.doMove(move);

    unsigned long long perftRes = perft(movedBoard, depth - 1);
    total += perftRes;

    std::cout << move.getNotation(board.getFrcMode()) << ": " << perftRes << std::endl;
  }
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  std::cout << std::endl << "==========================" << std::endl;
  std::cout << "Total time (ms) : " << static_cast<int>(elapsed.count() * 1000) << std::endl;
  std::cout << "Nodes searched  : " << total << std::endl;
  std::cout << "Nodes / second  : " << static_cast<int>(total / elapsed.count()) << std::endl;
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

#ifdef _TUNE_
  std::cout << "This is _TUNE_ build, it can be slower" << std::endl;
#endif

#ifdef __DEBUG__
  std::cout << "***DEBUG BUILD (This will be slow)***" << std::endl;
#endif

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
      MoveGen movegen(board, false);
      MoveList * moves = movegen.getMoves();
      for (auto &move : *moves) {
        std::cout << move.getNotation(board.getFrcMode()) << " ";
      }
      std::cout << std::endl;
    } else if (token == "perft") {
      int depth = 1;
      is >> depth;
      perftDivide(depth);
    } else {
      std::cout << "what?" << std::endl;
    }
  }
}
}

void Uci::init() {
  initOptions();

/* Shit for SPSA
        for(auto it = optionsMap.cbegin(); it != optionsMap.cend(); ++it)
        {
            auto step = (int)((it->second.getMin() + it->second.getMax()) / 20);
            step = std::max(1, step);
            std::cout << it->first << " int " << it->second.getDefaultValue() << " " << it->second.getMin() << " " << it->second.getMax() << " " <<  step << " 0.002" << std::endl;
        }
*/

}

void Uci::start() {
  loop();
}
