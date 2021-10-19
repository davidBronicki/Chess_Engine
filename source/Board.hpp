#pragma once

#include <vector>

#include "BitBoard.hpp"

struct Move
{
	uc moveType;
	uc sourceSquare;
	uc targetSquare;
	uc deltaSource;
	uc deltaTarget;
	uc deltaExtraInfo;
	uc deltaPawnOrCapturePly;
};

inline bool operator==(Move a, Move b)
{
	return (*(ull*)(void*)(&a) >>  8) == (*(ull*)(void*)(&b) >> 8);
}

typedef ull Hash;

extern Move nonMove;

struct Board
{
	static Hash SquareHashes[64][16];

	static Hash ExtraHashes[256];
	static Hash TurnHash;

	static BitBoard KnightMoves[64];
	static BitBoard SlideMoves[8][64];

	static BitBoard KingMoves[64];

	static uc RelativeDirection[64][64];

	static void initializeGlobals();

	bool blacksTurn;
	uc extraInfo;
	uc plySinceLastPawnOrCapture;
	short plyNumber;

	// std::vector<Move> moveStack;

	BitBoard pieceBoards[16];
	uc fullBoard[64];

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

	ull hash;


	Board();

	void resetHash();
	void resetCongregateData();
	void initPieceBoards();

	Move buildMoveFromContext(uc sourceSquare, uc targetSquare, uc moveType) const;

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
	bitBoards[Piece::IndexAll] = bitBoards[Piece::White] | bitBoards[Piece::Black];
	bitBoards[Piece::IndexNone] = ~bitBoards[Piece::IndexAll];
}

Move constructMove(Board const& board, uc sourceSquare, uc targetSquare);
