#include "Engine.hpp"

#include <thread>

using namespace std;

bool Engine::globalsReady = false;

void Engine::initializeGlobals()
{
	Board::initializeGlobals();
}

Engine::Engine()
:
	interfaceParsingTokens("[\\s]"),
	fenParsingTokens("[/]"),
	quitFlag(false),
	stopFlag(false),
	debugFlag(false),
	hashTable(1 << 15),
	board(make_shared<Board>()),
	keepNStacks(1),
	depthWalkValue(4)
{
}

void Engine::initPos()
{
	initPos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
}

void Engine::initPos(string fenBoard)
{
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
		int j = 0;
		for (auto&& c : tokenizedLine[i])
		{
			switch (c)
			{
				case 'p':
				board->fullBoard[8*(7 - i) + j] = Piece::Pawn | Piece::Black;
				board->pieceBoards[Piece::Pawn | Piece::Black] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'r':
				board->fullBoard[8*(7 - i) + j] = Piece::Rook | Piece::Black;
				board->pieceBoards[Piece::Rook | Piece::Black] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'n':
				board->fullBoard[8*(7 - i) + j] = Piece::Knight | Piece::Black;
				board->pieceBoards[Piece::Knight | Piece::Black] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'b':
				board->fullBoard[8*(7 - i) + j] = Piece::Bishop | Piece::Black;
				board->pieceBoards[Piece::Bishop | Piece::Black] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'q':
				board->fullBoard[8*(7 - i) + j] = Piece::Queen | Piece::Black;
				board->pieceBoards[Piece::Queen | Piece::Black] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'k':
				board->fullBoard[8*(7 - i) + j] = Piece::King | Piece::Black;
				board->pieceBoards[Piece::King | Piece::Black] |= indexToBitBoard(8 * (7 - i) + j);
				break;

				case 'P':
				board->fullBoard[8*(7 - i) + j] = Piece::Pawn | Piece::White;
				board->pieceBoards[Piece::Pawn | Piece::White] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'R':
				board->fullBoard[8*(7 - i) + j] = Piece::Rook | Piece::White;
				board->pieceBoards[Piece::Rook | Piece::White] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'N':
				board->fullBoard[8*(7 - i) + j] = Piece::Knight | Piece::White;
				board->pieceBoards[Piece::Knight | Piece::White] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'B':
				board->fullBoard[8*(7 - i) + j] = Piece::Bishop | Piece::White;
				board->pieceBoards[Piece::Bishop | Piece::White] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'Q':
				board->fullBoard[8*(7 - i) + j] = Piece::Queen | Piece::White;
				board->pieceBoards[Piece::Queen | Piece::White] |= indexToBitBoard(8 * (7 - i) + j);
				break;
				case 'K':
				board->fullBoard[8*(7 - i) + j] = Piece::King | Piece::White;
				board->pieceBoards[Piece::King | Piece::White] |= indexToBitBoard(8 * (7 - i) + j);
				break;

				default:
				board->fullBoard[8*(7 - i) + j] = 0;
				for (uc temp_j = j + 1; temp_j < c - '0'; ++temp_j)
				{
					++j;
					board->fullBoard[8*(7 - i) + temp_j] = 0;
				}
			}
			++j;
		}
	}

	board->initPieceBoards();
	board->resetHash();
}
