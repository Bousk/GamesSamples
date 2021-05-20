#pragma once

#include <RangedInteger.hpp>

#include <Game.hpp>

namespace Bousk
{
	namespace Serialization
	{
		class Deserializer;
		class Serializer;
	}
}
namespace TicTacToe
{
	namespace Net
	{
		struct Play
		{
			Bousk::RangedInteger<0, 2> x;
			Bousk::RangedInteger<0, 2> y;
			
			bool write(Bousk::Serialization::Serializer&) const;
			bool read(Bousk::Serialization::Deserializer&);
		};

		//struct Start
		//{
		//	Case symbol;

		//	bool write(Bousk::Serialization::Serializer&) const;
		//	bool read(Bousk::Serialization::Deserializer&);
		//};
	}
}