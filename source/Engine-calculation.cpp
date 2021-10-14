#include "Engine.hpp"

using namespace std;

void Engine::calculationLoop()
{
	while (true)
	{
		if (quitFlag) return;
		if (stopFlag)
		{
			goFlag = false;
			stopFlag = false;

			//TODO
		}
		if (goFlag)
		{
			//TODO
		}
	}
}
