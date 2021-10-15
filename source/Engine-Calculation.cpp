#include "Engine.hpp"

#include "EngineState.hpp"

#include <math.h>
#include <algorithm>
#include <iostream>

using namespace std;

Value evaluate(Board const& board)
{
	return Value();
}

tuple<Value, vector<Move>> Engine::quiescentSearch(Engine* engine, Move move,
	Value alpha, Value beta)
{
	if (!engine->good())
	{
		return {Value(), {nonMove}};
	}
	//TODO: actual quiescent search

	Board& board(*engine->board);
	board.performMove(move);
	Value value = evaluate(board);
	if (engine->hashTable.get(board.hash).hash == 0)
		engine->hashTable.set({board.hash, value, nonMove,
			board.plyNumber, 0, engine->searchIter, HashBoard::leaf});
	board.reverseMove(move);
	return {value, {nonMove}};
}

tuple<Value, vector<Move>> Engine::nonQuiescentSearch(Engine* engine, Move move,
	Value alpha, Value beta,
	short searchDepth)
{
	if (!engine->good())
	{
		return {Value(), {nonMove}};
	}
	if (searchDepth == 0)
		return quiescentSearch(engine, move, alpha, beta);
	Board& board(*engine->board);
	std::vector<Move> moves(board.generateLegalMoves());

	std::vector<std::vector<Move>> reverseMoveStacks{};
	std::vector<Value> values;
	std::vector<short> indexList;
	for (auto&& move : moves)
	{
		board.performMove(move);
		
		//TODO: actual search

		Value value = evaluate(board);

		//TODO: actual hash board logic

		if (engine->hashTable.get(board.hash).hash == 0)
			engine->hashTable.set({board.hash, value, nonMove,
				board.plyNumber, 0, engine->searchIter, HashBoard::leaf});
		
		values.push_back(value);
		indexList.push_back(indexList.size());
		reverseMoveStacks.push_back({move});

		board.reverseMove(move);
	}

	sort(indexList.data(), &indexList.back(), [values](short a, short b){return values[a] > values[b];});

	return {values[indexList[0]], reverseMoveStacks[indexList[0]]};
}

void Engine::calculationLoop(Engine* engine)
{
	Value alpha(Value{-HUGE_VAL});
	Value beta(Value{HUGE_VAL});
	Value value;
	short searchDepth = engine->infiniteFlag || engine->maxDepth == 0 ?
		INT16_MAX :
		engine->maxDepth;
	short currentSearchDepth = min(engine->depthWalkValue, searchDepth);
	//TODO: iterate through search depths
	auto info = Engine::nonQuiescentSearch(engine, nonMove,
		alpha, beta, currentSearchDepth);
	
	engine->bestMoveStacks.push_back(get<1>(info));

	engine->stopFlag = true;
	
	cout << "bestmove " << moveToAlgebraic(engine->bestMoveStacks[0].back()) << endl;
}
