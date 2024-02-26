#ifndef DATAGEN_H
#define DATAGEN_H

#define RND_OPENING_PLY   (8)
#define CUSTOM_BOOK_PC    (100)
#define DG_NODES_SEARCHED (5000)
#define ADJ_WIN_EVAL      (1500)
#define ADJ_WIN_PLY       (4)
#define ADJ_DRAW_EVAL     (10)
#define ADJ_DRAW_PLY      (8)
#define ADJ_DRAW_MOVE     (40)


namespace DataGen
{

    Move getRandomLegalMove(const Board &);

    /*
        params are board and opening depth in ply
    */
    bool createRandomOpening(Board &, int);

    void playGame(int);
    void WorkerFunction(int);

    void readBook();

    inline std::string stmWin(const Board &);
    inline std::string stmLose(const Board &);

    const std::string W_WIN = "[1.0]";
    const std::string DRAW  = "[0.5]";
    const std::string B_WIN = "[0.0]";

    const std::string BOOK_FILE = "DFRC.epd";

    struct dataReady
    {
        std::string fen;
        int         eval;

        dataReady() : fen(""), eval(0) {};
        dataReady(std::string s, int e) : fen(s), eval(e) {};
    };


} // namespace DataGen




#endif
