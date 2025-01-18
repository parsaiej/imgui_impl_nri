// dear imgui: Renderer Backend for Vulkan
// This needs to be used along with a Platform Backend (e.g. GLFW, SDL, Win32, custom..)

// CHANGELOG
// 2025-01-20: NRI: Initial commit.

#include "imgui_impl_nri.h"
#include "NRI/NRIDescs.h"
#include <imgui.h>

//-----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-----------------------------------------------------------------------------

bool ImGui_ImplNRI_CreateDeviceObjects();
void ImGui_ImplNRI_DestroyDeviceObjects();
void ImGui_ImplVulkan_CreatePipeline(nri::Device* device, nri::Pipeline* pipeline);

// clang-format off
struct ImGui_ImplNRI_Interface
    : public nri::CoreInterface,
      public nri::HelperInterface {};
// clang-format on

struct ImGui_ImplNRI_Data
{
    ImGui_ImplNRI_InitInfo  NRIInitInfo;
    nri::Pipeline*          Pipeline;
    ImGui_ImplNRI_Interface Interface;

    ImGui_ImplNRI_Data() { memset((void*)this, 0, sizeof(*this)); }
};

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// SPIR-V
#include "shader/header/spv/UI.vert.spv.h"
#include "shader/header/spv/UI.frag.spv.h"

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
    IM_DELETE(bd);
}

void ImGui_ImplNRI_CreatePipeline(nri::Device* device, nri::Pipeline* pipeline)
{
    ImGui_ImplNRI_Data* bd = ImGui_ImplNRI_GetBackendData();

    auto& nri = bd->Interface;

    nri::GraphicsPipelineDesc temp;
    {
    }
    nri.CreateGraphicsPipeline(*device, temp, pipeline);
}

bool ImGui_ImplNRI_CreateDeviceObjects()
{
    ImGui_ImplNRI_Data*     bd        = ImGui_ImplNRI_GetBackendData();
    ImGui_ImplNRI_InitInfo* init_info = &bd->NRIInitInfo;

    nri::nriGetInterface(*bd->NRIInitInfo.Device, NRI_INTERFACE(nri::CoreInterface), (nri::CoreInterface*)&bd->Interface);
    nri::nriGetInterface(*bd->NRIInitInfo.Device, NRI_INTERFACE(nri::HelperInterface), (nri::HelperInterface*)&bd->Interface);

    return true;
}

void ImGui_ImplNRI_DestroyDeviceObjects() {}
