#include "Board.hpp"

#define standardMove {moveType, sourceSquare, targetSquare,\
	fullBoard[sourceSquare], static_cast<uc>(fullBoard[sourceSquare] ^ fullBoard[targetSquare]),\
	deltaExtraInfo, static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)}

#define promoMove(newPiece) {moveType, sourceSquare, targetSquare,\
	fullBoard[sourceSquare], static_cast<uc>(fullBoard[sourceSquare] ^ (newPiece)),\
	deltaExtraInfo, static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)}

inline uc rookChecks(uc sourceSquare, uc targetSquare)
{
	uc output = 0;
	if (sourceSquare == 0 || targetSquare == 0)
	{
		output |= Extra::Black_Short;
	}
	if (sourceSquare == 7 || targetSquare == 7)
	{
		output |= Extra::Black_Long;
	}
	if (sourceSquare == 7 * 8 || targetSquare == 7 * 8)
	{
		output |= Extra::White_Short;
	}
	if (sourceSquare == 7 * 8 + 7 || targetSquare == 7 * 8 + 7)
	{
		output |= Extra::White_Long;
	}
	return output;
}

Move Board::constructMove(uc sourceSquare, uc targetSquare, uc moveType)
{
	us nextIrreversiblePly = plySinceLastPawnOrCapture + 1;
	uc deltaExtraInfo = extraInfo & Extra::EnPassantInfo;

	switch (moveType)
	{
		case MoveType::Normal:
		if ((fullBoard[sourceSquare] & Piece::Occupied) == Piece::Pawn
			|| fullBoard[targetSquare])
			nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return standardMove;

		case MoveType::EnPassant:
		nextIrreversiblePly = 0;
		return standardMove;

		case MoveType::PromoQueen:
		nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return promoMove(Piece::Queen | (blacksTurn * Piece::Black));

		case MoveType::PromoRook:
		nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return promoMove(Piece::Rook | (blacksTurn * Piece::Black));

		case MoveType::PromoBishop:
		nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return promoMove(Piece::Bishop | (blacksTurn * Piece::Black));

		case MoveType::PromoKnight:
		nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return promoMove(Piece::Knight | (blacksTurn * Piece::Black));

		case MoveType::WhiteShort:
		deltaExtraInfo |= extraInfo & (Extra::White_Short | Extra::White_Long);
		return standardMove;

		case MoveType::BlackShort:
		deltaExtraInfo |= extraInfo & (Extra::Black_Short | Extra::Black_Long);
		return standardMove;

		case MoveType::WhiteLong:
		deltaExtraInfo |= extraInfo & (Extra::White_Short | Extra::White_Long);
		return standardMove;

		case MoveType::BlackLong:
		deltaExtraInfo |= extraInfo & (Extra::Black_Short | Extra::Black_Long);
		return standardMove;
	}
	return {0, 0, 0, 0, 0, 0, 0};
}

void Board::performMove(Move move)
{
	hash ^= Board::ExtraHashes[extraInfo]
		^ Board::SquareHashes[move.sourceSquare][fullBoard[move.sourceSquare]]
		^ Board::SquareHashes[move.targetSquare][fullBoard[move.targetSquare]]
		^ Board::TurnHash;
	BitBoard sourceBoard = indexToBitBoard(move.sourceSquare);
	BitBoard targetBoard = indexToBitBoard(move.targetSquare);
	switch (move.moveType)
	{
		case MoveType::EnPassant:
		{
			uc enPassantSquare = (0b111000 & move.sourceSquare)
				| (0b000111 & move.targetSquare);
			pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
			pieceBoards[fullBoard[move.sourceSquare] ^ Piece::Team]
				^= indexToBitBoard(enPassantSquare);

			pieceBoards[blacksTurn] ^= targetBoard | sourceBoard;
			pieceBoards[!blacksTurn] ^= indexToBitBoard(enPassantSquare);

			resetFullOccupancy(pieceBoards);

			fullBoard[move.targetSquare] = fullBoard[move.sourceSquare];
			fullBoard[move.sourceSquare] = 0;
			fullBoard[enPassantSquare] = 0;
		}
		break;

		case MoveType::BlackShort:
		pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
		pieceBoards[fullBoard[0]] ^= 0b101ull;

		pieceBoards[blacksTurn] ^= 0b1111ull;

		fullBoard[move.targetSquare] = fullBoard[move.sourceSquare];
		fullBoard[move.sourceSquare] = 0;
		fullBoard[2] = fullBoard[0];
		fullBoard[0] = 0;
		break;

		case MoveType::BlackLong:
		pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
		pieceBoards[fullBoard[0]] ^= 0b00001001ull;

		pieceBoards[blacksTurn] ^= 0b00011101ull;

		fullBoard[move.targetSquare] = fullBoard[move.sourceSquare];
		fullBoard[move.sourceSquare] = 0;
		fullBoard[4] = fullBoard[7];
		fullBoard[7] = 0;
		break;

		case MoveType::WhiteShort:
		pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
		pieceBoards[fullBoard[0]] ^= 0b101ull << 7 * 8;

		pieceBoards[blacksTurn] ^= 0b1111ull << 7 * 8;

		fullBoard[move.targetSquare] = fullBoard[move.sourceSquare];
		fullBoard[move.sourceSquare] = 0;
		fullBoard[2 + 7 * 8] = fullBoard[0 + 7 * 8];
		fullBoard[0 + 7 * 8] = 0;
		break;

		case MoveType::WhiteLong:
		pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
		pieceBoards[fullBoard[0]] ^= 0b00001001ull << 7 * 8;

		pieceBoards[blacksTurn] ^= 0b00011101ull << 7 * 8;

		fullBoard[move.targetSquare] = fullBoard[move.sourceSquare];
		fullBoard[move.sourceSquare] = 0;
		fullBoard[4 + 7 * 8] = fullBoard[7 + 7 * 8];
		fullBoard[7 + 7 * 8] = 0;
		break;

		default://normal moves and promotions
		pieceBoards[fullBoard[move.sourceSquare]] ^= sourceBoard;

		pieceBoards[blacksTurn] ^= sourceBoard | targetBoard;
		if (fullBoard[move.targetSquare])
		{
			pieceBoards[!blacksTurn] ^= targetBoard;
			pieceBoards[fullBoard[move.targetSquare]] ^= targetBoard;
		}
		pieceBoards[fullBoard[move.targetSquare]] ^= fullBoard[move.targetSquare] ?
			targetBoard : 0;
		fullBoard[move.targetSquare] ^= move.deltaTarget;
		fullBoard[move.sourceSquare] ^= move.deltaSource;
		pieceBoards[fullBoard[move.targetSquare]] ^= targetBoard;
		break;
	}
	blacksTurn = !blacksTurn;
	extraInfo ^= move.deltaExtraInfo;
	plySinceLastPawnOrCapture ^= move.deltaPawnOrCapturePly;
	++plyNumber;

	resetFullOccupancy(pieceBoards);
	hash ^= Board::ExtraHashes[extraInfo]
		^ Board::SquareHashes[move.sourceSquare][fullBoard[move.sourceSquare]]
		^ Board::SquareHashes[move.targetSquare][fullBoard[move.targetSquare]];
}

void Board::reverseMove(Move move)
{
	hash ^= Board::ExtraHashes[extraInfo]
		^ Board::SquareHashes[move.sourceSquare][fullBoard[move.sourceSquare]]
		^ Board::SquareHashes[move.targetSquare][fullBoard[move.targetSquare]]
		^ Board::TurnHash;
	BitBoard sourceBoard = indexToBitBoard(move.sourceSquare);
	BitBoard targetBoard = indexToBitBoard(move.targetSquare);
	blacksTurn = !blacksTurn;
	extraInfo ^= move.deltaExtraInfo;
	plySinceLastPawnOrCapture ^= move.deltaPawnOrCapturePly;
	--plyNumber;
	switch (move.moveType)
	{
		case MoveType::EnPassant:
		{
			uc enPassantSquare = (0b111000 & move.sourceSquare)
				| (0b000111 & move.targetSquare);
			pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
			pieceBoards[fullBoard[move.sourceSquare] ^ Piece::Team]
				^= indexToBitBoard(enPassantSquare);

			pieceBoards[blacksTurn] ^= targetBoard | sourceBoard;
			pieceBoards[!blacksTurn] ^= indexToBitBoard(enPassantSquare);

			fullBoard[move.sourceSquare] = fullBoard[move.targetSquare];
			fullBoard[move.targetSquare] = 0;
			fullBoard[enPassantSquare] = fullBoard[move.sourceSquare] ^ Piece::Team;
		}
		break;

		case MoveType::BlackShort:
		pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
		pieceBoards[fullBoard[0]] ^= 0b101ull;

		pieceBoards[blacksTurn] ^= 0b1111ull;

		fullBoard[move.sourceSquare] = fullBoard[move.targetSquare];
		fullBoard[move.targetSquare] = 0;
		fullBoard[0] = fullBoard[2];
		fullBoard[2] = 0;
		break;

		case MoveType::BlackLong:
		pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
		pieceBoards[fullBoard[0]] ^= 0b00001001ull;

		pieceBoards[blacksTurn] ^= 0b00011101ull;

		fullBoard[move.sourceSquare] = fullBoard[move.targetSquare];
		fullBoard[move.targetSquare] = 0;
		fullBoard[7] = fullBoard[4];
		fullBoard[4] = 0;
		break;

		case MoveType::WhiteShort:
		pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
		pieceBoards[fullBoard[0]] ^= 0b101ull << 7 * 8;

		pieceBoards[blacksTurn] ^= 0b1111ull << 7 * 8;

		fullBoard[move.sourceSquare] = fullBoard[move.targetSquare];
		fullBoard[move.targetSquare] = 0;
		fullBoard[0 + 7 * 8] = fullBoard[2 + 7 * 8];
		fullBoard[2 + 7 * 8] = 0;
		break;

		case MoveType::WhiteLong:
		pieceBoards[fullBoard[move.sourceSquare]] ^= targetBoard | sourceBoard;
		pieceBoards[fullBoard[0]] ^= 0b00001001ull << 7 * 8;

		pieceBoards[blacksTurn] ^= 0b00011101ull << 7 * 8;

		fullBoard[move.sourceSquare] = fullBoard[move.targetSquare];
		fullBoard[move.targetSquare] = 0;
		fullBoard[7 + 7 * 8] = fullBoard[4 + 7 * 8];
		fullBoard[4 + 7 * 8] = 0;
		break;

		default://normal moves and promotions
		pieceBoards[fullBoard[move.targetSquare]] ^= targetBoard;
		fullBoard[move.sourceSquare] ^= move.deltaSource;
		fullBoard[move.targetSquare] ^= move.deltaTarget;
		pieceBoards[fullBoard[move.targetSquare]] ^= fullBoard[move.targetSquare] ?
			targetBoard : 0;
		if (fullBoard[move.targetSquare])
		{
			pieceBoards[!blacksTurn] ^= targetBoard;
			pieceBoards[fullBoard[move.targetSquare]] ^= targetBoard;
		}
		pieceBoards[blacksTurn] ^= sourceBoard | targetBoard;
		pieceBoards[fullBoard[move.sourceSquare]] ^= sourceBoard;
		break;
	}
	
	resetFullOccupancy(pieceBoards);
	hash ^= Board::ExtraHashes[extraInfo]
		^ Board::SquareHashes[move.sourceSquare][fullBoard[move.sourceSquare]]
		^ Board::SquareHashes[move.targetSquare][fullBoard[move.targetSquare]];
}
