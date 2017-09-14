#include "loaders/model.h"
#include "math/mat4.h"
#include <stdlib.h>
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

static void assimpNodeTraversal(ModelNode* node, struct aiNode* assimpNode) {

  // Transform
  struct aiMatrix4x4 m = assimpNode->mTransformation;
  aiTransposeMatrix4(&m);
  mat4_set(node->transform, (float*) &m);

  // Children
  node->children = malloc(assimpNode->mNumChildren * sizeof(ModelNode*));
  for (unsigned int n = 0; n < assimpNode->mNumChildren; n++) {
    ModelNode* child = malloc(sizeof(ModelNode));
    child->parent = node;
    assimpNodeTraversal(child, assimpNode->mChildren[n]);
    node->children[n] = child;
  }
}

ModelData* lovrModelDataCreate(Blob* blob) {
  ModelData* modelData = malloc(sizeof(ModelData));
  if (!modelData) return NULL;

  unsigned int flags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_OptimizeGraph | aiProcess_FlipUVs;
  const struct aiScene* scene = aiImportFileFromMemory(blob->data, blob->size, flags, NULL);

  // Figure out how much data we need
  modelData->vertexCount = 0;
  modelData->indexCount = 0;
  modelData->hasNormals = 0;
  modelData->hasUVs = 0;

  for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
    struct aiMesh* assimpMesh = scene->mMeshes[m];
    modelData->vertexCount += assimpMesh->mNumVertices;
    modelData->indexCount += assimpMesh->mNumFaces * 3;
    modelData->hasNormals |= assimpMesh->mNormals != NULL;
    modelData->hasUVs |= assimpMesh->mTextureCoords[0] != NULL;
  }

  // Allocate
  modelData->vertexSize = 3 + (modelData->hasNormals ? 3 : 0) + (modelData->hasUVs ? 2 : 0);
  modelData->vertexData = malloc(modelData->vertexSize * modelData->vertexCount * sizeof(float));
  modelData->indexData = malloc(modelData->indexCount * sizeof(uint32_t));

  // Load
  int vertex = 0;
  int index = 0;
  for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
    struct aiMesh* assimpMesh = scene->mMeshes[m];

    // Vertices
    for (unsigned int v = 0; v < assimpMesh->mNumVertices; v++) {
      modelData->vertexData[vertex++] = assimpMesh->mVertices[v].x;
      modelData->vertexData[vertex++] = assimpMesh->mVertices[v].y;
      modelData->vertexData[vertex++] = assimpMesh->mVertices[v].z;

      if (modelData->hasNormals) {
        if (assimpMesh->mNormals) {
          modelData->vertexData[vertex++] = assimpMesh->mNormals[v].x;
          modelData->vertexData[vertex++] = assimpMesh->mNormals[v].y;
          modelData->vertexData[vertex++] = assimpMesh->mNormals[v].z;
        } else {
          modelData->vertexData[vertex++] = 0;
          modelData->vertexData[vertex++] = 0;
          modelData->vertexData[vertex++] = 0;
        }
      }

      if (modelData->hasUVs) {
        if (assimpMesh->mTextureCoords[0]) {
          modelData->vertexData[vertex++] = assimpMesh->mTextureCoords[0][v].x;
          modelData->vertexData[vertex++] = assimpMesh->mTextureCoords[0][v].y;
        } else {
          modelData->vertexData[vertex++] = 0;
          modelData->vertexData[vertex++] = 0;
        }
      }
    }

    // Indices
    for (unsigned int f = 0; f < assimpMesh->mNumFaces; f++) {
      struct aiFace assimpFace = assimpMesh->mFaces[f];
      lovrAssert(assimpFace.mNumIndices == 3, "Only triangular faces are supported");

      for (unsigned int i = 0; i < assimpFace.mNumIndices; i++) {
        modelData->indexData[index++] = assimpFace.mIndices[i];
      }
    }
  }

  // Nodes
  modelData->root = malloc(sizeof(ModelNode));
  modelData->root->parent = NULL;
  assimpNodeTraversal(modelData->root, scene->mRootNode);

  aiReleaseImport(scene);
  return modelData;
}

void lovrModelDataDestroy(ModelData* modelData) {
  free(modelData->vertexData);
  free(modelData->indexData);

  vec_void_t nodes;
  vec_init(&nodes);
  vec_push(&nodes, modelData->root);
  while (nodes.length > 0) {
    ModelNode* node = vec_first(&nodes);
    vec_pusharr(&nodes, &node->children, node->childCount);
    for (int i = 0; i < node->primitiveCount; i++) {
      free(node->primitives[i]);
    }
    free(node->primitives);
    vec_splice(&nodes, 0, 1);
    free(node);
  }
  vec_deinit(&nodes);

  free(modelData);
}
