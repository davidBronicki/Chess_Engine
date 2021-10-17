#include "Engine.hpp"

// #include "EngineState.hpp"

#include <math.h>
#include <algorithm>
#include <iostream>

using namespace std;

Value staleEval(Board const& board)//no moves possible, draw or death
{
	return Value();
}

Value evaluate(Board const& board)
{
	Value value;
	value.value += 1 * cardinality(board.pieceBoards[Piece::Pawn | Piece::White]);
	value.value += 3 * cardinality(board.pieceBoards[Piece::Knight | Piece::White]);
	value.value += 3 * cardinality(board.pieceBoards[Piece::Bishop | Piece::White]);
	value.value += 5 * cardinality(board.pieceBoards[Piece::Rook | Piece::White]);
	value.value += 9 * cardinality(board.pieceBoards[Piece::Queen | Piece::White]);
	
	value.value -= 1 * cardinality(board.pieceBoards[Piece::Pawn | Piece::Black]);
	value.value -= 3 * cardinality(board.pieceBoards[Piece::Knight | Piece::Black]);
	value.value -= 3 * cardinality(board.pieceBoards[Piece::Bishop | Piece::Black]);
	value.value -= 5 * cardinality(board.pieceBoards[Piece::Rook | Piece::Black]);
	value.value -= 9 * cardinality(board.pieceBoards[Piece::Queen | Piece::Black]);
	return Value();
}

tuple<Value, vector<Move>> Engine::quiescentSearch(Engine* engine,
	Value alpha, Value beta)
{
	if (!engine->good())
	{
		return {Value(), {nonMove}};
	}
	//TODO: actual quiescent search

	Board& board(*engine->board);
	
	Value value = evaluate(board);
	if (engine->hashTable.get(board.hash).hash == 0)
		engine->hashTable.set({board.hash, value, nonMove,
			board.plyNumber, 0, engine->searchIter, HashBoard::leaf});
			
	return {value, {}};
}

tuple<Value, vector<Move>> Engine::nonQuiescentSearch(Engine* engine,
	Value alpha, Value beta,
	short searchDepth)
{
	if (!engine->good())
	{
		return {Value(), {nonMove}};
	}
	if (searchDepth == 0)
		return quiescentSearch(engine, alpha, beta);
	Board& board(*engine->board);

	std::vector<Move> moves(board.generateLegalMoves());

	if (moves.size() == 0)//game has reached an end condition
	{
		return {staleEval(board), {}};
	}

	std::vector<std::tuple<Value, std::vector<Move>>> valueStackPairs;
	std::vector<short> indexList;
	for (auto&& move : moves)
	{
		board.performMove(move);
		
		//TODO: actual search

		valueStackPairs.push_back(nonQuiescentSearch(
			engine, alpha, beta, searchDepth - 1));

		if (isinf(std::get<0>(valueStackPairs.back()).value))
		{
			++std::get<0>(valueStackPairs.back()).movesToMate;
		}


		//TODO: actual hash board logic

		if (engine->hashTable.get(board.hash).hash == 0)
			engine->hashTable.set({board.hash,
				std::get<0>(valueStackPairs.back()),
				std::get<1>(valueStackPairs.back()).back(),
				board.plyNumber, 0, engine->searchIter, HashBoard::leaf});
		

		std::get<1>(valueStackPairs.back()).push_back(move);
		indexList.push_back(indexList.size());
		board.reverseMove(move);
	}

	sort(indexList.begin(), indexList.end(), [&valueStackPairs](short a, short b)
		{
			return std::get<0>(valueStackPairs[a]) > std::get<0>(valueStackPairs[b]);
		});

	return valueStackPairs[indexList[0]];
}

void Engine::calculationLoop(Engine* engine)
{
	engine->bestMoveStacks = std::vector<std::vector<Move>>();

	Value alpha(Value{-HUGE_VALF});
	Value beta(Value{HUGE_VALF});
	Value value;
	short searchDepth = engine->infiniteFlag || engine->maxDepth == 0 ?
		INT16_MAX :
		engine->maxDepth;
	short currentSearchDepth = min(engine->depthWalkValue, searchDepth);
	//TODO: iterate through search depths
	auto info = Engine::nonQuiescentSearch(engine,
		alpha, beta, currentSearchDepth);
	
	engine->bestMoveStacks.push_back(get<1>(info));

	engine->stopFlag = true;
	
	cout << "bestmove " << moveToAlgebraic(engine->bestMoveStacks[0].back()) << endl;
}
