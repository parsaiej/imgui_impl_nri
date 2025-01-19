#ifndef IMGUI_IMPL_NRI_H
#define IMGUI_IMPL_NRI_H

#ifndef IMGUI_DISABLE

#include <NRI/NRI.h>
#include <NRI/NRIDescs.h>

struct ImGui_ImplNRI_InitInfo
{
    nri::Device*       Device;
    nri::CommandQueue* Queue;
    void               (*CheckNRIResultFn)(nri::Result err);
};

IMGUI_IMPL_API bool ImGui_ImplNRI_Init(ImGui_ImplNRI_InitInfo* info);
IMGUI_IMPL_API void ImGui_ImplNRI_Shutdown();
IMGUI_IMPL_API void ImGui_ImplNRI_NewFrame();
IMGUI_IMPL_API void ImGui_ImplNRI_RenderDrawData(ImDrawData* draw_data, nri::CommandBuffer* command_buffer);
IMGUI_IMPL_API void ImGui_ImplNRI_CreateFontsTexture();
IMGUI_IMPL_API void ImGui_ImplNRI_DestroyFontsTexture();
IMGUI_IMPL_API void ImGui_ImplNRI_SetMinImageCount(uint32_t min_image_count);

#endif

#endif
