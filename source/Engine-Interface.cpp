#include "Engine.hpp"
#include <iostream>

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
	return 'f' - square[0] + 8 * ('8' - square[1]);
}

void Engine::ioLoop()
{
	while (true)
	{
		string inputLine;
		cin >> inputLine;
		handleString(inputLine);
		if (quitFlag)
			return;
	}
}

Move Engine::makeMove(uc startIndex, uc endIndex, char promotion)
{
	uc moveType = MoveType::Normal;
	if (promotion == 0)
	{
		if ((bool)(board->fullBoard[startIndex] ^ Piece::Pawn) 
			&& (bool)((startIndex - endIndex) % 8)
			&& (bool)(board->fullBoard[endIndex] == 0))
		{
			moveType = MoveType::EnPassant;
		}
		else if ((bool)(board->fullBoard[startIndex] ^ Piece::King)
			&& ((startIndex - endIndex) % 4) == 2)
		{
			//castling, need to figure out which one
			switch (endIndex)
			{
				case 2:
				moveType = MoveType::WhiteLong;
				break;
				case 6:
				moveType = MoveType::WhiteShort;
				break;
				case 2 + 7 * 8:
				moveType = MoveType::BlackLong;
				break;
				case 6 + 7 * 8:
				moveType = MoveType::BlackShort;
				break;
			}
		}
	}
	else
	{
		switch (promotion)
		{
			case Piece::Queen:
			moveType = MoveType::PromoQueen;
			break;
			
			case Piece::Rook:
			moveType = MoveType::PromoRook;
			break;
			
			case Piece::Bishop:
			moveType = MoveType::PromoBishop;
			break;
			
			case Piece::Knight:
			moveType = MoveType::PromoKnight;
			break;
		}
	}
	return board->constructMove(startIndex, endIndex, moveType);
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
				/*
				tell engine to use the uci (universal chess interface),
				this will be send once as a first command after program boot
				to tell the engine to switch to uci mode.
				After receiving the uci command the engine must identify itself with the "id" command
				and sent the "option" commands to tell the GUI which engine settings the engine supports if any.
				After that the engine should sent "uciok" to acknowledge the uci mode.
				If no uciok is sent within a certain time period, the engine task will be killed by the GUI.
				*/
				cout << "id author bronicki" << endl;
				cout << "uciok" << endl;
				return;
			}
			else if (item == "isready")
			{
				/*
				this is used to synchronize the engine with the GUI. When the GUI has sent a command or
				multiple commands that can take some time to complete,
				this command can be used to wait for the engine to be ready again or
				to ping the engine to find out if it is still alive.
				E.g. this should be sent after setting the path to the tablebases as this can take some time.
				This command is also required once before the engine is asked to do any search
				to wait for the engine to finish initializing.
				This command must always be answered with "readyok" and can be sent also when the engine is calculating
				in which case the engine should also immediately answer with "readyok" without stopping the search.
				*/
				cout << "readyok" << endl;
				return;
			}
			else if (item == "position")
			{
				state = "position";
			}
			else if (item == "go")
			{
				resetGoFlags();
				goFlag = true;
				state = "go";
			}
			else if (item == "stop")
			{
				/*
				stop calculating as soon as possible,
				don't forget the "bestmove" and possibly the "ponder" token when finishing the search
				*/
				stopFlag = true;
				return;
			}
			else if (item == "quit")
			{
				/*
				quit the program as soon as possible
				*/
				quitFlag = true;
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
				/*
				this is sent to the engine when the next search (started with "position" and "go") will be from
				a different game. This can be a new game the engine should play or a new game it should analyse but
				also the next position from a testsuite with positions only.
				If the GUI hasn't sent a "ucinewgame" before the first "position" command, the engine shouldn't
				expect any further ucinewgame commands as the GUI is probably not supporting the ucinewgame command.
				So the engine should not rely on this command even though all new GUIs should support it.
				As the engine's reaction to "ucinewgame" can take some time the GUI should always send "isready"
				after "ucinewgame" to wait for the engine to finish its operation.
				*/

				//TODO
			}
			else if (item == "ponderhit")
			{
				/*
				the user has played the expected move. This will be sent if the engine was told to ponder on the same move
				the user has played. The engine should continue searching but switch from pondering to normal search.
				*/

				//TODO
			}
		}

		else if (state == "debug")
		{
			/*
			[on | off]

			switch the debug mode of the engine on and off.
			In debug mode the engine should sent additional infos to the GUI, e.g. with the "info string" command,
			to help debugging, e.g. the commands that the engine has received etc.
			This mode should be switched off by default and this command can be sent
			any time, also when the engine is thinking.
			*/
			debugFlag = item == "on";
		}

		else if (state == "position")
		{
			/*
			[fen  | startpos ]  moves  ....

			set up the position described in fenstring on the internal board and
			play the moves on the internal chess board.
			if the game was played  from the start position the string "startpos" will be sent
			Note: no "new" command is needed. However, if this position is from a different game than
			the last position sent to the engine, the GUI should have sent a "ucinewgame" inbetween.
			*/

			if (item == "startpos")
			{
				initPos();
				state = "position moves";
			}
			else
			{
				//first fen entry: board state
				initPos(item);
				state = "fen 2";
			}
		}
		else if (state == "fen 2")
		{
			//whos turn it is
			board->blacksTurn = (item == "b");
			state = "fen 3";
		}
		else if (state == "fen 3")
		{
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
				board->extraInfo = board->extraInfo & Extra::CastleInfo;

			if (item != "-")
			{
				board->extraInfo |= (algebraicToIndex(item) & Extra::EnPassantFile)
					| Extra::EnPassantAvailable;
			}

			state = "fen 5";
		}
		else if (state == "fen 5")
		{
			//fifty move rule number (in half moves)

			board->plySinceLastPawnOrCapture = stoll(item);

			state = "fen 6";
		}
		else if (state == "fen 6")
		{
			//move number

			board->plyNumber = stoll(item) * 2 + board->blacksTurn - 2;

			state = "position moves";
		}
		else if (state == "position moves")
		{
			if (item == "0000")
			{
				board->performMove(nullMove(board->plySinceLastPawnOrCapture));
			}
			else
			{
				uc startIndex = algebraicToIndex(item.substr(0, 2));
				uc endIndex = algebraicToIndex(item.substr(2, 2));
				if (item.size() == 5)
				{
					board->performMove(makeMove(startIndex, endIndex, item[4]));
				}
				else
				{
					board->performMove(makeMove(startIndex, endIndex, 0));
				}
			}
		}
		else if (state == "go")
		{
			/*
			start calculating on the current position set up with the "position" command.
			There are a number of commands that can follow this command, all will be sent in the same string.
			If one command is not send its value should be interpreted as it would not influence the search.
			on the suggested move.
			*/

			if (item == "searchmoves")
			{
				/*
				searchmoves  .... 
					restrict search to this moves only
					Example: After "position startpos" and "go infinite searchmoves e2e4 d2d4"
					the engine should only search the two moves e2e4 and d2d4 in the initial position.
				*/

				//TODO
			}
			else if (item == "ponder")
			{
				/*
				ponder
					start searching in pondering mode.
					Do not exit the search in ponder mode, even if it's mate!
					This means that the last move sent in in the position string is the ponder move.
					The engine can do what it wants to do, but after a "ponderhit" command
					it should execute the suggested move to ponder on. This means that the ponder move sent by
					the GUI can be interpreted as a recommendation about which move to ponder. However, if the
					engine decides to ponder on a different move, it should not display any mainlines as they are
					likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
				*/

				ponderFlag = true;
			}
			else if (item == "mate")
			{
				/*
				mate 
					search for a mate in x moves
				*/

				mateSearchFlag = true;
			}
			else if (item == "infinite")
			{
				/*
				infinite
					search until the "stop" command. Do not exit the search without being told so in this mode!
				*/
			
				infiniteFlag = true;
			}
			else if (item == "movetime")
			{
				/*
				movetime 
					search exactly x mseconds
				*/
			
				state = "setMoveTime";
			}
			else if (item == "wtime")
			{
				/*
				wtime 
					white has x msec left on the clock
				*/
			
				state = "setwtime";
			}
			else if (item == "btime")
			{
				/*
				btime 
					black has x msec left on the clock
				*/
			
				state = "setbtime";
			}
			else if (item == "winc")
			{
				/*
				winc 
					white increment per move in mseconds if x > 0
				*/
			
				state = "setwinc";
			}
			else if (item == "binc")
			{
				/*
				binc 
					black increment per move in mseconds if x > 0
				*/
			
				state = "setbinc";
			}
			else if (item == "movestogo")
			{
				/*
				movestogo 
					there are x moves to the next time control,
					this will only be sent if x > 0,
					if you don't get this and get the wtime and btime it's sudden death
				*/
			
				state = "setMovesToGo";
			}
			else if (item == "depth")
			{
				/*
				depth 
					search x plies only.
				*/
			
				state = "setMaxDepth";
			}
			else if (item == "nodes")
			{
				/*
				nodes 
					search x nodes only
				*/
			
				state = "setMaxNodes";
			}
		}
		else if (state == "setMoveTime")
		{
			moveTime = stoi(item);
			state = "go";
		}
		else if (state == "setwtime")
		{
			wTime = stoi(item);
			state = "go";
		}
		else if (state == "setbtime")
		{
			bTime = stoi(item);
			state = "go";
		}
		else if (state == "setwinc")
		{
			wInc = stoi(item);
			state = "go";
		}
		else if (state == "setbinc")
		{
			bInc = stoi(item);
			state = "go";
		}
		else if (state == "setMovesToGo")
		{
			movesToGo = stoi(item);
			state = "go";
		}
		else if (state == "setMaxDepth")
		{
			maxDepth = stoi(item);
			state = "go";
		}
		else if (state == "setMaxNodes")
		{
			maxNodes = stoi(item);
			state = "go";
		}
		else if (state == "setoption")
		{
			/*
			name  [value ]

			this is sent to the engine when the user wants to change the internal parameters
			of the engine. For the "button" type no value is needed.
			One string will be sent for each parameter and this will only be sent when the engine is waiting.
			The name of the option in  should not be case sensitive and can inludes spaces like also the value.
			The substrings "value" and "name" should be avoided in  and  to allow unambiguous parsing,
			for example do not use  = "draw value".
			Here are some strings for the example below:
			"setoption name Nullmove value true\n"
			"setoption name Selectivity value 3\n"
			"setoption name Style value Risky\n"
			"setoption name Clear Hash\n"
			"setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
			*/

			//TODO
		}
	}
}

void Engine::resetGoFlags()
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
