#include "Board.hpp"

#define standardMove {moveType, sourceSquare, targetSquare,\
	fullBoard[sourceSquare], static_cast<uc>(fullBoard[sourceSquare] ^ fullBoard[targetSquare]),\
	deltaExtraInfo, static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)}

#define promoMove(newPiece) {moveType, sourceSquare, targetSquare,\
	fullBoard[sourceSquare], static_cast<uc>(fullBoard[sourceSquare] ^ (newPiece)),\
	deltaExtraInfo, static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)}

#define boardLoop(bitBoard, copyBoard, index) for (ull copyBoard = (bitBoard), index = firstIndex((bitBoard));\
	index < 64; copyBoard &= copyBoard - 1, index = firstIndex(copyBoard))

#define backwardBoardLoop(bitBoard, copyBoard, index) for (ull copyBoard = (bitBoard), index = lastIndex((bitBoard));\
	index < 64; copyBoard ^= 1ull << index, index = lastIndex(copyBoard))

#define genSlideMoves_ForwardScanning(slideBoards)\
	{\
		BitBoard moveScan = slideBoards[i];\
		int firstHit = firstIndex(boardIntersect(moveScan, pieceBoards[Piece::IndexAll]));\
		BitBoard moveScanFromHit = firstHit == 64 ? 0ull : slideBoards[firstHit];\
		BitBoard selfCaptureBoard = boardIntersect(indexToBitBoard(firstHit), pieceBoards[blacksTurn]);\
		BitBoard availableMovesBoard = moveScan ^ moveScanFromHit ^ selfCaptureBoard;\
		boardLoop(availableMovesBoard, copyBoard2, j)\
		{\
			currentMoves.push_back(buildMoveFromContext(i, j, MoveType::Normal));\
		}\
	}

#define genSlideMoves_BackwardScanning(slideBoards)\
	{\
		BitBoard moveScan = slideBoards[i];\
		int firstHit = lastIndex(boardIntersect(moveScan, pieceBoards[Piece::IndexAll]));\
		BitBoard moveScanFromHit = firstHit == 64 ? 0ull : slideBoards[firstHit];\
		BitBoard selfCaptureBoard = boardIntersect(indexToBitBoard(firstHit), pieceBoards[blacksTurn]);\
		BitBoard availableMovesBoard = moveScan ^ moveScanFromHit ^ selfCaptureBoard;\
		boardLoop(availableMovesBoard, copyBoard2, j)\
		{\
			currentMoves.push_back(buildMoveFromContext(i, j, MoveType::Normal));\
		}\
	}


Move nonMove{MoveType::NullMove, 0, 0, 0, 0, 0, 0};

inline uc rookChecks(uc sourceSquare, uc targetSquare)
{
	uc output = 0;
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

Move Board::buildMoveFromContext(uc sourceSquare, uc targetSquare, uc moveType) const
{
	uc nextIrreversiblePly = plySinceLastPawnOrCapture + 1;
	uc deltaExtraInfo = extraInfo & Extra::EnPassantInfo;

	switch (moveType)
	{
		case MoveType::Normal:
		if ((fullBoard[sourceSquare] & Piece::Occupied) == Piece::Pawn)
		{
			nextIrreversiblePly = 0;
			if ((targetSquare - sourceSquare) / 8 % 2 == 0)//moved two squares
			{
				deltaExtraInfo ^= Extra::EnPassantAvailable;
				deltaExtraInfo ^= sourceSquare % 8;
			}
		}
		if (fullBoard[targetSquare])
			nextIrreversiblePly = 0;
		
		deltaExtraInfo |= extraInfo & rookChecks(sourceSquare, targetSquare);
		return standardMove;

		case MoveType::EnPassant://fix this
		nextIrreversiblePly = 0;
		return {moveType, sourceSquare, targetSquare,
			fullBoard[sourceSquare], fullBoard[targetSquare + (blacksTurn ? 8 : -8)],
			deltaExtraInfo, static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)};

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
	return {MoveType::NullMove, 0, 0, 0, 0, deltaExtraInfo,
		static_cast<uc>(nextIrreversiblePly ^ plySinceLastPawnOrCapture)};
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
		case MoveType::NullMove:
		if (move.deltaPawnOrCapturePly == 0)//non-move, no state change
		{
			hash ^= Board::ExtraHashes[extraInfo]
				^ Board::SquareHashes[move.sourceSquare][fullBoard[move.sourceSquare]]
				^ Board::SquareHashes[move.targetSquare][fullBoard[move.targetSquare]]
				^ Board::TurnHash;
			return;
		}
		break;//null move, need to advance ply and modify extraInfo
		case MoveType::EnPassant:
		{
			uc enPassantSquare = (0b111000 & move.sourceSquare)
				| (0b000111 & move.targetSquare);
			pieceBoards[move.deltaSource] ^= targetBoard | sourceBoard;
			pieceBoards[move.deltaTarget] ^= indexToBitBoard(enPassantSquare);

			pieceBoards[blacksTurn] ^= targetBoard | sourceBoard;
			pieceBoards[!blacksTurn] ^= indexToBitBoard(enPassantSquare);

			resetFullOccupancy(pieceBoards);

			fullBoard[move.targetSquare] ^= move.deltaSource;
			fullBoard[move.sourceSquare] ^= move.deltaSource;
			fullBoard[enPassantSquare] ^= move.deltaTarget;
		}
		break;

		case MoveType::BlackShort:
		pieceBoards[Piece::King | Piece::Black] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::Black] ^= 0b10010000ull << (7 * 8);

		pieceBoards[Piece::Black] ^= 0b11110000ull << (7 * 8);

		fullBoard[move.targetSquare] ^= Piece::King | Piece::Black;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::Black;
		fullBoard[61] ^= Piece::Rook | Piece::Black;
		fullBoard[63] ^= Piece::Rook | Piece::Black;
		break;

		case MoveType::BlackLong:
		pieceBoards[Piece::King | Piece::Black] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::Black] ^= 0b10001ull << (7 * 8);

		pieceBoards[Piece::Black] ^= 0b11101ull << (7 * 8);

		fullBoard[move.targetSquare] ^= Piece::King | Piece::Black;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::Black;
		fullBoard[7 * 8 + 3] ^= Piece::Rook | Piece::Black;
		fullBoard[7 * 8] ^= Piece::Rook | Piece::Black;
		break;

		case MoveType::WhiteShort:
		pieceBoards[Piece::King | Piece::White] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::White] ^= 0b10010000ull;

		pieceBoards[Piece::White] ^= 0b11110000ull;

		fullBoard[move.targetSquare] ^= Piece::King | Piece::White;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::White;
		fullBoard[5] ^= Piece::Rook | Piece::White;
		fullBoard[7] ^= Piece::Rook | Piece::White;
		break;

		case MoveType::WhiteLong:
		pieceBoards[Piece::King | Piece::White] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::White] ^= 0b10001ull;

		pieceBoards[Piece::White] ^= 0b11101ull;

		fullBoard[move.targetSquare] ^= Piece::King | Piece::White;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::White;
		fullBoard[3] ^= Piece::Rook | Piece::White;
		fullBoard[0] ^= Piece::Rook | Piece::White;
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
		case MoveType::NullMove:
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
		case MoveType::EnPassant:
		{
			uc enPassantSquare = (0b111000 & move.sourceSquare)
				| (0b000111 & move.targetSquare);
			pieceBoards[move.deltaSource] ^= targetBoard | sourceBoard;
			pieceBoards[move.deltaTarget] ^= indexToBitBoard(enPassantSquare);

			pieceBoards[blacksTurn] ^= targetBoard | sourceBoard;
			pieceBoards[!blacksTurn] ^= indexToBitBoard(enPassantSquare);

			resetFullOccupancy(pieceBoards);

			fullBoard[move.targetSquare] ^= move.deltaSource;
			fullBoard[move.sourceSquare] ^= move.deltaSource;
			fullBoard[enPassantSquare] ^= move.deltaTarget;
		}
		break;

		case MoveType::BlackShort:
		pieceBoards[Piece::King | Piece::Black] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::Black] ^= 0b10010000ull << (7 * 8);

		pieceBoards[Piece::Black] ^= 0b11110000ull << (7 * 8);

		fullBoard[move.targetSquare] ^= Piece::King | Piece::Black;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::Black;
		fullBoard[61] ^= Piece::Rook | Piece::Black;
		fullBoard[63] ^= Piece::Rook | Piece::Black;
		break;

		case MoveType::BlackLong:
		pieceBoards[Piece::King | Piece::Black] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::Black] ^= 0b10001ull << (7 * 8);

		pieceBoards[Piece::Black] ^= 0b11101ull << (7 * 8);

		fullBoard[move.targetSquare] ^= Piece::King | Piece::Black;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::Black;
		fullBoard[7 * 8 + 3] ^= Piece::Rook | Piece::Black;
		fullBoard[7 * 8] ^= Piece::Rook | Piece::Black;
		break;

		case MoveType::WhiteShort:
		pieceBoards[Piece::King | Piece::White] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::White] ^= 0b10010000ull;

		pieceBoards[Piece::White] ^= 0b11110000ull;

		fullBoard[move.targetSquare] ^= Piece::King | Piece::White;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::White;
		fullBoard[5] ^= Piece::Rook | Piece::White;
		fullBoard[7] ^= Piece::Rook | Piece::White;
		break;

		case MoveType::WhiteLong:
		pieceBoards[Piece::King | Piece::White] ^= targetBoard | sourceBoard;
		pieceBoards[Piece::Rook | Piece::White] ^= 0b10001ull;

		pieceBoards[Piece::White] ^= 0b11101ull;

		fullBoard[move.targetSquare] ^= Piece::King | Piece::White;
		fullBoard[move.sourceSquare] ^= Piece::King | Piece::White;
		fullBoard[3] ^= Piece::Rook | Piece::White;
		fullBoard[0] ^= Piece::Rook | Piece::White;
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

std::vector<Move> Board::generateLegalMoves() const
{
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
		BitBoard pushable = boardIntersect(pawns, shiftUp(pieceBoards[Piece::IndexNone], 1));
		BitBoard rightCaptureReady = boardIntersect(
			pawns, shiftLeft(shiftUp(pieceBoards[Piece::White], 1), 1));
		BitBoard leftCaptureReady = boardIntersect(
			pawns, shiftRight(shiftUp(pieceBoards[Piece::White], 1), 1));
		boardLoop(pushable, copyBoard, i)
		{
			int rank = i / 8;
			
			if (rank == 6 && fullBoard[i - 16] == 0)
			{
				currentMoves.push_back(buildMoveFromContext(i, i - 16, MoveType::Normal));
			}
			if (rank == 1)
			{
				currentMoves.push_back(buildMoveFromContext(i, i - 8, MoveType::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i - 8, MoveType::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i - 8, MoveType::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i - 8, MoveType::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i - 8, MoveType::Normal));
		}
		boardLoop(rightCaptureReady, copyBoard, i)
		{
			if (i / 8 == 1)
			{
				currentMoves.push_back(buildMoveFromContext(i, i - 7, MoveType::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i - 7, MoveType::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i - 7, MoveType::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i - 7, MoveType::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i - 7, MoveType::Normal));
		}
		boardLoop(leftCaptureReady, copyBoard, i)
		{
			if (i / 8 == 1)
			{
				currentMoves.push_back(buildMoveFromContext(i, i - 9, MoveType::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i - 9, MoveType::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i - 9, MoveType::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i - 9, MoveType::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i - 9, MoveType::Normal));
		}
		if (extraInfo & Extra::EnPassantAvailable)
		{
			BitBoard candidates = boardIntersect(pawns, 0xffull << (8 * 3));
			boardLoop(candidates, copyBoard, i)
			{
				if (i % 8 == (extraInfo & Extra::EnPassantFile) - 1)
				{
					currentMoves.push_back(buildMoveFromContext(i, i - 7, MoveType::EnPassant));
				}
				if (i % 8 == (extraInfo & Extra::EnPassantFile) + 1)
				{
					currentMoves.push_back(buildMoveFromContext(i, i - 9, MoveType::EnPassant));
				}
			}
		}
	}
	else
	{
		BitBoard pawns = pieceBoards[Piece::Pawn | Piece::White];
		BitBoard pushable = boardIntersect(pawns, shiftDown(pieceBoards[Piece::IndexNone], 1));
		BitBoard rightCaptureReady = boardIntersect(
			pawns, shiftLeft(shiftDown(pieceBoards[Piece::Black], 1), 1));
		BitBoard leftCaptureReady = boardIntersect(
			pawns, shiftRight(shiftDown(pieceBoards[Piece::Black], 1), 1));
		boardLoop(pushable, copyBoard, i)
		{
			int rank = i / 8;
			
			if (rank == 1 && fullBoard[i + 16] == 0)
			{
				currentMoves.push_back(buildMoveFromContext(i, i + 16, MoveType::Normal));
			}
			if (rank == 7)
			{
				currentMoves.push_back(buildMoveFromContext(i, i + 8, MoveType::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i + 8, MoveType::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i + 8, MoveType::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i + 8, MoveType::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i + 8, MoveType::Normal));
		}
		boardLoop(rightCaptureReady, copyBoard, i)
		{
			if (i / 8 == 7)
			{
				currentMoves.push_back(buildMoveFromContext(i, i + 9, MoveType::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i + 9, MoveType::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i + 9, MoveType::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i + 9, MoveType::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i + 9, MoveType::Normal));
		}
		boardLoop(leftCaptureReady, copyBoard, i)
		{
			if (i / 8 == 7)
			{
				currentMoves.push_back(buildMoveFromContext(i, i + 7, MoveType::PromoQueen));
				currentMoves.push_back(buildMoveFromContext(i, i + 7, MoveType::PromoRook));
				currentMoves.push_back(buildMoveFromContext(i, i + 7, MoveType::PromoBishop));
				currentMoves.push_back(buildMoveFromContext(i, i + 7, MoveType::PromoKnight));
				continue;
			}
			currentMoves.push_back(buildMoveFromContext(i, i + 7, MoveType::Normal));
		}
		if (extraInfo & Extra::EnPassantAvailable)
		{
			BitBoard candidates = boardIntersect(pawns, 0xffull << (8 * 4));
			boardLoop(candidates, copyBoard, i)
			{
				if (i % 8 == (extraInfo & Extra::EnPassantFile) - 1)
				{
					currentMoves.push_back(buildMoveFromContext(i, i + 9, MoveType::EnPassant));
				}
				if (i % 8 == (extraInfo & Extra::EnPassantFile) + 1)
				{
					currentMoves.push_back(buildMoveFromContext(i, i + 7, MoveType::EnPassant));
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
			currentMoves.push_back(buildMoveFromContext(i, j, MoveType::Normal));
		}
	}
}
void Board::addBishopMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::Bishop | blacksTurn], copyBoard, i)
	{
		genSlideMoves_ForwardScanning(DiagMoves[UpLeft]);
		genSlideMoves_ForwardScanning(DiagMoves[UpRight]);

		genSlideMoves_BackwardScanning(DiagMoves[DownLeft]);
		genSlideMoves_BackwardScanning(DiagMoves[DownRight]);
	}
}
void Board::addRookMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::Rook | blacksTurn], copyBoard, i)
	{
		genSlideMoves_ForwardScanning(RookMoves[Up]);
		genSlideMoves_ForwardScanning(RookMoves[Right]);

		genSlideMoves_BackwardScanning(RookMoves[Down]);
		genSlideMoves_BackwardScanning(RookMoves[Left]);
	}
}
void Board::addQueenMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::Queen | blacksTurn], copyBoard, i)
	{
		//diagonal moves

		genSlideMoves_ForwardScanning(DiagMoves[UpLeft]);
		genSlideMoves_ForwardScanning(DiagMoves[UpRight]);

		genSlideMoves_BackwardScanning(DiagMoves[DownLeft]);
		genSlideMoves_BackwardScanning(DiagMoves[DownRight]);

		//rook moves

		genSlideMoves_ForwardScanning(RookMoves[Up]);
		genSlideMoves_ForwardScanning(RookMoves[Right]);

		genSlideMoves_BackwardScanning(RookMoves[Down]);
		genSlideMoves_BackwardScanning(RookMoves[Left]);
	}
}
void Board::addKingMoves(std::vector<Move>& currentMoves) const
{
	boardLoop(pieceBoards[Piece::King | blacksTurn], copyBoard, i)
	{
		boardLoop(boardIntersect(KingMoves[i], ~pieceBoards[blacksTurn]), copyBoard2, j)
		{
			//need to check for danger
			currentMoves.push_back(buildMoveFromContext(i, j, MoveType::Normal));
		}
	}

	//castling

	//need to check for being in check, in check at destination, and move through check
	if (extraInfo & Extra::CastleInfo)
	{
		if (blacksTurn)
		{
			if (extraInfo & Extra::Black_Long)
			{
				buildMoveFromContext(7 * 8 + 4, 7 * 8 + 2, MoveType::BlackLong);
			}
			if (extraInfo & Extra::Black_Short)
			{
				buildMoveFromContext(7 * 8 + 4, 7 * 8 + 6, MoveType::BlackShort);
			}
		}
		else
		{
			if (extraInfo & Extra::White_Long)
			{
				buildMoveFromContext(4, 2, MoveType::WhiteLong);
			}
			if (extraInfo & Extra::White_Short)
			{
				buildMoveFromContext(4, 6, MoveType::WhiteShort);
			}
		}
	}
}

// #undef standardMove
// #undef promoMove
// #undef forwardBoardLoop
// #undef backwardBoardLoop
