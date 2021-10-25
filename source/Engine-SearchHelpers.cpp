#include "Engine.hpp"

#include "Board.hpp"
#include "EvaluationContext.hpp"
#include "HashTable.hpp"

#include <math.h>
// #include <algorithm>//clamp function

bool Engine::good()
{
	return !context->stopFlag && !context->quitFlag;
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
	context->activeMoves.push_back(move);
	context->activeHashes.push_back(board->hash);
	return true;
}
void Engine::back()
{
	board->reverseMove(context->activeMoves.back());
	context->activeMoves.pop_back();
	context->activeHashes.pop_back();
}
bool Engine::threeMoveRepetition()
{
	//TODO: check off-by-one issues
	int count = 0;
	for (int i = 0; i < std::min(context->activeHashes.size(),
		static_cast<size_t>(board->plySinceLastPawnOrCapture + 1)); i += 2)
	{
		if (context->activeHashes[context->activeHashes.size() - 1 - i] == context->activeHashes.back())
			++count;
	}
	return count >= 3;
}


bool HashTable::main_HashWindowSetup(
	HashBoard const& hBoard, Value& alpha, Value& beta,
	std::vector<Move>& moves, PlyType searchDepth, PlyType rootPly) const
{
	bool located = false;//gaurds against type I hash collisions
	for (int i = 0; i < moves.size(); ++i)
	{
		if (moves[i] == hBoard.bestResponse)
		{
			std::swap(moves[0], moves[i]);
			located = true;
			break;
		}
	}
	if (!located) return false;

	if (isNullWindow(alpha, beta)) return false;
	
	long trustWindow = hBoard.searchDepth == searchDepth ?
		1 ://super trusted, use a null window
		500;//somewhat trusted, use an asperation window
	//if searched to equal or better depth
	//then we can trust the result
	switch (hBoard.nodeType)
	{
		case HashBoard::PrincipleVariation://good value
		alpha = std::clamp(hBoard.value.worse(trustWindow), alpha, beta);
		beta = std::clamp(hBoard.value.better(trustWindow), alpha, beta);

		case HashBoard::AllNode://upper bound value
		beta = std::clamp(hBoard.value.better(trustWindow), alpha.better(trustWindow), beta);
		break;

		case HashBoard::CutNode://lower bound value
		alpha = std::clamp(hBoard.value.worse(trustWindow), alpha, beta.worse(trustWindow));
		break;

		default://Can't trust quiescence searches durring non-quiescence search
		break;
	}
	return true;
}

bool HashTable::quiescence_HashWindowSetup(
	HashBoard const& hBoard, Value& alpha, Value& beta,
	std::vector<Move>& moves, PlyType searchDepth, PlyType rootPly) const
{
	bool located = false;//gaurds against type I hash collisions
	for (int i = 0; i < moves.size(); ++i)
	{
		if (moves[i] == hBoard.bestResponse)
		{
			std::swap(moves[0], moves[i]);
			located = true;
			break;
		}
	}
	if (!located) return false;

	if (isNullWindow(alpha, beta)) return false;
	
	// long trustWindow = !(hBoard.nodeType & HashBoard::Quiescence_Mask) ?
	// 	0 :
	// 	hBoard.searchDepth == searchDepth ?
	// 		1 ://super trusted, use a null window
	// 		500;//somewhat trusted, use an asperation window

	long trustWindow = hBoard.searchDepth == searchDepth ?
		1 ://super trusted, use a null window
		500;//somewhat trusted, use an asperation window

	//if searched to equal or better depth
	//then we can trust the result
	switch (hBoard.nodeType)
	{
		case HashBoard::PrincipleVariation://good value
		alpha = std::clamp(hBoard.value, alpha, beta);
		beta = alpha;
		break;

		case HashBoard::AllNode://upper bound value
		beta = std::clamp(hBoard.value, alpha, beta);
		break;

		case HashBoard::CutNode://lower bound value
		alpha = std::clamp(hBoard.value, alpha, beta);
		break;

		case HashBoard::Quiescence_PV:
		alpha = std::clamp(hBoard.value.worse(trustWindow), alpha, beta.worse(1));
		beta = std::clamp(hBoard.value.better(trustWindow), alpha.better(1), beta);
		break;

		case HashBoard::Quiescence_All:
		beta = std::clamp(hBoard.value.better(trustWindow), alpha.better(1), beta);
		break;

		case HashBoard::Quiescence_Cut:
		alpha = std::clamp(hBoard.value.worse(trustWindow), alpha, beta.worse(1));
		break;

		default://no remaining cases, this stops a warning message
		break;
	}
	return !(hBoard.nodeType & HashBoard::Quiescence_Mask);
}

void Engine::getPVLine(std::vector<Move>& movesThusFar)
{
	//can encounter null moves for an unknown reason
	//happen to reach hash with 24 0 bits?
	//generate moves and check that best is in there
	//to circumvent this

	//if repeated moves are performed but not searched
	//deep enough to break repetition then
	//it is possible to get stuck in a loop.
	//compare moves found to HashBoard.searchDepth to circumvent.

	Move move{movesThusFar.back()};

	board->performMove(move);

	HashBoard const& hBoard{hashTable->get(board->hash)};
	if (hBoard.hash == board->hash)
	{
		if (movesThusFar.size() + hBoard.searchDepth <= context->searchDepth)
		{
			std::vector<Move> availableMoves{board->generateMoves()};
			bool moveExists = false;
			for (auto&& pseudoLegalMove : availableMoves)
			{
				if (pseudoLegalMove == hBoard.bestResponse)
				{
					moveExists = true;
					break;
				}
			}
			if (moveExists)
			{
				movesThusFar.push_back(hBoard.bestResponse);
				getPVLine(movesThusFar);
			}
		}
	}
	board->reverseMove(move);
}

std::vector<Move> Engine::getPVLine(Move move)
{
	std::vector<Move> output{move};
	getPVLine(output);
	return output;
}
