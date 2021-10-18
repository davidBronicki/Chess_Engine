#include "Engine.hpp"

// #include "EngineState.hpp"

#include <math.h>
#include <algorithm>
#include <iostream>

using namespace std;

inline void sortPositions(Board const& board, vector<tuple<Value, vector<Move>>> const& positions, vector<short>& indices)
{
	if (board.blacksTurn)//ascending order so lowest score is first (best for black)
	{
		sort(indices.begin(), indices.end(),
			[&positions](short a, short b)
			{
				return get<0>(positions[a]) < get<0>(positions[b]);
			});
	}
	else//descending order so highest score is first (best for white)
	{
		sort(indices.begin(), indices.end(),
			[&positions](short a, short b)
			{
				return get<0>(positions[a]) > get<0>(positions[b]);
			});
	}
}

Value staleEval(Board const& board)//no moves possible, draw or death
{
	if (board.inCheck())
	{
		if (board.blacksTurn)
		{
			return Value(HUGE_VALF, board.plyNumber);
		}
		else
		{
			return Value(-HUGE_VALF, board.plyNumber);
		}
	}
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
	return value;
}

tuple<Value, vector<Move>> Engine::quiescenceSearch(Engine* engine,
	Value alpha, Value beta, short searchDepth)
{
	if (!engine->good())
	{
		return {Value(), {}};
	}

	Board& board(*engine->board);

	if (searchDepth == 0)
		return {evaluate(board), {}};
	//TODO: actual quiescent search

	if (board.plySinceLastPawnOrCapture >= 100 ||
		engine->threeMoveRepetition())
	{
		//TODO: check for off-by-one issue
		return {Value(), {}};
	}

	bool legalMoveExists = false;
	bool nonQuiescentMoveExists = false;

	vector<Move> moves(board.generateMoves());

	Value bestValue{board.blacksTurn ? HUGE_VALF : -HUGE_VALF, board.plyNumber};
	vector<Move> bestMoveStack;
	for (auto&& move : moves)
	{
		char temp = engine->nonQuiescentAdvance(move);
		legalMoveExists = legalMoveExists || temp != 0;
		if (temp != 2)
			continue;

		nonQuiescentMoveExists = true;

		//TODO: alpha beta pruning

		auto tempPair = quiescenceSearch(
			engine, alpha, beta, searchDepth - 1);

		if (!board.blacksTurn)//move has been performed, so turn is flipped
		{
			if (get<0>(tempPair) < bestValue)
			{
				bestValue = get<0>(tempPair);
				bestMoveStack = get<1>(tempPair);
				bestMoveStack.push_back(move);
			}
			beta = min(beta, bestValue);
		}
		else
		{
			if (get<0>(tempPair) > bestValue)
			{
				bestValue = get<0>(tempPair);
				bestMoveStack = get<1>(tempPair);
				bestMoveStack.push_back(move);
			}
			alpha = max(alpha, bestValue);
		}
		engine->back();
		if (alpha >= beta)
		{
			return make_tuple(bestValue, bestMoveStack);
		}
	}

	if (!legalMoveExists)//game has reached an end state
	{
		return {staleEval(board), {}};
	}
	if (nonQuiescentMoveExists)//game has reached a quiescent position
	{
		return {evaluate(board), {}};
	}

	return make_tuple(bestValue, bestMoveStack);
}

tuple<Value, vector<Move>> Engine::nonQuiescenceSearch(Engine* engine,
	Value alpha, Value beta, short searchDepth)
{
	if (searchDepth == 0)
		return quiescenceSearch(engine, alpha, beta, engine->quiescenceSearchDepth);
	if (!engine->good())
	{
		return {Value(), {}};
	}

	Board& board(*engine->board);

	if (board.plySinceLastPawnOrCapture >= 100 ||
		engine->threeMoveRepetition())
	{
		//TODO: check for off-by-one issue
		return {Value(), {}};
	}

	vector<Move> moves(board.generateMoves());

	Value bestValue{board.blacksTurn ? HUGE_VALF : -HUGE_VALF, board.plyNumber};
	vector<Move> bestMoveStack;

	bool legalMoveExists = false;
	for (auto&& move : moves)
	{
		if (!engine->advance(move))
			continue;

		legalMoveExists = true;

		//TODO: alpha beta pruning

		auto tempPair = nonQuiescenceSearch(
			engine, alpha, beta, searchDepth - 1);

		if (!board.blacksTurn)//move has been performed, so turn is flipped
		{
			if (get<0>(tempPair) < bestValue)
			{
				bestValue = get<0>(tempPair);
				bestMoveStack = get<1>(tempPair);
				bestMoveStack.push_back(move);
			}
			beta = min(beta, bestValue);
		}
		else
		{
			if (get<0>(tempPair) > bestValue)
			{
				bestValue = get<0>(tempPair);
				bestMoveStack = get<1>(tempPair);
				bestMoveStack.push_back(move);
			}
			alpha = max(alpha, bestValue);
		}
		engine->back();
		if (alpha >= beta)
		{
			return make_tuple(bestValue, bestMoveStack);
		}


		//TODO: actual hash board logic

		//not guaranteed that the return move list will not be empty here...

		// if (searchDepth > 1 && engine->hashTable.get(board.hash).hash == 0)
		// 	engine->hashTable.set({board.hash,
		// 		get<0>(valueStackPairs.back()),
		// 		get<1>(valueStackPairs.back()).back(),
		// 		board.plyNumber, 0, engine->searchIter, HashBoard::leaf});
	}

	if (!legalMoveExists)//game has reached an end condition
	{
		return {staleEval(board), {}};
	}

	return make_tuple(bestValue, bestMoveStack);
}

void Engine::calculationLoop(Engine* engine)
{
	engine->bestMoveStacks.resize(0);

	Value alpha(Value{-HUGE_VALF, engine->board->plyNumber});
	Value beta(Value{HUGE_VALF, engine->board->plyNumber});
	Value value;
	short searchDepth = engine->infiniteFlag || engine->maxDepth == 0 ?
		INT16_MAX :
		engine->maxDepth;
	short currentSearchDepth = min(engine->depthWalkValue, searchDepth);
	//TODO: iterate through search depths
	auto info = Engine::nonQuiescenceSearch(engine,
		alpha, beta, currentSearchDepth);
	
	engine->bestMoveStacks.push_back(get<1>(info));

	engine->stopFlag = true;
	
	if (engine->bestMoveStacks[0].size() > 0)
		cout << "bestmove " << moveToAlgebraic(engine->bestMoveStacks[0].back()) << endl;
	else
		cout << "bestmove " << "0000" << endl;
}

char Engine::nonQuiescentAdvance(Move move)
{
	bool quiescent = board->isQuiescent(move);
	if (!board->miscLegalityCheck(move))
		return 0;
	board->performMove(move);
	if (board->positionAttacked(firstIndex(
		board->pieceBoards[Piece::King | !board->blacksTurn]), board->blacksTurn))
	{
		board->reverseMove(move);
		return 0;
	}
	if (quiescent)
	{
		board->reverseMove(move);
		return 1;
	}
	activeMoves.push_back(move);
	activeHashes.push_back(board->hash);
	return 2;
}
bool Engine::advance(Move move)
{
	if (!board->miscLegalityCheck(move))
		return false;
	board->performMove(move);
	if (board->positionAttacked(firstIndex(
		board->pieceBoards[Piece::King | !board->blacksTurn]), board->blacksTurn))
	{
		board->reverseMove(move);
		return false;
	}
	activeMoves.push_back(move);
	activeHashes.push_back(board->hash);
	return true;
}
void Engine::back()
{
	board->reverseMove(activeMoves.back());
	activeMoves.pop_back();
	activeHashes.pop_back();
}
bool Engine::threeMoveRepetition()
{
	//TODO: check off-by-one issues
	int count = 0;
	for (int i = 0; i < min(activeHashes.size(),
		static_cast<size_t>(board->plySinceLastPawnOrCapture)); i += 2)
	{
		if (activeHashes[activeHashes.size() - 1 - i] == activeHashes.back())
			++count;
	}
	return count >= 3;
}
