#pragma once

#include "Board.hpp"
#include "EvalTypes.hpp"

struct EngineState
{
	Move move;
	short depth;
	short currentDepth;

	std::vector<Move> availableMoves;
	bool generated;

	Value alpha;
	Value beta;

	bool forward;
	// bool returning;
	std::vector<Value> moveValues;

	std::vector<us> orderedIndexList;//for move ranking

	EngineState() :
		move(nullMove(0)),
		depth(0),
		availableMoves(),
		alpha(-HUGE_VAL),
		beta(HUGE_VAL),
		forward(true)
	{}

	EngineState(Move move, int depth, Value alpha, Value beta, bool forward) :
		move(move),
		depth(depth),
		alpha(alpha),
		beta(beta),
		forward(forward)
	{}

	void generateMoves(Board const& board)
	{
		generated = true;
		availableMoves = board.generateLegalMoves();
	}
};