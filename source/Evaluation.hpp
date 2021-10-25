#pragma once

#include "Value.hpp"
#include "Board.hpp"

extern long linearPiecePositionValue[16][64];

void evaluationInitialize();

inline Value staleEval(Board const& board) // no moves possible, draw or death
{
	if (board.inCheck())
	{
		return Value(0, -board.plyNumber);
	}
	return Value();
}

Value evaluate(Board const& board);

// bool exchangeDeltaValid(Board const& board, Move move);

long captureDelta(Board const& board, Move move);
