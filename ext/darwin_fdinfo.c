#include <python2.7/Python.h>
#include <libproc.h>
#include <stdlib.h>
#include <unistd.h>

static PyObject *
fdinfo_get_open_fds(PyObject *self, PyObject *args)
{
    int pid = -1;

    if (!PyArg_ParseTuple(args, "|i", &pid))
        return NULL;

    if (pid == -1) {
        pid = getpid();
    }

    // Figure out the size of the buffer needed to hold the list of open FDs
    int bufferSize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, 0, 0);
    if (bufferSize == -1) {
        PyErr_SetString(PyExc_OSError, "Unable to get open file handles for fd");
        return NULL;
    }

    // Get the list of open FDs
    struct proc_fdinfo *procFDInfo = (struct proc_fdinfo *)malloc(bufferSize);
    if (!procFDInfo) {
        PyErr_SetString(PyExc_MemoryError, "Error allocating memory for fdinfo buffer");
        return NULL;
    }

    int bufferUsed = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, procFDInfo, bufferSize);
    int numberOfProcFDs = bufferUsed / PROC_PIDLISTFD_SIZE;

    PyObject *result = Py_BuildValue("[]");
    if (!result) {
        goto cleanup_error;
    }

    int i;
    for(i = 0; i < numberOfProcFDs; i++) {
        //printf("FD #%d: %d\n", procFDInfo[i].proc_fd, procFDInfo[i].proc_fdtype);
        PyObject *iPid = Py_BuildValue("i", procFDInfo[i].proc_fd);
        if (!iPid) {
            goto cleanup_error;
        }
        int append_result = PyList_Append(result, iPid);
        Py_DECREF(iPid);
        if (append_result == -1) {
            goto cleanup_error;
        }
    }

    free(procFDInfo);
    return result;

cleanup_error:
    Py_XDECREF(result);
    free(procFDInfo);
    return NULL;
}


static PyMethodDef FDInfoMethods[] = {
    {"get_open_fds",  fdinfo_get_open_fds, METH_VARARGS,
      "Returns the open FDs for the given ppid."
    },
    {NULL, NULL, 0, NULL}        /* Sentinel */
};



PyMODINIT_FUNC
init_fdinfo(void)
{
    PyObject *m;

    m = Py_InitModule("_fdinfo", FDInfoMethods);
    if (m == NULL)
        return;
}