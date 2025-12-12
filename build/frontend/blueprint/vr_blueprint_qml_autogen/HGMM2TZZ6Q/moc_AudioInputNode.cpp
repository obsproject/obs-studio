/****************************************************************************
** Meta object code from reading C++ file 'AudioInputNode.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../frontend/blueprint/nodes/AudioInputNode.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AudioInputNode.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_NeuralStudio__AudioInputNode_t {
    uint offsetsAndSizes[18];
    char stringdata0[29];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[23];
    char stringdata4[18];
    char stringdata5[14];
    char stringdata6[7];
    char stringdata7[19];
    char stringdata8[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_NeuralStudio__AudioInputNode_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_NeuralStudio__AudioInputNode_t qt_meta_stringdata_NeuralStudio__AudioInputNode = {
    {
        QT_MOC_LITERAL(0, 28),  // "NeuralStudio::AudioInputNode"
        QT_MOC_LITERAL(29, 13),  // "sourceChanged"
        QT_MOC_LITERAL(43, 0),  // ""
        QT_MOC_LITERAL(44, 22),  // "audioPropertiesChanged"
        QT_MOC_LITERAL(67, 17),  // "position3DChanged"
        QT_MOC_LITERAL(85, 13),  // "volumeChanged"
        QT_MOC_LITERAL(99, 6),  // "volume"
        QT_MOC_LITERAL(106, 18),  // "activeStateChanged"
        QT_MOC_LITERAL(125, 6)   // "active"
    },
    "NeuralStudio::AudioInputNode",
    "sourceChanged",
    "",
    "audioPropertiesChanged",
    "position3DChanged",
    "volumeChanged",
    "volume",
    "activeStateChanged",
    "active"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_NeuralStudio__AudioInputNode[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    1 /* Public */,
       3,    0,   45,    2, 0x06,    2 /* Public */,
       4,    0,   46,    2, 0x06,    3 /* Public */,
       5,    1,   47,    2, 0x06,    4 /* Public */,
       7,    1,   50,    2, 0x06,    6 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Float,    6,
    QMetaType::Void, QMetaType::Bool,    8,

       0        // eod
};

Q_CONSTINIT const QMetaObject NeuralStudio::AudioInputNode::staticMetaObject = { {
    QMetaObject::SuperData::link<NodeItem::staticMetaObject>(),
    qt_meta_stringdata_NeuralStudio__AudioInputNode.offsetsAndSizes,
    qt_meta_data_NeuralStudio__AudioInputNode,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_NeuralStudio__AudioInputNode_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AudioInputNode, std::true_type>,
        // method 'sourceChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'audioPropertiesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'position3DChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'volumeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        // method 'activeStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void NeuralStudio::AudioInputNode::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AudioInputNode *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sourceChanged(); break;
        case 1: _t->audioPropertiesChanged(); break;
        case 2: _t->position3DChanged(); break;
        case 3: _t->volumeChanged((*reinterpret_cast< std::add_pointer_t<float>>(_a[1]))); break;
        case 4: _t->activeStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AudioInputNode::*)();
            if (_t _q_method = &AudioInputNode::sourceChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AudioInputNode::*)();
            if (_t _q_method = &AudioInputNode::audioPropertiesChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AudioInputNode::*)();
            if (_t _q_method = &AudioInputNode::position3DChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AudioInputNode::*)(float );
            if (_t _q_method = &AudioInputNode::volumeChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (AudioInputNode::*)(bool );
            if (_t _q_method = &AudioInputNode::activeStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *NeuralStudio::AudioInputNode::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NeuralStudio::AudioInputNode::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NeuralStudio__AudioInputNode.stringdata0))
        return static_cast<void*>(this);
    return NodeItem::qt_metacast(_clname);
}

int NeuralStudio::AudioInputNode::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NodeItem::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void NeuralStudio::AudioInputNode::sourceChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NeuralStudio::AudioInputNode::audioPropertiesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NeuralStudio::AudioInputNode::position3DChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void NeuralStudio::AudioInputNode::volumeChanged(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void NeuralStudio::AudioInputNode::activeStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
