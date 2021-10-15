#pragma once

#include <vector>

#include "BitDefs.hpp"

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

inline Move nullMove(uc fiftyMovePly)
{
	return {
		MoveType::NullMove,
		0, 0, 0, 0, 0,
		static_cast<uc>(fiftyMovePly ^ (fiftyMovePly + 1))
	};
}

typedef ull BitBoard;

struct Board
{
	static ull SquareHashes[64][16];

	static ull ExtraHashes[256];
	static ull TurnHash;

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

	Move constructMove(uc sourceSquare, uc targetSquare, uc moveType);

	std::vector<Move> generateLegalMoves() const;

	void performMove(Move move);
	void reverseMove(Move move);
};

inline void resetFullOccupancy(BitBoard* bitBoards)
{
	bitBoards[Piece::IndexAll] = bitBoards[Piece::White] | bitBoards[Piece::Black];
	bitBoards[Piece::IndexNone] = ~bitBoards[Piece::IndexAll];
}

Move constructMove(Board const& board, uc sourceSquare, uc targetSquare);

inline constexpr BitBoard boardUnion(
	BitBoard b1,
	BitBoard const& b2)
{
	return b1 |= b2;
}

inline constexpr BitBoard boardIntersect(
	BitBoard b1,
	BitBoard const& b2)
{
	return b1 &= b2;
}

namespace
{
	ull leftMoveMask[8]	=
	{
		0xffffffffffffffffull,
		0x7f7f7f7f7f7f7f7full,
		0x3f3f3f3f3f3f3f3full,
		0x1f1f1f1f1f1f1f1full,
		0x0f0f0f0f0f0f0f0full,
		0x0707070707070707ull,
		0x0303030303030303ull,
		0x0101010101010101ull
	};
	ull rightMoveMask[8] =
	{
		0xffffffffffffffffull,
		0xfefefefefefefefeull,
		0xfcfcfcfcfcfcfcfcull,
		0xf8f8f8f8f8f8f8f8ull,
		0xf0f0f0f0f0f0f0f0ull,
		0xe0e0e0e0e0e0e0e0ull,
		0xc0c0c0c0c0c0c0c0ull,
		0x8080808080808080ull
	};
}

inline BitBoard shiftLeft(
	BitBoard board,
	uc shift)
{
	return (board & leftMoveMask[shift]) << shift;
}

inline BitBoard shiftRight(
	BitBoard board,
	uc shift)
{
	return (board & rightMoveMask[shift]) >> shift;
}

inline constexpr BitBoard shiftUp(
	BitBoard board,
	uc shift)
{
	return board >>= (shift * 8);
}

inline constexpr BitBoard shiftDown(
	BitBoard board,
	uc shift)
{
	return board <<= (shift * 8);
}

inline constexpr BitBoard indexToBitBoard(
	uc boardIndex)
{
	return BitBoard{1ull << boardIndex};
}

inline constexpr ull firstIndex(
	BitBoard board)
{
	return __builtin_ctzll(board);
}

inline constexpr ull lastIndex(
	BitBoard board)
{
	return 63 - __builtin_clzll(board);
}

inline constexpr ull popCount(
	BitBoard board)
{
	return __builtin_popcountll(board);
}
