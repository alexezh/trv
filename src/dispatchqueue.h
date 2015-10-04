#pragma once

struct IDispatchQueue
{
	virtual void Post(const std::function<void()>& func) = 0;
};
