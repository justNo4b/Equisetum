#include "defs.h"
#include "search.h"
#include "movepicker.h"
#include "datagen.h"
#include <thread>
#include <fstream>


std::vector <std::string> BOOK;

Move DataGen::getRandomLegalMove(const Board * board ){
    // Generate all moves
    MoveList allMoves = MoveList();
    MoveGen movegen(board, false, &allMoves);
    MoveList legalMoves;

    for (auto & m : allMoves){
        Board movedBoard = *board;
        if (movedBoard.doMove(m))legalMoves.push_back(m);
    }

    if (legalMoves.size() > 0){
        int rndnum = std::rand() % legalMoves.size();
        Move res = legalMoves.at(rndnum);
        return res;
    }

    return Move(0);

}

void DataGen::readBook(){
    std::string s;
    std::ifstream file(BOOK_FILE);


    // Exit the program if file is not found or cannot be opened
    if (!file){
        std::cout << "Failed to open file. Exit"<< std::endl;
        exit(0);
    }
    else
    {
        std::cout << "Book file opened. Processing positions...\n"<< std::endl;
    }


    while (std::getline(file, s))
    {
        BOOK.push_back(s);
    }

    std::cout << "Book processed."<< BOOK.size() << " positions opened \n"<< std::endl;
}


bool DataGen::createRandomOpening(Board &board, int openingDepth){

    for (int i = 0; i < openingDepth; i++){
        Move rndMove = getRandomLegalMove(&board);
        if (rndMove.getMoveINT() != 0){
            board.doMove(rndMove);
        }else{
            return false;
        }
    }
    // check final position to be leagl
    Move rndMove = getRandomLegalMove(&board);
    if (rndMove.getMoveINT() != 0){
        return true;
    }

    return false;
}


void DataGen::playGame(int threadNum){
    // Initialize counters
    int drawCounter = 0;
    int winCounter  = 0;
    std::string result = "";
    std::vector<dataReady> readyFen;

    // First, create everything needed
    Limits limits;
    limits.nodes = DG_NODES_SEARCHED;

    Hist history = Hist();
    Board b = Board();
    OrderingInfo * o = new OrderingInfo();
    HASH * h = new HASH();

    std::shared_ptr<Search> search;


    // create fine random opening
    bool fineOpening = false;
    while (!fineOpening)
    {
        bool usedBook = false;
        b = Board();
        // chance percent to use FRC-book
        if (std::rand() % 100 <= CUSTOM_BOOK_PC){
            usedBook = true;
            std::string s = BOOK.at(std::rand() % BOOK.size());
            b = Board(s, true);
        }
        fineOpening = DataGen::createRandomOpening(b, RND_OPENING_PLY / (1 + usedBook) );
    }



    while (true){
        history.Add(b.getZKey().getValue());
        o->clearKillers();
        search = std::make_shared<Search>(b, limits, history, o, h, false);
        search->iterDeep();
        Move best = search->getBestMove();
        int eval = search->getBestScore();
        // if move is not capture and we are not in check, write fen
        if (abs(eval) < 10000 &&
            !b.colorIsInCheck(b.getActivePlayer()) &&
            best.isQuiet()){
                readyFen.push_back(dataReady(b.getFenRep(), (b.getActivePlayer() == WHITE ? eval : -eval)));
            }

        b.doMove(best);

        // test if position is valid, otherwise adjudigate
        if (getRandomLegalMove(&b).getMoveINT() == 0){
            // No legal moves after move is made, Win for moving side
            if (b.colorIsInCheck(b.getActivePlayer())){
                result =  stmWin(b);
            }else{
                result = DRAW;
            }

            break;
        }

        // test adjudigation conditions
        if (abs(eval) >= ADJ_WIN_EVAL){
            winCounter++;
            drawCounter = 0;
            if (winCounter >= ADJ_WIN_PLY) {
                result = eval > 0 ? stmWin(b) : stmLose(b);
                break;
                }
        }else if (abs(eval) <= ADJ_DRAW_EVAL){
            drawCounter++;
            winCounter = 0;
            if (drawCounter >= ADJ_DRAW_PLY && b._getGameClock() >= ADJ_DRAW_MOVE * 2) {
                result = DRAW;
                break;
                }
        }

    }

    // dont forget to destroy everything
    delete o;
    h->HASH_Destroy();
    delete h;
    // print shit into file
    std::fstream file;
    file.open("data" + std::to_string(threadNum) + ".txt", std::ios::out | std::ios::app);
    if (!file.is_open()){
        std::cout << "sad" << std::endl;
    }
    for (auto &d : readyFen){
        file << d.fen << " " << result << " " << std::to_string(d.eval) <<  "\n";
    }

    file.close();
}

void DataGen::WorkerFunction(int tNum){
    while (true)
    {
        playGame(tNum);
    }

}


inline std::string DataGen::stmWin(const Board &b){
    return b.getActivePlayer() == WHITE ? B_WIN : W_WIN;
}


inline std::string DataGen::stmLose(const Board &b){
    return b.getActivePlayer() == WHITE ? W_WIN : B_WIN;
}