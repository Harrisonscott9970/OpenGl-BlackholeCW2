#pragma once
// ModelLoader.h
// Loads all meshes from an Assimp-supported file into a single combined VAO.

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <iostream>
#include "helper/stb/stb_image.h"

struct MeshVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

class ModelLoader
{
public:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint diffuseTex = 0;
    GLuint normalTex = 0;
    int    indexCount = 0;
    bool   loaded = false;

    bool load(const std::string& path)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "[ModelLoader] Assimp error: " << importer.GetErrorString() << "\n";
            return false;
        }

        if (scene->mNumMeshes == 0) {
            std::cerr << "[ModelLoader] No meshes found in: " << path << "\n";
            return false;
        }

        std::vector<MeshVertex>   allVertices;
        std::vector<unsigned int> allIndices;
        std::string dir = path.substr(0, path.find_last_of("/\\") + 1);

        // Process ALL meshes and merge them into one VAO
        for (unsigned int m = 0; m < scene->mNumMeshes; m++)
        {
            aiMesh* mesh = scene->mMeshes[m];
            unsigned int indexOffset = (unsigned int)allVertices.size();

            // Extract vertices from this submesh
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                MeshVertex v;

                v.Position = {
                    mesh->mVertices[i].x,
                    mesh->mVertices[i].y,
                    mesh->mVertices[i].z
                };

                v.Normal = mesh->HasNormals()
                    ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
                    : glm::vec3(0, 1, 0);

                v.TexCoords = mesh->mTextureCoords[0]
                    ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                    : glm::vec2(0.f);

                v.Tangent = mesh->HasTangentsAndBitangents()
                    ? glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z)
                    : glm::vec3(1, 0, 0);

                v.Bitangent = mesh->HasTangentsAndBitangents()
                    ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z)
                    : glm::vec3(0, 0, 1);

                allVertices.push_back(v);
            }

            // Extract indices, offset by how many verts came before
            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                aiFace& face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    allIndices.push_back(face.mIndices[j] + indexOffset);
            }

            // Grab textures from first mesh that has a material
            if (diffuseTex == 0 && mesh->mMaterialIndex < scene->mNumMaterials) {
                aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
                if (diffuseTex == 0)
                    diffuseTex = loadTexFromMaterial(mat, aiTextureType_DIFFUSE, dir);
                if (normalTex == 0)
                    normalTex = loadTexFromMaterial(mat, aiTextureType_NORMALS, dir);
                if (normalTex == 0)
                    normalTex = loadTexFromMaterial(mat, aiTextureType_HEIGHT, dir);
            }
        }

        indexCount = (int)allIndices.size();

        if (allVertices.empty() || allIndices.empty()) {
            std::cerr << "[ModelLoader] Model data empty after processing: " << path << "\n";
            return false;
        }

        // Upload merged geometry to GPU
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        if (VAO == 0 || VBO == 0 || EBO == 0) {
            std::cerr << "[ModelLoader] Failed to create OpenGL buffers for: " << path << "\n";
            return false;
        }

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
            allVertices.size() * sizeof(MeshVertex),
            allVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            allIndices.size() * sizeof(unsigned int),
            allIndices.data(), GL_STATIC_DRAW);

        // layout 0: position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(MeshVertex), (void*)offsetof(MeshVertex, Position));
        glEnableVertexAttribArray(0);

        // layout 1: normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
            sizeof(MeshVertex), (void*)offsetof(MeshVertex, Normal));
        glEnableVertexAttribArray(1);

        // layout 2: uv
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
            sizeof(MeshVertex), (void*)offsetof(MeshVertex, TexCoords));
        glEnableVertexAttribArray(2);

        // layout 3: tangent
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,
            sizeof(MeshVertex), (void*)offsetof(MeshVertex, Tangent));
        glEnableVertexAttribArray(3);

        // layout 4: bitangent
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE,
            sizeof(MeshVertex), (void*)offsetof(MeshVertex, Bitangent));
        glEnableVertexAttribArray(4);

        glBindVertexArray(0);

        loaded = true;
        std::cout << "[ModelLoader] Loaded model: " << path
            << " | meshes=" << scene->mNumMeshes
            << " | verts=" << allVertices.size()
            << " | indices=" << allIndices.size() << "\n";

        return true;
    }

    void draw() const
    {
        if (!loaded || VAO == 0 || indexCount <= 0) return;
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    GLuint loadTexFromMaterial(aiMaterial* mat, aiTextureType type, const std::string& dir)
    {
        if (mat->GetTextureCount(type) == 0) return 0;

        aiString str;
        if (mat->GetTexture(type, 0, &str) != AI_SUCCESS) return 0;

        std::string filename = dir + std::string(str.C_Str());

        int w, h, ch;
        unsigned char* data = stbi_load(filename.c_str(), &w, &h, &ch, 0);
        if (!data) {
            std::cerr << "[ModelLoader] Failed to load texture: " << filename << "\n";
            return 0;
        }

        GLenum format = GL_RGB;
        if (ch == 1) format = GL_RED;
        else if (ch == 3) format = GL_RGB;
        else if (ch == 4) format = GL_RGBA;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        std::cout << "[ModelLoader] Loaded texture: " << filename << "\n";
        return tex;
    }
};