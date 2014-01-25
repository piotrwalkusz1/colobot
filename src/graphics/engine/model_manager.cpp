#include "graphics/engine/model_manager.h"

#include "app/app.h"

#include "common/logger.h"
#include <common/stringutils.h>

#include "graphics/engine/engine.h"
#include "graphics/engine/model_io.h"

#include <cstdio>
#include <functional>

template<> Gfx::CModelManager* CSingleton<Gfx::CModelManager>::m_instance = nullptr;

namespace Gfx {

CModelManager::CModelManager(CEngine* engine, bool useNewModels)
 : m_engine(engine)
 , m_useNewModels(useNewModels)
{
}

CModelManager::~CModelManager()
{
}

bool CModelManager::LoadModel(const std::string& fileName, bool mirrored)
{
    std::string actualFileName = m_useNewModels
        ? StrUtils::Replace(fileName, ".mod", ".txt")
        : fileName;

    GetLogger()->Debug("Loading model '%s'\n", actualFileName.c_str());

    DataDir dir = m_useNewModels ? DIR_MODEL_NEW : DIR_MODEL;
    std::string filePath = CApplication::GetInstance().GetDataFilePath(dir, actualFileName);

    CModelIO::SetPrintDebugInfo(CApplication::GetInstance().IsDebugModeActive(DEBUG_MODELS));

    CModel model;

    typedef bool (*ReadFunction)(const std::string&, Gfx::CModel&);
    ReadFunction readFunction = m_useNewModels ? ReadFunction(&CModelIO::ReadTextModel) : ReadFunction(&CModelIO::ReadModel);

    if (!readFunction(filePath, model))
    {
        GetLogger()->Error("Loading model '%s' failed\n", filePath.c_str());
        return false;
    }

    ModelInfo modelInfo;
    modelInfo.baseObjRank = m_engine->CreateBaseObject();
    modelInfo.triangles = model.GetRawTriangles();

    if (mirrored)
        Mirror(modelInfo.triangles);

    FileInfo fileInfo(fileName, mirrored);
    m_models[fileInfo] = modelInfo;

    std::vector<VertexTex2> vs(3, VertexTex2());

    for (int i = 0; i < static_cast<int>( modelInfo.triangles.size() ); i++)
    {
        int state = modelInfo.triangles[i].state;
        std::string tex2Name = modelInfo.triangles[i].tex2Name;

        if (modelInfo.triangles[i].variableTex2)
        {
            int texNum = m_engine->GetSecondTexture();

            if (texNum >= 1 && texNum <= 10)
                state |= ENG_RSTATE_DUAL_BLACK;

            if (texNum >= 11 && texNum <= 20)
                state |= ENG_RSTATE_DUAL_WHITE;

            char name[20] = { 0 };
            sprintf(name, "dirty%.2d.png", texNum);
            tex2Name = name;
        }

        vs[0] = modelInfo.triangles[i].p1;
        vs[1] = modelInfo.triangles[i].p2;
        vs[2] = modelInfo.triangles[i].p3;

        m_engine->AddBaseObjTriangles(modelInfo.baseObjRank, vs, ENG_TRIANGLE_TYPE_TRIANGLES,
                                      modelInfo.triangles[i].material, state,
                                      modelInfo.triangles[i].tex1Name, tex2Name,
                                      false);
    }

    return true;
}

bool CModelManager::AddModelReference(const std::string& fileName, bool mirrored, int objRank)
{
    auto it = m_models.find(FileInfo(fileName, mirrored));
    if (it == m_models.end())
    {
        if (!LoadModel(fileName, mirrored))
            return false;

        it = m_models.find(FileInfo(fileName, mirrored));
    }

    m_engine->SetObjectBaseRank(objRank, (*it).second.baseObjRank);

    return true;
}

bool CModelManager::AddModelCopy(const std::string& fileName, bool mirrored, int objRank)
{
    auto it = m_models.find(FileInfo(fileName, mirrored));
    if (it == m_models.end())
    {
        if (!LoadModel(fileName, mirrored))
            return false;

        it = m_models.find(FileInfo(fileName, mirrored));
    }

    int copyBaseObjRank = m_engine->CreateBaseObject();
    m_engine->CopyBaseObject((*it).second.baseObjRank, copyBaseObjRank);
    m_engine->SetObjectBaseRank(objRank, copyBaseObjRank);

    m_copiesBaseRanks.push_back(copyBaseObjRank);

    return true;
}

bool CModelManager::IsModelLoaded(const std::string& fileName, bool mirrored)
{
    return m_models.count(FileInfo(fileName, mirrored)) > 0;
}

int CModelManager::GetModelBaseObjRank(const std::string& fileName, bool mirrored)
{
    auto it = m_models.find(FileInfo(fileName, mirrored));
    if (it == m_models.end())
        return -1;

    return (*it).second.baseObjRank;
}

void CModelManager::DeleteAllModelCopies()
{
    for (int baseObjRank : m_copiesBaseRanks)
    {
        m_engine->DeleteBaseObject(baseObjRank);
    }

    m_copiesBaseRanks.clear();
}

void CModelManager::UnloadModel(const std::string& fileName, bool mirrored)
{
    auto it = m_models.find(FileInfo(fileName, mirrored));
    if (it == m_models.end())
        return;

    m_engine->DeleteBaseObject((*it).second.baseObjRank);

    m_models.erase(it);
}

void CModelManager::UnloadAllModels()
{
    for (auto& mf : m_models)
        m_engine->DeleteBaseObject(mf.second.baseObjRank);

    m_models.clear();
}

void CModelManager::Mirror(std::vector<RawModelTriangle>& triangles)
{
    for (int i = 0; i < static_cast<int>( triangles.size() ); i++)
    {
        VertexTex2  t = triangles[i].p1;
        triangles[i].p1 = triangles[i].p2;
        triangles[i].p2 = t;

        triangles[i].p1.coord.z = -triangles[i].p1.coord.z;
        triangles[i].p2.coord.z = -triangles[i].p2.coord.z;
        triangles[i].p3.coord.z = -triangles[i].p3.coord.z;

        triangles[i].p1.normal.z = -triangles[i].p1.normal.z;
        triangles[i].p2.normal.z = -triangles[i].p2.normal.z;
        triangles[i].p3.normal.z = -triangles[i].p3.normal.z;
    }
}


} // namespace Gfx

