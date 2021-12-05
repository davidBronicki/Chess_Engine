#include "Board.hpp"

#include <x86intrin.h>

#define standardMove {moveType, sourceSquare, targetSquare,\
	fullBoard[sourceSquare], static_cast<uc>(fullBoard[sourceSquare] ^ fullBoard[targetSquare]),\
	deltaExtraInfo, static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)}

#define promoMove(newPiece) {moveType, sourceSquare, targetSquare,\
	fullBoard[sourceSquare], static_cast<uc>(fullBoard[targetSquare] ^ (newPiece)),\
	deltaExtraInfo, static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)}

#define boardLoop(bitBoard, copyBoard, index) for (ull copyBoard = (bitBoard), index = firstIndex((bitBoard));\
	index < 64; copyBoard &= copyBoard - 1, index = firstIndex(copyBoard))

#define backwardBoardLoop(bitBoard, copyBoard, index) for (ull copyBoard = (bitBoard), index = lastIndex((bitBoard));\
	index < 64; copyBoard ^= 1ull << index, index = lastIndex(copyBoard))

#define genSlideMoves_ForwardScanning(slideBoards)\
	{\
		BitBoard moveScan = slideBoards[i];\
		int firstHit = firstIndex(boardIntersect(moveScan, pieceBoards[Piece::All]));\
		BitBoard moveScanFromHit = firstHit == 64 ? 0ull : slideBoards[firstHit];\
		BitBoard selfCaptureBoard = boardIntersect(indexToBitBoard(firstHit), pieceBoards[blacksTurn]);\
		BitBoard availableMovesBoard = moveScan ^ moveScanFromHit ^ selfCaptureBoard;\
		boardLoop(availableMovesBoard, copyBoard2, j)\
		{\
			currentMoves.push_back(buildMoveFromContext(i, j, Move::Normal));\
		}\
	}

#define genSlideMoves_BackwardScanning(slideBoards)\
	{\
		BitBoard moveScan = slideBoards[i];\
		int firstHit = lastIndex(boardIntersect(moveScan, pieceBoards[Piece::All]));\
		BitBoard moveScanFromHit = firstHit == 64 ? 0ull : slideBoards[firstHit];\
		BitBoard selfCaptureBoard = boardIntersect(indexToBitBoard(firstHit), pieceBoards[blacksTurn]);\
		BitBoard availableMovesBoard = moveScan ^ moveScanFromHit ^ selfCaptureBoard;\
		boardLoop(availableMovesBoard, copyBoard2, j)\
		{\
			currentMoves.push_back(buildMoveFromContext(i, j, Move::Normal));\
		}\
	}

#define slideHitsForward(slideBoards, index) (slideBoards[index] ^ slideBoards[firstIndex(boardIntersect(slideBoards[index], pieceBoards[Piece::IndexAll]))])

#define slideHitsBackward(slideBoards, index) (slideBoards[index] ^ slideBoards[lastIndex(boardIntersect(slideBoards[index], pieceBoards[Piece::IndexAll]))])

Move nonMove{Move::NullMove, 0, 0, 0, 0, 0, 0};

inline ExtraType rookChecks(BoardSquare sourceSquare, BoardSquare targetSquare)
{
	ExtraType output = 0;
	if (sourceSquare == 0 || targetSquare == 0)
	{
		output |= Extra::White_Long;
	}
	if (sourceSquare == 7 || targetSquare == 7)
	{
		output |= Extra::White_Short;
	}
	if (sourceSquare == 7 * 8 || targetSquare == 7 * 8)
	{
		output |= Extra::Black_Long;
	}
	if (sourceSquare == 7 * 8 + 7 || targetSquare == 7 * 8 + 7)
	{
		output |= Extra::Black_Short;
	}
	return output;
}

Move Board::buildMoveFromContext(BoardSquare sourceSquare, BoardSquare targetSquare, Move::Type moveType) const
{
	uc nextIrreversiblePly = plySinceLastPawnOrCapture + 1;
	uc deltaExtraInfo = extraInfo & Extra::EnPassantInfo;

	switch (moveType)
	{
		case Move::Normal:
		if ((fullBoard[sourceSquare] & Piece::Occupied) == Piece::Pawn)
		{
			nextIrreversiblePly = 0;
			if (static_cast<ul>(targetSquare / 8 - sourceSquare / 8) % 2 == 0)//moved two squares
			{
				deltaExtraInfo ^= Extra::EnPassantAvailable;
				deltaExtraInfo ^= sourceSquare % 8;
			}
		}
		if (fullBoard[targetSquare])
			nextIrreversiblePly = 0;
		
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return standardMove;

		case Move::EnPassant:
		nextIrreversiblePly = 0;
		return {moveType, sourceSquare, targetSquare,
			fullBoard[sourceSquare], fullBoard[targetSquare + (blacksTurn ? 8 : -8)],
			deltaExtraInfo, static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)};

		case Move::PromoQueen:
		nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return promoMove(Piece::Queen | blacksTurn);

		case Move::PromoRook:
		nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return promoMove(Piece::Rook | blacksTurn);

		case Move::PromoBishop:
		nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return promoMove(Piece::Bishop | blacksTurn);

		case Move::PromoKnight:
		nextIrreversiblePly = 0;
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return promoMove(Piece::Knight | blacksTurn);

		case Move::WhiteShort:
		deltaExtraInfo |= extraInfo & Extra::WhiteCastleInfo;//could be done with normal move?
		return standardMove;

		case Move::BlackShort:
		deltaExtraInfo |= extraInfo & Extra::BlackCastleInfo;
		return standardMove;

		case Move::WhiteLong:
		deltaExtraInfo |= extraInfo & Extra::WhiteCastleInfo;
		return standardMove;

		case Move::BlackLong:
		deltaExtraInfo |= extraInfo & Extra::BlackCastleInfo;
		return standardMove;

		default:
		return {Move::NullMove, 0, 0, 0, 0, deltaExtraInfo,
			static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)};
	}
}

Move Board::buildNullMove() const
{
	return buildMoveFromContext(0, 0, Move::NullMove);
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
		case Move::NullMove:
		if (move.deltaPawnOrCapturePly == 0)//non-move, no state change
		{
			hash ^= Board::ExtraHashes[extraInfo]
				^ Board::SquareHashes[move.sourceSquare][fullBoard[move.sourceSquare]]
				^ Board::SquareHashes[move.targetSquare][fullBoard[move.targetSquare]]
				^ Board::TurnHash;
			return;
		}
		break;//null move, need to advance ply and modify extraInfo
		case Move::EnPassant:
		{
			uc enPassantSquare = (0b111000 & move.sourceSquare)
				| (0b000111 & move.targetSquare);

			hash ^= Board::SquareHashes[enPassantSquare][fullBoard[enPassantSquare]];

			pieceBoards[move.deltaSource] ^= targetBoard | sourceBoard;
			pieceBoards[move.deltaTarget] ^= indexToBitBoard(enPassantSquare);

			pieceBoards[blacksTurn] ^= targetBoard | sourceBoard;
			pieceBoards[!blacksTurn] ^= indexToBitBoard(enPassantSquare);

			fullBoard[move.targetSquare] ^= move.deltaSource;
			fullBoard[move.sourceSquare] ^= move.deltaSource;
			fullBoard[enPassantSquare] ^= move.deltaTarget;

			hash ^= Board::SquareHashes[enPassantSquare][fullBoard[enPassantSquare]];
		}
		break;

		case Move::BlackShort:
		hash ^= Board::SquareHashes[61][fullBoard[61]]
			^ Board::SquareHashes[63][fullBoard[63]];

		pieceBoards[Piece::King | Piece::Black] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::Black] ^= 0b10100000ull << (7 * 8);

		pieceBoards[Piece::Black] ^= 0b11110000ull << (7 * 8);

		fullBoard[move.targetSquare] ^= Piece::King | Piece::Black;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::Black;
		fullBoard[61] ^= Piece::Rook | Piece::Black;
		fullBoard[63] ^= Piece::Rook | Piece::Black;

		hash ^= Board::SquareHashes[61][fullBoard[61]]
			^ Board::SquareHashes[63][fullBoard[63]];
		break;

		case Move::BlackLong:
		hash ^= Board::SquareHashes[7 * 8 + 3][fullBoard[7 * 8 + 3]]
			^ Board::SquareHashes[7 * 8][fullBoard[7 * 8]];

		pieceBoards[Piece::King | Piece::Black] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::Black] ^= 0b1001ull << (7 * 8);

		pieceBoards[Piece::Black] ^= 0b11101ull << (7 * 8);

		fullBoard[move.targetSquare] ^= Piece::King | Piece::Black;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::Black;
		fullBoard[7 * 8 + 3] ^= Piece::Rook | Piece::Black;
		fullBoard[7 * 8] ^= Piece::Rook | Piece::Black;

		hash ^= Board::SquareHashes[7 * 8 + 3][fullBoard[7 * 8 + 3]]
			^ Board::SquareHashes[7 * 8][fullBoard[7 * 8]];
		break;

		case Move::WhiteShort:
		hash ^= Board::SquareHashes[5][fullBoard[5]]
			^ Board::SquareHashes[7][fullBoard[7]];

		pieceBoards[Piece::King | Piece::White] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::White] ^= 0b10100000ull;

		pieceBoards[Piece::White] ^= 0b11110000ull;

		fullBoard[move.targetSquare] ^= Piece::King | Piece::White;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::White;
		fullBoard[5] ^= Piece::Rook | Piece::White;
		fullBoard[7] ^= Piece::Rook | Piece::White;

		hash ^= Board::SquareHashes[5][fullBoard[5]]
			^ Board::SquareHashes[7][fullBoard[7]];
		break;

		case Move::WhiteLong:
		hash ^= Board::SquareHashes[0][fullBoard[0]]
			^ Board::SquareHashes[3][fullBoard[3]];

		pieceBoards[Piece::King | Piece::White] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::White] ^= 0b1001ull;

		pieceBoards[Piece::White] ^= 0b11101ull;

		fullBoard[move.targetSquare] ^= Piece::King | Piece::White;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::White;
		fullBoard[3] ^= Piece::Rook | Piece::White;
		fullBoard[0] ^= Piece::Rook | Piece::White;

		hash ^= Board::SquareHashes[0][fullBoard[0]]
			^ Board::SquareHashes[3][fullBoard[3]];
		break;

		default://normal moves and promotions

		//pieceboards for source
		pieceBoards[fullBoard[move.sourceSquare]] ^= sourceBoard;
		pieceBoards[blacksTurn] ^= sourceBoard | targetBoard;

		//pieceboards for target pre move
		if (fullBoard[move.targetSquare])
		{
			pieceBoards[!blacksTurn] ^= targetBoard;
			pieceBoards[fullBoard[move.targetSquare]] ^= targetBoard;
		}

		//change full board
		fullBoard[move.targetSquare] ^= move.deltaTarget;
		fullBoard[move.sourceSquare] ^= move.deltaSource;

		//pieceboard for target post move
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
		case Move::NullMove:
		if (move.deltaPawnOrCapturePly == 0)//non-move, no state change
		{
			hash ^= Board::ExtraHashes[extraInfo]
				^ Board::SquareHashes[move.sourceSquare][fullBoard[move.sourceSquare]]
				^ Board::SquareHashes[move.targetSquare][fullBoard[move.targetSquare]]
				^ Board::TurnHash;
			blacksTurn = !blacksTurn;
			++plyNumber;
			return;
		}
		break;//null move, need to advance ply and modify extraInfo
		case Move::EnPassant:
		{
			uc enPassantSquare = (0b111000 & move.sourceSquare)
				| (0b000111 & move.targetSquare);

			hash ^= Board::SquareHashes[enPassantSquare][fullBoard[enPassantSquare]];
			
			pieceBoards[move.deltaSource] ^= targetBoard | sourceBoard;
			pieceBoards[move.deltaTarget] ^= indexToBitBoard(enPassantSquare);

			pieceBoards[blacksTurn] ^= targetBoard | sourceBoard;
			pieceBoards[!blacksTurn] ^= indexToBitBoard(enPassantSquare);

			fullBoard[move.targetSquare] ^= move.deltaSource;
			fullBoard[move.sourceSquare] ^= move.deltaSource;
			fullBoard[enPassantSquare] ^= move.deltaTarget;

			hash ^= Board::SquareHashes[enPassantSquare][fullBoard[enPassantSquare]];
		}
		break;

		case Move::BlackShort:
		hash ^= Board::SquareHashes[61][fullBoard[61]]
			^ Board::SquareHashes[63][fullBoard[63]];

		pieceBoards[Piece::King | Piece::Black] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::Black] ^= 0b10100000ull << (7 * 8);

		pieceBoards[Piece::Black] ^= 0b11110000ull << (7 * 8);

		fullBoard[move.targetSquare] ^= Piece::King | Piece::Black;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::Black;
		fullBoard[61] ^= Piece::Rook | Piece::Black;
		fullBoard[63] ^= Piece::Rook | Piece::Black;

		hash ^= Board::SquareHashes[61][fullBoard[61]]
			^ Board::SquareHashes[63][fullBoard[63]];
		break;

		case Move::BlackLong:
		hash ^= Board::SquareHashes[7 * 8 + 3][fullBoard[7 * 8 + 3]]
			^ Board::SquareHashes[7 * 8][fullBoard[7 * 8]];

		pieceBoards[Piece::King | Piece::Black] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::Black] ^= 0b1001ull << (7 * 8);

		pieceBoards[Piece::Black] ^= 0b11101ull << (7 * 8);

		fullBoard[move.targetSquare] ^= Piece::King | Piece::Black;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::Black;
		fullBoard[7 * 8 + 3] ^= Piece::Rook | Piece::Black;
		fullBoard[7 * 8] ^= Piece::Rook | Piece::Black;

		hash ^= Board::SquareHashes[7 * 8 + 3][fullBoard[7 * 8 + 3]]
			^ Board::SquareHashes[7 * 8][fullBoard[7 * 8]];
		break;

		case Move::WhiteShort:
		hash ^= Board::SquareHashes[5][fullBoard[5]]
			^ Board::SquareHashes[7][fullBoard[7]];

		pieceBoards[Piece::King | Piece::White] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::White] ^= 0b10100000ull;

		pieceBoards[Piece::White] ^= 0b11110000ull;

		fullBoard[move.targetSquare] ^= Piece::King | Piece::White;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::White;
		fullBoard[5] ^= Piece::Rook | Piece::White;
		fullBoard[7] ^= Piece::Rook | Piece::White;

		hash ^= Board::SquareHashes[5][fullBoard[5]]
			^ Board::SquareHashes[7][fullBoard[7]];
		break;

		case Move::WhiteLong:
		hash ^= Board::SquareHashes[0][fullBoard[0]]
			^ Board::SquareHashes[3][fullBoard[3]];

		pieceBoards[Piece::King | Piece::White] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::White] ^= 0b1001ull;

		pieceBoards[Piece::White] ^= 0b11101ull;

		fullBoard[move.targetSquare] ^= Piece::King | Piece::White;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::White;
		fullBoard[3] ^= Piece::Rook | Piece::White;
		fullBoard[0] ^= Piece::Rook | Piece::White;

		hash ^= Board::SquareHashes[0][fullBoard[0]]
			^ Board::SquareHashes[3][fullBoard[3]];
		break;

		default://normal moves and promotions

		//pieceboard for target post move
		pieceBoards[fullBoard[move.targetSquare]] ^= targetBoard;

		//change full board
		fullBoard[move.targetSquare] ^= move.deltaTarget;
		fullBoard[move.sourceSquare] ^= move.deltaSource;

		//pieceboards for target pre move
		if (fullBoard[move.targetSquare])
		{
			pieceBoards[!blacksTurn] ^= targetBoard;
			pieceBoards[fullBoard[move.targetSquare]] ^= targetBoard;
		}

		//pieceboards for source
		pieceBoards[fullBoard[move.sourceSquare]] ^= sourceBoard;
		pieceBoards[blacksTurn] ^= sourceBoard | targetBoard;
		break;
	}
	
	resetFullOccupancy(pieceBoards);
	hash ^= Board::ExtraHashes[extraInfo]
		^ Board::SquareHashes[move.sourceSquare][fullBoard[move.sourceSquare]]
		^ Board::SquareHashes[move.targetSquare][fullBoard[move.targetSquare]];
}

std::vector<Move> Board::generateMoves() const
{
	/*
	try table of possible moves using pext processor instruction
	viableMoveDestinations = moveTable[_pext_u64(occupancyBoard, pieceHits)] & notFriendlyBoard;

	excellent for sliding pieces and potentially useful for pawn moves
	*/
	std::vector<Move> output;
	addKnightMoves(output);
	addBishopMoves(output);
	addRookMoves(output);
	addQueenMoves(output);
	addPawnMoves(output);
	addKingMoves(output);
	return output;
}


void Board::addPawnMoves(std::vector<Move>& currentMoves) const
{
	if (blacksTurn)
	{
		BitBoard pawns = pieceBoards[Piece::Pawn | Piece::Black];
		BitBoard pushable = boardIntersect(pawns, shiftUp(pieceBoards[Piece::None], 1));
		BitBoard rightCaptureReady = boardIntersect(
			pawns, shiftLeft(shiftUp(pieceBoards[Piece::White], 1), 1));
		BitBoard leftCaptureReady = boardIntersect(
			pawns, shiftRight(shiftUp(pieceBoards[Piece::White], 1), 1));
		boardLoop(pushable, copyBoard, i)
		{
			int rank = i / 8;
			
			if (rank == 6 && fullBoard[i - 16] == 0)
			{
				currentMoves.push_back(buildMoveFromContext(i, i - 16, Move::Normal));
			}
			if (rank == 1)
			{
				currentMoves.push_back(buildMoveFromContext(i, i - 8, Move::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i - 8, Move::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i - 8, Move::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i - 8, Move::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i - 8, Move::Normal));
		}
		boardLoop(rightCaptureReady, copyBoard, i)
		{
			if (i / 8 == 1)
			{
				currentMoves.push_back(buildMoveFromContext(i, i - 7, Move::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i - 7, Move::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i - 7, Move::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i - 7, Move::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i - 7, Move::Normal));
		}
		boardLoop(leftCaptureReady, copyBoard, i)
		{
			if (i / 8 == 1)
			{
				currentMoves.push_back(buildMoveFromContext(i, i - 9, Move::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i - 9, Move::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i - 9, Move::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i - 9, Move::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i - 9, Move::Normal));
		}
		if (extraInfo & Extra::EnPassantAvailable)
		{
			BitBoard candidates = boardIntersect(pawns, 0xffull << (8 * 3));
			boardLoop(candidates, copyBoard, i)
			{
				if (i % 8 == (extraInfo & Extra::EnPassantFile) - 1)
				{
					currentMoves.push_back(buildMoveFromContext(i, i - 7, Move::EnPassant));
				}
				if (i % 8 == (extraInfo & Extra::EnPassantFile) + 1)
				{
					currentMoves.push_back(buildMoveFromContext(i, i - 9, Move::EnPassant));
				}
			}
		}
	}
	else
	{
		BitBoard pawns = pieceBoards[Piece::Pawn | Piece::White];
		BitBoard pushable = boardIntersect(pawns, shiftDown(pieceBoards[Piece::None], 1));
		BitBoard rightCaptureReady = boardIntersect(
			pawns, shiftLeft(shiftDown(pieceBoards[Piece::Black], 1), 1));
		BitBoard leftCaptureReady = boardIntersect(
			pawns, shiftRight(shiftDown(pieceBoards[Piece::Black], 1), 1));
		boardLoop(pushable, copyBoard, i)
		{
			int rank = i / 8;
			
			if (rank == 1 && fullBoard[i + 16] == 0)
			{
				currentMoves.push_back(buildMoveFromContext(i, i + 16, Move::Normal));
			}
			if (rank == 6)
			{
				currentMoves.push_back(buildMoveFromContext(i, i + 8, Move::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i + 8, Move::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i + 8, Move::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i + 8, Move::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i + 8, Move::Normal));
		}
		boardLoop(rightCaptureReady, copyBoard, i)
		{
			if (i / 8 == 6)
			{
				currentMoves.push_back(buildMoveFromContext(i, i + 9, Move::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i + 9, Move::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i + 9, Move::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i + 9, Move::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i + 9, Move::Normal));
		}
		boardLoop(leftCaptureReady, copyBoard, i)
		{
			if (i / 8 == 6)
			{
				currentMoves.push_back(buildMoveFromContext(i, i + 7, Move::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i + 7, Move::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i + 7, Move::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i + 7, Move::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i + 7, Move::Normal));
		}
		if (extraInfo & Extra::EnPassantAvailable)
		{
			BitBoard candidates = boardIntersect(pawns, 0xffull << (8 * 4));
			boardLoop(candidates, copyBoard, i)
			{
				if (i % 8 == (extraInfo & Extra::EnPassantFile) - 1)
				{
					currentMoves.push_back(buildMoveFromContext(i, i + 9, Move::EnPassant));
				}
				if (i % 8 == (extraInfo & Extra::EnPassantFile) + 1)
				{
					currentMoves.push_back(buildMoveFromContext(i, i + 7, Move::EnPassant));
				}
			}
		}
	}
}
void Board::addKnightMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::Knight | blacksTurn], copyBoard, i)
	{
		boardLoop(boardIntersect(KnightMoves[i], ~pieceBoards[blacksTurn]), copyBoard2, j)
		{
			currentMoves.push_back(buildMoveFromContext(i, j, Move::Normal));
		}
	}
}
void Board::addBishopMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::Bishop | blacksTurn], copyBoard, i)
	{
		genSlideMoves_ForwardScanning(SlideMoves[Move::UpLeft]);
		genSlideMoves_ForwardScanning(SlideMoves[Move::UpRight]);

		genSlideMoves_BackwardScanning(SlideMoves[Move::DownLeft]);
		genSlideMoves_BackwardScanning(SlideMoves[Move::DownRight]);
	}
}
void Board::addRookMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::Rook | blacksTurn], copyBoard, i)
	{
		genSlideMoves_ForwardScanning(SlideMoves[Move::Up]);
		genSlideMoves_ForwardScanning(SlideMoves[Move::Right]);

		genSlideMoves_BackwardScanning(SlideMoves[Move::Down]);
		genSlideMoves_BackwardScanning(SlideMoves[Move::Left]);
	}
}
void Board::addQueenMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::Queen | blacksTurn], copyBoard, i)
	{
		//diagonal moves

		genSlideMoves_ForwardScanning(SlideMoves[Move::UpLeft]);
		genSlideMoves_ForwardScanning(SlideMoves[Move::UpRight]);

		genSlideMoves_BackwardScanning(SlideMoves[Move::DownLeft]);
		genSlideMoves_BackwardScanning(SlideMoves[Move::DownRight]);

		//rook moves

		genSlideMoves_ForwardScanning(SlideMoves[Move::Up]);
		genSlideMoves_ForwardScanning(SlideMoves[Move::Right]);

		genSlideMoves_BackwardScanning(SlideMoves[Move::Down]);
		genSlideMoves_BackwardScanning(SlideMoves[Move::Left]);
	}
}
void Board::addKingMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::King | blacksTurn], copyBoard, i)
	{
		boardLoop(boardIntersect(KingMoves[i], ~pieceBoards[blacksTurn]), copyBoard2, j)
		{
			currentMoves.push_back(buildMoveFromContext(i, j, Move::Normal));
		}
	}

	//castling

	//need to check for being in check, in check at destination, and move through check
	if (extraInfo & Extra::CastleInfo)
	{
		if (blacksTurn)
		{
			if (extraInfo & Extra::Black_Long &&
				fullBoard[7 * 8 + 1] == 0 &&
				fullBoard[7 * 8 + 2] == 0 &&
				fullBoard[7 * 8 + 3] == 0)
			{
				currentMoves.push_back(buildMoveFromContext(7 * 8 + 4, 7 * 8 + 2, Move::BlackLong));
			}
			if (extraInfo & Extra::Black_Short &&
				fullBoard[7 * 8 + 5] == 0 &&
				fullBoard[7 * 8 + 6] == 0)
			{
				currentMoves.push_back(buildMoveFromContext(7 * 8 + 4, 7 * 8 + 6, Move::BlackShort));
			}
		}
		else
		{
			if (extraInfo & Extra::White_Long &&
				fullBoard[1] == 0 &&
				fullBoard[2] == 0 &&
				fullBoard[3] == 0)
			{
				currentMoves.push_back(buildMoveFromContext(4, 2, Move::WhiteLong));
			}
			if (extraInfo & Extra::White_Short &&
				fullBoard[5] == 0 &&
				fullBoard[6] == 0)
			{
				currentMoves.push_back(buildMoveFromContext(4, 6, Move::WhiteShort));
			}
		}
	}
}
