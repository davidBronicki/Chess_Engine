#include "Engine.hpp"
#include "Board.hpp"
#include "EvaluationContext.hpp"
#include "HashTable.hpp"

// #include "EngineState.hpp"

#include <math.h>
#include <algorithm>
#include <iostream>

using namespace std;

inline Value staleEval(Board const& board) // no moves possible, draw or death
{
	if (board.inCheck())
	{
		return Value(0, board.plyNumber);
	}
	return Value();
}

Value evaluate(Board const& board)
{
	Value value;
	value.materialValue += 1000 * cardinality(board.pieceBoards[Piece::Pawn | Piece::White]);
	value.materialValue += 3000 * cardinality(board.pieceBoards[Piece::Knight | Piece::White]);
	value.materialValue += 3000 * cardinality(board.pieceBoards[Piece::Bishop | Piece::White]);
	value.materialValue += 5000 * cardinality(board.pieceBoards[Piece::Rook | Piece::White]);
	value.materialValue += 9000 * cardinality(board.pieceBoards[Piece::Queen | Piece::White]);

	value.materialValue -= 1000 * cardinality(board.pieceBoards[Piece::Pawn | Piece::Black]);
	value.materialValue -= 3000 * cardinality(board.pieceBoards[Piece::Knight | Piece::Black]);
	value.materialValue -= 3000 * cardinality(board.pieceBoards[Piece::Bishop | Piece::Black]);
	value.materialValue -= 5000 * cardinality(board.pieceBoards[Piece::Rook | Piece::Black]);
	value.materialValue -= 9000 * cardinality(board.pieceBoards[Piece::Queen | Piece::Black]);

	value.materialValue += 500;
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
		return (hBoard.nodeType & HashBoard::Quiescence_Mask) || // dont replace non-quiescent search
			   hBoard.rootPly + 2 <= rootPly;					 // not sure of this
		case HashTable::HashesNotEqual:
		return hBoard.rootPly + 2 <= rootPly || // not sure of this
			   (hBoard.nodeType == HashBoard::Quiescence_All &&
				hBoard.searchDepth < searchDepth);
		default: return false;
	}
}

inline bool quiescence_AllNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hashExistence == HashTable::HashNotPresent ||
		   (hBoard.nodeType & HashBoard::Quiescence_Mask) ||
		   hBoard.rootPly + 2 <= rootPly; // not sure of this
}

inline bool quiescence_PVNode_RegisterCondition(
	HashBoard const& hBoard, HashTable::HashOccupancyType hashExistence,
	PlyType searchDepth, PlyType rootPly)
{
	return hashExistence == HashTable::HashNotPresent ||
		   (hashExistence == HashTable::HashesEqual &&
			(hBoard.nodeType & HashBoard::TypeInfo_Mask)) ||
		   hBoard.rootPly + 2 <= rootPly; // not sure of this
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

	if (searchDepth <= 0)
	{
		return -evaluate(*board);
	}

	if (!board->inCheck()) // can't choose a different move when in check
	{
		// //we can always just not take, but not if we're in check
		// //should become a good position quickly, so search shallow
		// Move nullMove = board->buildMoveFromContext(0, 0, Move::NullMove);
		// board->performMove(nullMove);

		// alpha = max(alpha, quiescenceSearch(-beta, -alpha, searchDepth - 2, rootPly));

		// board->reverseMove(nullMove);

		alpha = max(alpha, evaluate(*board));
	}

	if (alpha >= beta) // null move (not taking) fails high
		return -beta;

	vector<Move> moves(board->generateMoves());

	// HashTable::HashOccupancyType hashExistence;
	// HashBoard const &hBoard = hashTable->quiescence_HandleHash(
	// 	board->hash, hashExistence, alpha, beta, moves, searchDepth, rootPly);

	if (alpha >= beta) // trusted exact or bounded move in the hash table
	{
		/*
		TODO:
		Asperation window to not acidentally take a draw.
		Alternatively we could add a "repetition number" hash
		which we give to the hash table as a modifier each time.

		I kind of like this second idea, but it will require some
		reworking of the code, so I'm trying the first idea first.
		*/
		return -alpha;
	}

	bool legalMoveExists = false;
	bool nonQuiescentMoveExists = false;
	short bestMoveIndex;
	bool inBounds = false; // if still false at end then we have failed-low
	for (int i = 0; i < moves.size(); ++i)
	{
		bool isQuiescent = board->isQuiescent(moves[i]);
		if (!advance(moves[i])) // not legal move
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
			if (alpha >= beta) // not in bounds, failed high
			{
				back();
				// if (quiescence_CutNode_RegisterCondition(hBoard,
				// 	hashExistence, searchDepth, rootPly))
				// {
				// 	hashTable->set({board->hash, beta, moves[i],
				// 		searchDepth, rootPly, HashBoard::Quiescence_Cut});
				// }

				// impose hard bound, beta is a lower bound. (value is "too good")
				return -beta;
			}
		}
		back();
	}

	if (!legalMoveExists) // game has reached an end state
	{
		return -staleEval(*board);
	}
	if (!nonQuiescentMoveExists) // game has reached a quiescent position
	{
		return -evaluate(*board);
	}

	if (inBounds) // in bounds, successful search
	{
		// if (quiescence_PVNode_RegisterCondition(hBoard,
		// 	hashExistence, searchDepth, rootPly))
		// {
		// 	hashTable->set({board->hash, alpha, moves[bestMoveIndex],
		// 		searchDepth, rootPly, HashBoard::Quiescence_PV});
		// }

		// alpha is an exact result (for quiescence search)
		return -alpha;
	}
	else // not in bounds, failed low
	{
		// if (quiescence_AllNode_RegisterCondition(hBoard,
		// 	hashExistence, searchDepth, rootPly))
		// {
		// 	hashTable->set({board->hash, alpha, moves[0],//no move info on all-nodes
		// 		searchDepth, rootPly, HashBoard::Quiescence_All});
		// }

		// impose hard bound, alpha is an upper bound. (value is "too bad")
		return -alpha;
	}
}

uc Engine::windowMainSearch(
	int& bestMoveIndex, vector<Move> const& moves,
	Value& alpha, Value beta, PlyType searchDepth, PlyType rootPly)
{
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
		alpha = -staleEval(*board);
		return HashBoard::PrincipleVariation;
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
	HashTable::HashOccupancyType hashExistence = hashTable->getExistence(hBoard, board->hash);
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

	stable_sort(moves.begin(), moves.end(), [this](Move a, Move b)
		{
			Value A = hashEval(a);
			if (A.matePlyNumber < 0) return false;//move not valid or getting mated
			return A > hashEval(b);
		});

	vector<Value> values;
	vector<short> indices;

	for (short i = 0; i < moves.size(); ++i)
	{
		if (!advance(moves[i]))
		{
			values.emplace_back(); // won't be used
			continue;
		}

		auto value = mainSearch(
			-beta, -alpha, searchDepth - 1, rootPly);
		alpha = max(alpha, value);
		values.push_back(value);
		indices.push_back(i); // keeps track of legal moves and implicitly pairs with value
		back();
	}

	// sort into decending order, best moves first
	stable_sort(indices.begin(), indices.end(), [values](short a, short b)
				{ return values[a] > values[b]; });

	// TODO: update hash

	vector<tuple<Value, Move>> output;

	for (auto &&index : indices)
	{
		output.emplace_back(values[index], moves[index]);
	}

	return output;
}

void Engine::calculationLoop(Engine *engine)
{
	engine->context->bestMoveStacks.resize(0);

	PlyType searchDepth = engine->context->infiniteFlag || engine->context->maxDepth == 0 ?
																						  // INT16_MAX :
							  4
																						  : engine->context->maxDepth;

	PlyType initialSearchDepth = searchDepth % engine->context->depthWalkValue;
	initialSearchDepth = initialSearchDepth == 0 ? engine->context->depthWalkValue : initialSearchDepth;

	for (PlyType currentSearchDepth = initialSearchDepth;
		 currentSearchDepth <= searchDepth;
		 currentSearchDepth += engine->context->depthWalkValue)
	{
		auto info = engine->rootSearch(currentSearchDepth);
		if (engine->context->bestMoveStacks.size() != 0 &&
			engine->context->stopFlag)
			break;
		engine->context->bestMoveStacks.resize(0);
		engine->context->bestMoveStacks.push_back({get<1>(info[0])});

		cout << "hash occupancy: " << engine->hashTable->getOccupancy() << " / " << engine->hashTable->getSize() << endl;

		if (engine->context->stopFlag)
			break;
	}

	engine->context->stopFlag = true;

	if (engine->context->bestMoveStacks[0].size() > 0)
		cout << "bestmove " << moveToAlgebraic(engine->context->bestMoveStacks[0][0]) << endl;
	else
		cout << "bestmove "
			 << "0000" << endl;
}
