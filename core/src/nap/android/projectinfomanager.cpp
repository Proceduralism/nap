#include <nap/projectinfomanager.h>
#include <nap/core.h>

#include <android/asset_manager.h>

namespace nap
{   
    bool loadProjectInfoFromFile(const Core& core, ProjectInfo& result, utility::ErrorState& errorState) 
    {
        bool loaded = false;

        // TODO ANDROID Check if file exists before attempting to load, allowing us to report on missing project.json

        // TODO ANDROID Temporary project info loading, likely won't handle files over 1MB, needs error handling, code re-use, etc
        AAsset* asset = AAssetManager_open(core.getAndroidAssetManager(), PROJECT_INFO_FILENAME, AASSET_MODE_UNKNOWN);
        if (asset != NULL) 
        {
            long size = AAsset_getLength(asset);
            char* buffer = (char*) malloc (sizeof(char) * size);
            AAsset_read (asset, buffer, size);
            loaded = deserializeProjectInfoJSON(std::string(buffer, size), result, errorState);
            AAsset_close(asset);
        }       

        return errorState.check(loaded, "Unable to open file %s using AssetManager", PROJECT_INFO_FILENAME);
    }
}