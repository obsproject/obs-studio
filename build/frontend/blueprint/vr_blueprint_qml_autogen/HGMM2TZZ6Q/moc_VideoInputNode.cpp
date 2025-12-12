/****************************************************************************
** Meta object code from reading C++ file 'VideoInputNode.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../frontend/blueprint/nodes/VideoInputNode.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'VideoInputNode.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_NeuralStudio__VideoInputNode_t {
    uint offsetsAndSizes[14];
    char stringdata0[29];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[23];
    char stringdata4[18];
    char stringdata5[19];
    char stringdata6[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_NeuralStudio__VideoInputNode_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_NeuralStudio__VideoInputNode_t qt_meta_stringdata_NeuralStudio__VideoInputNode = {
    {
        QT_MOC_LITERAL(0, 28),  // "NeuralStudio::VideoInputNode"
        QT_MOC_LITERAL(29, 13),  // "sourceChanged"
        QT_MOC_LITERAL(43, 0),  // ""
        QT_MOC_LITERAL(44, 22),  // "videoPropertiesChanged"
        QT_MOC_LITERAL(67, 17),  // "position3DChanged"
        QT_MOC_LITERAL(85, 18),  // "activeStateChanged"
        QT_MOC_LITERAL(104, 6)   // "active"
    },
    "NeuralStudio::VideoInputNode",
    "sourceChanged",
    "",
    "videoPropertiesChanged",
    "position3DChanged",
    "activeStateChanged",
    "active"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_NeuralStudio__VideoInputNode[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    1 /* Public */,
       3,    0,   39,    2, 0x06,    2 /* Public */,
       4,    0,   40,    2, 0x06,    3 /* Public */,
       5,    1,   41,    2, 0x06,    4 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    6,

       0        // eod
};

Q_CONSTINIT const QMetaObject NeuralStudio::VideoInputNode::staticMetaObject = { {
    QMetaObject::SuperData::link<NodeItem::staticMetaObject>(),
    qt_meta_stringdata_NeuralStudio__VideoInputNode.offsetsAndSizes,
    qt_meta_data_NeuralStudio__VideoInputNode,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_NeuralStudio__VideoInputNode_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<VideoInputNode, std::true_type>,
        // method 'sourceChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'videoPropertiesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'position3DChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'activeStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void NeuralStudio::VideoInputNode::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VideoInputNode *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sourceChanged(); break;
        case 1: _t->videoPropertiesChanged(); break;
        case 2: _t->position3DChanged(); break;
        case 3: _t->activeStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (VideoInputNode::*)();
            if (_t _q_method = &VideoInputNode::sourceChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (VideoInputNode::*)();
            if (_t _q_method = &VideoInputNode::videoPropertiesChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (VideoInputNode::*)();
            if (_t _q_method = &VideoInputNode::position3DChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (VideoInputNode::*)(bool );
            if (_t _q_method = &VideoInputNode::activeStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *NeuralStudio::VideoInputNode::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NeuralStudio::VideoInputNode::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NeuralStudio__VideoInputNode.stringdata0))
        return static_cast<void*>(this);
    return NodeItem::qt_metacast(_clname);
}

int NeuralStudio::VideoInputNode::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = NodeItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void NeuralStudio::VideoInputNode::sourceChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NeuralStudio::VideoInputNode::videoPropertiesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NeuralStudio::VideoInputNode::position3DChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void NeuralStudio::VideoInputNode::activeStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
