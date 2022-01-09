#pragma once

#include <array>

namespace TicTacToe
{
	enum class Case
	{
		Empty,
		X,
		O,
	};
	class Grid
	{
	public:
		Grid() = default;
		~Grid() = default;

		// Reset the game to its initial state
		void reset();

		// Returns true if the play is valid, false otherwise.
		bool canPlay(unsigned int x, unsigned int y, Case player) const;
		// Play a move from given player in given case. Return true if it's valid, false otherwise.
		bool play(unsigned int x, unsigned int y, Case player);
		// Return true if the game is over, false otherwise
		bool isFinished() const { return mFinished; }
		// Return winner, if any, Case::Empty otherwise
		Case winner() const { return mWinner; }

		const std::array<std::array<Case, 3>, 3>& grid() const { return mGrid; }

	private:
		// Check if the grid is full
		bool isGridFull() const;

	private:
		std::array<std::array<Case, 3>, 3> mGrid{ Case::Empty, Case::Empty, Case::Empty, Case::Empty, Case::Empty, Case::Empty, Case::Empty, Case::Empty, Case::Empty };
		Case mWinner{ Case::Empty };
		bool mFinished{ false };
	};

	class Game
	{
	public:
		void start(Case firstPlayer);
		inline bool isStarted() const { return mCurrentPlayer != Case::Empty; }
		bool play(unsigned int x, unsigned int y);
		void reset();

		inline const Grid& grid() const { return mGrid; }
		inline Case currentPlayer() const { return mCurrentPlayer; }

	private:
		Grid mGrid;
		Case mCurrentPlayer{ Case::Empty };
	};
}