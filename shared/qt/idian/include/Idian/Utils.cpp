#include <Idian/Utils.hpp>
#include <Idian/StateEventFilter.hpp>

namespace idian {

void Utils::applyStateStylingEventFilter(QWidget *widget)
{
	widget->installEventFilter(new StateEventFilter(this, widget));
}
} // namespace idian
