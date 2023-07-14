
// header part
#ifndef __DONKEYOBJ_H__
#define __DONKEYOBJ_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DONKEYOBJ_BUFFER_SIZE 512
#define DONKEYOBJ_STRING_SIZE 512

typedef enum donkey_status {
    DONKEYOBJ_SUCCESS = 0,
    DONKEYOBJ_ERROR_OPEN, // cant open the file
    DONKEYOBJ_ERROR_READ, // while reading the object file an error happend
    DONKEYOBJ_ERROR_BAD_OBJ, // invalid .obj file
} donkey_status;

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

// [0] = vertex geometry
// [1] = vertex texture
// [2] = vertex normals
typedef uint32_t donkey_face[3];

typedef struct donkey_ctx {
    // for determining what number are we reading a vg/vt/vn
    uint8_t face_vidx;
    // are the in a comment?
    bool comment;

    kwtype_t kwtype;

    char *path;
    size_t line_number;
    size_t bi;
    size_t si; // str index
    size_t sa; // str args - added with each space

    size_t fi; // faces index
    size_t vgi; // vertex geometry index

    char str[DONKEYOBJ_STRING_SIZE]; // for string temp strings
    char buf[DONKEYOBJ_BUFFER_SIZE]; // buffer the we read from a file
    double vertex_vg[1024];  // geometric
    // double vertex_vt[1024]; // texture
    // double vertex_vn[1024]; // normals
    // double vertex_vp[1024]; // parameter space - Free-form curve/surface attr

    donkey_face faces[1024];
    
    // size_t vtidx = 0;
    // size_t vnidx = 0;
    // size_t vpidx = 0;
} donkey_ctx;

donkey_status donkeyobj(const char *path, donkey_ctx *ctx);


#endif // __DONKEYOBJ_H__


// impementation part
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


#define str_append()\
ctx->str[ctx->si] = ctx->buf[ctx->bi];\
ctx->si++;\
if (ctx->si >= DONKEYOBJ_STRING_SIZE) {\
    ctx->str[DONKEYOBJ_STRING_SIZE-1] = 0;\
    printf(\
        "%s:%ld string '%s' is too long. max size: %d\n",\
        ctx->path, ctx->line_number, ctx->str, DONKEYOBJ_STRING_SIZE\
    );\
    return DONKEYOBJ_ERROR_BAD_OBJ;\
}

#define IS_WHITESPACE(x) (x == ' ' || x == '\t')






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


donkey_status donkeyobj(const char *path, donkey_ctx *ctx) {
    printf("\n\n");
    int fd = open(path, O_RDONLY);
    if (fd < 0) return DONKEYOBJ_ERROR_OPEN;


    // donkey_ctx ctx;
    memset(ctx, 0, sizeof(donkey_ctx));
    ctx->line_number = 1;
    ctx->path = (char *)path;


    while (true) {
        ssize_t read_size = read(fd, ctx->buf, DONKEYOBJ_BUFFER_SIZE);
        if (read_size < 0) return DONKEYOBJ_ERROR_READ;
        if (read_size == 0) break;

        printf("\nread size: %ld\n", read_size);

        for (; ctx->bi < (size_t)read_size; ctx->bi++) {
            if (ctx->buf[ctx->bi] == '#') {
                ctx->comment = true;
                continue;
            } else if (ctx->buf[ctx->bi] == '\n') {
                ctx->comment = false;

                if (ctx->si && !ctx->kwtype) {
                    printf("%s:%ld bad object!\n", path, ctx->line_number);
                    return DONKEYOBJ_ERROR_BAD_OBJ;
                }

                switch (ctx->kwtype) {
                    case DOKWT_MTLLIB:
                    case DOKWT_G:
                        if (!ctx->si) break;
                        ctx->str[ctx->si] = 0;
                        printf("arg[%ld]: '%s'\n", ctx->sa, ctx->str);
                        break;

                    case DOKWT_USEMTL:
                        if (!ctx->si) {
                            printf(
                                "%s:%ld this statement need an argument\n",
                                ctx->path, ctx->line_number
                            );
                            return DONKEYOBJ_ERROR_BAD_OBJ;
                        }
                        ctx->str[ctx->si] = 0;
                        printf("\033[31musemtl\033[0m: '%s'\n", ctx->str);
                        break;

                    case DOKWT_O:
                        if (!ctx->si) {
                            printf(
                                "%s:%ld this statement need an argument\n",
                                ctx->path, ctx->line_number
                            );
                            return DONKEYOBJ_ERROR_BAD_OBJ;
                        }
                        ctx->str[ctx->si] = 0;
                        printf("\033[33mobject name\033[0m: '%s'\n", ctx->str);
                        break;

                    case DOKWT_V:
                        ctx->str[ctx->si] = 0;
                        ctx->vertex_vg[ctx->vgi] = atof(ctx->str);
                        ctx->vgi++;
                        printf(
                            "arg[%ld]: '%s' | \033[32m%.30f\033[0m\n",
                            ctx->sa, ctx->str, ctx->vertex_vg[ctx->vgi]
                        );
                        break;

                    case DOKWT_F:
                        ctx->str[ctx->si] = 0;
                        ctx->faces[ctx->fi][ctx->face_vidx] = atof(ctx->str);
                        ctx->fi++;

                        ctx->sa++;
                        printf("found %ld faces on this line:\n", ctx->sa);
                        for (size_t tg = ctx->sa; tg > 0; tg--) {
                            printf(
                                "\033[32m%d\033[0m/\033[32m%d\033[0m/"
                                "\033[32m%d\033[0m ",
                                ctx->faces[ctx->fi-tg][0],
                                ctx->faces[ctx->fi-tg][1],
                                ctx->faces[ctx->fi-tg][2]
                            );
                        }
                        printf("\n");
                        break;
                        
                    default:
                        break;
                }


                ctx->si = 0;
                ctx->sa = 0;
                ctx->face_vidx = 0;
                ctx->kwtype = DOKWT_UNKNOWN;
                ctx->line_number++;
                continue;
            }

            if (ctx->comment) {
                printf("\033[33m#");
                for (; ctx->buf[ctx->bi] != '\n' && ctx->bi < (size_t)read_size; ctx->bi++)
                    printf("%c", ctx->buf[ctx->bi]);
                printf("\033[0m\n");
                ctx->bi--;
                continue;
            }

            switch (ctx->kwtype) {
                case DOKWT_UNKNOWN:
                    // we are looking for a keyword
                    if (!ctx->si && IS_WHITESPACE(ctx->buf[ctx->bi]))
                        continue;

                    // end of keyword
                    if (ctx->si && IS_WHITESPACE(ctx->buf[ctx->bi])) {
                        ctx->str[ctx->si] = 0;
                        // kwtype = get_kwtype(ctx->str, ctx->si);
                        for (size_t j = 0; j < DOKWT_MAX; j++) {
                            if (!strcmp(ctx->str, kwtable[j])) {
                                ctx->kwtype = j;
                                break;
                            }
                        }

                        if (!ctx->kwtype) {
                            printf(
                                "%s:%ld keyword '%s' not found\n",
                                path, ctx->line_number, ctx->str
                            );
                            return DONKEYOBJ_ERROR_BAD_OBJ;
                        }

                        printf("\nkw[%ld]: '%s'\n", ctx->si, ctx->str);
                        ctx->si = 0;
                        continue;
                    }

                    str_append();
                    continue;

                case DOKWT_MTLLIB:
                case DOKWT_G:
                    if (IS_WHITESPACE(ctx->buf[ctx->bi])) {
                        if (!ctx->si) continue;
                        ctx->str[ctx->si] = 0;
                        ctx->si = 0;
                        printf("arg[%ld]: '%s'\n", ctx->sa, ctx->str);
                        ctx->sa++;
                        continue;
                    }

                    str_append();
                    continue;

                case DOKWT_O:
                case DOKWT_USEMTL:
                    // TODO: Check for valid chars
                    if (!ctx->si && IS_WHITESPACE(ctx->buf[ctx->bi]))
                        continue;
                    str_append();
                    continue;

                case DOKWT_V:
                    if (IS_WHITESPACE(ctx->buf[ctx->bi])) {
                        if (!ctx->si) continue;
                        ctx->str[ctx->si] = 0;
                        ctx->vertex_vg[ctx->vgi] = atof(ctx->str);
                        printf(
                            "arg[%ld]: '%s' | \033[32m%.50f\033[0m\n",
                            ctx->sa, ctx->str, ctx->vertex_vg[ctx->vgi]
                        );
                        ctx->vgi++;
                        ctx->si = 0;
                        ctx->sa++;
                        continue;
                    }

                    str_append();
                    continue;

                case DOKWT_F:
                    if (ctx->buf[ctx->bi] == '/') {
                        if (ctx->si) {
                            ctx->str[ctx->si] = 0;
                            ctx->faces[ctx->fi][ctx->face_vidx] = atof(ctx->str);
                        } else {
                            ctx->faces[ctx->fi][ctx->face_vidx] = 0;
                        }

                        ctx->si = 0;
                        ctx->face_vidx++;
                        continue;
                    }

                    if (IS_WHITESPACE(ctx->buf[ctx->bi])) {
                        if (!ctx->si) continue;

                        ctx->str[ctx->si] = 0;
                        ctx->faces[ctx->fi][ctx->face_vidx] = atof(ctx->str);

                        ctx->face_vidx = 0;
                        ctx->fi++;
                        ctx->si = 0;
                        ctx->sa++;
                        continue;
                    }

                    str_append();
                    continue;

                default:
                    break;
            }
        }

        if (read_size != DONKEYOBJ_BUFFER_SIZE)
            break;
    }

    printf("\n\n");
    close(fd);
    return DONKEYOBJ_SUCCESS;
}



#endif // DONKEYOBJ_IMPLEMENTATION

