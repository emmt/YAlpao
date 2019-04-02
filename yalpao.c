/*
 * yalpao.c --
 *
 * Implements Yorick interface to Alpao deformable mirrors.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the YAlpao plugin (https://github.com/emmt/YAlpao)
 * licensed under the MIT license.
 *
 * Copyright (C) 2019: Éric Thiébaut.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <sys/param.h>

#include <asdkWrapper.h>

#include <pstdlib.h>
#include <play.h>
#include <yapi.h>

/* Define some macros to get rid of some GNU extensions when not compiling
   with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#   define __attribute__(x)
#   define __inline__
#   define __FUNCTION__        ""
#   define __PRETTY_FUNCTION__ ""
#endif

#define CLAMP(x, lo, hi) ((x) <= (lo) ? (lo) : ((x) >= (hi) ? (hi) : (x)))

PLUG_API void y_error(const char *) __attribute__ ((noreturn));

typedef struct deformable_mirror_ deformable_mirror_t;
typedef struct parameter_ parameter_t;

/* throw last Alpao SDK error */
static void asdk_error() __attribute__ ((noreturn));

/* get attribute value */
static double asdk_get(deformable_mirror_t* dm, const char* key);

/* set attribute value */
static void asdk_set(deformable_mirror_t* dm, const char* key, double val);

/* set attribute value as a string*/
static void asdk_set_string(deformable_mirror_t* dm, const char* key,
                            const char* str);

/* find entry in table of parameters */
static int find_parameter(const char* key, bool throwerrors);

/* push the value of a parameter on top of Yorick stack */
static void push_parameter(deformable_mirror_t* dm, const char* key);

#if 0
/* push a string on top of Yorick stack */
static void push_string(const char* str);
static void push_string(const char* str)
{
    ypush_q(NULL)[0] = p_strcpy(str);
}
#endif

/*---------------------------------------------------------------------------*/

/* deformable mirror structure */
struct deformable_mirror_ {
    asdkDM*     device; /* handle to device */
    long        number; /* number of actuators */
    const char* serial; /* name of device */
    double*   commands; /* buffer for last commands */
    void*       handle; /* reference to Yorick object with last commands */
};

/* parameter flags */
#define BOOLEAN     1
#define INTEGER     2
#define FLOAT       3
#define STRING      4
#define READABLE   (1 << 4)
#define WRITABLE   (1 << 5)

struct parameter_ {
    const char*   name;
    unsigned int flags;
    const char*  descr;
};

static parameter_t parameters[] = {
    {
        "AckTimeout", FLOAT|READABLE|WRITABLE,
        "For Ethernet / USB interface only, set the time-out (ms); "
        "can be set in synchronous mode only (see SyncMode)."
    }, {
        "DacReset", BOOLEAN|WRITABLE,
        "Reset all digital to analog converters of drive electronics."
    }, {
        "ItfState", INTEGER|READABLE,
        "Return 1 if PCI interface is busy or 0 otherwise."
    }, {
        "LogDump", INTEGER|WRITABLE,
        "Dump the log stack on the standard output."
    }, {
        "LogPrintLevel", INTEGER|READABLE|WRITABLE,
        "Changes the output level of the logger to the standard output."
    }, {
        "NbOfActuator", INTEGER|READABLE,
        "Get the numbers of actuator for that mirror."
    }, {
        "SyncMode", BOOLEAN|WRITABLE,
        "0: Synchronous mode, will return when send is done."
        "1: Asynchronous mode, return immediately after safety checks."
    }, {
        "TriggerMode", BOOLEAN|WRITABLE,
        "Set mode of the (optional) electronics trigger output."
        "0: long pulse width or 1: short pulse width on each command."
    }, {
        "TriggerIn", INTEGER|WRITABLE,
        "Set mode of the (optional) input trigger."
        "0: disabled, 1: trig on rising edge or 2: trig on falling edge."
    }, {
        "UseException", BOOLEAN|READABLE|WRITABLE,
        "Enables or disables the throwing of an exception on error."
     }, {
        "VersionInfo", FLOAT|READABLE,
        "Alpao SDK core version, e.g. 3040500612 is SDK v3.04.05.0612 "
        "where 0612 is build number."
    }, {
        NULL, 0, NULL
    }
};

static int find_parameter(const char* key, bool throwerrors)
{
    int i;
    if (key == NULL || key[0] == '\0') {
        if (throwerrors) {
            y_error("invalid parameter name");
        }
        return -1;
    }
    for (i = 0; parameters[i].name != NULL; ++i) {
        if (strcasecmp(parameters[i].name, key) == 0) {
            return i;
        }
    }
    if (throwerrors) {
        y_error("unknown parameter");
    }
    return -1;
}

static void push_parameter(deformable_mirror_t* dm, const char* key)
{
    double val;
    int i = find_parameter(key, true);
    if ((parameters[i].flags & READABLE) != READABLE) {
        y_error("unreadable parameter");
    }
    switch ((parameters[i].flags & 0x7)) {
    case BOOLEAN:
        val = asdk_get(dm, parameters[i].name);
        ypush_int(val != 0.0);
        return;
    case INTEGER:
        val = asdk_get(dm, parameters[i].name);
        ypush_long(lround(val));
        return;
    case FLOAT:
        val = asdk_get(dm, parameters[i].name);
        ypush_double(val);
        return;
    default:
        y_error("unknown parameter type");
    }
}

static void free_dm(void* addr)
{
    deformable_mirror_t* dm = (deformable_mirror_t*)addr;
    if (dm->device != NULL) {
        asdkDM* tmp = dm->device;
        dm->device = NULL;
        asdkRelease(tmp);
    }
    if (dm->serial != NULL) {
        void* tmp = (void*)dm->serial;
        dm->serial = NULL;
        p_free(tmp);
    }
    if (dm->handle != NULL) {
        void* tmp = dm->handle;
        dm->number = 0;
        dm->handle = NULL;
        dm->commands = NULL;
        ydrop_use(tmp);
    }
}

static void print_dm(void* addr)
{
    deformable_mirror_t* dm = (deformable_mirror_t*)addr;
    char buffer[20];
    if (dm->device == NULL) {
        y_error("unconnected device");
    }
    y_print("alpao_deformable_mirror (", 0);
    sprintf(buffer, "%ld", dm->number);
    y_print(buffer, 0);
    if (dm->serial == NULL) {
        y_print(" actuators, serial: (null))", 1);
    } else {
        y_print(" actuators, serial: \"", 0);
        y_print(dm->serial, 0);
        y_print("\")", 1);
    }
}

static void eval_dm(void* addr, int argc)
{
    deformable_mirror_t* dm = (deformable_mirror_t*)addr;
    int type, subroutine;
    long i, ntot;

    if (dm->device == NULL) {
        y_error("unconnected device");
    }
    subroutine = yarg_subroutine();
    if (subroutine && argc == 0) {
        /* stupid hack */
        fprintf(stdout, "alpao_deformable_mirror (%ld actuators, serial: ",
                dm->number);
        if (dm->serial == NULL) {
            fputs("(null))\n", stdout);
        } else {
            fprintf(stdout, "\"%s\")\n", dm->serial);
        }
        return;
    }
    if (argc != 1) {
         y_error("expecting a single argument");
    }
    type = yarg_typeid(0);
    if (type == Y_STRING) {
        push_parameter(dm, ygets_q(0));
    } else if (type == Y_DOUBLE || type == Y_FLOAT) {
        if (type == Y_DOUBLE) {
            const double cmin = -1;
            const double cmax = +1;
            double* cmd = ygeta_d(0, &ntot, NULL);
            if (ntot != dm->number) {
                y_error("bad number of commands");
            }
            for (i = 0; i < ntot; ++i) {
                double val = cmd[i];
                if (isnan(val)) {
                    y_error("invalid command value");
                }
                dm->commands[i] = CLAMP(val, cmin, cmax);
            }
        } else {
            const float cmin = -1;
            const float cmax = +1;
            float* cmd = ygeta_f(0, &ntot, NULL);
            if (ntot != dm->number) {
                y_error("bad number of commands");
            }
            for (i = 0; i < ntot; ++i) {
                float val = cmd[i];
                if (isnanf(val)) {
                    y_error("invalid command value");
                }
                dm->commands[i] = CLAMP(val, cmin, cmax);
            }
        }
        if (asdkSend(dm->device, dm->commands) != SUCCESS) {
            asdk_error();
        }
        ykeep_use(dm->handle);
    } else if (type == Y_VOID) {
        ykeep_use(dm->handle);
    } else {
        y_error("invalid argument");
    }
}

static void extract_dm(void* addr, char* key)
{
    deformable_mirror_t* dm = (deformable_mirror_t*)addr;
    if (dm->device == NULL) {
        y_error("unconnected device");
    }
    push_parameter(dm, key);
}

static y_userobj_t deformable_mirror_type = {
    "alpao_deformable_mirror",
    free_dm,
    print_dm,
    eval_dm,
    extract_dm,
    NULL
};

static deformable_mirror_t* get_dm(int iarg)
{
    deformable_mirror_t* dm;
    dm = (deformable_mirror_t*)yget_obj(iarg, &deformable_mirror_type);
    if (dm->device == NULL) {
        y_error("unconnected device");
    }
    return dm;
}

static double asdk_get(deformable_mirror_t* dm, const char* key)
{
    Scalar val;
    if (asdkGet(dm->device, key, &val) != SUCCESS) {
        asdk_error();
    }
    return val;
}

static void asdk_set(deformable_mirror_t* dm, const char* key, double val)
{
    if (asdkSet(dm->device, key, val) != SUCCESS) {
        asdk_error();
    }
}

static void asdk_set_string(deformable_mirror_t* dm, const char* key,
                            const char* str)
{
    if (asdkSetString(dm->device, key, (str == NULL ? "" : str)) != SUCCESS) {
        asdk_error();
    }
}

static void asdk_error()
{
    const int len = 255;
    int status;
    UInt code;
    char mesg[len+1];
    status = asdkGetLastError(&code, mesg, len);
    if (status != SUCCESS) {
        strcpy(mesg, "failed to retrieve last error message");
    } else {
        mesg[len] = '\0';
    }
    y_error(mesg);
}

void Y_alpao_open(int argc)
{
    char* basename;
    char* dirname;
    char* ptr;
    char* serial;
    char workdir[PATH_MAX+1];
    char path[PATH_MAX+1];
    const char* filename;
    deformable_mirror_t* dm;
    long dims[2];

    if (argc != 1) {
        y_error("usage: alpao_open(filename);");
    }
    if (sizeof(Scalar) != sizeof(double)) {
        y_error("sizeof(Scalar) != sizeof(double)");
    }
    filename = ygets_q(0);
    if (filename == NULL || filename[0] == '\0') {
        y_error("invalid mirror name");
    }
    if (strlen(filename) >= sizeof(path)) {
        y_error("file name too long");
    }
    dm = (deformable_mirror_t*)ypush_obj(yfunc_obj(&deformable_mirror_type),
                                         sizeof(deformable_mirror_t));

    strcpy(path, filename);
    ptr = strrchr(path, '/');
    if (ptr != NULL) {
        *ptr = '\0';
        dirname = path;
        basename = ptr + 1;
        if (getcwd(workdir, sizeof(workdir)) == NULL) {
            y_error("cannot get working directory");
        }
        if (chdir(path) != 0) {
            y_error("cannot change working directory");
        }
    } else {
        dirname = NULL;
        basename = path;
    }
    ptr = strrchr(basename, '.');
    if (ptr != NULL && strcmp(ptr, ".acfg") == 0) {
        /* strip extension */
        *ptr = '\0';
    }
    serial = basename;
    dm->device = asdkInit(serial);
    if (dirname != NULL && chdir(workdir) != 0) {
        y_error("cannot go back to working directory");
    }
    if (dm->device == NULL) {
        asdk_error();
    }
    dm->serial = p_strcpy(serial);
    dm->number = lround(asdk_get(dm, "NbOfActuator"));
    asdk_set(dm, "UseException", 0);

    /* allocate a Yorick vector for the commands and keep a reference on it */
    dims[0] = 1;
    dims[1] = dm->number;
    dm->commands = ypush_d(dims);
    dm->handle = yget_use(0);

    /* left deformable mirror object on top of stack */
    yarg_drop(1);
}

void Y_alpao_reset(int argc)
{
    deformable_mirror_t* dm;
    if (argc != 1) {
        y_error("usage: alpao_reset(dm);");
    }
    dm = get_dm(0);
    if (asdkReset(dm->device) != SUCCESS) {
        asdk_error();
    }
    memset(dm->commands, 0, dm->number*sizeof(double));
}

void Y_alpao_stop(int argc)
{
   if (argc != 1) {
        y_error("usage: alpao_stop(dm);");
    }
    if (asdkStop(get_dm(0)->device) != SUCCESS) {
        asdk_error();
    }
}

void Y_alpao_get(int argc)
{
    if (argc != 2) {
        y_error("usage: alpao_get(dm, key);");
    }
    push_parameter(get_dm(1), ygets_q(0));
}

void Y_alpao_set(int argc)
{
    deformable_mirror_t* dm;
    int i;
    if (argc != 3) {
        y_error("usage: alpao_set(dm, key, val);");
    }
    dm = get_dm(2);
    i = find_parameter(ygets_q(1), true);
    if ((parameters[i].flags & WRITABLE) != WRITABLE) {
        y_error("unwritable parameter");
    }
    switch ((parameters[i].flags & 0x7)) {
    case BOOLEAN:
        asdk_set(dm, parameters[i].name, yarg_true(0));
        break;
    case INTEGER:
        asdk_set(dm, parameters[i].name, ygets_l(0));
        break;
    case FLOAT:
        asdk_set(dm, parameters[i].name, ygets_d(0));
        break;
    case STRING:
        asdk_set_string(dm, parameters[i].name, ygets_q(0));
        break;
    default:
        y_error("unknown parameter type");
    }
}
