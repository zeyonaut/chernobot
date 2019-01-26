#pragma once

#include <functional>
#include <vector>

class Fin
{
public:
	Fin (Fin&&) = default;
	Fin () = default;

	template <class Function> Fin (Function&& destructor)
	{
		push<Function>(std::forward<Function>(destructor));
	}

	template <class Function> Fin& operator+= (Function&& destructor)
	{
		push<Function>(std::forward<Function>(destructor));
		return *this;
	}

	~Fin ()
	{
		for (auto d = destructors.rbegin(); d != destructors.rend(); d++) (*d)();
	}

	template <class Function> void push (Function&& destructor)
	{
		destructors.push_back(std::forward<Function>(destructor));
	}

	void pop ()
	{
		if (destructors.size() > 0) destructors.pop_back();
	}

	void clear () noexcept
	{
		destructors.clear();
	}

private:
	Fin(const Fin&) = delete;
	void operator= (const Fin&) = delete;

	std::vector<std::function<void()>> destructors;
};