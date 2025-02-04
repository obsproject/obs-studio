#include "Rect.hpp"

namespace OBS {
bool Rect::isZero() const
{
	return width_ == 0.0 || height_ == 0.0;
}

bool operator==(const Rect &lhs, const Rect &rhs)
{
	return lhs.width_ == rhs.width_ && lhs.height_ == rhs.height_;
}

bool operator!=(const Rect &lhs, const Rect &rhs)
{
	return lhs.width_ != rhs.width_ || lhs.height_ != rhs.height_;
}
} // namespace OBS
