#pragma once

#include "Value.hpp"

struct HashBoard
{
	ull hash;
	Value value;
	Move bestResponse;

	PlyType searchDepth;
	PlyType rootPly;

	enum CutType : uc
	{
		PrincipleVariation,
		AllNode,
		CutNode,
		Quiescent
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
		HashNotPresent = 0,
		HashesNotEqual = 1,
		HashesEqual = 2
	};
	
	HashTable(size_t tableSize)
	:
		table{new HashBoard[1ull << tableSize]{}},
		size(1ull << tableSize),
		mask((1ull << tableSize) - 1)
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

	size_t getSize() {return size;}
	size_t getOccupancy() {return occupancy;}
};