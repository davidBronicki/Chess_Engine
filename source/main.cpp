#include "Engine.hpp"

int main()
{
	Engine* thing{new Engine()};
	thing->run();
	delete thing;
	return 0;
}
