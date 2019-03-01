#pragma once

#include <vector>
#include <string>
#include <any>

namespace advd
{
	class StreamRef
	{
		std::any m_handle;

		template <class T> StreamRef(T t): m_handle(t) {}

	public:
		~StreamRef();

		std::string id () const;
		std::string label () const;

		static std::vector<StreamRef> enumerate ();
	};
}