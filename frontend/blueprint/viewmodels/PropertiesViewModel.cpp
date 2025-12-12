#include "PropertiesViewModel.h"
#include <QDebug>

PropertiesViewModel::PropertiesViewModel(QObject *parent) : QAbstractListModel(parent) {}

PropertiesViewModel::~PropertiesViewModel()
{
	// Mock cleanup
	// if (m_properties) obs_properties_destroy(m_properties);
	// if (m_settings) obs_data_release(m_settings);
	// if (m_source) obs_source_release(m_source);
}

QString PropertiesViewModel::sourceUuid() const
{
	return m_sourceUuid;
}

void PropertiesViewModel::setSourceUuid(const QString &uuid)
{
	if (m_sourceUuid == uuid)
		return;
	m_sourceUuid = uuid;

	// Cleanup old (Mock)
	m_properties = nullptr;
	m_settings = nullptr;
	m_source = nullptr;

	beginResetModel();
	m_items.clear();

	if (uuid.isEmpty()) {
		endResetModel();
		emit sourceUuidChanged();
		return;
	}

	// Mock Property Population based on Node ID (simulating obs_source_properties)
	if (uuid.startsWith("111")) { // Camera
		m_items.append({"device", OBS_PROPERTY_LIST, "Video0", "Input Device", {}});
		m_items.append({"resolution", OBS_PROPERTY_LIST, "1920x1080", "Resolution", {}});
		m_items.append({"format", OBS_PROPERTY_LIST, "NV12", "Video Format", {}});
		m_items.append({"color_space", OBS_PROPERTY_LIST, "Rec.709", "Color Space", {}});
		m_items.append({"fps", OBS_PROPERTY_INT, 60, "Frame Rate", {}});
		m_items.append({"buffering", OBS_PROPERTY_BOOL, false, "Use Buffering", {}});
	} else if (uuid.startsWith("222")) { // Filter
		m_items.append({"blur_amount", OBS_PROPERTY_FLOAT, 5.0, "Blur Radius", {}});
		m_items.append({"mask_type", OBS_PROPERTY_LIST, "Circle", "Mask Shape", {}});
	} else {
		// Generic
		m_items.append({"enabled", OBS_PROPERTY_BOOL, true, "Enabled", {}});
		m_items.append({"opacity", OBS_PROPERTY_FLOAT, 1.0, "Opacity", {}});
		m_items.append({"name", OBS_PROPERTY_TEXT, "Node " + uuid.left(4), "Label", {}});
	}

	endResetModel();
	emit sourceUuidChanged();
}

void PropertiesViewModel::refresh()
{
	// Real implementation would reload properties from OBS source here
}

int PropertiesViewModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	return m_items.count();
}

QVariant PropertiesViewModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= m_items.count())
		return QVariant();

	const PropertyItem &item = m_items[index.row()];

	switch (role) {
	case NameRole:
		return item.name;
	case TypeRole:
		return (int)item.type;
	case ValueRole:
		return item.value;
	case DescriptionRole:
		return item.description;
	case MinRole:
		return item.extra.value("min");
	case MaxRole:
		return item.extra.value("max");
		// ...
	}
	return QVariant();
}

QHash<int, QByteArray> PropertiesViewModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[NameRole] = "p_name";
	roles[TypeRole] = "p_type";
	roles[ValueRole] = "p_value";
	roles[DescriptionRole] = "p_desc";
	roles[MinRole] = "p_min";
	roles[MaxRole] = "p_max";
	roles[OptionsRole] = "p_options";
	return roles;
}

void PropertiesViewModel::updatePropertyValue(int row, const QVariant &newValue)
{
	if (row < 0 || row >= m_items.count())
		return;

	// Update local model
	m_items[row].value = newValue;
	emit dataChanged(index(row), index(row), {ValueRole});

	// Update OBS backend
	// obs_data_set_...
	// obs_source_update(m_source, m_settings);
}
