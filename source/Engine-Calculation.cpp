#include "Engine.hpp"
#include "Board.hpp"
#include "EvaluationContext.hpp"
#include "HashTable.hpp"

// #include "EngineState.hpp"

#include <math.h>
#include <algorithm>
#include <iostream>

using namespace std;

bool Engine::good(){return !context->stopFlag && !context->quitFlag;}

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
	return board.blacksTurn ? -value : value;
}

Value Engine::quiescenceSearch(
	Value alpha, Value beta, short searchDepth, short rootPly)
{
	if (!good())
	{
		return Value();
	}

	if (board->plySinceLastPawnOrCapture >= 100 ||
		threeMoveRepetition())
	{
		//TODO: check for off-by-one issue
		//TODO: clamp?
		return Value();
	}

	if (searchDepth == 0)
	{
		return -evaluate(*board);
	}

	//TODO: perform search on null move node instead of just evaluate?

	{
		Move nullMove = board->buildMoveFromContext(0, 0, Move::NullMove);
		board->performMove(nullMove);

		//we can always just not take
		alpha = max(alpha, quiescenceSearch(-beta, -alpha, searchDepth - 1, rootPly));

		board->reverseMove(nullMove);
	}

	if (alpha >= beta)//null move (not taking) fails high
		return -beta;

	vector<Move> moves(board->generateMoves());

	//TODO: use hash table

	bool legalMoveExists = false;
	bool nonQuiescentMoveExists = false;
	bool inBounds = false;//if still false at end then we have failed-low
	for (auto&& move : moves)
	{
		bool isQuiescent = board->isQuiescent(move);
		if (!advance(move))//not legal move
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
			alpha = value;
			if (alpha >= beta)//not in bounds, failed high
			{
				//impose hard bound, beta is a lower bound. (value is "too good")
				back();
				//This is a cut-node

				//TODO: hash table

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
	if (inBounds)//in bounds, we have an exact search result
	{
		//This is a principle variation node

		//TODO: hash table

		return -alpha;
	}
	else//not in bounds, failed low
	{
		//impose hard bound, alpha is an upper bound. (value is "too bad")

		//This is an all-node

		//TODO: hash table

		return -alpha;
	}
}

Value Engine::nonQuiescenceSearch(
	Value alpha, Value beta, short searchDepth, short rootPly)
{
	//negamax search algorithm

	if (searchDepth == 0)
	{
		// short initialSearchDepth = engine.quiescenceSearchDepth % engine.depthWalkValue;
		// initialSearchDepth = initialSearchDepth == 0 ?
		// 	engine.depthWalkValue :
		// 	initialSearchDepth;
		
		// for (short currentSearchDepth = initialSearchDepth;
		// 	currentSearchDepth <= engine.quiescenceSearchDepth;
		// 	currentSearchDepth += engine.depthWalkValue)
		// {
		// 	auto value = Engine::quiescenceSearch(engine,
		// 		-beta, -alpha, currentSearchDepth, rootPly);
		// 	if (currentSearchDepth == engine.quiescenceSearchDepth)
		// 		return -value;
		// }
		return Engine::quiescenceSearch(
			alpha, beta, context->quiescenceSearchDepth, rootPly);
	}
	if (!good())
	{
		return Value();
	}

	if (board->plySinceLastPawnOrCapture >= 100 ||
		threeMoveRepetition())
	{
		//TODO: check for off-by-one issue
		return Value();
	}

	vector<Move> moves(board->generateMoves());


	HashBoard const& hBoard = hashTable->get(board->hash);
	HashTable::HashOccupancyType hashExistence;
 
	if (hBoard.hash == 0)
	{
		hashExistence = HashTable::HashNotPresent;
	}
	else if (hBoard.hash == board->hash)
	{
		hashExistence = HashTable::HashesEqual;
		//if searched to equal or better depth
		//then we can trust the result
		if (hBoard.searchDepth >= searchDepth)
		{
			switch (hBoard.nodeType)
			{
				case HashBoard::PrincipleVariation://exact value, clamp?
				return -hBoard.value;

				case HashBoard::AllNode://upper bound value
				if (hBoard.value <= alpha) return -alpha;
				else beta = hBoard.value;//try setting depth to hBoard depth
				break;

				case HashBoard::CutNode://lower bound value
				if (hBoard.value >= beta) return -beta;
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
		hashExistence = HashTable::HashesNotEqual;
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

		Value value{nonQuiescenceSearch(-beta, -alpha, searchDepth - 1, rootPly)};

		if (value > alpha)
		{
			inBounds = true;
			bestMoveIndex = i;
			alpha = value;
			if (alpha >= beta)//not in bounds, failed high
			{
				//impose hard bound, beta is a lower bound. (value is "too good")
				back();
				//This is a cut-node
				if (hBoard.rootPly + 2 <= rootPly)
					hashTable->set({board->hash, beta, moves[i],
						searchDepth, rootPly, HashBoard::CutNode});
				else
				{
					if (hashExistence != HashTable::HashesNotEqual)
					{
						hashTable->set({board->hash, beta, moves[i],
							searchDepth, rootPly, HashBoard::CutNode});
					}
					else if (hBoard.nodeType != HashBoard::PrincipleVariation)
						hashTable->set({board->hash, beta, moves[i],
							searchDepth, rootPly, HashBoard::CutNode});
				}
				

				return -beta;
			}
		}
		back();
	}

	if (!legalMoveExists)//game has reached an end condition
	{
		return -staleEval(*board);
	}

	if (inBounds)//in bounds, we have an exact search result
	{
		//This is a principle variation node
		hashTable->set({board->hash, alpha, moves[bestMoveIndex],
			searchDepth, rootPly, HashBoard::PrincipleVariation});

		return -alpha;
	}
	else//not in bounds, failed low
	{
		//impose hard bound, alpha is an upper bound. (value is "too bad")

		//This is an all-node
		if (hashExistence != HashTable::HashesNotEqual || hBoard.rootPly + 2 <= rootPly)
			hashTable->set({board->hash, alpha, moves[0],//no move info on all-nodes
				searchDepth, rootPly, HashBoard::AllNode});

		return -alpha;
	}
}

vector<tuple<Value, Move>> Engine::rootSearch(short searchDepth)
{
	short rootPly = board->plyNumber;

	Value alpha{-HUGE_VALF, rootPly};
	Value beta{HUGE_VALF, rootPly};

	vector<Move> moves(board->generateMoves());

	vector<Value> values;
	vector<short> indices;

	for (short i = 0; i < moves.size(); ++i)
	{
		if (!advance(moves[i]))
		{
			values.emplace_back();//won't be used
			continue;
		}

		auto value = nonQuiescenceSearch(
			-beta, -alpha, searchDepth - 1, rootPly);
		// alpha = max(alpha, value);
		values.push_back(value);
		indices.push_back(i);//keeps track of legal moves and implicitly pairs with value
		back();
	}

	//sort into decending order, best moves first
	sort(indices.begin(), indices.end(), [values](short a, short b)
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

	short searchDepth = engine->context->infiniteFlag || engine->context->maxDepth == 0 ?
		// INT16_MAX :
		4 :
		engine->context->maxDepth;

	short initialSearchDepth = searchDepth % engine->context->depthWalkValue;
	initialSearchDepth = initialSearchDepth == 0 ?
		engine->context->depthWalkValue :
		initialSearchDepth;
	
	for (short currentSearchDepth = initialSearchDepth;
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
