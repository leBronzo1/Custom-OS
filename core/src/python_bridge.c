#include "python_bridge.h"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PyObject *gModule = NULL;
static PyObject *gFunction = NULL;

int PythonBridge_Init(void) {
    Py_Initialize();

    /* Add current directory to Python path */
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('.')");

    gModule = PyImport_ImportModule("system_monitor");

    if (!gModule) {
        PyErr_Print();
        return 0;
    }

    gFunction = PyObject_GetAttrString(gModule, "system_info_json");

    if (!gFunction || !PyCallable_Check(gFunction)) {
        PyErr_Print();
        return 0;
    }

    return 1;
}

char *PythonBridge_GetSystemInfoJSON(void) {
    PyObject *result = PyObject_CallObject(gFunction, NULL);

    if (!result) {
        PyErr_Print();
        return NULL;
    }

    const char *json = PyUnicode_AsUTF8(result);

    if (!json) {
        Py_DECREF(result);
        return NULL;
    }

    char *copy = malloc(strlen(json) + 1);

    if (copy)
        strcpy(copy, json);

    Py_DECREF(result);

    return copy;
}

void PythonBridge_Shutdown(void) {
    Py_XDECREF(gFunction);
    Py_XDECREF(gModule);

    Py_Finalize();
}