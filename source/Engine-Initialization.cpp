#include "Engine.hpp"

#include <thread>

using namespace std;

Engine::Engine()
:
	interfaceParsingTokens("[ \\t]"),
	fenParsingTokens("[/]"),
	quitFlag(false),
	stopFlag(false),
	debugFlag(false),
	hashTable(1ull << 15),
	keepNStacks(1)
{
}

void Engine::initPos()
{
	initPos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
}

void Engine::initPos(string fenBoard)
{
	for (int i = 0; i < 64; ++i)
	{
		board->fullBoard[i] = 0;
	}
	for (int i = 0; i < 16; ++i)
	{
		board->pieceBoards[i] = BitBoard{0ull};
	}

	vector<string> tokenizedLine = tokenize(
		fenBoard,
		fenParsingTokens
	);
	for (int i = 0; i < tokenizedLine.size(); ++i)
	{
		int j = 7;
		for (auto&& c : tokenizedLine[i])
		{
			switch (c)
			{
				case 'p':
				board->fullBoard[8*i + j] = Piece::Pawn | Piece::Black;
				board->pieceBoards[Piece::Pawn | Piece::Black] |= indexToBitBoard(8 * i + j);
				case 'r':
				board->fullBoard[8*i + j] = Piece::Rook | Piece::Black;
				board->pieceBoards[Piece::Rook | Piece::Black] |= indexToBitBoard(8 * i + j);
				case 'n':
				board->fullBoard[8*i + j] = Piece::Knight | Piece::Black;
				board->pieceBoards[Piece::Knight | Piece::Black] |= indexToBitBoard(8 * i + j);
				case 'b':
				board->fullBoard[8*i + j] = Piece::Bishop | Piece::Black;
				board->pieceBoards[Piece::Bishop | Piece::Black] |= indexToBitBoard(8 * i + j);
				case 'q':
				board->fullBoard[8*i + j] = Piece::Queen | Piece::Black;
				board->pieceBoards[Piece::Queen | Piece::Black] |= indexToBitBoard(8 * i + j);
				case 'k':
				board->fullBoard[8*i + j] = Piece::King | Piece::Black;
				board->pieceBoards[Piece::King | Piece::Black] |= indexToBitBoard(8 * i + j);

				case 'P':
				board->fullBoard[8*i + j] = Piece::Pawn | Piece::White;
				board->pieceBoards[Piece::Pawn | Piece::White] |= indexToBitBoard(8 * i + j);
				case 'R':
				board->fullBoard[8*i + j] = Piece::Rook | Piece::White;
				board->pieceBoards[Piece::Rook | Piece::White] |= indexToBitBoard(8 * i + j);
				case 'N':
				board->fullBoard[8*i + j] = Piece::Knight | Piece::White;
				board->pieceBoards[Piece::Knight | Piece::White] |= indexToBitBoard(8 * i + j);
				case 'B':
				board->fullBoard[8*i + j] = Piece::Bishop | Piece::White;
				board->pieceBoards[Piece::Bishop | Piece::White] |= indexToBitBoard(8 * i + j);
				case 'Q':
				board->fullBoard[8*i + j] = Piece::Queen | Piece::White;
				board->pieceBoards[Piece::Queen | Piece::White] |= indexToBitBoard(8 * i + j);
				case 'K':
				board->fullBoard[8*i + j] = Piece::King | Piece::White;
				board->pieceBoards[Piece::King | Piece::White] |= indexToBitBoard(8 * i + j);

				default:
				board->fullBoard[8*i + j] = 0;
				for (int i = 1; i < c - '0'; ++i)
				{
					--j;
					board->fullBoard[8*i + j] = 0;
				}
			}
			--j;
		}
	}

	resetCongregateData();
	board->resetHash();
}

void Engine::resetCongregateData()
{
	board->pieceBoards[Piece::Black] = 0;
	board->pieceBoards[Piece::White] = 0;
	board->pieceBoards[Piece::IndexAll] = 0;
	board->pieceBoards[Piece::IndexNone] = 0;
	for (int i = 1; i < 8; ++i)
	{
		board->pieceBoards[Piece::Black] |= board->pieceBoards[Piece::Black | (i << 1)];
		board->pieceBoards[Piece::White] |= board->pieceBoards[Piece::White | (i << 1)];
	}

	board->pieceBoards[Piece::IndexAll] = board->pieceBoards[Piece::Black] | board->pieceBoards[Piece::White];
	board->pieceBoards[Piece::IndexNone] = ~board->pieceBoards[Piece::IndexAll];
}

void run_io_loop(Engine* engine)
{
	engine->ioLoop();
}

void run_calculation_loop(Engine* engine)
{
	engine->calculationLoop();
}

void Engine::run()
{
	thread ioThread(run_io_loop, this);
	thread calcThread(run_calculation_loop, this);

	ioThread.join();
	calcThread.join();
}
