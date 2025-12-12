/****************************************************************************
** Meta object code from reading C++ file 'SettingsViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../frontend/blueprint/viewmodels/SettingsViewModel.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SettingsViewModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_SettingsViewModel_t {
    uint offsetsAndSizes[36];
    char stringdata0[18];
    char stringdata1[23];
    char stringdata2[1];
    char stringdata3[22];
    char stringdata4[22];
    char stringdata5[21];
    char stringdata6[21];
    char stringdata7[14];
    char stringdata8[9];
    char stringdata9[4];
    char stringdata10[6];
    char stringdata11[13];
    char stringdata12[6];
    char stringdata13[16];
    char stringdata14[15];
    char stringdata15[15];
    char stringdata16[14];
    char stringdata17[14];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_SettingsViewModel_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_SettingsViewModel_t qt_meta_stringdata_SettingsViewModel = {
    {
        QT_MOC_LITERAL(0, 17),  // "SettingsViewModel"
        QT_MOC_LITERAL(18, 22),  // "generalSettingsChanged"
        QT_MOC_LITERAL(41, 0),  // ""
        QT_MOC_LITERAL(42, 21),  // "streamSettingsChanged"
        QT_MOC_LITERAL(64, 21),  // "outputSettingsChanged"
        QT_MOC_LITERAL(86, 20),  // "audioSettingsChanged"
        QT_MOC_LITERAL(107, 20),  // "videoSettingsChanged"
        QT_MOC_LITERAL(128, 13),  // "updateSetting"
        QT_MOC_LITERAL(142, 8),  // "category"
        QT_MOC_LITERAL(151, 3),  // "key"
        QT_MOC_LITERAL(155, 5),  // "value"
        QT_MOC_LITERAL(161, 12),  // "applyChanges"
        QT_MOC_LITERAL(174, 5),  // "reset"
        QT_MOC_LITERAL(180, 15),  // "generalSettings"
        QT_MOC_LITERAL(196, 14),  // "streamSettings"
        QT_MOC_LITERAL(211, 14),  // "outputSettings"
        QT_MOC_LITERAL(226, 13),  // "audioSettings"
        QT_MOC_LITERAL(240, 13)   // "videoSettings"
    },
    "SettingsViewModel",
    "generalSettingsChanged",
    "",
    "streamSettingsChanged",
    "outputSettingsChanged",
    "audioSettingsChanged",
    "videoSettingsChanged",
    "updateSetting",
    "category",
    "key",
    "value",
    "applyChanges",
    "reset",
    "generalSettings",
    "streamSettings",
    "outputSettings",
    "audioSettings",
    "videoSettings"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_SettingsViewModel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       5,   76, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    6 /* Public */,
       3,    0,   63,    2, 0x06,    7 /* Public */,
       4,    0,   64,    2, 0x06,    8 /* Public */,
       5,    0,   65,    2, 0x06,    9 /* Public */,
       6,    0,   66,    2, 0x06,   10 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       7,    3,   67,    2, 0x02,   11 /* Public */,
      11,    0,   74,    2, 0x02,   15 /* Public */,
      12,    0,   75,    2, 0x02,   16 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QVariant,    8,    9,   10,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
      13, QMetaType::QVariantMap, 0x00015001, uint(0), 0,
      14, QMetaType::QVariantMap, 0x00015001, uint(1), 0,
      15, QMetaType::QVariantMap, 0x00015001, uint(2), 0,
      16, QMetaType::QVariantMap, 0x00015001, uint(3), 0,
      17, QMetaType::QVariantMap, 0x00015001, uint(4), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject SettingsViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SettingsViewModel.offsetsAndSizes,
    qt_meta_data_SettingsViewModel,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_SettingsViewModel_t,
        // property 'generalSettings'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'streamSettings'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'outputSettings'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'audioSettings'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'videoSettings'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SettingsViewModel, std::true_type>,
        // method 'generalSettingsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'streamSettingsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'outputSettingsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'audioSettingsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'videoSettingsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateSetting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'applyChanges'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reset'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void SettingsViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SettingsViewModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->generalSettingsChanged(); break;
        case 1: _t->streamSettingsChanged(); break;
        case 2: _t->outputSettingsChanged(); break;
        case 3: _t->audioSettingsChanged(); break;
        case 4: _t->videoSettingsChanged(); break;
        case 5: _t->updateSetting((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[3]))); break;
        case 6: _t->applyChanges(); break;
        case 7: _t->reset(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SettingsViewModel::*)();
            if (_t _q_method = &SettingsViewModel::generalSettingsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SettingsViewModel::*)();
            if (_t _q_method = &SettingsViewModel::streamSettingsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SettingsViewModel::*)();
            if (_t _q_method = &SettingsViewModel::outputSettingsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SettingsViewModel::*)();
            if (_t _q_method = &SettingsViewModel::audioSettingsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SettingsViewModel::*)();
            if (_t _q_method = &SettingsViewModel::videoSettingsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<SettingsViewModel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QVariantMap*>(_v) = _t->generalSettings(); break;
        case 1: *reinterpret_cast< QVariantMap*>(_v) = _t->streamSettings(); break;
        case 2: *reinterpret_cast< QVariantMap*>(_v) = _t->outputSettings(); break;
        case 3: *reinterpret_cast< QVariantMap*>(_v) = _t->audioSettings(); break;
        case 4: *reinterpret_cast< QVariantMap*>(_v) = _t->videoSettings(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *SettingsViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SettingsViewModel.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SettingsViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SettingsViewModel::generalSettingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SettingsViewModel::streamSettingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SettingsViewModel::outputSettingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SettingsViewModel::audioSettingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SettingsViewModel::videoSettingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
