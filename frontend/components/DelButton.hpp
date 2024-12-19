#pragma once

#include <QPushButton>

class DelButton : public QPushButton {
public:
	inline DelButton(QModelIndex index_) : QPushButton(), index(index_) {}

	QPersistentModelIndex index;
};
