#include <Net.hpp>
#include <Serialization/Deserializer.hpp>
#include <Serialization/Serializer.hpp>

namespace TicTacToe
{
	namespace Net
	{
		std::unique_ptr<Message> Message::Deserialize(const Bousk::uint8* buffer, const size_t bufferSize)
		{
			Bousk::Serialization::Deserializer deserializer(buffer, bufferSize);
			Type type;
			if (deserializer.read(type))
			{
				std::unique_ptr<Message> msg = [&]() -> std::unique_ptr<Message>
				{
					switch (type)
					{
						case Type::Setup: return std::make_unique<Setup>();
						case Type::Start: return std::make_unique<Start>();
						case Type::PlayRequest: return std::make_unique<PlayRequest>();
						case Type::PlayResult: return std::make_unique<PlayResult>();
							// TODO : log error
						default: assert(false); return nullptr;
					}
				}();
				if (msg && msg->read(deserializer))
					return msg;
			}
			// TODO : log error
			return nullptr;
		}
		bool Message::write(Bousk::Serialization::Serializer& stream) const
		{
			return stream.write(mType);
		}
		bool Message::read(Bousk::Serialization::Deserializer& stream)
		{
			// Type is extracted outside the read method to create the typed message
			return true;
		}

		bool Setup::write(Bousk::Serialization::Serializer& stream) const
		{
			return Message::write(stream)
				&& stream.write(symbol, Case::Empty, Case::O);
		}
		bool Setup::read(Bousk::Serialization::Deserializer& stream)
		{
			return Message::read(stream)
				&& stream.read(symbol, Case::Empty, Case::O);
		}

		bool Start::write(Bousk::Serialization::Serializer& stream) const
		{
			return Message::write(stream)
				&& stream.write(symbol, Case::Empty, Case::O);
		}
		bool Start::read(Bousk::Serialization::Deserializer& stream)
		{
			return Message::read(stream)
				&& stream.read(symbol, Case::Empty, Case::O);
		}

		bool PlayRequest::write(Bousk::Serialization::Serializer& stream) const
		{
			return Message::write(stream)
				&& stream.write(x)
				&& stream.write(y);
		}
		bool PlayRequest::read(Bousk::Serialization::Deserializer& stream)
		{
			return Message::read(stream)
				&& stream.read(x)
				&& stream.read(y);
		}

		bool PlayResult::write(Bousk::Serialization::Serializer& stream) const
		{
			return Message::write(stream)
				&& stream.write(valid)
				&& stream.write(x)
				&& stream.write(y);
		}
		bool PlayResult::read(Bousk::Serialization::Deserializer& stream)
		{
			return Message::read(stream)
				&& stream.read(valid)
				&& stream.read(x)
				&& stream.read(y);
		}
	}
}