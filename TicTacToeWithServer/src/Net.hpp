#pragma once

#include <RangedInteger.hpp>

#include <Game.hpp>

#include <memory>

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
		struct Message : public Bousk::Serialization::Serializable
		{
			enum class Type {
				Min,

				Setup = Min,
				Start,
				PlayRequest,
				PlayResult,

				Max = PlayResult
			};

			Type type() const { return mType; }

			static std::unique_ptr<Message> Deserialize(const Bousk::uint8* buffer, const size_t bufferSize);

		protected:
			Message(Type t)
				: mType(t)
			{}

			bool write(Bousk::Serialization::Serializer&) const override;
			bool read(Bousk::Serialization::Deserializer&) override;

		private:
			Type mType;
		};

		struct Setup : public Message
		{
			Case symbol{ Case::Empty };

			bool write(Bousk::Serialization::Serializer&) const override;
			bool read(Bousk::Serialization::Deserializer&) override;

			Setup()
				: Message(Message::Type::Setup)
			{}
		};

		struct Start : public Message
		{
			Case symbol{ Case::Empty };

			bool write(Bousk::Serialization::Serializer&) const override;
			bool read(Bousk::Serialization::Deserializer&) override;

			Start()
				: Message(Message::Type::Start)
			{}
		};

		struct PlayRequest : public Message
		{
			Bousk::RangedInteger<0, 2> x;
			Bousk::RangedInteger<0, 2> y;

			bool write(Bousk::Serialization::Serializer&) const override;
			bool read(Bousk::Serialization::Deserializer&) override;

			PlayRequest()
				: Message(Message::Type::PlayRequest)
			{}
		};

		struct PlayResult : public Message
		{
			bool valid{ false };
			Bousk::RangedInteger<0, 2> x;
			Bousk::RangedInteger<0, 2> y;
			
			bool write(Bousk::Serialization::Serializer&) const override;
			bool read(Bousk::Serialization::Deserializer&) override;

			PlayResult()
				: Message(Message::Type::PlayResult)
			{}
		};
	}
}