#pragma once

#include <vector>

#include "BitBoard.hpp"

struct Board
{
	static HashType SquareHashes[64][16];

	static HashType ExtraHashes[256];
	static HashType TurnHash;

	static BitBoard KnightMoves[64];
	static BitBoard SlideMoves[8][64];

	static BitBoard KingMoves[64];

	static Move::Direction RelativeDirection[64][64];

	static void initializeGlobals();

	bool blacksTurn;
	ExtraType extraInfo;
	PlyType plySinceLastPawnOrCapture;
	PlyType plyNumber;

	// std::vector<Move> moveStack;

	BitBoard pieceBoards[16];
	PieceType fullBoard[64];

	// bool inCheck;

	// BitBoard diagThreatSquares[2][4];//each side and each direction
	// BitBoard rookThreatSquares[2][4];//each side and each direction
	// BitBoard knightThreatSquares[2];//each side
	// BitBoard rightPawnThreatSquares[2];//each side
	// BitBoard leftPawnThreatSquares[2];//each side
	// BitBoard threatSquares[2];//each side, union of all previous plus king

	// BitBoard diagPinnedPieces[2][4];//each side and each direction
	// BitBoard rookPinnedPieces[2][4];//each side and each direction
	// BitBoard pinnedPieces[2];//each side, union of all previous

	HashType hash;


	Board();

	void resetHash();
	void resetCongregateData();
	void initPieceBoards();

	Move buildMoveFromContext(uc sourceSquare, uc targetSquare, Move::Type moveType) const;

	std::vector<Move> generateMoves() const;

	// bool moveIsLegal(Move move) const;
	bool inCheck() const;
	bool miscLegalityCheck(Move move) const;
	bool positionAttacked(int pos, bool byBlack) const;

	bool isQuiescent(Move move) const;

	void addPawnMoves(std::vector<Move>& currentMoves) const;
	void addKnightMoves(std::vector<Move>& currentMoves) const;
	void addBishopMoves(std::vector<Move>& currentMoves) const;
	void addRookMoves(std::vector<Move>& currentMoves) const;
	void addQueenMoves(std::vector<Move>& currentMoves) const;
	void addKingMoves(std::vector<Move>& currentMoves) const;

	void performMove(Move move);
	void reverseMove(Move move);
};

inline void resetFullOccupancy(BitBoard* bitBoards)
{
	bitBoards[Piece::All] = bitBoards[Piece::White] | bitBoards[Piece::Black];
	bitBoards[Piece::None] = ~bitBoards[Piece::All];
}

Move constructMove(Board const& board, uc sourceSquare, uc targetSquare);
