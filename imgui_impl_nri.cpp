#include "imgui_impl_nri.h"
#include <imgui.h>

//-----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-----------------------------------------------------------------------------

bool ImGui_ImplNRI_CreateDeviceObjects();
void ImGui_ImplNRI_DestroyDeviceObjects();

struct ImGui_ImplNRI_Data
{
    ImGui_ImplNRI_InitInfo NRIInitInfo;
    nri::Pipeline*         Pipeline;

    ImGui_ImplNRI_Data() { memset((void*)this, 0, sizeof(*this)); }
};

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// TODO

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------

static ImGui_ImplNRI_Data* ImGui_ImplNRI_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplNRI_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

bool ImGui_ImplNRI_Init(ImGui_ImplNRI_InitInfo* info)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    ImGui_ImplNRI_Data* bd     = IM_NEW(ImGui_ImplNRI_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName     = "imgui_impl_nri";

    IM_ASSERT(info->Device != nullptr);

    bd->NRIInitInfo = *info;

    return true;
}

void ImGui_ImplNRI_Shutdown()
{
    ImGui_ImplNRI_Data* bd = ImGui_ImplNRI_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplNRI_DestroyDeviceObjects();
    io.BackendRendererName = nullptr;
    io.BackendRendererName = nullptr;
    IM_DELETE(bd);
}

void ImGui_ImplNRI_DestroyDeviceObjects() {}
