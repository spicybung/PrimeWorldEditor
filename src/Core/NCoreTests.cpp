#include "Core/NCoreTests.h"

#include "Core/IUIRelay.h"
#include "Core/GameProject/CGameProject.h"
#include "Core/GameProject/CResourceEntry.h"
#include "Core/Resource/Cooker/CResourceCooker.h"
#include <Common/FileUtil.h>
#include <Common/Log.h>
#include <Common/FileIO/CFileInStream.h>
#include <Common/FileIO/CFileOutStream.h>
#include <Common/Math/MathUtil.h>

#include <algorithm>

namespace NCoreTests
{

/** Checks for a parameter in the commandline stream */
static const char* ParseParameter(const char* pkParmName, int argc, char* argv[])
{
    const auto kParmLen = strlen(pkParmName);

    for (int i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], pkParmName, kParmLen) == 0)
        {
            // Found the parameter. Make sure there is enough space in the
            // string for the parameter value before returning it.
            if (strlen(argv[i]) >= kParmLen + 2 &&
                argv[i][kParmLen] == '=')
            {
                return &argv[i][kParmLen + 1];
            }
        }
    }

    // Couldn't find the parameter.
    return nullptr;
}

/** Checks for the existence of a token in the commandline stream */
static bool ParseToken(const char* pkToken, int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], pkToken) == 0)
        {
            return true;
        }
    }

    // Couldn't find the token.
    return false;
}

/** Check commandline input to see if the user is running a test */
bool RunTests(int argc, char* argv[])
{
    if (ParseToken("ValidateCooker", argc, argv))
    {
        // Fetch parameters
        const char* pkType = ParseParameter("-type", argc, argv);
        EResourceType Type = TEnumReflection<EResourceType>::ConvertStringToValue(pkType);
        bool AllowDump = ParseToken("-allowdump", argc, argv);

        if (Type == EResourceType::Invalid)
        {
            gpUIRelay->ShowMessageBox("ValidateCooker", "Usage: ValidateCooker -type=<ResourceType> [-allowdump] [-project=<Project>]");
        }
        else if (gpUIRelay->OpenProject(ParseParameter("-project", argc, argv)))
        {
            ValidateCooker(Type, AllowDump);
        }
        return true;
    }

    // No test being run.
    return false;
}

/** Validate all cooker output for the given resource type matches the original asset data */
bool ValidateCooker(EResourceType ResourceType, bool DumpInvalidFileContents)
{
    NLog::Debug("Validating output of {} cooker...",
                TEnumReflection<EResourceType>::ConvertValueToString(ResourceType));

    // There must be a project loaded
    CResourceStore* pStore = gpResourceStore;
    CGameProject* pProject = (pStore ? pStore->Project() : nullptr);

    if (!pProject)
    {
        NLog::Error("Cooker unit test failed; no project loaded");
        return false;
    }

    TString ResourcesDir = pProject->ResourcesDir(false);
    uint32_t NumValid = 0, NumInvalid = 0;

    // Iterate through all resources
    for (const auto& It : MakeResourceView(pStore))
    {
        if (It->ResourceType() != ResourceType || !It->HasCookedVersion())
            continue;

        // Get original cooked data
        TString CookedPath = It->CookedAssetPath(true);
        CFileInStream FileStream(ResourcesDir / CookedPath, std::endian::big);

        if (!FileStream.IsValid())
            continue;

        std::vector<uint8_t> OriginalData(FileStream.Size());
        FileStream.ReadBytes(OriginalData.data(), OriginalData.size());
        FileStream.Close();

        // Generate new cooked data
        std::vector<char> NewData;
        CVectorOutStream MemoryStream(&NewData, std::endian::big);
        CResourceCooker::CookResource(It.get(), MemoryStream);

        // Start our comparison by making sure the sizes match up
        const size_t kAlignment         = (It->Game() >= EGame::Corruption ? 64 : 32);
        const auto kAlignedOriginalSize = Math::Align(OriginalData.size(), kAlignment);
        const auto kAlignedNewSize      = Math::Align(NewData.size(), kAlignment);
        const char* pkInvalidReason     = "";
        bool IsValid                    = false;

        if (kAlignedOriginalSize == kAlignedNewSize &&
            OriginalData.size() >= NewData.size())
        {
            // Compare actual data. Note that the original asset can have alignment padding
            // at the end, which is applied by the pak but usually preserved in extracted
            // files. We do not include this in the comparison as missing padding does not
            // indicate malformed data.
            const auto DataSize = std::min(OriginalData.size(), NewData.size());

            if (memcmp(OriginalData.data(), NewData.data(), DataSize) == 0)
            {
                // Verify any missing data at the end is padding.
                bool MissingData = false;

                if (OriginalData.size() > NewData.size())
                {
                    for (size_t i = DataSize; i < OriginalData.size(); i++)
                    {
                        if (OriginalData[i] != 0xFF)
                        {
                            MissingData = true;
                            break;
                        }
                    }
                }

                if (!MissingData)
                {
                    // All tests passed!
                    IsValid = true;
                }
                else
                {
                    pkInvalidReason = "missing data";
                }
            }
            else
            {
                pkInvalidReason = "data mismatch";
            }
        }
        else
        {
            pkInvalidReason = "size mismatch";
        }

        // Print test results
        if (IsValid)
        {
            NLog::Debug("[SUCCESS] {}", *CookedPath);
            NumValid++;
        }
        else
        {
            NLog::Debug("[FAILED: {}] {}", pkInvalidReason, *CookedPath);
            NumInvalid++;
        }

        if (DumpInvalidFileContents)
        {
            TString DumpPath = "dump" / CookedPath;
            FileUtil::MakeDirectory(DumpPath.GetFileDirectory());

            CFileOutStream DumpFile(DumpPath, std::endian::big);
            DumpFile.WriteBytes(NewData.data(), NewData.size());
            DumpFile.Close();
        }

        if (NumInvalid >= 100)
        {
            NLog::Debug("Test aborted; at least 100 invalid resources. Checked {} resources, {} passed, {} failed",
                        NumValid + NumInvalid, NumValid, NumInvalid);
            return false;
        }
    }

    // Test complete
    bool TestSuccess = (NumInvalid == 0);
    NLog::Debug("Test {}; checked {} resources, {} passed, {} failed",
                TestSuccess ? "SUCCEEDED" : "FAILED",
                NumValid + NumInvalid, NumValid, NumInvalid);

    return TestSuccess;
}

} // end namespace NCoreTests
