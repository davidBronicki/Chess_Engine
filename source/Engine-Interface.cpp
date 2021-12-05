#include "Engine.hpp"
#include <iostream>

#include "EvaluationContext.hpp"
#include "Board.hpp"
#include "HashTable.hpp"

using namespace std;

vector<string> tokenize(
	const string& str,
	const regex& re)
{
	sregex_token_iterator it{
		str.begin(), 
		str.end(),
		re,
		-1};
	
	vector<string> tokenized{it, {}};

	// Additional check to remove empty strings
	tokenized.erase(
		remove_if(tokenized.begin(), 
			tokenized.end(),
			[](string const& s) {
				return s.size() == 0;
				}),
		tokenized.end());

	return tokenized;
}

uc algebraicToIndex(std::string square)
{
	return square[0] - 'a' + 8 * (square[1] - '1');
}

string indexToAlgebraic(uc index)
{
	return {static_cast<char>('a' + index % 8),
		static_cast<char>('1' + index / 8)};
}

string moveToAlgebraic(Move move)
{
	string promoString = "";
	switch (move.moveType)
	{
		case Move::PromoQueen:
		promoString = "q";
		break;
		case Move::PromoRook:
		promoString = "r";
		break;
		case Move::PromoBishop:
		promoString = "b";
		break;
		case Move::PromoKnight:
		promoString = "n";
		break;
		default:
		break;
	}
	return indexToAlgebraic(move.sourceSquare) + indexToAlgebraic(move.targetSquare) + promoString;
}

void Engine::run()
{
	while (true)
	{
		string inputLine;
		getline(cin, inputLine);
		handleString(inputLine);
		if (context->stopFlag)
		{
			calculationThread.join();
			context->stopFlag = false;
		}
		if (context->quitFlag)
		{
			context->stopFlag = true;
			if (calculationThread.joinable())
				calculationThread.join();
			return;
		}
		if (context->goFlag)
		{
			context->goFlag = false;
			calculationThread = thread(Engine::calculationLoop, this);
		}
	}
}

Move Engine::buildMoveFromFragments(uc startIndex, uc endIndex, char promotion)
{
	Move::Type moveType = Move::Normal;
	if (promotion == 0)
	{
		if ((board->fullBoard[startIndex] & Piece::Occupied) == Piece::Pawn//pawn move
			&& static_cast<ul>(startIndex - endIndex) % 2//capture move
			&& (board->fullBoard[endIndex] == 0))//going to an empty square
		{
			moveType = Move::EnPassant;
		}
		else if ((board->fullBoard[startIndex] & Piece::Occupied) == Piece::King//king move
			&& (static_cast<ul>(startIndex - endIndex) % 4) == 2)//moved two squares
		{
			//castling, need to figure out which one
			switch (endIndex)
			{
				case 2:
				moveType = Move::WhiteLong;
				break;
				case 6:
				moveType = Move::WhiteShort;
				break;
				case 2 + 7 * 8:
				moveType = Move::BlackLong;
				break;
				case 6 + 7 * 8:
				moveType = Move::BlackShort;
				break;
			}
		}
	}
	else
	{
		switch (promotion)
		{
			case 'q':
			moveType = Move::PromoQueen;
			break;
			
			case 'r':
			moveType = Move::PromoRook;
			break;
			
			case 'b':
			moveType = Move::PromoBishop;
			break;
			
			case 'n':
			moveType = Move::PromoKnight;
			break;
		}
	}
	return board->buildMoveFromContext(startIndex, endIndex, moveType);
}

void Engine::handleString(string inputLine)
{
	vector<string> tokenizedLine = tokenize(
		inputLine,
		interfaceParsingTokens
	);

	string state = "base";

	for (auto&& item : tokenizedLine)
	{
		if (state == "base")
		{
			if (item == "uci")
			{
				cout << "id author bronicki" << endl;
				cout << "uciok" << endl;
				if (!Engine::globalsReady)
					Engine::initializeGlobals();
				return;
			}
			else if (item == "isready")
			{
				if (!Engine::globalsReady)
					Engine::initializeGlobals();
				cout << "readyok" << endl;
				return;
			}
			else if (item == "position")
			{
				state = "position";
			}
			else if (item == "go")
			{
				context->resetGoFlags();
				// context->bestMoveStacks.resize(0);
				context->goFlag = true;
				state = "go";
			}
			else if (item == "stop")
			{
				context->stopFlag = true;
				return;
			}
			else if (item == "quit")
			{
				context->quitFlag = true;
				return;
			}
			else if (item == "debug")
			{
				state = "debug";
			}
			else if (item == "setoption")
			{
				state = "setoption";
			}
			else if (item == "ucinewgame")
			{
				//TODO
			}
			else if (item == "ponderhit")
			{
				//TODO
			}
		}

		else if (state == "debug")
		{
			context->debugFlag = item == "on";
		}

		else if (state == "position")
		{
			if (item == "startpos")
			{
				initPos();
				state = "check for moves";
			}
			else if (item == "fen")
			{
				state = "fen 1";
			}
		}
		else if (state == "fen 1")
		{
			//first fen entry: board state
			initPos(item);
			state = "fen 2";
		}
		else if (state == "fen 2")
		{
			//whos turn it is
			board->blacksTurn = (item == "b");
			state = "fen 3";
		}
		else if (state == "fen 3")
		{
			board->extraInfo = 0;
			//castling
			for (auto&& c : item)
			{
				if (c == 'K')
					board->extraInfo |= Extra::White_Short;
				if (c == 'Q')
					board->extraInfo |= Extra::White_Long;
				if (c == 'k')
					board->extraInfo |= Extra::Black_Short;
				if (c == 'q')
					board->extraInfo |= Extra::Black_Long;
			}
			state = "fen 4";
		}
		else if (state == "fen 4")
		{
			//en passant square
			if (item != "-")
			{
				board->extraInfo |= stol(item)
					| Extra::EnPassantAvailable;
			}

			state = "fen 5";
		}
		else if (state == "fen 5")
		{
			//fifty move rule number (in half moves)

			board->plySinceLastPawnOrCapture = stol(item);

			state = "fen 6";
		}
		else if (state == "fen 6")
		{
			//move number

			board->plyNumber = stol(item) * 2 + board->blacksTurn - 2;

			board->resetHash();

			state = "check for moves";
		}
		else if (state == "check for moves")
		{
			if (item == "moves")
				state = "position moves";
		}
		else if (state == "position moves")
		{
			if (item == "0000")
			{
				advance(board->buildMoveFromContext(0, 0, Move::NullMove));
			}
			else
			{
				uc startIndex = algebraicToIndex(item.substr(0, 2));
				uc endIndex = algebraicToIndex(item.substr(2, 2));
				if (item.size() == 5)
				{
					advance(buildMoveFromFragments(startIndex, endIndex, item[4]));
				}
				else
				{
					advance(buildMoveFromFragments(startIndex, endIndex, 0));
				}
			}
		}
		else if (state == "go")
		{
			if (item == "searchmoves")
			{
				//TODO
			}
			else if (item == "ponder")
			{
				context->ponderFlag = true;
			}
			else if (item == "mate")
			{
				context->mateSearchFlag = true;
			}
			else if (item == "infinite")
			{
				context->infiniteFlag = true;
			}
			else if (item == "movetime")
			{
				state = "setMoveTime";
			}
			else if (item == "wtime")
			{
				state = "setwtime";
			}
			else if (item == "btime")
			{
				state = "setbtime";
			}
			else if (item == "winc")
			{
				state = "setwinc";
			}
			else if (item == "binc")
			{
				state = "setbinc";
			}
			else if (item == "movestogo")
			{
				state = "setMovesToGo";
			}
			else if (item == "depth")
			{
				state = "setMaxDepth";
			}
			else if (item == "nodes")
			{
				state = "setMaxNodes";
			}
		}
		else if (state == "setMoveTime")
		{
			context->moveTime = stoi(item);
			state = "go";
		}
		else if (state == "setwtime")
		{
			context->wTime = stoi(item);
			state = "go";
		}
		else if (state == "setbtime")
		{
			context->bTime = stoi(item);
			state = "go";
		}
		else if (state == "setwinc")
		{
			context->wInc = stoi(item);
			state = "go";
		}
		else if (state == "setbinc")
		{
			context->bInc = stoi(item);
			state = "go";
		}
		else if (state == "setMovesToGo")
		{
			context->movesToGo = stoi(item);
			state = "go";
		}
		else if (state == "setMaxDepth")
		{
			context->maxDepth = stoi(item);
			state = "go";
		}
		else if (state == "setMaxNodes")
		{
			context->maxNodes = stoi(item);
			state = "go";
		}
		else if (state == "setoption")
		{
			//TODO
		}
	}
}

void Engine::updateUI()
{
	UpdateableData<PV_Data>& best = context->pvLines[0];
	if (!best.changed())
	{
		return;
	}
	PV_Data const& pv_data = best.get();
	cout << "info depth ";
	cout << pv_data.specificDepth;
	cout << " seldepth ";
	cout << pv_data.specificSelectiveDepth;
	// cout << " multipv 1 ";
	if (pv_data.value.matePlyNumber == 0)
	{
		cout << " score cp ";
		cout << (pv_data.value.materialValue / 10);
	}
	else
	{
		if (pv_data.value.matePlyNumber > 0)
		{
			int plyToMate = pv_data.value.matePlyNumber - board->plyNumber;
			cout << " score mate ";
			cout << (plyToMate / 2 + 1);
		}
		else
		{
			int plyToMate = pv_data.value.matePlyNumber + board->plyNumber;
			cout << " score mate ";
			cout << (plyToMate / 2);
		}
	}

	cout << " nodes ";
	cout << context->nodesReached;
	if (context->timeSpent != 0)
	{
		cout << " nps ";
		cout << ((long long)context->nodesReached * 1000 / context->timeSpent);
	}
	cout << " hashfull ";
	cout << (hashTable->getOccupancy() * 1000 / hashTable->getSize());
	// cout << " tbhits ";//table base hits
	cout << " time ";
	cout << context->timeSpent;
	cout << " pv ";

	cout << moveToAlgebraic(pv_data.moves[0]);
	for (int i = 1; i < pv_data.moves.size(); ++i)
	{
		cout << " ";
		cout << moveToAlgebraic(pv_data.moves[i]);
	}
	cout << endl;
}

void Engine::returnResult()
{
	UpdateableData<PV_Data>& best = context->pvLines[0];
	PV_Data const& pv_data = best.get();
	if (pv_data.moves.size() > 0)
	{
		cout << "bestmove " << moveToAlgebraic(pv_data.moves[0]) << endl;
	}
	else
	{
		cout << "bestmove " << "0000" << endl;
	}
}
