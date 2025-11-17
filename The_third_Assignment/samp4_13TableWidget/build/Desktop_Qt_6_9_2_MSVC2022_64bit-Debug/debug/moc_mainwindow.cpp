/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "on_btnSetHeader_clicked",
        "",
        "on_btnSetRows_clicked",
        "on_btnIniData_clicked",
        "on_chkBoxTabEditable_clicked",
        "checked",
        "on_chkBoxHeaderH_clicked",
        "on_chkBoxHeaderV_clicked",
        "on_chkBoxRowColor_clicked",
        "on_rBtnSelectItem_clicked",
        "on_rBtnSelectRow_clicked",
        "on_btnReadToEdit_clicked",
        "on_tableInfo_currentCellChanged",
        "currentRow",
        "currentColumn",
        "previousRow",
        "previousColumn",
        "on_btnInsertRow_clicked",
        "on_btnAppendRow_clicked",
        "on_btnDelCurRow_clicked",
        "on_btnAutoHeght_clicked",
        "on_btnAutoWidth_clicked",
        "on_action_triggered",
        "on_actSetStudentList_triggered"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_btnSetHeader_clicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_btnSetRows_clicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_btnIniData_clicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_chkBoxTabEditable_clicked'
        QtMocHelpers::SlotData<void(bool)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Slot 'on_chkBoxHeaderH_clicked'
        QtMocHelpers::SlotData<void(bool)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Slot 'on_chkBoxHeaderV_clicked'
        QtMocHelpers::SlotData<void(bool)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Slot 'on_chkBoxRowColor_clicked'
        QtMocHelpers::SlotData<void(bool)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Slot 'on_rBtnSelectItem_clicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_rBtnSelectRow_clicked'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_btnReadToEdit_clicked'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_tableInfo_currentCellChanged'
        QtMocHelpers::SlotData<void(int, int, int, int)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 14 }, { QMetaType::Int, 15 }, { QMetaType::Int, 16 }, { QMetaType::Int, 17 },
        }}),
        // Slot 'on_btnInsertRow_clicked'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_btnAppendRow_clicked'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_btnDelCurRow_clicked'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_btnAutoHeght_clicked'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_btnAutoWidth_clicked'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_action_triggered'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actSetStudentList_triggered'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_btnSetHeader_clicked(); break;
        case 1: _t->on_btnSetRows_clicked(); break;
        case 2: _t->on_btnIniData_clicked(); break;
        case 3: _t->on_chkBoxTabEditable_clicked((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 4: _t->on_chkBoxHeaderH_clicked((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 5: _t->on_chkBoxHeaderV_clicked((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->on_chkBoxRowColor_clicked((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->on_rBtnSelectItem_clicked(); break;
        case 8: _t->on_rBtnSelectRow_clicked(); break;
        case 9: _t->on_btnReadToEdit_clicked(); break;
        case 10: _t->on_tableInfo_currentCellChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 11: _t->on_btnInsertRow_clicked(); break;
        case 12: _t->on_btnAppendRow_clicked(); break;
        case 13: _t->on_btnDelCurRow_clicked(); break;
        case 14: _t->on_btnAutoHeght_clicked(); break;
        case 15: _t->on_btnAutoWidth_clicked(); break;
        case 16: _t->on_action_triggered(); break;
        case 17: _t->on_actSetStudentList_triggered(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 18;
    }
    return _id;
}
QT_WARNING_POP
