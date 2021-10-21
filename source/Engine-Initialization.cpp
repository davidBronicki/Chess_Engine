#include "Engine.hpp"

#include "HashTable.hpp"
#include "EvaluationContext.hpp"
#include "Board.hpp"

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
	context{new EvaluationContext()},
	hashTable{new HashTable(24)},
	board{new Board()}
{
	
}

Engine::~Engine(){}

void Engine::initPos()
{
	initPos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
	board->extraInfo = 0;
	board->blacksTurn = false;
	board->plyNumber = 0;
	board->plySinceLastPawnOrCapture = 0;
}

void Engine::initPos(string fenBoard)
{
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
				break;
				case 'r':
				board->fullBoard[8*(7 - i) + j] = Piece::Rook | Piece::Black;
				break;
				case 'n':
				board->fullBoard[8*(7 - i) + j] = Piece::Knight | Piece::Black;
				break;
				case 'b':
				board->fullBoard[8*(7 - i) + j] = Piece::Bishop | Piece::Black;
				break;
				case 'q':
				board->fullBoard[8*(7 - i) + j] = Piece::Queen | Piece::Black;
				break;
				case 'k':
				board->fullBoard[8*(7 - i) + j] = Piece::King | Piece::Black;
				break;

				case 'P':
				board->fullBoard[8*(7 - i) + j] = Piece::Pawn | Piece::White;
				break;
				case 'R':
				board->fullBoard[8*(7 - i) + j] = Piece::Rook | Piece::White;
				break;
				case 'N':
				board->fullBoard[8*(7 - i) + j] = Piece::Knight | Piece::White;
				break;
				case 'B':
				board->fullBoard[8*(7 - i) + j] = Piece::Bishop | Piece::White;
				break;
				case 'Q':
				board->fullBoard[8*(7 - i) + j] = Piece::Queen | Piece::White;
				break;
				case 'K':
				board->fullBoard[8*(7 - i) + j] = Piece::King | Piece::White;
				break;

				default:
				board->fullBoard[8*(7 - i) + j] = 0;
				for (int iter = 1; iter < c - '0'; ++iter)
				{
					++j;
					board->fullBoard[8*(7 - i) + j] = 0;
				}
			}
			++j;
		}
	}

	board->initPieceBoards();
	board->resetHash();
}


EvaluationContext::EvaluationContext()
:
	quitFlag(false),
	stopFlag(false),
	debugFlag(false),
	keepNStacks(1),
	depthWalkValue(1),
	quiescenceSearchDepth(4)
{
	resetGoFlags();
}

void EvaluationContext::resetGoFlags()
{
	ponderFlag = false;
	mateSearchFlag = false;
	infiniteFlag = false;

	moveTime = 0;
	wTime = 0;
	bTime = 0;
	wInc = 0;
	bInc = 0;
	movesToGo = 0;
	maxDepth = 0;
	maxNodes = 0;
}

void HashTable::reset()
{
	delete[] table;
	table = new HashBoard[size]{};
	occupancy = 0;
}
