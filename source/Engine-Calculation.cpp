#include "Engine.hpp"
#include "Board.hpp"
#include "EvaluationContext.hpp"
#include "HashTable.hpp"

// #include "EngineState.hpp"

#include <math.h>
#include <algorithm>
#include <iostream>

using namespace std;

inline Value staleEval(Board const& board)//no moves possible, draw or death
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

	value.value += 0.5;
	return board.blacksTurn ? -value : value;
}

inline bool quiescence_CutNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	switch (hashExistence)
	{
		case HashTable::HashNotPresent:
		return true;
		case HashTable::HashesEqual:
		return (hBoard.nodeType & HashBoard::Quiescence_Mask) ||//dont replace non-quiescent search
			hBoard.rootPly + 2 <= rootPly;//not sure of this
		case HashTable::HashesNotEqual:
		return hBoard.rootPly + 2 <= rootPly ||//not sure of this
			(hBoard.nodeType == HashBoard::Quiescence_All &&
				hBoard.searchDepth < searchDepth);
	}
}

inline bool quiescence_AllNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hashExistence == HashTable::HashNotPresent ||
		(hBoard.nodeType & HashBoard::Quiescence_Mask) ||
		hBoard.rootPly + 2 <= rootPly;//not sure of this
}

inline bool quiescence_PVNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hashExistence == HashTable::HashNotPresent ||
		(hashExistence == HashTable::HashesEqual &&
			(hBoard.nodeType & HashBoard::TypeInfo_Mask)) ||
		hBoard.rootPly + 2 <= rootPly;//not sure of this
}

inline bool main_CutNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hBoard.rootPly + 2 <= rootPly ||
		hashExistence != HashTable::HashesNotEqual ||
		hBoard.nodeType != HashBoard::PrincipleVariation;
}

inline bool main_AllNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hashExistence != HashTable::HashesNotEqual ||
		hBoard.rootPly + 2 <= rootPly;
}

inline bool main_PVNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hBoard.rootPly + 2 <= rootPly ||
		hashExistence != HashTable::HashesNotEqual ||
		hBoard.nodeType != HashBoard::PrincipleVariation ||
		hBoard.searchDepth < searchDepth;
}

Value Engine::quiescenceSearch(
	Value alpha, Value beta, PlyType searchDepth, PlyType rootPly)
{
	if (!good())
	{
		return Value();
	}

	if (board->plySinceLastPawnOrCapture >= 99 ||
		threeMoveRepetition())
	{
		//TODO: check for off-by-one issue
		//TODO: clamp?
		return Value();
	}

	if (searchDepth <= 0)
	{
		return -evaluate(*board);
	}

	if (!board->inCheck())
	{
		// //we can always just not take, but not if we're in check
		// //should become a good position quickly, so search shallow
		// Move nullMove = board->buildMoveFromContext(0, 0, Move::NullMove);
		// board->performMove(nullMove);

		// alpha = max(alpha, quiescenceSearch(-beta, -alpha, searchDepth - 2, rootPly));

		// board->reverseMove(nullMove);

		alpha = max(alpha, evaluate(*board));
	}

	if (alpha >= beta)//null move (not taking) fails high
		return -beta;

	vector<Move> moves(board->generateMoves());

	HashTable::HashOccupancyType hashExistence;
	HashBoard const& hBoard = hashTable->quiescence_HandleHash(
		board->hash, hashExistence, alpha, beta, moves, searchDepth, rootPly);

	bool legalMoveExists = false;
	bool nonQuiescentMoveExists = false;
	short bestMoveIndex;
	bool inBounds = false;//if still false at end then we have failed-low
	for (int i = 0; i < moves.size(); ++i)
	{
		bool isQuiescent = board->isQuiescent(moves[i]);
		if (!advance(moves[i]))//not legal move
			continue;
		legalMoveExists = true;
		if (isQuiescent)
		{
			back();
			continue;
		}
		nonQuiescentMoveExists = true;

		Value value{quiescenceSearch(
			-beta, -alpha, searchDepth - 1, rootPly)};

		if (value > alpha)
		{
			inBounds = true;
			bestMoveIndex = i;
			alpha = value;
			if (alpha >= beta)//not in bounds, failed high
			{
				back();
				// if (quiescence_CutNode_RegisterCondition(hBoard,
				// 	hashExistence, searchDepth, rootPly))
				// {
				// 	hashTable->set({board->hash, beta, moves[i],
				// 		searchDepth, rootPly, HashBoard::Quiescence_Cut});
				// }

				//impose hard bound, beta is a lower bound. (value is "too good")
				return -beta;
			}
		}
		back();
	}

	if (!legalMoveExists)//game has reached an end state
	{
		return -staleEval(*board);
	}
	if (!nonQuiescentMoveExists)//game has reached a quiescent position
	{
		return -evaluate(*board);
	}

	if (inBounds)//in bounds, successful search
	{
		// if (quiescence_PVNode_RegisterCondition(hBoard,
		// 	hashExistence, searchDepth, rootPly))
		// {
		// 	hashTable->set({board->hash, alpha, moves[bestMoveIndex],
		// 		searchDepth, rootPly, HashBoard::Quiescence_PV});
		// }

		//alpha is an exact result (for quiescence search)
		return -alpha;
	}
	else//not in bounds, failed low
	{
		// if (quiescence_AllNode_RegisterCondition(hBoard,
		// 	hashExistence, searchDepth, rootPly))
		// {
		// 	hashTable->set({board->hash, alpha, moves[0],//no move info on all-nodes
		// 		searchDepth, rootPly, HashBoard::Quiescence_All});
		// }

		//impose hard bound, alpha is an upper bound. (value is "too bad")
		return -alpha;
	}
}

Value Engine::mainSearch(
	Value alpha, Value beta, PlyType searchDepth, PlyType rootPly)
{
	//negamax search algorithm

	if (searchDepth <= 0)
	{
		return Engine::quiescenceSearch(
			alpha, beta, context->quiescenceSearchDepth, rootPly);
	}
	if (!good())
	{
		return Value();
	}

	if (board->plySinceLastPawnOrCapture >= 99 ||
		threeMoveRepetition())
	{
		//TODO: check for off-by-one issue
		return Value();
	}

	vector<Move> moves(board->generateMoves());


	HashTable::HashOccupancyType hashExistence;
	HashBoard const& hBoard = hashTable->main_HandleHash(
		board->hash, hashExistence, alpha, beta, moves, searchDepth, rootPly);

	if (alpha == beta)//hashTable may set it this way to indicate a return condition
	{
		//make sure we aren't about to draw and the opponents can't force a draw
		bool aboutToDraw = false;
		if (advance(hBoard.bestResponse))
		{
			aboutToDraw = threeMoveRepetition();
			back();
		}
		if (board->plySinceLastPawnOrCapture < 97 && !aboutToDraw)
			return alpha;
	}

	//TODO: perform heuristic ordering

	//fail-hard algorithm

	bool inBounds = false;//if still false at end then we have failed-low
	short bestMoveIndex;

	bool legalMoveExists = false;
	for (int i = 0; i < moves.size(); ++i)
	{
		if (!advance(moves[i]))
			continue;

		legalMoveExists = true;

		Value value{mainSearch(-beta, -alpha, searchDepth - 1, rootPly)};

		if (value > alpha)
		{
			inBounds = true;
			bestMoveIndex = i;
			alpha = value;
			if (alpha >= beta)//not in bounds, failed high
			{
				back();
				// if (main_CutNode_RegisterCondition(hBoard,
				// 	hashExistence, searchDepth, rootPly))
				// {
				// 	hashTable->set({board->hash, beta, moves[i],
				// 		searchDepth, rootPly, HashBoard::CutNode});
				// }

				//impose hard bound, beta is a lower bound. (value is "too good")
				return -beta;
			}
		}
		back();
	}

	if (!legalMoveExists)//game has reached an end condition
	{
		return -staleEval(*board);
	}

	if (inBounds)//in bounds, successful search
	{
		// if (main_PVNode_RegisterCondition(hBoard,
		// 	hashExistence, searchDepth, rootPly))
		// {
		// 	hashTable->set({board->hash, alpha, moves[bestMoveIndex],
		// 		searchDepth, rootPly, HashBoard::PrincipleVariation});
		// }

		//alpha is an exact result
		return -alpha;
	}
	else//not in bounds, failed low
	{
		// if (main_AllNode_RegisterCondition(hBoard,
		// 	hashExistence, searchDepth, rootPly))
		// {
		// 	hashTable->set({board->hash, alpha, moves[0],//no move info on all-nodes
		// 		searchDepth, rootPly, HashBoard::AllNode});
		// }

		//impose hard bound, alpha is an upper bound. (value is "too bad")
		return -alpha;
	}
}

vector<tuple<Value, Move>> Engine::rootSearch(PlyType searchDepth)
{
	PlyType rootPly = board->plyNumber;

	Value alpha{-HUGE_VALF, rootPly};
	Value beta{HUGE_VALF, rootPly};

	vector<Move> moves(board->generateMoves());

	stable_sort(moves.begin(), moves.end(), [this](Move a, Move b)
	{
		Value A = hashEval(a);
		if (A.value == -HUGE_VALF) return false;//move not valid or getting mated
		return A > hashEval(b);
	});

	vector<Value> values;
	vector<short> indices;

	for (short i = 0; i < moves.size(); ++i)
	{
		if (!advance(moves[i]))
		{
			values.emplace_back();//won't be used
			continue;
		}

		auto value = mainSearch(
			-beta, -alpha, searchDepth - 1, rootPly);
		alpha = max(alpha, value);
		values.push_back(value);
		indices.push_back(i);//keeps track of legal moves and implicitly pairs with value
		back();
	}

	//sort into decending order, best moves first
	stable_sort(indices.begin(), indices.end(), [values](short a, short b)
	{
		return values[a] > values[b];
	});

	//TODO: update hash

	vector<tuple<Value, Move>> output;

	for (auto&& index : indices)
	{
		output.emplace_back(values[index], moves[index]);
	}

	return output;
}

void Engine::calculationLoop(Engine* engine)
{
	engine->context->bestMoveStacks.resize(0);

	PlyType searchDepth = engine->context->infiniteFlag || engine->context->maxDepth == 0 ?
		// INT16_MAX :
		4 :
		engine->context->maxDepth;

	PlyType initialSearchDepth = searchDepth % engine->context->depthWalkValue;
	initialSearchDepth = initialSearchDepth == 0 ?
		engine->context->depthWalkValue :
		initialSearchDepth;
	
	for (PlyType currentSearchDepth = initialSearchDepth;
		currentSearchDepth <= searchDepth;
		currentSearchDepth += engine->context->depthWalkValue)
	{
		auto info = engine->rootSearch(currentSearchDepth);
		if (engine->context->bestMoveStacks.size() != 0 &&
			engine->context->stopFlag) break;
		engine->context->bestMoveStacks.resize(0);
		engine->context->bestMoveStacks.push_back({get<1>(info[0])});

		cout << "hash occupancy: " << engine->hashTable->getOccupancy() << " / " << engine->hashTable->getSize() << endl;

		if (engine->context->stopFlag) break;
	}

	engine->context->stopFlag = true;
	
	if (engine->context->bestMoveStacks[0].size() > 0)
		cout << "bestmove " << moveToAlgebraic(engine->context->bestMoveStacks[0][0]) << endl;
	else
		cout << "bestmove " << "0000" << endl;
}
