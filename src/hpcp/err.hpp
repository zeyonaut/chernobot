#pragma once

namespace hpcp
{
	template<typename T> class Result
	{
		union
		{
			T ham;
			std::exception_ptr spam;
		};
		bool is_ham;
		Result() {}

	public:

		Result(const T& rhs) : ham(rhs), is_ham(true) {}
		Result(T&& rhs) : ham(std::move(rhs)), is_ham(true) {}
		Result(const Result& rhs) : is_ham(rhs.is_ham)
		{
			if (is_ham) new(&ham) T(rhs.ham);
			else new(&spam) std::exception_ptr(rhs.spam);
		}
		Result(Result&& rhs) : is_ham(rhs.is_ham)
		{
			if (is_ham) new(&ham) T(std::move(rhs.ham));
			else new(&spam) std::exception_ptr(std::move(rhs.spam));
		}

		static Result<T> from_exception(std::exception_ptr p)
		{
			Result<T> result;
			result.is_ham = false;
			new(&result.spam) std::exception_ptr(std::move(p));
			return result;
		}

		template<typename E> static Result<T> from_exception(const E& exception)
		{
			return from_exception(std::make_exception_ptr(exception));
		}

		static Result<T> capture()
		{
			return from_exception(std::current_exception());
		}

		template<typename F> static Result<T> capture(F f)
		{
			try
			{
				return Result(f());
			}
			catch (...)
			{
				return capture();
			}
		}

		operator bool() const {return is_ham;};

		T& get()
		{
			if (!is_ham) rethrow_exception(spam);
			return ham;
		}

		const T& get() const
		{
			if (!is_ham) rethrow_exception(spam);
			return ham;
		}
	};
}