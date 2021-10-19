#include "Engine.hpp"

// #include "EngineState.hpp"

#include <math.h>
#include <algorithm>
#include <iostream>

using namespace std;

Value staleEval(Board const& board)//no moves possible, draw or death
{
	if (board.inCheck())
	{
		return Value(-HUGE_VALF, board.plyNumber);
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
	return board.blacksTurn ? -value : value;
}

Value Engine::quiescenceSearch(Engine& engine,
	Value alpha, Value beta, short searchDepth, short rootPly)
{
	if (!engine.good())
	{
		return Value();
	}

	Board& board(*engine.board);
	if (searchDepth == 0)
	{
		return evaluate(board);
	}

	if (board.plySinceLastPawnOrCapture >= 100 ||
		engine.threeMoveRepetition())
	{
		//TODO: check for off-by-one issue
		return Value();
	}

	vector<Move> moves(board.generateMoves());

	//TODO: use hash table

	// HashBoard const& hBoard = engine->hashTable.get(board.hash);
	// HashOccupancyType hashExistence;

	// if (hBoard.hash == 0)
	// {
	// 	hashExistence = HashNotPresent;
	// }
	// else if (hBoard.hash == board.hash)
	// {
	// 	hashExistence = HashesEqual;
	// 	if (hBoard.searchDepth >= searchDepth)
	// 	{
	// 		return {hBoard.eval, {hBoard.bestResponse}};
	// 	}
	// 	for (int i = 0; i < moves.size(); ++i)
	// 	{
	// 		if (moves[i] == hBoard.bestResponse)
	// 		{
	// 			swap(moves[0], moves[i]);
	// 			break;
	// 		}
	// 	}
	// }
	// else
	// {
	// 	hashExistence = HashesNotEqual;
	// }

	bool legalMoveExists = false;
	bool nonQuiescentMoveExists = false;
	Value new_alpha{alpha};
	for (auto&& move : moves)
	{
		char temp = engine.nonQuiescentAdvance(move);
		legalMoveExists = legalMoveExists || temp != Illegal;
		if (temp != NonQuiescent)
			continue;

		nonQuiescentMoveExists = true;

		new_alpha = max(quiescenceSearch(
			engine, -beta, -new_alpha, searchDepth - 1, rootPly), new_alpha);
		if (new_alpha >= beta)
		{
			//hard fail-high beta cutoff, beta is a lower bound. (value is "too good")
			//TODO: hash update
			engine.back();
			return -beta;
		}
		engine.back();

	}

	if (!legalMoveExists)//game has reached an end state
	{
		return -staleEval(board);
	}
	if (!nonQuiescentMoveExists)//game has reached a quiescent position
	{
		return -evaluate(board);
	}

	//TODO: update hash

	// if (hashExistence != HashesNotEqual || hBoard.rootPly + 1 < rootPly)
	// {
	// 	engine->hashTable.set({board.hash, bestValue, bestMoveStack.back(), static_cast<short>(board.plyNumber + searchDepth)});
	// }

	return -new_alpha;
}

Value Engine::nonQuiescenceSearch(Engine& engine,
	Value alpha, Value beta, short searchDepth, short rootPly)
{
	//negamax search algorithm

	if (searchDepth == 0)
	{
		short initialSearchDepth = engine.quiescenceSearchDepth % engine.depthWalkValue;
		initialSearchDepth = initialSearchDepth == 0 ?
			engine.depthWalkValue :
			initialSearchDepth;
		
		for (short currentSearchDepth = initialSearchDepth;
			currentSearchDepth <= engine.quiescenceSearchDepth;
			currentSearchDepth += engine.depthWalkValue)
		{
			auto value = Engine::quiescenceSearch(engine,
				-beta, -alpha, currentSearchDepth, rootPly);
			if (currentSearchDepth == engine.quiescenceSearchDepth)
				return -value;
		}
	}
	if (!engine.good())
	{
		return Value();
	}

	Board& board(*engine.board);

	if (board.plySinceLastPawnOrCapture >= 100 ||
		engine.threeMoveRepetition())
	{
		//TODO: check for off-by-one issue
		return Value();
	}

	vector<Move> moves(board.generateMoves());


	HashBoard const& hBoard = engine.hashTable.get(board.hash);
	HashOccupancyType hashExistence;

	if (hBoard.hash == 0)
	{
		hashExistence = HashNotPresent;
	}
	else if (hBoard.hash == board.hash)
	{
		hashExistence = HashesEqual;
		//if searched to equal or better depth
		//then we can trust the result
		if (hBoard.searchDepth >= searchDepth)
		{
			switch (hBoard.nodeType)
			{
				case HashBoard::PrincipleVariation://exact value
				return -hBoard.value;

				case HashBoard::AllNode://upper bound value
				if (hBoard.value <= alpha) return alpha;
				else beta = hBoard.value;//try setting depth to hBoard depth
				break;

				case HashBoard::CutNode://lower bound value
				if (hBoard.value >= beta) return beta;
				else alpha = hBoard.value;//try setting depth to hBoard depth
				break;
			}
		}

		//if we have an all-node then there is no ordering knowledge to be gained
		if (hBoard.nodeType != HashBoard::AllNode)
		for (int i = 0; i < moves.size(); ++i)
		{
			if (moves[i] == hBoard.bestResponse)
			{
				swap(moves[0], moves[i]);
				break;
			}
		}
	}
	else
	{
		hashExistence = HashesNotEqual;
	}

	//TODO: perform heuristic ordering

	//fail-hard algorithm

	Value new_alpha{alpha};

	bool legalMoveExists = false;
	for (auto&& move : moves)
	{
		if (!engine.advance(move))
			continue;

		legalMoveExists = true;

		new_alpha = max(nonQuiescenceSearch(
			engine, -beta, -new_alpha, searchDepth - 1, rootPly), new_alpha);
		if (new_alpha >= beta)
		{
			//hard fail-high beta cutoff, beta is a lower bound. (value is "too good")
			//TODO: hash update
			engine.back();
			//This is a cut-node

			return -beta;
		}
		engine.back();
	}

	if (!legalMoveExists)//game has reached an end condition
	{
		return -staleEval(board);
	}

	if (new_alpha == alpha)
	{
		//hard fail-low alpha cutoff, alpha is an upper bound. (value is "too bad")

		//This is an all-node
	}
	else
	{
		//exact value
		
		//This is a principle variation
	}

	//TODO: update hash

	// if (hashExistence != HashesNotEqual || hBoard.rootPly + 1 < rootPly)
	// {
	// 	engine->hashTable.set({board.hash, bestValue, bestMoveStack.back(), static_cast<short>(board.plyNumber + searchDepth)});
	// }

	return -new_alpha;
}

vector<tuple<Value, Move>> Engine::rootSearch(Engine& engine, short searchDepth)
{
	Board& board(*engine.board);
	short rootPly = board.plyNumber;

	Value alpha{-HUGE_VALF, rootPly};
	Value beta{HUGE_VALF, rootPly};

	vector<Move> moves(board.generateMoves());

	vector<Value> values;
	vector<short> indices;

	for (short i = 0; i < moves.size(); ++i)
	{
		if (!engine.advance(moves[i]))
		{
			values.emplace_back();//won't be used
			continue;
		}

		auto value = nonQuiescenceSearch(
			engine, -beta, -alpha, searchDepth - 1, rootPly);
		alpha = max(alpha, value);
		values.push_back(value);
		indices.push_back(i);//keeps track of legal moves and implicitly pairs with value
		engine.back();
	}

	//sort into decending order, best moves first
	sort(indices.begin(), indices.end(), [values](short a, short b)
	{
		return values[a] > values[b];
	});

	//TODO: update hash

	// if (hashExistence != HashesNotEqual || hBoard.rootPly + 1 < rootPly)
	// {
	// 	engine->hashTable.set({board.hash, bestValue, bestMoveStack.back(), static_cast<short>(board.plyNumber + searchDepth)});
	// }

	vector<tuple<Value, Move>> output;

	for (auto&& index : indices)
	{
		output.emplace_back(values[index], moves[index]);
	}

	return output;
}

void Engine::calculationLoop(Engine* engine)
{
	engine->bestMoveStacks.resize(0);

	short searchDepth = engine->infiniteFlag || engine->maxDepth == 0 ?
		// INT16_MAX :
		4 :
		engine->maxDepth;

	short initialSearchDepth = searchDepth % engine->depthWalkValue;
	initialSearchDepth = initialSearchDepth == 0 ?
		engine->depthWalkValue :
		initialSearchDepth;
	
	for (short currentSearchDepth = initialSearchDepth;
		currentSearchDepth <= searchDepth;
		currentSearchDepth += engine->depthWalkValue)
	{
		auto info = Engine::rootSearch(*engine, currentSearchDepth);
		if (engine->bestMoveStacks.size() != 0 &&
			engine->stopFlag) break;
		engine->bestMoveStacks.resize(0);
		engine->bestMoveStacks.push_back({get<1>(info[0])});

		cout << "hash occupancy: " << engine->hashTable.getOccupancy() << " / " << engine->hashTable.getSize() << endl;

		if (engine->stopFlag) break;
	}

	engine->stopFlag = true;
	
	if (engine->bestMoveStacks[0].size() > 0)
		cout << "bestmove " << moveToAlgebraic(engine->bestMoveStacks[0][0]) << endl;
	else
		cout << "bestmove " << "0000" << endl;
}

Engine::QuiescentType Engine::nonQuiescentAdvance(Move move)
{
	bool quiescent = board->isQuiescent(move);
	if (!board->miscLegalityCheck(move))
		return Illegal;
	board->performMove(move);
	if (board->positionAttacked(firstIndex(
		board->pieceBoards[Piece::King | !board->blacksTurn]), board->blacksTurn))
	{
		board->reverseMove(move);
		return Illegal;
	}
	if (quiescent)
	{
		board->reverseMove(move);
		return Quiescent;
	}
	activeMoves.push_back(move);
	activeHashes.push_back(board->hash);
	return NonQuiescent;
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
