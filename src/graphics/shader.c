#include "graphics/shader.h"
#include "graphics/graphics.h"
#include "math/mat4.h"
#include <stdio.h>
#include <stdlib.h>

static const char* lovrShaderVertexPrefix = ""
#ifdef EMSCRIPTEN
"#version 300 es \n"
"precision mediump float; \n"
#else
"#version 150 \n"
#endif
"in vec3 lovrPosition; \n"
"in vec3 lovrNormal; \n"
"in vec2 lovrTexCoord; \n"
"out vec2 texCoord; \n"
"uniform mat4 lovrModel; \n"
"uniform mat4 lovrView; \n"
"uniform mat4 lovrProjection; \n"
"uniform mat4 lovrTransform; \n"
"uniform mat3 lovrNormalMatrix; \n";

static const char* lovrShaderFragmentPrefix = ""
#ifdef EMSCRIPTEN
"#version 300 es \n"
"precision mediump float; \n"
#else
"#version 150 \n"
"in vec4 gl_FragCoord; \n"
#endif
"in vec2 texCoord; \n"
"out vec4 lovrFragColor; \n"
"uniform vec4 lovrColor; \n"
"uniform sampler2D lovrTexture; \n";

static const char* lovrShaderVertexSuffix = ""
"void main() { \n"
"  texCoord = lovrTexCoord; \n"
"  gl_Position = position(lovrProjection, lovrTransform, vec4(lovrPosition, 1.0)); \n"
"}";

static const char* lovrShaderFragmentSuffix = ""
"void main() { \n"
"  lovrFragColor = color(lovrColor, lovrTexture, texCoord); \n"
"}";

static const char* lovrDefaultVertexShader = ""
"vec4 position(mat4 projection, mat4 transform, vec4 vertex) { \n"
"  return projection * transform * vertex; \n"
"}";

static const char* lovrDefaultFragmentShader = ""
"vec4 color(vec4 graphicsColor, sampler2D image, vec2 uv) { \n"
"  return graphicsColor * texture(image, uv); \n"
"}";

static const char* lovrSkyboxVertexShader = ""
"out vec3 texturePosition; \n"
"vec4 position(mat4 projection, mat4 transform, vec4 vertex) { \n"
"  texturePosition = vertex.xyz; \n"
"  return projection * transform * vertex; \n"
"}";

static const char* lovrSkyboxFragmentShader = ""
"in vec3 texturePosition; \n"
"uniform samplerCube cube; \n"
"vec4 color(vec4 graphicsColor, sampler2D image, vec2 uv) { \n"
"  return graphicsColor * texture(cube, texturePosition); \n"
"}";

static const char* lovrFontFragmentShader = ""
"float median(float r, float g, float b) { \n"
"  return max(min(r, g), min(max(r, g), b)); \n"
"} \n"
"vec4 color(vec4 graphicsColor, sampler2D image, vec2 uv) { \n"
"  vec3 col = texture(image, uv).rgb; \n"
"  float sdf = median(col.r, col.g, col.b); \n"
"  float w = fwidth(sdf); \n"
"  float alpha = smoothstep(.5 - w, .5 + w, sdf); \n"
"  return vec4(graphicsColor.rgb, graphicsColor.a * alpha); \n"
"}";

static const char* lovrNoopVertexShader = ""
"vec4 position(mat4 projection, mat4 transform, vec4 vertex) { \n"
"  return vertex; \n"
"}";

static GLuint compileShader(GLenum type, const char* source) {
  GLuint shader = glCreateShader(type);

  glShaderSource(shader, 1, (const GLchar**)&source, NULL);
  glCompileShader(shader);

  int isShaderCompiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
  if (!isShaderCompiled) {
    int logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    char* log = malloc(logLength);
    glGetShaderInfoLog(shader, logLength, &logLength, log);
    lovrThrow("Could not compile shader %s", log);
  }

  return shader;
}

static GLuint linkShaders(GLuint vertexShader, GLuint fragmentShader) {
  GLuint shader = glCreateProgram();

  if (vertexShader) {
    glAttachShader(shader, vertexShader);
  }

  if (fragmentShader) {
    glAttachShader(shader, fragmentShader);
  }

  glBindAttribLocation(shader, LOVR_SHADER_POSITION, "lovrPosition");
  glBindAttribLocation(shader, LOVR_SHADER_NORMAL, "lovrNormal");
  glBindAttribLocation(shader, LOVR_SHADER_TEX_COORD, "lovrTexCoord");

  glLinkProgram(shader);

  int isShaderLinked;
  glGetProgramiv(shader, GL_LINK_STATUS, (int*)&isShaderLinked);
  if (!isShaderLinked) {
    int logLength;
    glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    char* log = malloc(logLength);
    glGetProgramInfoLog(shader, logLength, &logLength, log);
    lovrThrow("Could not link shader %s", log);
  }

  glDetachShader(shader, vertexShader);
  glDeleteShader(vertexShader);
  glDetachShader(shader, fragmentShader);
  glDeleteShader(fragmentShader);

  return shader;
}

static UniformType getUniformType(GLenum type, const char* debug) {
  switch (type) {
    case GL_FLOAT:
    case GL_FLOAT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_FLOAT_VEC4:
      return UNIFORM_FLOAT;

    case GL_INT:
    case GL_INT_VEC2:
    case GL_INT_VEC3:
    case GL_INT_VEC4:
      return UNIFORM_INT;

    case GL_FLOAT_MAT2:
    case GL_FLOAT_MAT3:
    case GL_FLOAT_MAT4:
      return UNIFORM_MATRIX;

    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
      return UNIFORM_SAMPLER;

    default:
      lovrThrow("Unknown uniform type for uniform '%s'", debug);
      return UNIFORM_FLOAT;
  }
}

static int getUniformComponents(GLenum type) {
  switch (type) {
    case GL_FLOAT:
    case GL_INT:
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
      return 1;

    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
    case GL_FLOAT_MAT2:
      return 2;

    case GL_FLOAT_VEC3:
    case GL_INT_VEC3:
    case GL_FLOAT_MAT3:
      return 3;

    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
    case GL_FLOAT_MAT4:
      return 4;

    default:
      return 1;
  }
}

Shader* lovrShaderCreate(const char* vertexSource, const char* fragmentSource) {
  Shader* shader = lovrAlloc(sizeof(Shader), lovrShaderDestroy);
  if (!shader) return NULL;

  // Vertex
  vertexSource = vertexSource == NULL ? lovrDefaultVertexShader : vertexSource;
  char fullVertexSource[4096];
  snprintf(fullVertexSource, sizeof(fullVertexSource), "%s\n%s\n%s", lovrShaderVertexPrefix, vertexSource, lovrShaderVertexSuffix);
  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, fullVertexSource);

  // Fragment
  fragmentSource = fragmentSource == NULL ? lovrDefaultFragmentShader : fragmentSource;
  char fullFragmentSource[4096];
  snprintf(fullFragmentSource, sizeof(fullFragmentSource), "%s\n%s\n%s", lovrShaderFragmentPrefix, fragmentSource, lovrShaderFragmentSuffix);
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fullFragmentSource);

  // Link
  GLuint program = linkShaders(vertexShader, fragmentShader);
  lovrGraphicsBindProgram(program);
  shader->program = program;

  // Store uniform info
  GLint uniformCount;
  GLsizei bufferSize = LOVR_MAX_UNIFORM_LENGTH / sizeof(GLchar);
  map_init(&shader->uniforms);
  glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformCount);
  for (int i = 0; i < uniformCount; i++) {
    Uniform uniform;
    GLenum type;
    glGetActiveUniform(program, i, bufferSize, NULL, &uniform.count, &type, uniform.name);

    char* subscript = strchr(uniform.name, '[');
    if (subscript) {
      *subscript = '\0';
    }

    uniform.index = i;
    uniform.location = glGetUniformLocation(program, uniform.name);
    uniform.type = getUniformType(type, uniform.name);
    uniform.components = getUniformComponents(type);
    uniform.dirty = 1;

    switch (uniform.type) {
      case UNIFORM_FLOAT:
        uniform.size = uniform.components * uniform.count * sizeof(float);
        uniform.value.data = malloc(uniform.size);
        break;

      case UNIFORM_INT:
      case UNIFORM_SAMPLER:
        uniform.size = uniform.components * uniform.count * sizeof(int);
        uniform.value.data = malloc(uniform.size);
        break;

      case UNIFORM_MATRIX:
        uniform.size = uniform.components * uniform.components * uniform.count * sizeof(int);
        uniform.value.data = malloc(uniform.size);
        break;
    }

    memset(uniform.value.data, 0, uniform.size);

    if (uniform.type == UNIFORM_MATRIX) {
      for (int j = 0; j < uniform.components; j++) {
        int index = j * uniform.components + j;
        uniform.value.floats[index] = 1.;
      }
    }

    map_set(&shader->uniforms, uniform.name, uniform);
  }

  lovrShaderBind(shader, 1);

  return shader;
}

Shader* lovrShaderCreateDefault(DefaultShader type) {
  switch (type) {
    case SHADER_DEFAULT: return lovrShaderCreate(NULL, NULL);
    case SHADER_SKYBOX: {
      Shader* shader = lovrShaderCreate(lovrSkyboxVertexShader, lovrSkyboxFragmentShader);
      Uniform* uniform = map_get(&shader->uniforms, "cube");
      if (uniform) {
        uniform->value.ints[0] = 1;
        lovrShaderUpdateUniform(shader, "cube", uniform->value);
      }
      return shader;
    }
    case SHADER_FONT: return lovrShaderCreate(NULL, lovrFontFragmentShader);
    case SHADER_FULLSCREEN: return lovrShaderCreate(lovrNoopVertexShader, NULL);
    default: lovrThrow("Unknown default shader type");
  }
}

void lovrShaderDestroy(const Ref* ref) {
  Shader* shader = containerof(ref, Shader);
  glDeleteProgram(shader->program);
  map_deinit(&shader->uniforms);
  free(shader);
}

void lovrShaderBind(Shader* shader, int force) {
  lovrGraphicsBindProgram(shader->program);

  const char* key;
  map_iter_t iter = map_iter(&shader->uniforms);
  while ((key = map_next(&shader->uniforms, &iter))) {
    Uniform* uniform = map_get(&shader->uniforms, key);

    if (force || uniform->dirty) {
      uniform->dirty = 0;

      switch (uniform->type) {
        case UNIFORM_FLOAT:
          switch (uniform->components) {
            case 1: glUniform1fv(uniform->location, uniform->count, uniform->value.floats); break;
            case 2: glUniform2fv(uniform->location, uniform->count, uniform->value.floats); break;
            case 3: glUniform3fv(uniform->location, uniform->count, uniform->value.floats); break;
            case 4: glUniform4fv(uniform->location, uniform->count, uniform->value.floats); break;
          }
          break;

        case UNIFORM_INT:
        case UNIFORM_SAMPLER:
          switch (uniform->components) {
            case 1: glUniform1iv(uniform->location, uniform->count, uniform->value.ints); break;
            case 2: glUniform2iv(uniform->location, uniform->count, uniform->value.ints); break;
            case 3: glUniform3iv(uniform->location, uniform->count, uniform->value.ints); break;
            case 4: glUniform4iv(uniform->location, uniform->count, uniform->value.ints); break;
          }
          break;

        case UNIFORM_MATRIX:
          switch (uniform->components) {
            case 2: glUniformMatrix2fv(uniform->location, uniform->count, GL_FALSE, uniform->value.floats); break;
            case 3: glUniformMatrix3fv(uniform->location, uniform->count, GL_FALSE, uniform->value.floats); break;
            case 4: glUniformMatrix4fv(uniform->location, uniform->count, GL_FALSE, uniform->value.floats); break;
          }
          break;
      }
    }
  }
}

int lovrShaderGetAttributeId(Shader* shader, const char* name) {
  if (!shader) {
    return -1;
  }

  return glGetAttribLocation(shader->program, name);
}

Uniform* lovrShaderGetUniform(Shader* shader, const char* name) {
  return map_get(&shader->uniforms, name);
}

void lovrShaderUpdateUniform(Shader* shader, const char* name, UniformValue value) {
  Uniform* uniform = map_get(&shader->uniforms, name);
  if (uniform && memcmp(uniform->value.data, value.data, uniform->size)) {
    memcpy(uniform->value.data, value.data, uniform->size);
    uniform->dirty = 1;
  }
}
