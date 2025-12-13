#include <QAbstractListModel>
#include <QVariant>
#include <QVector>
#include <QString>
#include <QHash>

// Mock OBS Types for UI Prototype
typedef struct obs_source obs_source_t;
typedef struct obs_properties obs_properties_t;
typedef struct obs_data obs_data_t;
enum obs_property_type {
    OBS_PROPERTY_INVALID,
    OBS_PROPERTY_BOOL,
    OBS_PROPERTY_INT,
    OBS_PROPERTY_FLOAT,
    OBS_PROPERTY_TEXT,
    OBS_PROPERTY_PATH,
    OBS_PROPERTY_LIST,
    OBS_PROPERTY_COLOR,
    OBS_PROPERTY_BUTTON
};

class PropertiesViewModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString sourceUuid READ sourceUuid WRITE setSourceUuid NOTIFY sourceUuidChanged)

      public:
    enum PropertyRoles {
        NameRole = Qt::UserRole + 1,
        TypeRole,
        ValueRole,
        DescriptionRole,
        MinRole,
        MaxRole,
        OptionsRole  // For List/Combobox properties
    };

    struct PropertyItem {
        QString name;
        obs_property_type type;
        QVariant value;
        QString description;
        // extended fields (min/max/step/options) omitted for brevity in first pass
        QVariantMap extra;
    };

    explicit PropertiesViewModel(QObject *parent = nullptr);
    ~PropertiesViewModel();

    QString sourceUuid() const;
    void setSourceUuid(const QString &uuid);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void updatePropertyValue(int row, const QVariant &newValue);
    Q_INVOKABLE void refresh();

      signals:
    void sourceUuidChanged();

      private:
    void loadProperties();

    QString m_sourceUuid;
    obs_source_t *m_source = nullptr;
    obs_properties_t *m_properties = nullptr;
    obs_data_t *m_settings = nullptr;

    QVector<PropertyItem> m_items;
};
