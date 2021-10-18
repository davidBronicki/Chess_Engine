#include "Board.hpp"

// using namespace std;

bool Board::inCheck() const
{
	int kingPos = firstIndex(pieceBoards[Piece::King | blacksTurn]);
	return positionAttacked(kingPos, !blacksTurn);
}
bool Board::miscLegalityCheck(Move move) const
{
	switch (move.moveType)
	{
	case MoveType::BlackShort:
	return !inCheck() && !positionAttacked(5 + 7 * 8, Piece::White);
	case MoveType::BlackLong:
	return !inCheck() && !positionAttacked(3 + 7 * 8, Piece::White);
	case MoveType::WhiteShort:
	return !inCheck() && !positionAttacked(5, Piece::Black);
	case MoveType::WhiteLong:
	return !inCheck() && !positionAttacked(3, Piece::Black);
	
	default:
	return true;
	}
}
bool Board::positionAttacked(int pos, bool byBlack) const
{
	BitBoard attacks = boardIntersect(
		Board::KnightMoves[pos],
		pieceBoards[Piece::Knight | byBlack]);
	if (attacks != 0ull) return true;
	

	for (int i = 0; i < 8; ++i)
	{
		int hitIndex = i < 4 ?
			firstIndex(boardIntersect(
				SlideMoves[i][pos],
				pieceBoards[Piece::IndexAll]
			)) :
			lastIndex(boardIntersect(
				SlideMoves[i][pos],
				pieceBoards[Piece::IndexAll]
			));
		if (boardIntersect(indexToBitBoard(hitIndex), pieceBoards[byBlack]))
		{
			if (i % 2) //rook move
			{
				if ((fullBoard[hitIndex] & Piece::Rook) == Piece::Rook)
					return true;
			}
			else
			{
				if ((fullBoard[hitIndex] & Piece::Bishop) == Piece::Bishop)
					return true;
			}
		}
	}


	BitBoard pawns = pieceBoards[Piece::Pawn | byBlack];

	if (byBlack)
	{
		attacks = boardIntersect(indexToBitBoard(pos),
			shiftDown(shiftRight(pawns, 1), 1) |
			shiftDown(shiftLeft(pawns, 1), 1));
	}
	else
	{
		attacks = boardIntersect(indexToBitBoard(pos),
			shiftUp(shiftRight(pawns, 1), 1) |
			shiftUp(shiftLeft(pawns, 1), 1));
	}
	if (attacks != 0ull) return true;
	if (boardIntersect(KingMoves[pos],
		pieceBoards[Piece::King | byBlack]) != 0ull) return true;

	return false;
}

bool Board::isQuiescent(Move move) const
{
	if (move.deltaTarget != fullBoard[move.sourceSquare])
		return false;//finds captures and promotions
	if (inCheck()) return false;//escaping check isn't quiescent
	us enemyKingPos = firstIndex(pieceBoards[Piece::King | !blacksTurn]);
	uc moveDirection = RelativeDirection[move.targetSquare][enemyKingPos];
	if ((move.deltaTarget & Piece::Bishop) == Piece::Bishop &&//bishop-type piece moved
		moveDirection % 2 == 1)//onto a diagonal with the enemy king
	{
		if (moveDirection < 4)//requires forward scan
		{
			if (firstIndex(boardIntersect(SlideMoves[moveDirection][move.targetSquare], pieceBoards[Piece::IndexAll]))
				== enemyKingPos) return false;//checking the enemy king isn't quiescent
		}
		else//requires backward scan
		{
			if (lastIndex(boardIntersect(SlideMoves[moveDirection][move.targetSquare], pieceBoards[Piece::IndexAll]))
				== enemyKingPos) return false;//checking the enemy king isn't quiescent
		}
		return true;
	}
	if ((move.deltaTarget & Piece::Rook) == Piece::Rook &&//rook-type piece moved
		moveDirection % 2 == 1)//onto a rank or file with the enemy king
	{
		if (moveDirection < 4)//requires forward scan
		{
			if (firstIndex(boardIntersect(SlideMoves[moveDirection][move.targetSquare], pieceBoards[Piece::IndexAll]))
				== enemyKingPos) return false;//checking the enemy king isn't quiescent
		}
		else//requires backward scan
		{
			if (lastIndex(boardIntersect(SlideMoves[moveDirection][move.targetSquare], pieceBoards[Piece::IndexAll]))
				== enemyKingPos) return false;//checking the enemy king isn't quiescent
		}
	}
	return true;
}
