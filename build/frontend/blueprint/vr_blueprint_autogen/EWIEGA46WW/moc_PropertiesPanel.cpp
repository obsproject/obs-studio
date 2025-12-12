/****************************************************************************
** Meta object code from reading C++ file 'PropertiesPanel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../frontend/blueprint/PropertiesPanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PropertiesPanel.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_PropertiesPanel_t {
    uint offsetsAndSizes[16];
    char stringdata0[16];
    char stringdata1[25];
    char stringdata2[1];
    char stringdata3[25];
    char stringdata4[22];
    char stringdata5[20];
    char stringdata6[6];
    char stringdata7[18];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_PropertiesPanel_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_PropertiesPanel_t qt_meta_stringdata_PropertiesPanel = {
    {
        QT_MOC_LITERAL(0, 15),  // "PropertiesPanel"
        QT_MOC_LITERAL(16, 24),  // "onSpatialPositionChanged"
        QT_MOC_LITERAL(41, 0),  // ""
        QT_MOC_LITERAL(42, 24),  // "onSpatialRotationChanged"
        QT_MOC_LITERAL(67, 21),  // "onSpatialScaleChanged"
        QT_MOC_LITERAL(89, 19),  // "onStereoModeChanged"
        QT_MOC_LITERAL(109, 5),  // "index"
        QT_MOC_LITERAL(115, 17)   // "onLensTypeChanged"
    },
    "PropertiesPanel",
    "onSpatialPositionChanged",
    "",
    "onSpatialRotationChanged",
    "onSpatialScaleChanged",
    "onStereoModeChanged",
    "index",
    "onLensTypeChanged"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_PropertiesPanel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x08,    1 /* Private */,
       3,    0,   45,    2, 0x08,    2 /* Private */,
       4,    0,   46,    2, 0x08,    3 /* Private */,
       5,    1,   47,    2, 0x08,    4 /* Private */,
       7,    1,   50,    2, 0x08,    6 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Int,    6,

       0        // eod
};

Q_CONSTINIT const QMetaObject PropertiesPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_PropertiesPanel.offsetsAndSizes,
    qt_meta_data_PropertiesPanel,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_PropertiesPanel_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PropertiesPanel, std::true_type>,
        // method 'onSpatialPositionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSpatialRotationChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSpatialScaleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onStereoModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onLensTypeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void PropertiesPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PropertiesPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onSpatialPositionChanged(); break;
        case 1: _t->onSpatialRotationChanged(); break;
        case 2: _t->onSpatialScaleChanged(); break;
        case 3: _t->onStereoModeChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->onLensTypeChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *PropertiesPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PropertiesPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PropertiesPanel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int PropertiesPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
