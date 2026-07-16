#include "python_bridge.h"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>

#ifdef __APPLE__
    #include <mach-o/dyld.h>
#else
    #include <unistd.h>
#endif

static PyObject *gModule = NULL;
static PyObject *gFunction = NULL;

/*
 * Resolve the directory the running executable lives in. Both
 * add_services_dir_to_syspath() and add_venv_site_packages_to_syspath()
 * need this, so it's factored out into `out` (must be at least
 * PATH_MAX bytes). Returns 1 on success, 0 on failure.
 */
static int get_exe_dir(char *out, size_t out_size) {
    char exe_path[PATH_MAX];

#ifdef __APPLE__
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) != 0) {
        fprintf(stderr, "PythonBridge: could not resolve executable path\n");
        return 0;
    }
#else
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        fprintf(stderr, "PythonBridge: could not resolve executable path\n");
        return 0;
    }
    exe_path[len] = '\0';
#endif

    char resolved[PATH_MAX];
    if (!realpath(exe_path, resolved)) {
        fprintf(stderr, "PythonBridge: realpath failed on '%s'\n", exe_path);
        return 0;
    }

    /* dirname() may modify its argument; resolved is consumed here */
    char *exe_dir = dirname(resolved);

    if (strlen(exe_dir) >= out_size) {
        fprintf(stderr, "PythonBridge: executable path too long\n");
        return 0;
    }

    strcpy(out, exe_dir);
    return 1;
}

/*
 * Appends "<exe_dir>/../services" to sys.path so system_monitor.py
 * (and friends) can be found regardless of the process's CWD.
 */
static void add_services_dir_to_syspath(const char *exe_dir) {
    char cmd[PATH_MAX + 64];
    int n = snprintf(cmd, sizeof(cmd), "import sys; sys.path.append(r'%s/../services')", exe_dir);

    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        fprintf(stderr, "PythonBridge: services path too long\n");
        return;
    }

    PyRun_SimpleString(cmd);
}

/*
 * Appends the project's .venv site-packages directory to sys.path so
 * third-party packages installed there (e.g. psutil) are importable
 * by the embedded interpreter, which otherwise only sees the base/
 * system Python's site-packages regardless of any active venv.
 *
 * IMPORTANT: the .venv MUST be created with the same Python version
 * this binary is linked against (see `python3-config` in the
 * Makefile) - psutil ships compiled C extensions tied to a specific
 * Python ABI/version, so a mismatched venv will still fail to import
 * even once its site-packages directory is on sys.path.
 *
 * The version segment (e.g. "python3.14") is computed here from
 * sys.version_info at runtime rather than hardcoded, so it stays
 * correct even if you rebuild against a different Python version.
 */
static void add_venv_site_packages_to_syspath(const char *exe_dir) {
    char cmd[PATH_MAX + 256];
    int n = snprintf(cmd, sizeof(cmd),
        "import sys, os\n"
        "_v = 'python%%d.%%d' %% (sys.version_info.major, sys.version_info.minor)\n"
        "_p = os.path.join(r'%s', '..', '.venv', 'lib', _v, 'site-packages')\n"
        "sys.path.append(_p)\n",
        exe_dir);

    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        fprintf(stderr, "PythonBridge: venv site-packages path too long\n");
        return;
    }

    PyRun_SimpleString(cmd);
}

int python_bridge_init(void) {
    Py_Initialize();

    PyRun_SimpleString("import sys");

    char exe_dir[PATH_MAX];
    if (get_exe_dir(exe_dir, sizeof(exe_dir))) {
        add_services_dir_to_syspath(exe_dir);
        add_venv_site_packages_to_syspath(exe_dir);
    }

    gModule = PyImport_ImportModule("system_monitor");

    if (!gModule) {
        PyErr_Print();
        return 0;
    }

    gFunction = PyObject_GetAttrString(gModule, "system_info_json");

    if (!gFunction || !PyCallable_Check(gFunction)) {
        PyErr_Print();
        Py_XDECREF(gFunction);
        gFunction = NULL;
        Py_XDECREF(gModule);
        gModule = NULL;
        return 0;
    }

    return 1;
}

char *python_bridge_get_system_info(void) {
    /* Guard against calling this when init() failed or wasn't called */
    if (!gFunction) {
        fprintf(stderr, "PythonBridge: system_info_json unavailable " "(bridge not initialized)\n");
        return NULL;
    }

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

void python_bridge_shutdown(void) {
    Py_XDECREF(gFunction);
    gFunction = NULL;

    Py_XDECREF(gModule);
    gModule = NULL;

    if (Py_IsInitialized())
        Py_Finalize();
}