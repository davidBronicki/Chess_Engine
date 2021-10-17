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

extern Move nonMove;

struct Board
{
	static ull SquareHashes[64][16];

	static ull ExtraHashes[256];
	static ull TurnHash;

	static BitBoard KnightMoves[64];
	static BitBoard RookMoves[4][64];
	static BitBoard DiagMoves[4][64];

	enum Directions : char
	{
		Up = 0,
		UpLeft = 0,
		Right = 1,
		UpRight = 1,
		Down = 2,
		DownRight = 2,
		Left = 3,
		DownLeft = 3
	};

	static BitBoard KingMoves[64];

	static void initializeGlobals();

	bool blacksTurn;
	uc extraInfo;
	uc plySinceLastPawnOrCapture;
	short plyNumber;

	std::vector<Move> moveStack;

	BitBoard pieceBoards[16];
	uc fullBoard[64];

	ull hash;


	Board();

	void resetHash();
	void resetCongregateData();
	void initPieceBoards();

	Move buildMoveFromContext(uc sourceSquare, uc targetSquare, uc moveType) const;

	std::vector<Move> generateLegalMoves() const;

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
