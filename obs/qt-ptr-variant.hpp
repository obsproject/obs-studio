/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QVariant>

/* in case you're wondering, I made this code because qt static asserts would
 * fail on forward pointers.  so I was forced to make a hack */

struct PtrVariantDummy {
	void *unused;
};

Q_DECLARE_METATYPE(PtrVariantDummy*);

template<typename T> static inline T VariantPtr(QVariant &v)
{
	return (T)v.value<PtrVariantDummy*>();
}

static inline QVariant PtrVariant(void* ptr)
{
	return QVariant::fromValue<PtrVariantDummy*>((PtrVariantDummy*)ptr);
}
