<h1 align="center">Equisetum</h1>

<p align="center">
 <img src="Logo/Equisetum.png" width="350"/>
</p>

A UCI chess engine written in C++11.
Equisetum uses Fail-Hard AB-search and NNUE evaluation.
This approach resulted in an engine of moderate strength. One of the few strong engines using a fail-hard approach

## NNUE

Equisetum uses an NNUE (efficiently updatable neural network) as an evaluation function.

Current implementation is a very simple (768-1024)x2 -> 1 network.
It is trained on Equisetum self-play games using a mix of a play from a randomized startposition and a randomized positions
derived from human high-bias positions.

Network is training using <a href="https://github.com/Luecx/CudAD">CudAD</a>


## Origins
Equisetum is basically a continuation of the <a href="https://github.com/justNo4b/Drofa">Drofa</a> chess engine,
which in turn is started as fork of the <a href="https://github.com/GunshipPenguin/shallow-blue">Shallow Blue</a> chess engine.
My initial intention was to take weak, but stable and working chess engine and try to improve it, learning c++ along the way.

During my Drofa/Equisetum experiments huge chunk of knowlenge were received from:

 - <a href="https://github.com/peterwankman/vice">VICE</a> chess engine and tutorials.
 - <a href="https://github.com/TerjeKir/weiss">Weiss</a> chess engine, with clean and understandable implementations of complex features. Drofa/Equisetum use Weiss 1.0
LMP base reduction formulas. As well as HCE-tuning for Drofa versions
 - Several open source engines, mostly <a href="https://github.com/AndyGrant/Ethereal">Ethereal</a> and <a href="https://github.com/official-stockfish/Stockfish">Stockfish</a>

## Special thanks to:
 - Terje Kirstihagen (Weiss author)
 - GediminasMasaitis (ChessDotCPP author) for explaining bulk of the NNUE concepts to me
 - Finn Eggers for creating <a href="https://github.com/Luecx/CudAD">CudAD</a> which i use for NNUE training
 - Andrew Grant. AdaGrad paper and Ethereal chess engine are great sources of knowledge; Ethereal tuning dataset was a great help in tuning. As well as allowing me on main OpenBench instance
 - Kim Kahre, Finn Eggers and Eugenio Bruno (Koivisto team) for allowing Drofa on Koi OpenBench instance and motivating me to work on the engine
 - Jay Honnold (Berserk author) for helping me with NN stuff
 - OpenBench community for helping me with motivation, in finding bugs, teaching me (even if unknowingly) good programming practices and interesting discussions

## Strength
Current Equisetum is estimated to be somewhere before Stockfish 11 and Stockfish 12 strength.

## UCI commands

Equisetum supports following UCI commands:

- BookPath
- OwnBook
- Threads (1 to 172),
- Hash    (16 to 65536)

These options can be set from your chess GUI or the UCI interface as follows:

```
setoption name OwnBook value true
setoption name BookPath value /path/to/book.bin
```

## License

During developement, many concepts found in GPL-3 engines were used.
Thus, as of 4.0.0, Equisetum also will be licensed as GPL-3.

2017 - 2019 © Rhys Rustad-Elliott (original Shallow Blue creator)

2020 - 2024 © Litov Alexander

