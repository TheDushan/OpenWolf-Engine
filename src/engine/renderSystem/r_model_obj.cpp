////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2016 Alexander Sago
// Copyright(C) 2011 - 2022 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   r_models_obj.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description: model loading and caching
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

static float32 sizeMult = 10.0f;

static std::vector<objectModel_t *> loaded_models;

float32 nullVerts[] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f
};
float32 nullUVs[] = {
    0.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 0.0f,
};
float32 nullNormals[] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f
};

#define OBJECT_MAX_INDICIES 1024
#define CMD_BUFFER_LEN 12
#define FLOAT_BUFER_LEN 15

valueType const defaultShader[] = "textures/colors/default_object";

objectModel_t *idRenderSystemLocal::Model_LoadObject(pointer name) {

    for(objectModel_t *mod : loaded_models) {
        if(!::strcmp(mod->name, name)) {
            return mod;
        }
    }

    fileHandle_t file;
    sint len = fileSystem->FOpenFileRead(name, &file, false);

    if(len < 0) {
        return nullptr;
    }

    if(len == 0) {
        fileSystem->FCloseFile(file);
        return nullptr;
    }

    valueType *object_buf = new valueType[len];
    fileSystem->Read(object_buf, len, file);
    fileSystem->FCloseFile(file);

    struct objectWorkElement {
        sint vert = 0;
        sint uv = 0;
        sint normal = 0;
    };

    struct objectWorkFace {
        objectWorkElement elements [3];
    };

    struct objectWorkSurface {
        valueType shader[MAX_QPATH];
        sint shaderIndex = 0;
        std::vector<objectWorkFace> faces;
    };

    std::vector<float32> verts;
    std::vector<float32> uvs;
    std::vector<float32> normals;
    std::vector<objectWorkSurface> surfs;

    bool allGood = true;
    bool seekline = false;
    sint32 object_buf_i = 0;

    while(object_buf_i < len && allGood) {

        switch(object_buf[object_buf_i]) {
            case '\r':
                object_buf_i++;
                continue;

            case '\n':
                object_buf_i++;
                seekline = false;
                continue;

            case '\0':
                allGood = false;
                continue;

            case '#':
                object_buf_i++;
                seekline = true;
                continue;

            default:
                if(seekline) {
                    object_buf_i++;
                    continue;
                }

                break;
        }

        valueType cmd_buf[CMD_BUFFER_LEN];
        sint ci;

        for(ci = 0; ci < CMD_BUFFER_LEN; ci++) {
            switch(object_buf[object_buf_i + ci]) {
                case ' ':
                    cmd_buf[ci] = '\0';
                    break;

                default:
                    cmd_buf[ci] = object_buf[object_buf_i + ci];
                    continue;
            }

            break;
        }

        if(ci == CMD_BUFFER_LEN) {
            allGood = false;
            break;
        } else {
            object_buf_i += ci + 1;
        }

        if(!::strcmp(cmd_buf, "o")) {
            surfs.emplace_back();
            seekline = true;
            continue;
        } else if(!::strcmp(cmd_buf, "v")) {
            valueType float_buf[FLOAT_BUFER_LEN];

            for(sint v = 0; v < 3; v++) {
                sint vi;

                for(vi = 0; vi < FLOAT_BUFER_LEN; vi++) {
                    switch(object_buf[object_buf_i + vi]) {
                        case '\r':
                        case '\n':
                        case ' ':
                            float_buf[vi] = '\0';
                            break;

                        default:
                            float_buf[vi] = object_buf[object_buf_i + vi];
                            continue;
                    }

                    break;
                }

                object_buf_i += vi + 1;
                float32 val = ::strtod(float_buf, nullptr) * sizeMult;
                verts.push_back(val);
                ::memset(float_buf, FLOAT_BUFER_LEN, sizeof(valueType));
            }

            object_buf_i -= 2;
            seekline = true;
            continue;
        } else if(!::strcmp(cmd_buf, "vn")) {
            valueType float_buf[FLOAT_BUFER_LEN];

            for(sint v = 0; v < 3; v++) {
                sint vi;

                for(vi = 0; vi < FLOAT_BUFER_LEN; vi++) {
                    switch(object_buf[object_buf_i + vi]) {
                        case '\r':
                        case '\n':
                        case ' ':
                            float_buf[vi] = '\0';
                            break;

                        default:
                            float_buf[vi] = object_buf[object_buf_i + vi];
                            continue;
                    }

                    break;
                }

                object_buf_i += vi + 1;
                float32 val = ::strtod(float_buf, nullptr);
                normals.push_back(val);
                ::memset(float_buf, FLOAT_BUFER_LEN, sizeof(valueType));
            }

            object_buf_i -= 2;
            seekline = true;
            continue;
        } else if(!::strcmp(cmd_buf, "vt")) {
            valueType float_buf[FLOAT_BUFER_LEN];

            for(sint v = 0; v < 2; v++) {
                sint vi;

                for(vi = 0; vi < FLOAT_BUFER_LEN; vi++) {
                    switch(object_buf[object_buf_i + vi]) {
                        case '\r':
                        case '\n':
                        case ' ':
                            float_buf[vi] = '\0';
                            break;

                        default:
                            float_buf[vi] = object_buf[object_buf_i + vi];
                            continue;
                    }

                    break;
                }

                object_buf_i += vi + 1;
                float32 val = ::strtod(float_buf, nullptr);

                if(v) {
                    val = 1 - val;
                }

                uvs.push_back(val);
                ::memset(float_buf, FLOAT_BUFER_LEN, sizeof(valueType));
            }

            object_buf_i -= 2;
            seekline = true;
            continue;
        } else if(!::strcmp(cmd_buf, "f")) {
            objectWorkSurface &cursurf = surfs.back();
            objectWorkFace face;

            for(sint fi = 0; fi < 3; fi++) {
                objectWorkElement element;
                valueType int_buf[FLOAT_BUFER_LEN];

                for(sint v = 0; v < 3; v++) {
                    sint vi;

                    for(vi = 0; vi < FLOAT_BUFER_LEN; vi++) {
                        switch(object_buf[object_buf_i + vi]) {
                            case '\r':
                            case '\n':
                            case ' ':
                                int_buf[vi] = '\0';
                                v += 3;
                                break;

                            case '\\':
                            case '/':
                                int_buf[vi] = '\0';
                                break;

                            default:
                                int_buf[vi] = object_buf[object_buf_i + vi];
                                continue;
                        }

                        break;
                    }

                    object_buf_i += vi + 1;
                    sint val = ::strtol(int_buf, nullptr, 10) - 1;

                    switch(v) {
                        case 0:
                        case 3:
                            element.vert = val;

                        case 1:
                        case 4:
                            element.uv = val;

                        case 2:
                        case 5:
                            element.normal = val;
                    }

                    ::memset(int_buf, FLOAT_BUFER_LEN, sizeof(valueType));
                }

                face.elements[fi] = element;
            }

            cursurf.faces.push_back(face);
            object_buf_i -= 2;
            seekline = true;
            continue;
        } else if(!::strcmp(cmd_buf, "usemtl")) {
            objectWorkSurface &cursurf = surfs.back();
            valueType nam_buf[MAX_QPATH];
            ::memset(nam_buf, '\0', MAX_QPATH);
            sint ci = 0;

            while(true) {
                if(ci >= MAX_QPATH) {
                    common->Error(ERR_DROP,
                                  "Wavefront object model shader field exceeds MAX_QPATH(%i)",
                                  sint(MAX_QPATH));
                }

                switch(object_buf[object_buf_i]) {
                    case '\r':
                    case '\n':
                    case ' ':
                        nam_buf[ci++] = '\0';
                        object_buf_i++;
                        break;

                    case '\\':
                        nam_buf[ci++] = '\0';
                        object_buf_i++;
                        break;

                    default:
                        nam_buf[ci++] = object_buf[object_buf_i];
                        object_buf_i++;
                        continue;
                }

                break;
            }

            ::strcpy(cursurf.shader, nam_buf);

            object_buf_i -= 2;
            seekline = true;
            continue;
        } else {
            seekline = true;
            continue;
        }
    }

    if(allGood) {
    } else {
        common->Printf("Object Load Failed.\n");
        return nullptr;
    }

    objectModel_t *mod = new objectModel_t;

    VectorSet(mod->mins, verts[0], verts[1], verts[2]);
    VectorSet(mod->maxs, verts[0], verts[1], verts[2]);

    for(sint i = 3; i < verts.size(); i += 3) {
        if(verts[i] < mod->mins[0]) {
            mod->mins[0] = verts[i];
        } else if(verts[i] > mod->maxs[0]) {
            mod->maxs[0] = verts[i];
        }

        if(verts[i + 1] < mod->mins[1]) {
            mod->mins[1] = verts[i + 1];
        } else if(verts[i + 1] > mod->maxs[1]) {
            mod->maxs[1] = verts[i + 1];
        }

        if(verts[i + 2] < mod->mins[2]) {
            mod->mins[2] = verts[i + 2];
        } else if(verts[i + 2] > mod->maxs[2]) {
            mod->maxs[2] = verts[i + 2];
        }
    }

    mod->numVerts = verts.size();
    mod->verts = new float32 [mod->numVerts];
    ::memcpy(mod->verts, verts.data(), mod->numVerts * sizeof(float32));

    mod->numUVs = uvs.size();
    mod->UVs = new float32[mod->numUVs];
    ::memcpy(mod->UVs, uvs.data(), mod->numUVs * sizeof(float32));

    mod->numNormals = normals.size();
    mod->normals = new float32[mod->numNormals];
    ::memcpy(mod->normals, normals.data(), mod->numNormals * sizeof(float32));

    mod->numVerts /= 3;
    mod->numUVs /= 2;
    mod->numNormals /= 3;

    mod->numSurfaces = surfs.size();
    mod->surfaces = new objectSurface_t [mod->numSurfaces];

    for(sint s = 0; s < mod->numSurfaces; s++) {

        objectSurface_t &surfTo = mod->surfaces[s];
        objectWorkSurface &surfFrom = surfs[s];

        if(::strlen(surfFrom.shader)) {
            ::strcpy(surfTo.shader, surfFrom.shader);
        } else {
            ::strcpy(surfTo.shader, defaultShader);
        }

        surfTo.shaderIndex = surfFrom.shaderIndex;

        surfTo.numFaces = surfFrom.faces.size();
        surfTo.faces = new objectFace_t [surfTo.numFaces];

        for(sint f = 0; f < surfTo.numFaces; f++) {

            objectFace_t &faceT = surfTo.faces[f];
            objectWorkFace &faceF = surfFrom.faces[f];

            faceT[0].vertex = &mod->verts[faceF.elements[2].vert * 3];
            faceT[1].vertex = &mod->verts[faceF.elements[1].vert * 3];
            faceT[2].vertex = &mod->verts[faceF.elements[0].vert * 3];

            faceT[0].uv = &mod->UVs[faceF.elements[2].uv * 2];
            faceT[1].uv = &mod->UVs[faceF.elements[1].uv * 2];
            faceT[2].uv = &mod->UVs[faceF.elements[0].uv * 2];

            faceT[0].normal = &mod->normals[faceF.elements[2].normal * 3];
            faceT[1].normal = &mod->normals[faceF.elements[1].normal * 3];
            faceT[2].normal = &mod->normals[faceF.elements[0].normal * 3];
        }
    }

    loaded_models.push_back(mod);

    return mod;
}
