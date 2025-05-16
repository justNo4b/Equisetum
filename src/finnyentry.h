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
#ifndef FINNYENTRY_H
#define FINNYENTRY_H

// Finny Table for super efficient accumulator updates.
// Base idea is from Koivisto, implementation is mine (probably better to look in original)

#include "nnue.h"
#include "board.h"

struct FinnyEntry
{
    NNueEvaluation evaluator;
    Board board;
    bool isReady;

    FinnyEntry() : evaluator(NNueEvaluation()), board(Board()), isReady(false) {};
};


#endif