
/* vim:fileencoding=utf-8:foldmethod=marker */


/* API: {{{ */
#ifndef __DONKEYOBJ_H__
#define __DONKEYOBJ_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DONK_BUFFER_SIZE 512

typedef enum donk_status_t {
    DONK_SUCCESS = 0,
    DONK_BAD_FILE,
    DONK_INVALID_OBJ,
    DONK_BAD_MALLOC,
} donk_status_t;

typedef struct donk {
    uint32_t *elements;
    size_t    elements_count;

    float    *vertices;
    size_t    vertices_count;

    float    *textures;
    size_t    textures_count;

    float    *normals;
    size_t    normals_count;
} donk_t;

donk_status_t donk(const char *path, donk_t *ctx);


#endif // __DONKEYOBJ_H__
/* }}} */

// impementation part
#define DONKEYOBJ_IMPLEMENTATION
#ifdef DONKEYOBJ_IMPLEMENTATION

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


/* Macros {{{ */

#define STRFMT "\033[92m'%s'\033[0m"
#define DFMT "\033[32m%d\033[0m"
#define LDFMT "\033[32m%ld\033[0m"

#define donk_log(fmt, ...)\
printf(\
    "\033[94mDONK\033[0m <\033[95mLOG\033[0m>\033[33m:"DFMT" "\
    fmt"\n",\
    __LINE__, __VA_ARGS__\
)

#define donk_err(fmt, ...)\
fprintf(\
    stderr,\
    "\033[34mDONK\033[0m <\033[31mERR\033[0m>\033[33m:"DFMT" "\
    fmt" "DFMT". "STRFMT"\n",\
    __LINE__, __VA_ARGS__, errno, strerror(errno)\
)

#define str_append()\
ctx.str[ctx.si] = ctx.buf[ctx.bi];\
ctx.si++;\
if (ctx.si >= DONK_BUFFER_SIZE) {\
    ctx.str[DONK_BUFFER_SIZE-1] = 0;\
    donk_err(\
        "file "STRFMT":"LDFMT" string "STRFMT\
        " is too long. max size: "DFMT"\n",\
        ctx.path, ctx.line_number, ctx.str, DONK_BUFFER_SIZE\
    );\
    return DONK_INVALID_OBJ;\
}

#define IS_WHITESPACE(x) (x == ' ' || x == '\t')

/* }}} */

/* obj keyword {{{ */
typedef enum kwtype_t {
    DOKWT_UNKNOWN = 0,
    // vertex data
    DOKWT_V, DOKWT_VN, DOKWT_VT, DOKWT_VP,
    DOKWT_CSTYPE, DOKWT_DEG, DOKWT_BMAT, DOKWT_STEP,
    // elements
    DOKWT_P, DOKWT_L, DOKWT_F, DOKWT_CURV, DOKWT_CURV2, DOKWT_SURF,
    // free-form curve/surface body statements
    DOKWT_PARM, DOKWT_TRIM, DOKWT_HOLE, DOKWT_SCRV, DOKWT_SP, DOKWT_END,
    // connectivity between free-form surfaces
    DOKWT_CON,
    // grouping
    DOKWT_G, DOKWT_S, DOKWT_MG, DOKWT_O,
    // display/render attributes
    DOKWT_BEVEL, DOKWT_C_INTERP, DOKWT_D_INTERP, DOKWT_LOD, DOKWT_USEMTL,
    DOKWT_MTLLIB, DOKWT_SHADOW_OBJ, DOKWT_TRACE_OBJ, DOKWT_CTECH, DOKWT_STECH,
    DOKWT_MAX
} kwtype_t;

static const char *kwtable[DOKWT_MAX] = {
    [DOKWT_UNKNOWN] = "#unknown",
    [DOKWT_V] = "v",
    [DOKWT_VN] = "vn",
    [DOKWT_VT] = "vt",
    [DOKWT_VP] = "vp",
    [DOKWT_CSTYPE] = "cstype",
    [DOKWT_DEG] = "deg",
    [DOKWT_BMAT] = "bmat",
    [DOKWT_STEP] = "step",
    [DOKWT_P] = "p",
    [DOKWT_L] = "l",
    [DOKWT_F] = "f",
    [DOKWT_CURV] = "curv",
    [DOKWT_CURV2] = "curv2",
    [DOKWT_SURF] = "surf",
    [DOKWT_PARM] = "parm",
    [DOKWT_TRIM] = "trim",
    [DOKWT_HOLE] = "hole",
    [DOKWT_SCRV] = "scrv",
    [DOKWT_SP] = "sp",
    [DOKWT_END] = "end",
    [DOKWT_CON] = "con",
    [DOKWT_G] = "g",
    [DOKWT_S] = "s",
    [DOKWT_MG] = "mg",
    [DOKWT_O] = "o",
    [DOKWT_BEVEL] = "bevel",
    [DOKWT_C_INTERP] = "c_interp",
    [DOKWT_D_INTERP] = "d_interp",
    [DOKWT_LOD] = "lod",
    [DOKWT_USEMTL] = "usemtl",
    [DOKWT_MTLLIB] = "mtllib",
    [DOKWT_SHADOW_OBJ] = "shadow_obj",
    [DOKWT_TRACE_OBJ] = "trace_obj",
    [DOKWT_CTECH] = "ctech",
    [DOKWT_STECH] = "stech",
};

/* }}} */

typedef union donkface {
    struct {
        uint32_t geo;
        uint32_t tex;
        uint32_t nor;
    };
    uint32_t arr[3];
} donkface_t;

struct _donk_ctx_t {
    // are the in a comment?
    bool comment;
    bool escape;

    kwtype_t kwtype;

    char *path;
    size_t line_number;
    size_t lx; // index use to mark start of line in any array
    size_t bi; // buffer index
    size_t si; // str index
    size_t sa; // str args - added with each space

    // size_t fi; // faces index
    // size_t vgi; // vertex geometry index

    char str[DONK_BUFFER_SIZE]; // for string temp strings
    char buf[DONK_BUFFER_SIZE]; // buffer the we read from a file
    // float vertex_vg[DONKEYOBJ_VERTEX_SIZE];  // geometric
    // float vertex_vt[1024]; // texture
    // float vertex_vn[1024]; // normals
    // float vertex_vp[1024]; // parameter space - Free-form curve/surface attr

    donkface_t faces[DONK_BUFFER_SIZE];
    size_t fi;

    // for determining what number are we reading a vg/vt/vn
    size_t fvi;
};


donk_status_t donk(const char *path, donk_t *result) {
    donk_log("start loading "STRFMT, path);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        donk_err("opening path: "STRFMT, path);
        return DONK_BAD_FILE;
    }

    struct _donk_ctx_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.line_number = 1;
    ctx.path = (char *)path;

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size < 0 || lseek(fd, 0, SEEK_SET) != 0) {
        donk_err("can't seek the object file: "STRFMT, path);
        return DONK_BAD_FILE;
    }

    result->elements = malloc(file_size * sizeof(uint32_t));
    result->elements_count = 0;
    result->vertices = malloc(file_size * sizeof(float));
    result->vertices_count = 0;

    if (result->elements == NULL || result->vertices == NULL) {
        donk_err("mallocing "LDFMT, file_size * 4);
        return DONK_BAD_MALLOC;
    }

    while (true) {
        ssize_t read_size = read(fd, ctx.buf, DONK_BUFFER_SIZE);
        if (read_size < 0) return DONK_BAD_FILE;
        if (read_size == 0) break;

        for (ctx.bi = 0; ctx.bi < (size_t)read_size; ctx.bi++) {
            assert(result->vertices_count < (size_t)file_size);
            assert(result->elements_count < (size_t)file_size);

            if (ctx.escape) {
                ctx.escape = false;
                continue;
            }

            if (ctx.buf[ctx.bi] == '#') {
                ctx.comment = true;
                continue;
            } else if (ctx.buf[ctx.bi] == '\\') {
                ctx.escape = true;
                continue;
            } else if (ctx.buf[ctx.bi] == '\n') {
                ctx.comment = false;

                if (ctx.si && !ctx.kwtype) {
                    ctx.str[ctx.si] = 0;
                    donk_err(
                        ""STRFMT":"LDFMT" keyword not found: "STRFMT,
                        path, ctx.line_number, ctx.str
                    );
                    return DONK_INVALID_OBJ;
                }

                switch (ctx.kwtype) {
                    case DOKWT_MTLLIB:
                    case DOKWT_G:
                        if (!ctx.si) break;
                        ctx.str[ctx.si] = 0;
                        break;

                    case DOKWT_USEMTL:
                        if (!ctx.si) {
                            printf(
                                "%s:%ld this statement need an argument\n",
                                ctx.path, ctx.line_number
                            );
                            return DONK_INVALID_OBJ;
                        }
                        ctx.str[ctx.si] = 0;
                        break;

                    case DOKWT_O:
                        if (!ctx.si) {
                            printf(
                                "%s:%ld this statement need an argument\n",
                                ctx.path, ctx.line_number
                            );
                            return DONK_INVALID_OBJ;
                        }
                        ctx.str[ctx.si] = 0;
                        break;

                    case DOKWT_V:
                        ctx.str[ctx.si] = 0;
                        result->vertices[result->vertices_count] = atof(ctx.str);
                        result->vertices_count++;
                        break;

                    case DOKWT_F:
                        ctx.lx = 0;
                        ctx.str[ctx.si] = 0;
                        ctx.faces[ctx.fi].arr[ctx.fvi] = atof(ctx.str);
                        ctx.fi++;

                        ctx.sa++;
                        size_t tg = ctx.fi - ctx.sa;

                        uint32_t i0 = ctx.faces[tg].arr[0] - 1;
                        uint32_t i1;
                        tg++;
                        uint32_t i2 = ctx.faces[tg].arr[0] - 1;
                        tg++;

                        for (; tg < ctx.fi; tg++) {
                            i1 = i2;
                            i2 = ctx.faces[tg].arr[0] - 1;
                       
                            result->elements[result->elements_count] = i0;
                            result->elements_count++;
                            result->elements[result->elements_count] = i1;
                            result->elements_count++;
                            result->elements[result->elements_count] = i2;
                            result->elements_count++;
                        }

                        // size_t k;
                        // size_t n = 0;
                        //
                        // tinyobj_vertex_index_t i0 = f[0];
                        // tinyobj_vertex_index_t i1;
                        // tinyobj_vertex_index_t i2 = f[1];
                        //
                        // assert(3 * num_f < TINYOBJ_MAX_FACES_PER_F_LINE);
                        //
                        // for (k = 2; k < num_f; k++) {
                        //     i1 = i2;
                        //     i2 = f[k];
                        //     command->f[3 * n + 0] = i0;
                        //     command->f[3 * n + 1] = i1;
                        //     command->f[3 * n + 2] = i2;
                        //
                        //     command->f_num_verts[n] = 3;
                        //     n++;
                        // }
                        break;
                        
                    default:
                        break;
                }


                ctx.si = 0;
                ctx.sa = 0;
                ctx.fi = 0;
                ctx.fvi = 0;
                ctx.kwtype = DOKWT_UNKNOWN;
                ctx.line_number++;
                continue;
            }

            if (ctx.comment) {
                for (;
                    ctx.buf[ctx.bi] != '\n' && ctx.bi < (size_t)read_size;
                    ctx.bi++
                );
                ctx.bi--;
                continue;
            }

            switch (ctx.kwtype) {
                case DOKWT_UNKNOWN:
                    // we are looking for a keyword
                    if (!ctx.si && IS_WHITESPACE(ctx.buf[ctx.bi]))
                        continue;

                    // end of keyword
                    if (ctx.si && IS_WHITESPACE(ctx.buf[ctx.bi])) {
                        ctx.str[ctx.si] = 0;
                        // kwtype = get_kwtype(ctx.str, ctx.si);
                        for (size_t j = 0; j < DOKWT_MAX; j++) {
                            if (!strcmp(ctx.str, kwtable[j])) {
                                ctx.kwtype = j;
                                break;
                            }
                        }

                        if (!ctx.kwtype) {
                            printf(
                                "%s:%ld keyword '%s' not found\n",
                                path, ctx.line_number, ctx.str
                            );
                            return DONK_BAD_FILE;
                        }

                        ctx.si = 0;
                        continue;
                    }

                    str_append();
                    continue;

                case DOKWT_MTLLIB:
                case DOKWT_G:
                    if (IS_WHITESPACE(ctx.buf[ctx.bi])) {
                        if (!ctx.si) continue;
                        ctx.str[ctx.si] = 0;
                        ctx.si = 0;
                        ctx.sa++;
                        continue;
                    }

                    str_append();
                    continue;

                case DOKWT_O:
                case DOKWT_USEMTL:
                    // TODO: Check for valid chars
                    if (!ctx.si && IS_WHITESPACE(ctx.buf[ctx.bi]))
                        continue;
                    str_append();
                    continue;

                case DOKWT_V:
                    if (IS_WHITESPACE(ctx.buf[ctx.bi])) {
                        if (!ctx.si) continue;
                        ctx.str[ctx.si] = 0;
                        result->vertices[result->vertices_count] = atof(ctx.str);
                        result->vertices_count++;
                        ctx.si = 0;
                        ctx.sa++;
                        continue;
                    }

                    str_append();
                    continue;

                case DOKWT_F:
                    if (!ctx.lx) ctx.lx = ctx.fi;

                    if (ctx.buf[ctx.bi] == '/') {
                        if (ctx.si) {
                            ctx.str[ctx.si] = 0;
                            ctx.faces[ctx.fi].arr[ctx.fvi] = atof(ctx.str);
                        } else {
                            ctx.faces[ctx.fi].arr[ctx.fvi] = 0;
                        }

                        ctx.si = 0;
                        ctx.fvi++;
                        continue;
                    }

                    if (IS_WHITESPACE(ctx.buf[ctx.bi])) {
                        if (!ctx.si) continue;

                        ctx.str[ctx.si] = 0;
                        ctx.faces[ctx.fi].arr[ctx.fvi] = atof(ctx.str);

                        ctx.fvi = 0;
                        ctx.fi++;
                        ctx.si = 0;
                        ctx.sa++;
                        continue;
                    }

                    str_append();
                    continue;

                default:
                    break;
            }
        }

        if (read_size != DONK_BUFFER_SIZE)
            break;
    }

    donk_log("elements_count: "LDFMT, result->elements_count);
    donk_log("file_size: "LDFMT, file_size);

    result->elements = realloc(
        result->elements,
        result->elements_count * sizeof(uint32_t)
    );
    result->vertices = realloc(
        result->vertices,
        result->vertices_count * sizeof(float)
    );

    close(fd);
    return DONK_SUCCESS;
}

#undef donk_err
#undef donk_log


#endif // DONKEYOBJ_IMPLEMENTATION

