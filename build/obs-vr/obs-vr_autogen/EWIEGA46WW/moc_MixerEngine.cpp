/****************************************************************************
** Meta object code from reading C++ file 'MixerEngine.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../obs-vr/MixerEngine.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MixerEngine.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_NeuralStudio__MixerEngine_t {
    uint offsetsAndSizes[22];
    char stringdata0[26];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[18];
    char stringdata4[7];
    char stringdata5[11];
    char stringdata6[7];
    char stringdata7[17];
    char stringdata8[11];
    char stringdata9[8];
    char stringdata10[8];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_NeuralStudio__MixerEngine_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_NeuralStudio__MixerEngine_t qt_meta_stringdata_NeuralStudio__MixerEngine = {
    {
        QT_MOC_LITERAL(0, 25),  // "NeuralStudio::MixerEngine"
        QT_MOC_LITERAL(26, 13),  // "layoutChanged"
        QT_MOC_LITERAL(40, 0),  // ""
        QT_MOC_LITERAL(41, 17),  // "activeFeedChanged"
        QT_MOC_LITERAL(59, 6),  // "feedId"
        QT_MOC_LITERAL(66, 10),  // "LayoutType"
        QT_MOC_LITERAL(77, 6),  // "Single"
        QT_MOC_LITERAL(84, 16),  // "PictureInPicture"
        QT_MOC_LITERAL(101, 10),  // "SideBySide"
        QT_MOC_LITERAL(112, 7),  // "Grid2x2"
        QT_MOC_LITERAL(120, 7)   // "Grid3x3"
    },
    "NeuralStudio::MixerEngine",
    "layoutChanged",
    "",
    "activeFeedChanged",
    "feedId",
    "LayoutType",
    "Single",
    "PictureInPicture",
    "SideBySide",
    "Grid2x2",
    "Grid3x3"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_NeuralStudio__MixerEngine[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       1,   30, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   26,    2, 0x06,    1 /* Public */,
       3,    1,   27,    2, 0x06,    2 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,

 // enums: name, alias, flags, count, data
       5,    5, 0x0,    5,   35,

 // enum data: key, value
       6, uint(NeuralStudio::MixerEngine::Single),
       7, uint(NeuralStudio::MixerEngine::PictureInPicture),
       8, uint(NeuralStudio::MixerEngine::SideBySide),
       9, uint(NeuralStudio::MixerEngine::Grid2x2),
      10, uint(NeuralStudio::MixerEngine::Grid3x3),

       0        // eod
};

Q_CONSTINIT const QMetaObject NeuralStudio::MixerEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_NeuralStudio__MixerEngine.offsetsAndSizes,
    qt_meta_data_NeuralStudio__MixerEngine,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_NeuralStudio__MixerEngine_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MixerEngine, std::true_type>,
        // method 'layoutChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'activeFeedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void NeuralStudio::MixerEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MixerEngine *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->layoutChanged(); break;
        case 1: _t->activeFeedChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MixerEngine::*)();
            if (_t _q_method = &MixerEngine::layoutChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MixerEngine::*)(const QString & );
            if (_t _q_method = &MixerEngine::activeFeedChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *NeuralStudio::MixerEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NeuralStudio::MixerEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NeuralStudio__MixerEngine.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NeuralStudio::MixerEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void NeuralStudio::MixerEngine::layoutChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NeuralStudio::MixerEngine::activeFeedChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
