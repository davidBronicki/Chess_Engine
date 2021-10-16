#include "Board.hpp"

#include <random>

ull Board::SquareHashes[64][16];
ull Board::ExtraHashes[256];
ull Board::TurnHash;

BitBoard Board::KnightMoves[64];
BitBoard Board::RookMoves[4][64];
BitBoard Board::DiagMoves[4][64];
BitBoard Board::KingMoves[64];

void Board::initializeGlobals()
{
	std::mt19937 eng(111111ull);
	std::uniform_int_distribution<ull> distr;
	for (int i = 0; i < 64; ++i)
	{
		for (int j = 0; j < 16; ++j)
		{
			Board::SquareHashes[i][j] = distr(eng);
		}
	}
	for (int i = 0; i < 256; ++i)
	{
		Board::ExtraHashes[i] = distr(eng);
	}
	Board::TurnHash = distr(eng);


	for (int i = 0; i < 64; ++i)
	{
		int x = i % 8;
		int y = i / 8;
		//knight moves

		KnightMoves[i] = 0ull;

		if (x + 2 < 8)
		{
			if (y + 1 < 8)
			{
				KnightMoves[i] |= 1ull << (x + 2 + 8 * y + 8);
			}
			if (y - 1 >= 0)
			{
				KnightMoves[i] |= 1ull << (x + 2 + 8 * y - 8);
			}
		}

		if (x + 1 < 8)
		{
			if (y + 2 < 8)
			{
				KnightMoves[i] |= 1ull << (x + 1 + 8 * y + 16);
			}
			if (y - 2 >= 0)
			{
				KnightMoves[i] |= 1ull << (x + 1 + 8 * y - 16);
			}
		}

		if (x - 2 >= 0)
		{
			if (y + 1 < 8)
			{
				KnightMoves[i] |= 1ull << (x - 2 + 8 * y + 8);
			}
			if (y - 1 >= 0)
			{
				KnightMoves[i] |= 1ull << (x - 2 + 8 * y - 8);
			}
		}

		if (x - 1 >= 0)
		{
			if (y + 2 < 8)
			{
				KnightMoves[i] |= 1ull << (x - 1 + 8 * y + 16);
			}
			if (y - 2 >= 0)
			{
				KnightMoves[i] |= 1ull << (x - 1 + 8 * y - 16);
			}
		}

		//rook moves

		RookMoves[Board::Right][i] = 0ull;
		for (int a = x + 1; a < 8; ++a)
		{
			RookMoves[Board::Right][i] |= 1ull << (a + 8 * y);
		}

		RookMoves[Board::Left][i] = 0ull;
		for (int a = x - 1; a >= 0; --a)
		{
			RookMoves[Board::Left][i] |= 1ull << (a + 8 * y);
		}

		RookMoves[Board::Up][i] = 0ull;
		for (int a = y + 1; a < 8; ++a)
		{
			RookMoves[Board::Up][i] |= 1ull << (x + 8 * a);
		}

		RookMoves[Board::Down][i] = 0ull;
		for (int a = y - 1; a >= 0; --a)
		{
			RookMoves[Board::Down][i] |= 1ull << (x + 8 * a);
		}

		//bishop moves / diagonal moves

		DiagMoves[Board::UpRight][i] = 0ull;
		for (int a = x + 1, b = y + 1; a < 8 && b < 8; ++a, ++b)
		{
			DiagMoves[Board::UpRight][i] |= 1ull << (a + 8 * b);
		}

		DiagMoves[Board::UpLeft][i] = 0ull;
		for (int a = x - 1, b = y + 1; a >= 0 && b < 8; --a, ++b)
		{
			DiagMoves[Board::UpLeft][i] |= 1ull << (a + 8 * b);
		}

		DiagMoves[Board::DownRight][i] = 0ull;
		for (int a = x + 1, b = y - 1; a < 8 && b >= 0; ++a, --b)
		{
			DiagMoves[Board::DownRight][i] |= 1ull << (a + 8 * b);
		}

		DiagMoves[Board::DownLeft][i] = 0ull;
		for (int a = x - 1, b = y - 1; a >= 0 && b >= 0; --a, --b)
		{
			DiagMoves[Board::DownLeft][i] |= 1ull << (a + 8 * b);
		}

		//king moves;

		KingMoves[i] = 0ull;
		if (x < 7)
		{
			KingMoves[i] |= 1ull << (x + 1 + 8 * y);
			if (y < 7)
				KingMoves[i] |= 1ull << (x + 1 + 8 * y + 8);
			if (y >= 1)
				KingMoves[i] |= 1ull << (x + 1 + 8 * y - 8);
		}
		if (x >= 1)
		{
			KingMoves[i] |= 1ull << (x - 1 + 8 * y);
			if (y < 7)
				KingMoves[i] |= 1ull << (x - 1 + 8 * y + 8);
			if (y >= 1)
				KingMoves[i] |= 1ull << (x - 1 + 8 * y - 8);
		}
		if (y < 7)
			KingMoves[i] |= 1ull << (x + 8 * y + 8);
		if (y >= 1)
			KingMoves[i] |= 1ull << (x + 8 * y - 8);
	}
}

Board::Board()
:
	blacksTurn(false),
	extraInfo(Extra::Black_Long | Extra::Black_Short
		| Extra::White_Long | Extra::White_Short),
	plySinceLastPawnOrCapture(0),
	plyNumber(0)
{
	for (int i = 0; i < 16; ++i)
	{
		pieceBoards[i] = BitBoard{0ull};
	}
	for (int i = 0; i < 64; ++i)
	{
		fullBoard[i] = 0;
	}
}

void Board::resetHash()
{
	hash = 0ull;
	for (int i = 0; i < 64; ++i)
	{
		hash ^= Board::SquareHashes[i][fullBoard[i]];
	}
	hash ^= Board::ExtraHashes[extraInfo];
	hash ^= blacksTurn * Board::TurnHash;
}

void Board::resetCongregateData()
{
	pieceBoards[Piece::Black] = 0;
	pieceBoards[Piece::White] = 0;
	pieceBoards[Piece::IndexAll] = 0;
	pieceBoards[Piece::IndexNone] = 0;
	for (int i = 1; i < 8; ++i)
	{
		pieceBoards[Piece::Black] |= pieceBoards[Piece::Black | (i << 1)];
		pieceBoards[Piece::White] |= pieceBoards[Piece::White | (i << 1)];
	}

	pieceBoards[Piece::IndexAll] = pieceBoards[Piece::Black] | pieceBoards[Piece::White];
	pieceBoards[Piece::IndexNone] = ~pieceBoards[Piece::IndexAll];
}

void Board::initPieceBoards()
{
	for (int i = 0; i < 16; ++i)
	{
		pieceBoards[i] = 0ull;
	}
	for (int i = 0; i < 64; ++i)
	{
		if (fullBoard[i])
		{
			pieceBoards[i] |= 1ull << i;
		}
	}
	resetCongregateData();
}
