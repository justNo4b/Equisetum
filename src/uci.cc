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

#ifdef _TUNE_
void loadCosts(){

Eval::SetupTuning(KNIGHT, atoi(optionsMap["cKnight"].getValue().c_str()));
Eval::SetupTuning(BISHOP, atoi(optionsMap["cBishop"].getValue().c_str()));

Eval::SetupTuning(ROOK, atoi(optionsMap["cRook"].getValue().c_str()));
Eval::SetupTuning(QUEEN, atoi(optionsMap["cQueen"].getValue().c_str()));
}
#endif

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

#ifdef _TUNE_
  optionsMap["cKnight"] =   Option(0, 0, 150, &loadCosts);
  optionsMap["cBishop"] =   Option(0, 0, 150, &loadCosts);
  optionsMap["cRook"] =     Option(0, 0, 150, &loadCosts);
  optionsMap["cQueen"] =    Option(0, 0, 150, &loadCosts);
#endif


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
      cSearch[i] = new Search(b, l, h, cOrdering[i], false);
      cThread[i] = std::thread(&Search::iterDeep, cSearch[i]);
    }
  }

  myOrdering->clearKillers();
  search = std::make_shared<Search>(board, limits, positionHistory, myOrdering);

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
}

void Uci::start() {
  loop();
}
