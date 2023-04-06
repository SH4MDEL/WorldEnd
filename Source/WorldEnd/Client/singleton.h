#pragma once
#include "stdafx.h"

template <typename T>
class Singleton
{
protected:
	Singleton() = default;

public:
	Singleton(const Singleton& rhs) = delete;
	Singleton(Singleton&& rhs) = delete;
	void operator=(const Singleton& rhs) = delete;
	void operator=(Singleton&&) = delete;

	static T& GetInstance()
	{
		static T instance;
		return instance;
	}
};