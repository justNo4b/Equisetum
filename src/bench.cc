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
#include "bench.h"
#include "searchdata.h"
#include "search.h"
#include "move.h"
#include "timer.h"
#include <iostream>
#include <memory>


extern HASH         * myHASH;
extern OrderingInfo * myOrdering;

void myBench(){
    std::cout << "Bench started..." << std::endl;
    std::chrono::time_point<std::chrono::steady_clock> timer_start = std::chrono::steady_clock::now();
    Board board = Board();
    int nodes_total = 0;

    Limits limits;
    limits.depth = BENCH_SEARCH_DEPTH;
    Hist history = Hist();
    std::shared_ptr<Search> search;

    for (int i = 0; i < BENCH_POS_NUMBER; i++){
        int curNodes = 0;
        board = Board(BENCH_POSITION[i], false);
        search = std::make_shared<Search>(board, limits, history, myOrdering, false);
        search->iterDeep();
        curNodes = search->getNodes();
        nodes_total += curNodes;
        myHASH->HASH_Clear();
        myOrdering->clearAllHistory();
        printf("Position [# %2d] Best: %6s %5i cp  Nodes: %12i", i + 1,search->getBestMove().getNotation(board.getFrcMode()).c_str(),
                 search->getBestScore(), curNodes);
        std::cout << std::endl;
    }

    int elapsed =std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timer_start).count();
    std::cout << "==============================================================="<<std::endl;
    printf("OVERALL: %12d nodes %8d nps\n", nodes_total, (int) (1000.0f * nodes_total / (elapsed + 1)));
    std::cout << std::flush;
};


void testSEE(){
    for (int j = 0; j < 10; j++){
        Board board = Board(SEE_POSITION[j], false);
        Move move = SEE_MOVE[j];
        int i = board.Calculate_SEE(move);
        bool k = board.SEE_GreaterOrEqual(move, -100);
        std::cout << i << " " << k << std::endl;
    }

}


void testMove(){
    int falsepos = 0;
    int propers = 0;

    Board board = Board("r2k3r/1Ppb3p/2pp1q2/p4PpQ/1nP1B3/1P2Ppp1/P1QN1nPP/R3K2R w KQkq g6 0 14", false);
    for (int k = 0; k < BENCH_POS_NUMBER; k++){
        propers = 0;
        falsepos = 0;
        board = Board(BENCH_POSITION[k], false);
        // create moves normally for comparasion
        MoveList normalmoves = MoveList();
        MoveGen(&board, false, &normalmoves);

        // create every possible move as INT and verify if it is pseudo-legal
        for (int i = -INF; i <= INF - 1; i++){
            Move move = Move(i);
            if (board.moveIsPseudoLegal(move)){
                //std::cout << "move is p-legal according to checker " << move.getNotation(false) << std::endl;
                bool isGenerated = false;
                for(size_t j = 0; j < normalmoves.size(); j++){
                    if (normalmoves[j].getMoveINT() == i){
                        isGenerated = true;
                        break;
                    }
                }
                if (isGenerated){
                //    std::cout << "Proper check!!! on "<< i << std::endl;
                    propers++;
                }else{
                //    std::cout << "False positive on " << i << std::endl;
                    falsepos++;
                }
                /*

                */

            }
            // report occasiaonally
            //if (i % (INF / 16) == 1) std::cout << "Moves checked " << i << std::endl;
        }
        if (falsepos == 0 && propers == normalmoves.size()){
            std::cout << "Position #" << k << " passed the test" << std::endl;
        }else{
            std::cout << "Position #" << k << " FAILED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        }
        //std::cout << "Total propers: " << propers << " FalsePositives: " << falsepos << " TotalMovePool " << normalmoves.size() << std::endl;
    }


}