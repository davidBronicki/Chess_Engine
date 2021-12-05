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
	case Move::BlackShort:
	return !inCheck() && !positionAttacked(5 + 7 * 8, Piece::White);
	case Move::BlackLong:
	return !inCheck() && !positionAttacked(3 + 7 * 8, Piece::White);
	case Move::WhiteShort:
	return !inCheck() && !positionAttacked(5, Piece::Black);
	case Move::WhiteLong:
	return !inCheck() && !positionAttacked(3, Piece::Black);
	
	default:
	return true;
	}
}
bool Board::positionAttacked(int pos, bool byBlack) const
{
	if (boardIntersect(KnightMoves[pos],
		pieceBoards[Piece::Knight | byBlack]) != 0ull) return true;
	
	for (int i = 0; i < 8; ++i)//each of the slide directions
	{
		int hitIndex = i < 4 ?
			firstIndex(boardIntersect(
				SlideMoves[i][pos],
				pieceBoards[Piece::All]
			)) :
			lastIndex(boardIntersect(
				SlideMoves[i][pos],
				pieceBoards[Piece::All]
			));
		if (hitIndex == 64) continue;
		if ((fullBoard[hitIndex] & Piece::Team) == byBlack)
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
		if (boardIntersect(indexToBitBoard(pos),
			shiftDown(shiftRight(pawns, 1), 1) |
			shiftDown(shiftLeft(pawns, 1), 1)) != 0ull) return true;
	}
	else
	{
		if (boardIntersect(indexToBitBoard(pos),
			shiftUp(shiftRight(pawns, 1), 1) |
			shiftUp(shiftLeft(pawns, 1), 1)) != 0ull) return true;
	}
	if (boardIntersect(KingMoves[pos],
		pieceBoards[Piece::King | byBlack]) != 0ull) return true;

	return false;
}

bool Board::isQuiescent_weak(Move move) const
{
	if (move.deltaTarget != fullBoard[move.sourceSquare])
		return false;//finds captures and promotions
	if (inCheck()) return false;//escaping check isn't (even weakly) quiescent
	return true;
}

bool Board::isQuiescent_strong(Move move) const
{
	if (move.deltaTarget != fullBoard[move.sourceSquare])
		return false;//finds captures and promotions
	if (inCheck()) return false;//escaping check isn't quiescent
	if (move.moveType == Move::Normal || move.moveType == Move::EnPassant)
	{
		//promotions have already been handled,
		//null moves are checked differently,
		//and castling is inherently quiescent.
		//these are the only possible non-quiescent moves left.

		//need to check if the piece is checking and
		//if the move reveals a check
		BoardSquare enemyKingPos = firstIndex(pieceBoards[Piece::King | !blacksTurn]);

		//for checking if the piece checks the king
		Move::Direction targetMoveDirection = RelativeDirection[move.targetSquare][enemyKingPos];

		//for checking for revealed check
		Move::Direction sourceMoveDirection = RelativeDirection[enemyKingPos][move.sourceSquare];

		PieceType newPiece = move.deltaTarget ^ fullBoard[move.targetSquare];

		if ((newPiece & Piece::Sliding))
		{
			//rook bishop and queen attacks
			if (targetMoveDirection < Move::Knight &&
				((newPiece & Piece::Bishop) == Piece::Bishop &&
					targetMoveDirection % 2 == 0 ||
				(newPiece & Piece::Rook) == Piece::Rook &&
					targetMoveDirection % 2 == 1))
			{
				if (targetMoveDirection < 4)//requires forward scan
				{
					if (firstIndex(boardIntersect(SlideMoves[targetMoveDirection][move.targetSquare], pieceBoards[Piece::All]))
						== enemyKingPos) return false;//checking the enemy king isn't quiescent
				}
				else//requires backward scan
				{
					if (lastIndex(boardIntersect(SlideMoves[targetMoveDirection][move.targetSquare], pieceBoards[Piece::All]))
						== enemyKingPos) return false;//checking the enemy king isn't quiescent
				}
			}
		}
		else
		{
			//pawn, knight, and king attacks
			if (targetMoveDirection == Move::Knight &&
				(newPiece & Piece::Occupied) == Piece::Knight) return false;
			if (targetMoveDirection % 2 == 0 &&//bishop or knight move
				(newPiece & Piece::Occupied) == Piece::Pawn)
			{
				if (blacksTurn)
				{
					if (enemyKingPos == move.targetSquare - 7 ||
						enemyKingPos == move.targetSquare - 9) return false;
				}
				else
				{
					if (enemyKingPos == move.targetSquare + 7 ||
						enemyKingPos == move.targetSquare + 9) return false;
				}
			}
			//kings can't attack kings
		}
		//check for revealed attack
		//TODO: will not work with en passant double piece removal
		if (sourceMoveDirection < Move::Knight)
		{
			//TODO: could be made more efficient
			BoardSquare firstHit = sourceMoveDirection < 4 ?
				firstIndex(boardIntersect(SlideMoves[sourceMoveDirection][enemyKingPos], pieceBoards[Piece::All])) :
				lastIndex(boardIntersect(SlideMoves[sourceMoveDirection][enemyKingPos], pieceBoards[Piece::All]));
			if (firstHit == move.sourceSquare)
			{
				firstHit = sourceMoveDirection < 4 ?
					firstIndex(boardIntersect(SlideMoves[sourceMoveDirection][move.sourceSquare], pieceBoards[Piece::All])) :
					lastIndex(boardIntersect(SlideMoves[sourceMoveDirection][move.sourceSquare], pieceBoards[Piece::All]));
				if (firstHit == 64) return true;
				if ((fullBoard[firstHit] & Piece::Team) == blacksTurn)
				{
					//the move reveals line of sight to the king.
					//need to check if its a bishop/rook etc
					if ((fullBoard[firstHit] & Piece::Bishop) == Piece::Bishop &&
							sourceMoveDirection % 2 == 0 ||
						(fullBoard[firstHit] & Piece::Rook) == Piece::Rook &&
							sourceMoveDirection % 2 == 1)
					{
						return false;
					}
				}
			}
		}
	}
	return true;
}
