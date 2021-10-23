#pragma once

#include "Value.hpp"

struct HashBoard
{
	ull hash;
	Value value;
	Move bestResponse;

	PlyType searchDepth;
	PlyType rootPly;

	enum NodeType : uc
	{
		PrincipleVariation	= 0b000,
		AllNode				= 0b001,
		CutNode				= 0b010,
		TypeInfo_Mask		= 0b011,
		Quiescence_PV		= 0b100,
		Quiescence_All		= 0b101,
		Quiescence_Cut		= 0b110,
		Quiescence_Mask		= 0b100,
	} nodeType;
};

class HashTable
{
	HashBoard* table;
	size_t size;
	size_t occupancy;
	size_t mask;
	public:
	
	enum HashOccupancyType : uc
	{
		HashNotPresent,
		HashesNotEqual,
		HashesEqual
	};

	// enum WindowType : uc
	// {
	// 	FullWindow,
	// 	ReducedWindow,
	// 	AsperationWindow,
	// 	NullWindow
	// };
	
	HashTable(size_t tableSize)
	:
		table{new HashBoard[1ull << tableSize]{}},
		size{1ull << tableSize},
		mask{(1ull << tableSize) - 1}
	{}
	~HashTable(){delete[] table;}

	HashTable(HashTable const&) = delete;
	HashTable& operator=(HashTable const&) = delete;

	HashBoard const& get(HashType hash) const {return table[hash & mask];}
	void set(HashBoard board) {//TODO: optimize to move instead of copy? build in place?
		HashBoard& slot = table[board.hash & mask];
		if (slot.hash == 0)
			occupancy++;
		slot = board;
	}

	HashOccupancyType getExistence(HashBoard const& hBoard, HashType hash);

	bool main_HashWindowSetup(
		HashBoard const& hBoard, Value& alpha, Value& beta,
		std::vector<Move>& moves, PlyType searchDepth, PlyType rootPly) const;

	void quiescence_HandleHash(
		HashBoard const& hBoard, Value& alpha, Value& beta,
		std::vector<Move>& moves, PlyType searchDepth, PlyType rootPly) const;

	size_t getSize() const {return size;}
	size_t getOccupancy() const {return occupancy;}

	void reset();
};
