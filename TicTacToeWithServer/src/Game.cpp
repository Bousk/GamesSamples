#include <Game.hpp>

namespace TicTacToe
{
	bool Grid::play(unsigned int x, unsigned int y, Case player)
	{
		if (x > 2 || y > 2)
			return false;
		if (mGrid[x][y] != Case::Empty)
			return false;

		mGrid[x][y] = player;
		// Check if the game is now over
		// Did this lead to a full horizontal line ?
		bool justWon = (mGrid[x][0] == mGrid[x][1]) && (mGrid[x][1] == mGrid[x][2]);
		// A vertical win ?
		justWon |= (mGrid[0][y] == mGrid[1][y]) && (mGrid[1][y] == mGrid[2][y]);
		// Diagonal ? If possible
		if (x == y)
		{
			// Top left to bottom right
			justWon |= (mGrid[0][0] == mGrid[1][1]) && (mGrid[1][1] == mGrid[2][2]);
		}
		if (x + y == 2)
		{
			// Bottom left to top right
			justWon |= (mGrid[2][0] == mGrid[1][1]) && (mGrid[1][1] == mGrid[0][2]);
		}
		if (justWon)
		{
			mWinner = player;
			mFinished = true;
		}
		else if (isGridFull())
		{
			mFinished = true;
		}
		return true;
	}
	bool Grid::isGridFull() const
	{
		for (unsigned x = 0; x < 3; ++x)
		{
			for (unsigned y = 0; y < 3; ++y)
			{
				if (mGrid[x][y] == Case::Empty)
					return false;
			}
		}
		return true;
	}
}