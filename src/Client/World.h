#pragma once
#include "Block.h"
#include <vector>

class Chunk;

class World {
public:
	std::vector<Chunk*> chunks;

	World() {}
	Chunk* getChunk(int cx, int cz) const;
	Block getBlockAt(int x, int y, int z) const;
	void setBlockAt(Block block, int x, int y, int z);

private:
	World(const World&) = delete;
	World(World&&) = delete;
};

