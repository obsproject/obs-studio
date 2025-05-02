#pragma once

#include <QPersistentModelIndex>
#include <QPushButton>

class DelButton : public QPushButton {
	Q_OBJECT

public:
	inline DelButton(QModelIndex index_) : QPushButton(), index(index_) {}

	QPersistentModelIndex index;
};
