#include "Engine.hpp"

#include "Board.hpp"
#include "EvaluationContext.hpp"
#include "HashTable.hpp"

#include <math.h>

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
		static_cast<size_t>(board->plySinceLastPawnOrCapture)); i += 2)
	{
		if (context->activeHashes[context->activeHashes.size() - 1 - i] == context->activeHashes.back())
			++count;
	}
	return count >= 3;
}

Value Engine::hashEval(Move move)
{
	board->performMove(move);
	HashBoard const& hBoard{hashTable->get(board->hash)};
	HashType hash = board->hash;
	board->reverseMove(move);

	if (hash != hBoard.hash) return Value{-HUGE_VALF};
	return -hBoard.value;
}

HashBoard const& HashTable::nonQuiescence_HandleHash(HashType const& hash,
	HashOccupancyType& existence, Value& alpha, Value& beta,
	std::vector<Move>& moves, PlyType searchDepth, PlyType rootPly) const
{
	HashBoard const& hBoard = get(hash);
	if (hBoard.hash == 0)
	{
		existence = HashTable::HashNotPresent;
	}
	else if (hBoard.hash == hash)
	{
		existence = HashTable::HashesEqual;
		//if searched to equal or better depth
		//then we can trust the result
		if (hBoard.searchDepth >= searchDepth && hBoard.rootPly + 1 < rootPly)
		{
			switch (hBoard.nodeType)
			{
				case HashBoard::PrincipleVariation://exact value, clamp?
				alpha = hBoard.value;
				beta = hBoard.value;

				case HashBoard::AllNode://upper bound value
				if (hBoard.value <= alpha)
				{
					beta = alpha;
					return hBoard;
				}
				else
				{
					beta = hBoard.value;
					searchDepth = hBoard.searchDepth;//allows the result to be equally trustworthy
				}
				break;

				case HashBoard::CutNode://lower bound value
				if (hBoard.value >= beta)
				{
					alpha = beta;
					return hBoard;
				}
				else
				{
					alpha = hBoard.value;
					searchDepth = hBoard.searchDepth;//allows the result to be equally trustworthy
				}
				break;

				default://Can't trust quiescence searches durring non-quiescence search
				break;
			}
		}

		//if we have an all-node then there is no ordering knowledge to be gained
		if ((hBoard.nodeType & HashBoard::TypeInfo_Mask) != HashBoard::AllNode)
		for (int i = 0; i < moves.size(); ++i)
		{
			if (moves[i] == hBoard.bestResponse)
			{
				std::swap(moves[0], moves[i]);
				break;
			}
		}
	}
	else
	{
		existence = HashTable::HashesNotEqual;
	}
	return hBoard;
}

HashBoard const& HashTable::quiescence_HandleHash(HashType const& hash,
	HashOccupancyType& existence, Value& alpha, Value& beta,
	std::vector<Move>& moves, PlyType searchDepth, PlyType rootPly) const
{
	HashBoard const& hBoard = get(hash);
	if (hBoard.hash == 0)
	{
		existence = HashTable::HashNotPresent;
	}
	else if (hBoard.hash == hash)
	{
		existence = HashTable::HashesEqual;
		//if searched to equal or better depth
		//then we can trust the result
		if (hBoard.searchDepth >= searchDepth && hBoard.rootPly + 1 < rootPly)
		{
			switch (hBoard.nodeType & HashBoard::TypeInfo_Mask)//we don't care if its quiescent or not
			{
				case HashBoard::PrincipleVariation://exact value, clamp?
				alpha = hBoard.value;
				beta = hBoard.value;

				case HashBoard::AllNode://upper bound value
				if (hBoard.value <= alpha)
				{
					beta = alpha;
					return hBoard;
				}
				else
				{
					beta = hBoard.value;
					searchDepth = hBoard.searchDepth;//allows the result to be equally trustworthy
				}
				break;

				case HashBoard::CutNode://lower bound value
				if (hBoard.value >= beta)
				{
					alpha = beta;
					return hBoard;
				}
				else
				{
					alpha = hBoard.value;
					searchDepth = hBoard.searchDepth;//allows the result to be equally trustworthy
				}
				break;

				default://will never be reached
				break;
			}
		}

		//if we have an all-node then there is no ordering knowledge to be gained
		if ((hBoard.nodeType & HashBoard::TypeInfo_Mask) != HashBoard::AllNode)
		for (int i = 0; i < moves.size(); ++i)
		{
			if (moves[i] == hBoard.bestResponse)
			{
				std::swap(moves[0], moves[i]);
				break;
			}
		}
	}
	else
	{
		existence = HashTable::HashesNotEqual;
	}
	return hBoard;
}
