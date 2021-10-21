#include "Engine.hpp"

#include "Board.hpp"
#include "EvaluationContext.hpp"
#include "HashTable.hpp"

#include <math.h>

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
