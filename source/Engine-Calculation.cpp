#include "Engine.hpp"
#include "Board.hpp"
#include "EvaluationContext.hpp"
#include "HashTable.hpp"

#include "Evaluation.hpp"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <chrono>

using namespace std;

inline HashTable::HashOccupancyType getExistence(HashBoard const& hBoard, HashType hash)
{
	if (hBoard.hash == 0) return HashTable::HashNotPresent;
	if (hBoard.hash == hash) return HashTable::HashesEqual;
	return HashTable::HashesNotEqual;
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
		return (hBoard.nodeType & HashBoard::Quiescence_Mask &&// dont replace non-quiescent search
					hBoard.searchDepth < searchDepth) ||
				hBoard.rootPly + 2 <= rootPly;
		case HashTable::HashesNotEqual:
		return hBoard.rootPly + 2 <= rootPly ||
				hBoard.nodeType == HashBoard::Quiescence_All ||
				hBoard.nodeType == HashBoard::Quiescence_Cut &&
					hBoard.searchDepth < searchDepth;
		default: return false;
	}
}

inline bool quiescence_AllNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hashExistence == HashTable::HashNotPresent ||
		   (hBoard.nodeType == HashBoard::Quiescence_All) ||
		   hBoard.rootPly + 2 <= rootPly;
}

inline bool quiescence_PVNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hashExistence == HashTable::HashNotPresent ||
		(hBoard.nodeType & HashBoard::Quiescence_Mask) ||
		hBoard.rootPly + 2 <= rootPly;
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

inline bool quiescence_RegisterCondition(HashBoard::NodeType nodeType,
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	switch (nodeType)
	{
		case HashBoard::Quiescence_PV:
		return quiescence_PVNode_RegisterCondition(
			hBoard, hashExistence, searchDepth, rootPly);
		case HashBoard::Quiescence_All:
		return quiescence_AllNode_RegisterCondition(
			hBoard, hashExistence, searchDepth, rootPly);
		case HashBoard::Quiescence_Cut:
		return quiescence_CutNode_RegisterCondition(
			hBoard, hashExistence, searchDepth, rootPly);
		default: return false;
	}
}

inline bool main_RegisterCondition(HashBoard::NodeType nodeType,
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	switch (nodeType)
	{
		case HashBoard::PrincipleVariation:
		return main_PVNode_RegisterCondition(
			hBoard, hashExistence, searchDepth, rootPly);
		case HashBoard::AllNode:
		return main_AllNode_RegisterCondition(
			hBoard, hashExistence, searchDepth, rootPly);
		case HashBoard::CutNode:
		return main_CutNode_RegisterCondition(
			hBoard, hashExistence, searchDepth, rootPly);
		default: return false;
	}
}

uc Engine::windowQuiescenceSearch(
	int& bestMoveIndex, std::vector<Move> const& moves,
	Value& alpha, Value beta, PlyType searchDepth, PlyType rootPly)
{
	++context->nodesReached;
	bool legalMoveExists = false;
	bool nonQuiescentMoveExists = false;
	bestMoveIndex = 0;
	bool inBounds = false; // if still false at end then we have failed-low
	for (int i = 0; i < moves.size(); ++i)
	{
		bool isQuiescent;
		if (searchDepth <= 0)
		{
			isQuiescent = board->isQuiescent_weak(moves[i]);
		}
		else
		{
			isQuiescent = board->isQuiescent_strong(moves[i]);
		}
		if (!advance(moves[i])) // not legal move
			continue;
		legalMoveExists = true;
		if (isQuiescent)
		{
			back();
			continue;
		}
		nonQuiescentMoveExists = true;

		//TODO: delta pruning and static capture analysis

		if (abs(captureDelta(*board, moves[i])) + 2000 < alpha)
		{
			//delta pruning: if a capture does not improve matters
			//by more that two pawns then its probably not
			//worth exploring.
			back();
			continue;
		}

		Value value{quiescenceSearch(
			-beta, -alpha, max(searchDepth - 1, 0), rootPly)};

		if (value > alpha)
		{
			inBounds = true;
			bestMoveIndex = i;
			alpha = value;
			if (alpha >= beta) // not in bounds, failed high
			{
				back();
				// impose hard bound, beta is a lower bound. (value is "too good")
				alpha = beta;
				return HashBoard::Quiescence_Cut;
			}
		}
		back();
	}

	if (!legalMoveExists) // game has reached an end state
	{
		Value value{staleEval(*board)};
		if (value <= alpha)
		{
			return HashBoard::Quiescence_All;
		}
		else if (value >= beta)
		{
			alpha = beta;
			return HashBoard::Quiescence_Cut;
		}
		else
		{
			alpha = value;
			return HashBoard::Quiescence_PV;
		}
	}
	if (legalMoveExists && !nonQuiescentMoveExists) // game has reached a quiescent position
	{
		Value value{evaluate(*board)};
		if (value <= alpha)
		{
			return HashBoard::Quiescence_All;
		}
		else if (value >= beta)
		{
			alpha = beta;
			return HashBoard::Quiescence_Cut;
		}
		else
		{
			alpha = value;
			return HashBoard::Quiescence_PV;
		}
	}

	return inBounds ?
		HashBoard::Quiescence_PV :
		HashBoard::Quiescence_All;
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
		// TODO: check for off-by-one issue
		return Value();
	}

	// if (searchDepth <= 0)
	// {
	// 	return -evaluate(*board);
	// }

	// if (searchDepth > 0 && advance(board->buildNullMove())) // can't choose a different move when in check
	// {
	// 	alpha = max(alpha, quiescenceSearch(alpha, beta, searchDepth - 1, rootPly));
	// 	back();
	// }

	if (!board->inCheck())
	{
		alpha = max(alpha, evaluate(*board));
	}

	if (alpha >= beta) // null move (not taking) fails high
		return -beta;

	vector<Move> moves(board->generateMoves());

	HashBoard const &hBoard = hashTable->get(board->hash);
	HashTable::HashOccupancyType hashExistence = getExistence(hBoard, board->hash);
	Value output{alpha};
	HashBoard::NodeType nodeType;
	int bestMoveIndex;
	if (hashExistence == HashTable::HashesEqual)
	{
		Value alpha_prime{alpha}; // copies in case we have to reset window
		Value beta_prime{beta};
		if (hashTable->quiescence_HashWindowSetup(
			hBoard, alpha, beta, moves, searchDepth, rootPly))
			return -alpha;

		output = alpha;

		/*
		--------alpha_prime---------alpha--------output----------beta---------beta_prime

		If output cuts, then it better cut on beta_prime. Otherwise we
		have to repeat the search between beta and beta_prime.

		If output->all node, then it better fail low on alpha_prime. Otherwise
		repeat the search between alpha_prime and alpha.

		Principle variations are always good.
		*/
		nodeType = static_cast<HashBoard::NodeType>(windowQuiescenceSearch(
			bestMoveIndex, moves, output, beta, searchDepth, rootPly));
		switch (nodeType)
		{
			case HashBoard::Quiescence_Cut:
			if (output != beta_prime)
			{
				//false window, check upper section
				alpha = beta.worse(1);//ensure capture of PV node with value = alpha
				beta = beta_prime;
				output = alpha;
				nodeType = static_cast<HashBoard::NodeType>(windowQuiescenceSearch(
					bestMoveIndex, moves, output, beta, searchDepth, rootPly));
			}
			break;
		
			case HashBoard::Quiescence_All:
			if (output != alpha_prime)
			{
				//false window, check lower section
				beta = alpha.better(1);//ensure capture of PV node with value = beta
				alpha = alpha_prime;
				output = alpha;
				nodeType = static_cast<HashBoard::NodeType>(windowQuiescenceSearch(
					bestMoveIndex, moves, output, beta, searchDepth, rootPly));
			}
			break;

			default://PV nodes are good
			break;
		}
	}
	else
	{
		nodeType = static_cast<HashBoard::NodeType>(windowQuiescenceSearch(
			bestMoveIndex, moves, output, beta, searchDepth, rootPly));
	}

	if (quiescence_RegisterCondition(nodeType, hBoard,
		hashExistence, searchDepth, rootPly))
	{
		hashTable->set({board->hash, output, moves[bestMoveIndex],
			searchDepth, rootPly, nodeType});
	}
	return -output;
}

uc Engine::windowMainSearch(
	int& bestMoveIndex, vector<Move> const& moves,
	Value& alpha, Value beta, PlyType searchDepth, PlyType rootPly)
{
	//alpha doubles as the main output
	++context->nodesReached;

	bestMoveIndex = 0;
	bool inBounds = false; // if still false at end then we have failed-low
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
			if (alpha >= beta) // not in bounds, failed high
			{
				back();
				// impose hard bound, beta is a lower bound. (value is "too good")
				alpha = beta;
				return HashBoard::CutNode;
			}
		}
		back();
	}

	if (!legalMoveExists) // game has reached an end condition
	{
		Value value{staleEval(*board)};
		if (value <= alpha)
		{
			return HashBoard::AllNode;
		}
		else if (value >= beta)
		{
			alpha = beta;
			return HashBoard::CutNode;
		}
		else
		{
			alpha = value;
			return HashBoard::PrincipleVariation;
		}
	}

	return inBounds ?
		HashBoard::PrincipleVariation :
		HashBoard::AllNode;
}

Value Engine::mainSearch(
	Value alpha, Value beta, PlyType searchDepth, PlyType rootPly)
{
	// negamax search algorithm

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
		// TODO: check for off-by-one issue
		return Value();
	}

	vector<Move> moves{board->generateMoves()};
	// TODO: perform heuristic ordering

	HashBoard const &hBoard = hashTable->get(board->hash);
	HashTable::HashOccupancyType hashExistence = getExistence(hBoard, board->hash);
	Value output{alpha};
	HashBoard::NodeType nodeType;
	int bestMoveIndex;
	if (hashExistence == HashTable::HashesEqual)
	{
		Value alpha_prime{alpha}; // copies in case we have to reset window
		Value beta_prime{beta};
		hashTable->main_HashWindowSetup(
			hBoard, alpha, beta, moves, searchDepth, rootPly);
		output = alpha;

		/*
		--------alpha_prime---------alpha--------output----------beta---------beta_prime

		If output cuts, then it better cut on beta_prime. Otherwise we
		have to repeat the search between beta and beta_prime.

		If output->all node, then it better fail low on alpha_prime. Otherwise
		repeat the search between alpha_prime and alpha.

		Principle variations are always good.
		*/
		nodeType = static_cast<HashBoard::NodeType>(windowMainSearch(
			bestMoveIndex, moves, output, beta, searchDepth, rootPly));
		switch (nodeType)
		{
			case HashBoard::CutNode:
			if (output != beta_prime)
			{
				//false window, check upper section
				alpha = beta.worse(1);//ensure capture of PV node with value = alpha
				beta = beta_prime;
				output = alpha;
				nodeType = static_cast<HashBoard::NodeType>(windowMainSearch(
					bestMoveIndex, moves, output, beta, searchDepth, rootPly));
			}
			break;
		
			case HashBoard::AllNode:
			if (output != alpha_prime)
			{
				//false window, check lower section
				beta = alpha.better(1);//ensure capture of PV node with value = beta
				alpha = alpha_prime;
				output = alpha;
				nodeType = static_cast<HashBoard::NodeType>(windowMainSearch(
					bestMoveIndex, moves, output, beta, searchDepth, rootPly));
			}
			break;

			default://PV nodes are good
			break;
		}
	}
	else
	{
		nodeType = static_cast<HashBoard::NodeType>(windowMainSearch(
			bestMoveIndex, moves, output, beta, searchDepth, rootPly));
	}
	if (main_RegisterCondition(nodeType, hBoard,
		hashExistence, searchDepth, rootPly))
	{
		hashTable->set({board->hash, output, moves[bestMoveIndex],
			searchDepth, rootPly, nodeType});
	}
	return -output;
}

vector<tuple<Value, Move>> Engine::rootSearch(PlyType searchDepth)
{
	PlyType rootPly = board->plyNumber;

	Value alpha{0, -rootPly};
	Value beta{0, rootPly};

	vector<Move> moves(board->generateMoves());

	vector<Value> values;
	vector<int> indices;

	for (int i = 0; i < moves.size(); ++i)
	{
		if (!advance(moves[i]))
		{
			values.emplace_back(); // won't be used
			continue;
		}

		HashBoard const& hBoard{hashTable->get(board->hash)};
		HashType hash = board->hash;

		if (hash != hBoard.hash)
			values.emplace_back(0, -board->plyNumber);
		else
			values.push_back(-hBoard.value);
		
		indices.push_back(i);

		back();
	}

	stable_sort(indices.begin(), indices.end(), [values](int a, int b)
		{
			return values[a] > values[b];
		});

	for (int i : indices)
	{
		advance(moves[i]);//already checked that it works

		values[i] = mainSearch(
			-beta, -alpha, searchDepth - 1, rootPly);
		alpha = max(alpha, values[i]);
		back();
	}

	// sort into decending order, best moves first
	stable_sort(indices.begin(), indices.end(), [values](short a, short b)
		{
			return values[a] > values[b];
		});

	// TODO: update hash

	vector<tuple<Value, Move>> output;

	for (int index : indices)
	{
		output.emplace_back(values[index], moves[index]);
	}

	return output;
}

void Engine::calculationLoop(Engine *engine)
{
	//setup search depth
	PlyType searchDepth = engine->context->infiniteFlag || engine->context->maxDepth == 0 ?
		// INT16_MAX :
		5 :
		engine->context->maxDepth;

	PlyType initialSearchDepth = searchDepth % engine->context->depthWalkValue;
	initialSearchDepth = initialSearchDepth == 0 ? engine->context->depthWalkValue : initialSearchDepth;



	engine->context->depth.set(0);
	engine->context->selectiveDepth.set(0);

	auto start = std::chrono::high_resolution_clock::now();
	engine->context->nodesReached = 0;
	engine->context->timeSpent = 0;//milliseconds
	engine->context->pvLines.resize(1);

	for (PlyType currentSearchDepth = initialSearchDepth;
		 currentSearchDepth <= searchDepth;
		 currentSearchDepth += engine->context->depthWalkValue)
	{
		auto info = engine->rootSearch(currentSearchDepth);


		auto bestTuple = info[0];

		engine->context->pvLines[0].set({
			engine->getPVLine(get<1>(bestTuple)),
			get<0>(bestTuple),
			currentSearchDepth,
			currentSearchDepth});

		auto end = chrono::high_resolution_clock::now();
		chrono::duration<double> elapsedTime = end - start;

		engine->context->timeSpent = elapsedTime.count() * 1000;

		if (currentSearchDepth > 1)
			engine->updateUI();

		if (engine->context->stopFlag)
			break;
	}

	//TODO: don't return if in infinite search mode

	engine->context->stopFlag = true;

	engine->returnResult();
}
