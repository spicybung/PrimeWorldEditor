#include "Core/OpenGL/CShader.h"

#include <Common/FileUtil.h>
#include <Common/Log.h>
#include <Common/TString.h>
#include "Core/GameProject/CResourceStore.h"
#include "Core/Render/CGraphics.h"

#include <cstdint>
#include <fstream>
#include <fmt/format.h>

static bool gDebugDumpShaders = false;
static uint64_t gFailedCompileCount = 0;
static uint64_t gSuccessfulCompileCount = 0;

static void DumpShaderSource(GLuint shader, const std::string& out)
{
    GLint SourceLen = 0;
    glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &SourceLen);
    auto Source = std::make_unique_for_overwrite<GLchar[]>(SourceLen);
    glGetShaderSource(shader, SourceLen, nullptr, Source.get());

    GLint LogLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &LogLen);
    auto pInfoLog = std::make_unique_for_overwrite<GLchar[]>(LogLen);
    glGetShaderInfoLog(shader, LogLen, nullptr, pInfoLog.get());

    std::ofstream ShaderOut(out);

    if (SourceLen > 0)
        ShaderOut.write(Source.get(), SourceLen - 1);
    if (LogLen > 0)
        ShaderOut.write(pInfoLog.get(), LogLen - 1);
}

CShader::CShader()
{
    smNumShaders++;
}

CShader::CShader(std::string_view vertexSource, std::string_view pixelSource)
{
    smNumShaders++;

    CompileVertexSource(vertexSource);
    CompilePixelSource(pixelSource);
    LinkShaders();
}

CShader::~CShader()
{
    if (mVertexShaderExists)
        glDeleteShader(mVertexShader);

    if (mPixelShaderExists)
        glDeleteShader(mPixelShader);

    if (mProgramExists)
        glDeleteProgram(mProgram);

    if (spCurrentShader == this)
        spCurrentShader = nullptr;

    smNumShaders--;
}

bool CShader::CompileVertexSource(std::string_view source)
{
    mVertexShader = glCreateShader(GL_VERTEX_SHADER);

    const auto* srcPtr = source.data();
    const auto srcLen = int(source.size());

    glShaderSource(mVertexShader, 1, &srcPtr, &srcLen);
    glCompileShader(mVertexShader);

    // Shader should be compiled - check for errors
    GLint CompileStatus{};
    glGetShaderiv(mVertexShader, GL_COMPILE_STATUS, &CompileStatus);

    if (CompileStatus == GL_FALSE)
    {
        const auto out = fmt::format("dump/BadVS_{:08d}.txt", gFailedCompileCount);
        DumpShaderSource(mVertexShader, out);
        NLog::Error("Unable to compile vertex shader; dumped to {}", out);

        gFailedCompileCount++;
        glDeleteShader(mVertexShader);
        return false;
    }

    // Debug dump
    if (gDebugDumpShaders)
    {
        const auto out = fmt::format("dump/VS_{:08d}.txt", gSuccessfulCompileCount);
        DumpShaderSource(mVertexShader, out);
        NLog::Debug("Debug shader dumping enabled; dumped to {}", out);

        gSuccessfulCompileCount++;
    }

    mVertexShaderExists = true;
    return true;
}

bool CShader::CompilePixelSource(std::string_view source)
{
    const auto* srcPtr = source.data();
    const auto srcLen = int(source.size());

    mPixelShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(mPixelShader, 1, &srcPtr, &srcLen);
    glCompileShader(mPixelShader);

    // Shader should be compiled - check for errors
    GLint CompileStatus{};
    glGetShaderiv(mPixelShader, GL_COMPILE_STATUS, &CompileStatus);

    if (CompileStatus == GL_FALSE)
    {
        const auto out = fmt::format("dump/BadPS_{:08d}.txt", gFailedCompileCount);
        NLog::Error("Unable to compile pixel shader; dumped to {}", out);
        DumpShaderSource(mPixelShader, out);

        gFailedCompileCount++;
        glDeleteShader(mPixelShader);
        return false;
    }

    // Debug dump
    if (gDebugDumpShaders)
    {
        const auto out = fmt::format("dump/PS_{:08d}.txt", gSuccessfulCompileCount);
        NLog::Debug("Debug shader dumping enabled; dumped to {}", out);
        DumpShaderSource(mPixelShader, out);

        gSuccessfulCompileCount++;
    }

    mPixelShaderExists = true;
    return true;
}

bool CShader::LinkShaders()
{
    if (!mVertexShaderExists || !mPixelShaderExists)
        return false;

    mProgram = glCreateProgram();
    glAttachShader(mProgram, mVertexShader);
    glAttachShader(mProgram, mPixelShader);
    glLinkProgram(mProgram);

    glDeleteShader(mVertexShader);
    glDeleteShader(mPixelShader);
    mVertexShaderExists = false;
    mPixelShaderExists = false;

    // Shader should be linked - check for errors
    GLint LinkStatus{};
    glGetProgramiv(mProgram, GL_LINK_STATUS, &LinkStatus);

    if (LinkStatus == GL_FALSE)
    {
        const auto out = fmt::format("dump/BadLink_{:08d}.txt", gFailedCompileCount);
        NLog::Error("Unable to link shaders. Dumped error log to {}", out);

        GLint LogLen{};
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &LogLen);
        auto pInfoLog = std::make_unique_for_overwrite<GLchar[]>(LogLen);
        glGetProgramInfoLog(mProgram, LogLen, nullptr, pInfoLog.get());

        std::ofstream LinkOut(out);
        if (LogLen > 0)
            LinkOut.write(pInfoLog.get(), LogLen - 1);

        gFailedCompileCount++;
        glDeleteProgram(mProgram);
        return false;
    }

    mMVPBlockIndex = GetUniformBlockIndex("MVPBlock");
    mVertexBlockIndex = GetUniformBlockIndex("VertexBlock");
    mPixelBlockIndex = GetUniformBlockIndex("PixelBlock");
    mLightBlockIndex = GetUniformBlockIndex("LightBlock");
    mBoneTransformBlockIndex = GetUniformBlockIndex("BoneTransformBlock");

    CacheCommonUniforms();
    mProgramExists = true;
    return true;
}

bool CShader::IsValidProgram() const
{
    return mProgramExists;
}

GLuint CShader::GetProgramID() const
{
    return mProgram;
}

GLuint CShader::GetUniformLocation(const char* pkUniform) const
{
    return glGetUniformLocation(mProgram, pkUniform);
}

GLuint CShader::GetUniformBlockIndex(const char* pkUniformBlock) const
{
    return glGetUniformBlockIndex(mProgram, pkUniformBlock);
}

void CShader::UniformBlockBinding(GLuint BlockIndex, GLuint BlockBinding)
{
    if (BlockIndex != GL_INVALID_INDEX)
        glUniformBlockBinding(mProgram, BlockIndex, BlockBinding);
}

void CShader::SetTextureUniforms(uint32_t NumTextures)
{
    for (uint32_t iTex = 0; iTex < NumTextures; iTex++)
        glUniform1i(mTextureUniforms[iTex], iTex);
}

void CShader::SetNumLights(uint32_t NumLights)
{
    glUniform1i(mNumLightsUniform, NumLights);
}

void CShader::SetCurrent()
{
    if (spCurrentShader != this)
    {
        glUseProgram(mProgram);
        spCurrentShader = this;

        UniformBlockBinding(mMVPBlockIndex, CGraphics::MVPBlockBindingPoint());
        UniformBlockBinding(mVertexBlockIndex, CGraphics::VertexBlockBindingPoint());
        UniformBlockBinding(mPixelBlockIndex, CGraphics::PixelBlockBindingPoint());
        UniformBlockBinding(mLightBlockIndex, CGraphics::LightBlockBindingPoint());
        UniformBlockBinding(mBoneTransformBlockIndex, CGraphics::BoneTransformBlockBindingPoint());
    }
}

// ************ STATIC ************
std::unique_ptr<CShader> CShader::FromResourceFile(const TString& rkShaderName)
{
    TString VertexShaderFilename = gDataDir + "resources/shaders/" + rkShaderName + ".vs";
    TString PixelShaderFilename = gDataDir + "resources/shaders/" + rkShaderName + ".ps";
    TString VertexShaderText, PixelShaderText;

    if (!FileUtil::LoadFileToString(VertexShaderFilename, VertexShaderText))
        NLog::Error("Couldn't load vertex shader file for {}", rkShaderName);
    if (!FileUtil::LoadFileToString(PixelShaderFilename, PixelShaderText))
        NLog::Error("Couldn't load pixel shader file for {}", rkShaderName);
    if (VertexShaderText.IsEmpty() || PixelShaderText.IsEmpty())
        return nullptr;

    return std::make_unique<CShader>(VertexShaderText, PixelShaderText);
}

CShader* CShader::CurrentShader()
{
    return spCurrentShader;
}

void CShader::KillCachedShader()
{
    spCurrentShader = nullptr;
}

// ************ PRIVATE ************
void CShader::CacheCommonUniforms()
{
    for (size_t iTex = 0; iTex < 8; iTex++)
    {
        const auto TexUniform = "Texture" + std::to_string(iTex);
        mTextureUniforms[iTex] = glGetUniformLocation(mProgram, TexUniform.c_str());
    }

    mNumLightsUniform = glGetUniformLocation(mProgram, "NumLights");
}
