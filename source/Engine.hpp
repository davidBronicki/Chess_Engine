#pragma once

#include <string>
#include <regex>
#include <memory>
#include <thread>

#include "Board.hpp"

std::vector<std::string> tokenize(
	const std::string& str,
	const std::regex& re);

uc algebraicToIndex(std::string square);

struct HashBoard
{
	ull hash;
	float eval;
	uc plyToMate;
	us plyNumber;
	us plySearchDepth;
};

class HashTable
{
	HashBoard* table;
	size_t size;
	size_t occupancy;
	public:
	HashTable(size_t tableSize): table{new HashBoard[tableSize]{}}{}
	~HashTable(){delete[] table;}

	HashTable(HashTable const&) = delete;
	HashTable& operator=(HashTable const&) = delete;

	HashBoard const& get(ull hash) const {return table[hash % size];}
	void set(HashBoard board) {
		HashBoard& slot = table[board.hash % size];
		if (slot.hash == 0)
			occupancy++;
		slot = board;
	}

	size_t getSize() {return size;}
	size_t getOccupancy() {return occupancy;}
};

// class MoveStack
// {
// 	std::vector<Move> moves;
// 	// HashBoard currentEval;
// 	public:
// 	MoveStack(){};
// 	void push(Move move){moves.push_back(move);}
// 	Move pop(){Move output = moves.back(); moves.pop_back(); return output;}
// 	Move const& top() const{return moves.back();}

// 	Move& operator[](size_t i){return moves[i];}
// 	size_t size(){return moves.size()}
// };

class Engine
{
	static bool globalsReady;
	

	//uci required parameters and flags
	bool goFlag, stopFlag, quitFlag, debugFlag;
	bool ponderFlag, mateSearchFlag, infiniteFlag;
	int moveTime, wTime, bTime, wInc, bInc, movesToGo, maxDepth, maxNodes;

	//custom parameters
	int keepNStacks;

	//state variables
	std::shared_ptr<Board> board;
	HashTable hashTable;

	std::vector<Move> currentMoveStack;
	std::vector<std::vector<Move>> bestMoveStacks;

	std::thread calculationThread;

	//io variables
	std::regex interfaceParsingTokens;
	std::regex fenParsingTokens;
	void handleString(std::string inputLine);

	//parameters set when io calls go. need to be reset when stopping
	void resetGoFlags();

	static void calculationLoop(Engine* engine);

	void initPos();
	void initPos(std::string positionString);
	void resetCongregateData();
	Move makeMove(uc startIndex, uc endIndex, char promotion);

	public:
	static void initializeGlobals();

	Engine();
	void run();
};