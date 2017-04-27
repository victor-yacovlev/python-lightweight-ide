#include "pyutils.h"

#include <iostream>

#ifdef PYTHON_SCRIPT_DEBUG
#include <QMessageBox>
#endif

namespace Python3Language {

extern QString PyUnicodeToQString(PyObject * unicode)
{
    Q_ASSERT(PyUnicode_Check(unicode));
    wchar_t * buffer = PyUnicode_AsWideCharString(unicode, 0);
    QString result = QString::fromWCharArray(buffer);
    PyMem_Free(buffer);
    return result;
}

extern QString PyObjectToQString(PyObject * o)
{
    PyObject* repr = PyObject_Repr(o);
    if (!repr) {
        repr = PyObject_Str(o);
    }
    return repr? PyUnicodeToQString(repr) : QString();
}

extern PyObject* QStringToPyUnicode(const QString & qstring)
{
    std::wstring wstring = qstring.toStdWString();
    PyObject * result = PyUnicode_FromWideChar(wstring.c_str(), -1);
    Q_ASSERT(PyUnicode_Check(result));
    return result;
}

extern PythonError fetchPythonErrorAsString()
{
    PythonError result;
    PyObject * ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    if (pvalue) {
        PyObject * pvalueRepr = PyObject_Repr(pvalue);
        PyObject * ptracebackRepr = PyObject_Repr(ptraceback);
        QString qvalue = PyUnicodeToQString(pvalueRepr);
        std::cerr << qvalue.toLocal8Bit().constData() << std::endl;
        PyTraceBack_Print(ptraceback, PyFile_NewStdPrinter(fileno(stderr)));
        QString qtraceback = PyUnicodeToQString(ptracebackRepr);
        result.value = qvalue;
        result.traceback = qtraceback;
    }
    return result;
}

extern void printPythonTraceback()
{
    const PythonError pyError = fetchPythonErrorAsString();

#ifdef PYTHON_SCRIPT_DEBUG
    QString message = QString("Error: %1\nTraceback:\n%2").arg(pyError.value).arg(pyError.traceback);
    QMessageBox::warning(0, "Python error", message);
#else
    qWarning() << pyError.value;
    qWarning() << pyError.traceback;
#endif
}

extern void printError(const QString & message)
{
#ifdef PYTHON_SCRIPT_DEBUG
    QMessageBox::warning(0, "Python error", message);
#else
    qWarning() << message;
#endif
}

QMap<QString,QVariant> PyDictToPropertyMap(PyObject *object)
{
    QMap<QString,QVariant> result;
    PyObject * py_keys = PyDict_Keys(object);
    size_t size = PyList_Size(py_keys);
    for (size_t i=0; i<size; i++) {
        PyObject * py_key = PyList_GetItem(py_keys, i);
        PyObject * py_value = PyDict_GetItem(object, py_key);
        PyObject * py_key_repr = PyObject_Repr(py_key);
        const QString key = PyUnicodeToQString(py_key_repr);
        const QVariant value = PyObjectToQVariant(py_value);
        result[key] = value;
        Py_DECREF(py_key_repr);
        Py_DECREF(py_key);
        Py_DECREF(py_value);
    }
    Py_DECREF(py_keys);
    return result;
}

extern QMap<QString,QVariant> PyObjectToPropertyMap(PyObject *object)
{
    PyObject * __dict__ = 0;
    if (PyDict_Check(object))
        __dict__ = object;
    else
        __dict__ = PyObject_GenericGetDict(object, 0);
    Q_ASSERT(PyDict_Check(__dict__));
    PyObject * keys = PyDict_Keys(__dict__);
    size_t count = PyObject_Length(keys);
    QMap<QString,QVariant> result;
    for (size_t i=0; i<count; i++) {
        PyObject * py_key = PyList_GetItem(keys, i);
        const QString key = PyUnicodeToQString(py_key);
        if (!key.startsWith("_")) {
            PyObject * py_value = PyDict_GetItem(__dict__, py_key);
            const QVariant value = PyObjectToQVariant(py_value);
            result[key] = value;
        }
    }    
    return result;
}

extern ValueRepresentation PyObjectToValueRepresentation(const QString & name, PyObject *object)
{
    ValueRepresentation result;
    result.name = name;
    if (name.startsWith('_')) {
        return result;
    }
    const PyTypeObject* const objectType = object->ob_type;
    const QByteArray objectTypeName(objectType->tp_name);
    if (objectTypeName == "NoneType") {
        result.repr = "None";
        return result;
    }
    if (!name.isEmpty()) {
        PyObject * repr = PyObject_Repr(object);
        if (repr) {
            result.repr = PyUnicodeToQString(repr);
        }
    }
    if (PyList_Check(object)) {
        size_t count = PyList_Size(object);
        for (size_t i=0; i<count; i++) {
            PyObject* child = PyList_GetItem(object, i);
            QString childName = QString("%1[%2]").arg(name).arg(i);
            result.children.append(
                        PyObjectToValueRepresentation(
                            childName, child
                            )
                        );
        }
    }
    else if (PyTuple_Check(object)) {
        size_t count = PyTuple_Size(object);
        for (size_t i=0; i<count; i++) {
            PyObject* child = PyTuple_GetItem(object, i);
            QString childName = QString("%1[%2]").arg(name).arg(i);
            result.children.append(
                        PyObjectToValueRepresentation(
                            childName, child
                            )
                        );
        }
    }
    else if (PyDict_Check(object)) {
        PyObject* keys = PyDict_Keys(object);
        size_t count = PyList_Size(keys);
        for (size_t i=0; i<count; i++) {
            PyObject * key = PyList_GetItem(keys, i);
            if (name.isEmpty()) {
                // Table value
                QString keyName = PyUnicodeToQString(key);
                if (!keyName.startsWith("_")) {
                    PyObject * value = PyDict_GetItem(object, key);
                    result.children.append(
                                PyObjectToValueRepresentation(
                                    keyName, value
                                    )
                                );
                }
            }
            else {
                // Pure dict value
                PyObject * keyRepr = PyObject_Repr(key);
                QString qKeyRepr = PyUnicodeToQString(keyRepr);
                QString childName = QString("%1[%2]").arg(name).arg(qKeyRepr);
                PyObject * value = PyDict_GetItem(object, key);
                result.children.append(
                            PyObjectToValueRepresentation(
                                childName, value
                                )
                            );
            }
        }
    }
    else {
        // Inspect top-level module
        if ("module"==objectTypeName && !name.contains('.') && !name.contains('[')) {
            PyObject * dict = PyObject_GenericGetDict(object, 0);
            PyObject* keys = PyDict_Keys(dict);
            size_t count = PyList_Size(keys);
            for (size_t i=0; i<count; i++) {
                PyObject * key = PyList_GetItem(keys, i);
                QString keyName = PyUnicodeToQString(key);
                if (!keyName.startsWith("_")) {
                    PyObject * value = PyDict_GetItem(dict, key);
                    result.children.append(
                                PyObjectToValueRepresentation(
                                    name + "." + keyName, value
                                    )
                                );
                }
            }
        }
    }
    return result;
}

extern QVariant PyObjectToQVariant(PyObject *object)
{
    if (!object)
        return QVariant::Invalid;
    const PyTypeObject* const objectType = object->ob_type;
    const QByteArray objectTypeName(objectType->tp_name);
    if (objectTypeName == "NoneType") {
        return QVariant();
    }
    if (PyTuple_Check(object) || PyList_Check(object))
        return PyListToQVariantList(object);
    else if (PyLong_Check(object) && PyLong_AsLong(object) >= 0)
        return QVariant::fromValue(PyLong_AsUnsignedLong(object));
    else if (PyLong_Check(object))
        return QVariant::fromValue(PyLong_AsLong(object));
    else if (PyFloat_Check(object))
        return QVariant(PyFloat_AsDouble(object));
    else if (PyBool_Check(object))
        return QVariant(PyLong_AsLong(object) != 0);
    else if (PyUnicode_Check(object))
        return QVariant(PyUnicodeToQString(object));
    else if (PyDict_Check(object))
        return QVariant(PyDictToPropertyMap(object));
    else {
        QMap<QString,QVariant> propertyMap = PyObjectToPropertyMap(object);
        if (propertyMap.isEmpty() || (propertyMap.size()==1 && propertyMap.keys().at(0)=="__name__")) {
            PyObject* __repr__ = PyObject_Repr(object);
            QString repr = PyUnicodeToQString(__repr__);
            Py_DecRef(__repr__);
            return QVariant(repr);
        }
        else {
            propertyMap["__type__.__name__"] = objectTypeName;
            return QVariant(propertyMap);
        }
    }
}

extern PyObject* QVariantToPyObject(const QVariant & value)
{
    PyObject* result = 0;
    if (
            QVariant::Int==value.type() ||
            QVariant::LongLong==value.type() ||
            QByteArray(value.typeName())=="int" ||
            QByteArray(value.typeName())=="long"
            )
    {
        result = PyLong_FromLong(value.toInt());
    }
    else if (
             QVariant::UInt==value.type() ||
             QVariant::ULongLong==value.type() ||
             QByteArray(value.typeName())=="uint" ||
             QByteArray(value.typeName())=="ulong"
             )
    {
        result = PyLong_FromUnsignedLong(value.toUInt());
    }
    else if (QVariant::Double==value.type())
        result = PyFloat_FromDouble(value.toDouble());
    else if (QVariant::Bool==value.type())
        result = value.toBool() ? Py_True : Py_False;
    else if (QVariant::Invalid==value.type())
        result = Py_None;
    else if (QVariant::Char==value.type() || QVariant::String==value.type())
        result = QStringToPyUnicode(value.toString());
    else if (QVariant::List==value.type())
        result = QVariantListToPyList(value.toList(), false);
    else if (QVariant::Map==value.type() &&
             value.toMap().contains("__type__.__name__") &&
             value.toMap().contains("__type__.__module__.__name__")
             )
    {
        const QMap<QString,QVariant> map = value.toMap();
        const QString & moduleName = map["__type__.__module__.__name__"].toString();
        const QString & className = map["__type__.__name__"].toString();
        PyObject * module = findCreatedModule(moduleName);
        const QString initInstanceName = "__init__" + className;
        PyObject * py_initInstanceName = QStringToPyUnicode(initInstanceName);
        PyObject * py_initInstance = PyObject_GetAttr(module, py_initInstanceName);
        Py_DECREF(py_initInstanceName);
        QVariantList initArgs;
        Q_FOREACH(const QString & key, map.keys()) {
            if (!key.startsWith("_")) {
                const QVariant arg = map[key];
                initArgs.append(arg);
            }
        }
        PyObject * py_initArgs = QVariantListToPyList(initArgs, true);
        PyObject * py_instance = PyObject_Call(py_initInstance, py_initArgs, 0);
        Py_DECREF(py_initArgs);
        result = py_instance;

    }
    // TODO support more types?
    Py_XINCREF(result);
    return result;
}

extern QVariantList PyListToQVariantList(PyObject *list)
{
    Q_ASSERT(PyTuple_Check(list) || PyList_Check(list));
    bool isTuple = PyTuple_Check(list);
    QVariantList result;
    size_t size = PyObject_Length(list);
    for (size_t i=0; i<size; i++) {
        PyObject * py_item = 0;
        if (isTuple)
            py_item = PyTuple_GetItem(list, i);
        else
            py_item = PyList_GetItem(list, i);
        result.append(PyObjectToQVariant(py_item));
    }
    return result;
}

extern PyObject* QVariantListToPyList(const QVariantList &list, bool toTuple)
{
    PyObject * result = 0;
    if (toTuple)
        result = PyTuple_New(list.size());
    else
        result = PyList_New(list.size());
    for (size_t i=0; i<list.size(); i++) {
        const QVariant & item = list.at(i);
        PyObject * py_item = QVariantToPyObject(item);
        if (toTuple)
            PyTuple_SetItem(result, i, py_item);
        else
            PyList_SetItem(result, i, py_item);
    }
    return result;
}

extern QVariant callPythonFunction(
        PyThreadState * interpreter,
        PyObject * callable,
        const QVariantList & args
        )
{
    Q_ASSERT(PyCallable_Check(callable));
    PyThreadState* prevState = PyThreadState_Swap(interpreter);
    PyGILState_STATE prevGIL = PyGILState_Ensure();
//    PyEval_AcquireThread(interpreter);
    PyObject * py_args = QVariantListToPyList(args, true);

    // Erase last occured error from previous call
    PythonError lastError = fetchPythonErrorAsString();
    PyErr_Clear();

    PyObject * py_result = PyObject_CallObject(callable, py_args);
    QVariant result = PyObjectToQVariant(py_result);
//    PyEval_ReleaseThread(interpreter);
    PyGILState_Release(prevGIL);
    PyThreadState_Swap(prevState);
    return result;
}

extern QVariant callPythonFunction(PyThreadState * interpreter, PyObject * callable)
{
    return callPythonFunction(interpreter, callable, QVariantList());
}

extern QVariant callPythonFunction(
        PyThreadState * interpreter,
        PyObject * callable,
        const QVariant & arg1
        )
{
    return callPythonFunction(interpreter, callable, QVariantList() << arg1);
}

extern QVariant callPythonFunction(
        PyThreadState * interpreter,
        PyObject * callable,
        const QVariant & arg1,
        const QVariant & arg2
        )
{
    return callPythonFunction(interpreter, callable, QVariantList() << arg1 << arg2);
}

extern void appendToSysPath(const QString & path)
{
    QString localPath = QDir::toNativeSeparators(path);
    PyObject * sysPath = PySys_GetObject("path");
    PyObject * extraPath = QStringToPyUnicode(localPath);
    PyList_Insert(sysPath, 0, extraPath);
}

#ifdef Q_OS_WIN32
extern void prepareBundledSysPath()
{
    static const QString KumirRoot =
            QDir::toNativeSeparators(
                QDir::cleanPath(
                    QCoreApplication::applicationDirPath() + "/../"
                    )
                );
    static const QStringList PathItems = QStringList()
            << KumirRoot + "\\share\\kumir2\\python3language"
            << KumirRoot + "\\python\\Lib"
            << KumirRoot + "\\python\\DLLs"
            << KumirRoot + "\\python\\Lib\\site-packages"
               ;    
    QDir siteRoot(QDir::fromNativeSeparators(KumirRoot)+"/python/Lib/site-packages");
    QStringList eggs = siteRoot.entryList(QStringList() << "*.egg", QDir::Dirs);
    PyObject * sysPath = PyList_New(PathItems.size()+eggs.size());
    for (int i=0; i<PathItems.size(); i++) {
        PyObject * pyItem = QStringToPyUnicode(PathItems[i]);
        PyList_SetItem(sysPath, i, pyItem);
    }
    for (int i=0; i<eggs.size(); i++) {
        const QString eggPath = KumirRoot + "\\python\\Lib\\site-packages\\" + eggs[i];
        PyObject * pyItem = QStringToPyUnicode(eggPath);
        int index = PathItems.size() + i;
        PyList_SetItem(sysPath, index, pyItem);
    }
    PySys_SetObject("path", sysPath);
}
#endif

extern PyObject* compileModule(
        const QString &fileName,
        const QString &source,
        int * errorLineNumber,
        QString * errorText,
        int flags
        )
{
    const std::string cfileName(fileName.toUtf8().constData());
    const std::string csource(source.toUtf8().constData());
    PyObject* result = Py_CompileString(csource.c_str(), cfileName.c_str(), flags);
    if (!result && errorLineNumber && errorText) {
        PyObject * ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyObject * pstr = PyObject_Str(pvalue);
        *errorText = PyUnicodeToQString(pstr);
    }
    return result;
}

static QMap<QString,PyObject*> CreatedModules;

extern void clearCreatedModules()
{
    Q_FOREACH(PyObject * obj, CreatedModules.values()) {
        Py_XDECREF(obj);
    }
    CreatedModules.clear();
}

extern PyObject* createModuleFromSource(
        PyThreadState *interpreter,
        const QString &moduleName,
        const QString &moduleSource
        )
{
    const std::string cname = moduleName.toStdString();
    PyObject* module = 0;
    PyEval_AcquireThread(interpreter);
    PyObject* code = compileModule("<generated>", moduleSource);
    if (!code) {
        qDebug() << "Error creating " << moduleName;
        printPythonTraceback();
        qDebug() << moduleSource;
    }
    else {
        module = PyImport_ExecCodeModule(const_cast<char*>(cname.c_str()), code);
        if (!module) {
            printPythonTraceback();
        }
    }
    PyEval_ReleaseThread(interpreter);
    Py_XINCREF(module);
    CreatedModules[moduleName] = module;
    return module;
}

extern PyObject* findCreatedModule(const QString &name)
{
    if (CreatedModules.contains(name)) {
        return CreatedModules[name];
    }
    else {
        return 0;
    }
}

void createSysArgv(const QStringList &arguments)
{
    PyObject * py_args = PyList_New(arguments.size());
    for (ssize_t i=0; i<arguments.size(); ++i) {
        PyObject * py_arg = QStringToPyUnicode(arguments.at(i));
        PyList_SetItem(py_args, i, py_arg);
    }
    PySys_SetObject("argv", py_args);
}



}

