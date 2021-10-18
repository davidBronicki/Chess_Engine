#include "Board.hpp"

#include <random>

Hash Board::SquareHashes[64][16];
Hash Board::ExtraHashes[256];
Hash Board::TurnHash;

BitBoard Board::KnightMoves[64];
BitBoard Board::SlideMoves[8][64];
BitBoard Board::KingMoves[64];

uc Board::RelativeDirection[64][64];

void Board::initializeGlobals()
{
	//initialize hashes
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


	//initialize move BitBoard tables
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
				KnightMoves[i] |= indexToBitBoard(x + 2 + 8 * y + 8);
			}
			if (y - 1 >= 0)
			{
				KnightMoves[i] |= indexToBitBoard(x + 2 + 8 * y - 8);
			}
		}

		if (x + 1 < 8)
		{
			if (y + 2 < 8)
			{
				KnightMoves[i] |= indexToBitBoard(x + 1 + 8 * y + 16);
			}
			if (y - 2 >= 0)
			{
				KnightMoves[i] |= indexToBitBoard(x + 1 + 8 * y - 16);
			}
		}

		if (x - 2 >= 0)
		{
			if (y + 1 < 8)
			{
				KnightMoves[i] |= indexToBitBoard(x - 2 + 8 * y + 8);
			}
			if (y - 1 >= 0)
			{
				KnightMoves[i] |= indexToBitBoard(x - 2 + 8 * y - 8);
			}
		}

		if (x - 1 >= 0)
		{
			if (y + 2 < 8)
			{
				KnightMoves[i] |= indexToBitBoard(x - 1 + 8 * y + 16);
			}
			if (y - 2 >= 0)
			{
				KnightMoves[i] |= indexToBitBoard(x - 1 + 8 * y - 16);
			}
		}

		//rook moves

		SlideMoves[Direction::Right][i] = 0ull;
		for (int a = x + 1; a < 8; ++a)
		{
			SlideMoves[Direction::Right][i] |= indexToBitBoard(a + 8 * y);
		}

		SlideMoves[Direction::Left][i] = 0ull;
		for (int a = x - 1; a >= 0; --a)
		{
			SlideMoves[Direction::Left][i] |= indexToBitBoard(a + 8 * y);
		}

		SlideMoves[Direction::Up][i] = 0ull;
		for (int a = y + 1; a < 8; ++a)
		{
			SlideMoves[Direction::Up][i] |= indexToBitBoard(x + 8 * a);
		}

		SlideMoves[Direction::Down][i] = 0ull;
		for (int a = y - 1; a >= 0; --a)
		{
			SlideMoves[Direction::Down][i] |= indexToBitBoard(x + 8 * a);
		}

		//bishop moves / diagonal moves

		SlideMoves[Direction::UpRight][i] = 0ull;
		for (int a = x + 1, b = y + 1; a < 8 && b < 8; ++a, ++b)
		{
			SlideMoves[Direction::UpRight][i] |= indexToBitBoard(a + 8 * b);
		}

		SlideMoves[Direction::UpLeft][i] = 0ull;
		for (int a = x - 1, b = y + 1; a >= 0 && b < 8; --a, ++b)
		{
			SlideMoves[Direction::UpLeft][i] |= indexToBitBoard(a + 8 * b);
		}

		SlideMoves[Direction::DownRight][i] = 0ull;
		for (int a = x + 1, b = y - 1; a < 8 && b >= 0; ++a, --b)
		{
			SlideMoves[Direction::DownRight][i] |= indexToBitBoard(a + 8 * b);
		}

		SlideMoves[Direction::DownLeft][i] = 0ull;
		for (int a = x - 1, b = y - 1; a >= 0 && b >= 0; --a, --b)
		{
			SlideMoves[Direction::DownLeft][i] |= indexToBitBoard(a + 8 * b);
		}

		//king moves;

		KingMoves[i] = 0ull;
		if (x < 7)
		{
			KingMoves[i] |= indexToBitBoard(x + 1 + 8 * y);
			if (y < 7)
				KingMoves[i] |= indexToBitBoard(x + 1 + 8 * y + 8);
			if (y >= 1)
				KingMoves[i] |= indexToBitBoard(x + 1 + 8 * y - 8);
		}
		if (x >= 1)
		{
			KingMoves[i] |= indexToBitBoard(x - 1 + 8 * y);
			if (y < 7)
				KingMoves[i] |= indexToBitBoard(x - 1 + 8 * y + 8);
			if (y >= 1)
				KingMoves[i] |= indexToBitBoard(x - 1 + 8 * y - 8);
		}
		if (y < 7)
			KingMoves[i] |= indexToBitBoard(x + 8 * y + 8);
		if (y >= 1)
			KingMoves[i] |= indexToBitBoard(x + 8 * y - 8);
	}

	//initialize move type lookup table
	for (int x1 = 0; x1 < 8; ++x1)
	{
		for (int y1 = 0; y1 < 8; ++y1)
		{
			for (int x2 = 0; x2 < 8; ++x2)
			{
				for (int y2 = 0; y2 < 8; ++y2)
				{
					int i = 8 * y1 + x1;
					int j = 8 * y2 + x2;
					if (i == j)
					{
						RelativeDirection[i][j] = Direction::None;
						continue;
					}
					int dx = x2 - x1;
					int dy = y2 - y1;
					if (dx == 0)
					{
						if (dy > 0)
							RelativeDirection[i][j] = Direction::Up;
						else
							RelativeDirection[i][j] = Direction::Down;
						continue;
					}
					if (dy == 0)
					{
						if (dx > 0)
							RelativeDirection[i][j] = Direction::Right;
						else
							RelativeDirection[i][j] = Direction::Left;
						continue;
					}
					RelativeDirection[i][j] = Direction::None;
					if (dx == dy)
					{
						if (dx > 0)
							RelativeDirection[i][j] = Direction::UpRight;
						else
							RelativeDirection[i][j] = Direction::DownLeft;
						continue;
					}
					if (dx == -dy)
					{
						if (dx > 0)
							RelativeDirection[i][j] = Direction::DownRight;
						else
							RelativeDirection[i][j] = Direction::UpLeft;
						continue;
					}
					if ((abs(dx) == 2 && abs(dy) == 1) || (abs(dx) == 1 && abs(dy) == 2))
						RelativeDirection[i][j] = Direction::Knight;
				}
			}
		}
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
	for (int i = 1; i < 8; ++i)//0 isnt a piece
	{
		//4 isn't a piece, but is currently set to zero
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
			pieceBoards[fullBoard[i]] |= indexToBitBoard(i);
		}
	}
	resetCongregateData();

	//TODO: init meta boards (pins and threats) and check for inCheck
}
