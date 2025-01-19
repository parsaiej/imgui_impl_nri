// dear imgui: Renderer Backend for Vulkan
// This needs to be used along with a Platform Backend (e.g. GLFW, SDL, Win32, custom..)

// CHANGELOG
// 2025-01-20: NRI: Initial commit.

#include "imgui_impl_nri.h"

#include <NRI/NRI.h>
#include <NRI/NRIDescs.h>
#include "NRI/Extensions/NRIHelper.h"

#include <cstddef>
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
    nri::PipelineLayout*    PipelineLayout;
    nri::DescriptorSet*     FontDescriptorSet;
    nri::Descriptor*        FontView;
    nri::Texture*           FontImage;
    nri::Memory*            FontMemory;
    nri::Descriptor*        FontSampler;
    ImGui_ImplNRI_Interface Interface;

    ImGui_ImplNRI_Data() { memset((void*)this, 0, sizeof(*this)); }
};

struct ImDrawVertOpt
{
    float    pos[2];
    uint32_t uv;
    uint32_t col;
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

static void check_nri_result(nri::Result err)
{
    ImGui_ImplNRI_Data* bd = ImGui_ImplNRI_GetBackendData();
    if (!bd)
        return;
    ImGui_ImplNRI_InitInfo* v = &bd->NRIInitInfo;
    if (v->CheckNRIResultFn)
        v->CheckNRIResultFn(err);
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
    IM_ASSERT(info->Queue != nullptr);

    bd->NRIInitInfo = *info;

    ImGui_ImplNRI_CreateDeviceObjects();

    return true;
}

void ImGui_ImplNRI_Shutdown()
{
    ImGui_ImplNRI_Data* bd = ImGui_ImplNRI_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplNRI_DestroyDeviceObjects();
    io.BackendRendererName     = nullptr;
    io.BackendRendererUserData = nullptr;
    IM_DELETE(bd);
}

void ImGui_ImplNRI_CreatePipeline(nri::Device* device, nri::Pipeline* pipeline, nri::Format render_target_foramt)
{
    ImGui_ImplNRI_Data* bd = ImGui_ImplNRI_GetBackendData();

    auto& nri = bd->Interface;

    nri::Result err;

    std::array<nri::ShaderDesc, 2> shaders;

    shaders[0].entryPointName = "main";
    shaders[1].entryPointName = "main";

    shaders[0].stage = nri::StageBits::VERTEX_SHADER;
    shaders[1].stage = nri::StageBits::FRAGMENT_SHADER;

    // Select byte-code based on the underlying API that is in use.
    {
        auto deviceInfo = bd->Interface.GetDeviceDesc(*device);

        switch (deviceInfo.graphicsAPI)
        {
            case nri::GraphicsAPI::VK:
            {
                shaders[0].bytecode = UI_VERT_SPV;
                shaders[0].size     = UI_VERT_SPV_LEN;

                shaders[1].bytecode = UI_FRAG_SPV;
                shaders[1].size     = UI_FRAG_SPV_LEN;
            }
            break;
            default: break;
        }
    }

    nri::VertexStreamDesc vertex_stream_info = {};
    {
        vertex_stream_info.bindingSlot = 0;
        vertex_stream_info.stride      = sizeof(ImDrawVertOpt);
    }

    nri::VertexAttributeDesc vertex_attribute_info[3] = {};
    {
        vertex_attribute_info[0].format      = nri::Format::RG32_SFLOAT;
        vertex_attribute_info[0].streamIndex = 0;
        vertex_attribute_info[0].offset      = offsetof(ImDrawVertOpt, pos);
        vertex_attribute_info[0].d3d         = { "POSITION", 0 };
        vertex_attribute_info[0].vk          = { 0 };

        vertex_attribute_info[1].format      = nri::Format::RG16_UNORM;
        vertex_attribute_info[1].streamIndex = 0;
        vertex_attribute_info[1].offset      = offsetof(ImDrawVertOpt, uv);
        vertex_attribute_info[1].d3d         = { "TEXCOORD", 0 };
        vertex_attribute_info[1].vk          = { 1 };

        vertex_attribute_info[2].format      = nri::Format::RGBA8_UNORM;
        vertex_attribute_info[2].streamIndex = 0;
        vertex_attribute_info[2].offset      = offsetof(ImDrawVertOpt, col);
        vertex_attribute_info[2].d3d         = { "COLOR", 0 };
        vertex_attribute_info[2].vk          = { 2 };
    }

    nri::VertexInputDesc vertex_input_info = {};
    {
        vertex_input_info.attributes   = vertex_attribute_info;
        vertex_input_info.attributeNum = 3u;
        vertex_input_info.streams      = &vertex_stream_info;
        vertex_input_info.streamNum    = 1;
    }

    nri::InputAssemblyDesc input_assembly_info = {};
    {
        input_assembly_info.topology = nri::Topology::TRIANGLE_LIST;
    }

    nri::RasterizationDesc rasterization_info = {};
    {
        rasterization_info.fillMode = nri::FillMode::SOLID;
        rasterization_info.cullMode = nri::CullMode::NONE;
    }

    nri::ColorAttachmentDesc color_attachment_info = {};
    {
        color_attachment_info.format         = render_target_foramt;
        color_attachment_info.colorWriteMask = nri::ColorWriteBits::RGBA;
        color_attachment_info.blendEnabled   = true;
        color_attachment_info.colorBlend     = { nri::BlendFactor::SRC_ALPHA, nri::BlendFactor::ONE_MINUS_SRC_ALPHA, nri::BlendFunc::ADD };
        color_attachment_info.alphaBlend     = { nri::BlendFactor::ONE_MINUS_SRC_ALPHA, nri::BlendFactor::ZERO, nri::BlendFunc::ADD };
    }

    nri::OutputMergerDesc output_merger_info = {};
    {
        output_merger_info.colors   = &color_attachment_info;
        output_merger_info.colorNum = 1;
    }

    nri::MultisampleDesc multisample_info = {};
    {
        multisample_info.sampleNum  = 1;
        multisample_info.sampleMask = 0xFFFFFFFF;
    }

    nri::GraphicsPipelineDesc pipeline_info;
    {
        pipeline_info.shaderNum      = shaders.size();
        pipeline_info.shaders        = shaders.data();
        pipeline_info.pipelineLayout = bd->PipelineLayout;
        pipeline_info.outputMerger   = output_merger_info;
        pipeline_info.rasterization  = rasterization_info;
        pipeline_info.inputAssembly  = input_assembly_info;
        pipeline_info.vertexInput    = &vertex_input_info;
        pipeline_info.multisample    = &multisample_info;
    }
    err = nri.CreateGraphicsPipeline(*device, pipeline_info, pipeline);
    check_nri_result(err);
}

bool ImGui_ImplNRI_CreateDeviceObjects()
{
    ImGui_ImplNRI_Data*     bd = ImGui_ImplNRI_GetBackendData();
    ImGui_ImplNRI_InitInfo* v  = &bd->NRIInitInfo;
    nri::Result             err;

    nri::nriGetInterface(*bd->NRIInitInfo.Device, NRI_INTERFACE(nri::CoreInterface), (nri::CoreInterface*)&bd->Interface);
    nri::nriGetInterface(*bd->NRIInitInfo.Device, NRI_INTERFACE(nri::HelperInterface), (nri::HelperInterface*)&bd->Interface);

    if (!bd->FontSampler)
    {
        nri::SamplerDesc sampler_info = {};
        {
            sampler_info.filters.mag    = nri::Filter::LINEAR;
            sampler_info.filters.min    = nri::Filter::LINEAR;
            sampler_info.filters.mip    = nri::Filter::LINEAR;
            sampler_info.addressModes.u = nri::AddressMode::CLAMP_TO_EDGE;
            sampler_info.addressModes.v = nri::AddressMode::CLAMP_TO_EDGE;
            sampler_info.addressModes.w = nri::AddressMode::CLAMP_TO_EDGE;
            sampler_info.mipMin         = -1000;
            sampler_info.mipMax         = +1000;
            sampler_info.anisotropy     = 1.0f;
        }
        err = bd->Interface.CreateSampler(*v->Device, sampler_info, bd->FontSampler);
        check_nri_result(err);
    }

    if (!bd->PipelineLayout)
    {
        nri::DescriptorRangeDesc descriptor_ranges[] = {
            { 0, 1, nri::DescriptorType::TEXTURE, nri::StageBits::FRAGMENT_SHADER },
            { 1, 1, nri::DescriptorType::SAMPLER, nri::StageBits::FRAGMENT_SHADER },
        };

        nri::DescriptorSetDesc descriptorSet = { 0, descriptor_ranges, 2 };

        nri::RootConstantDesc rootConstants = {};
        rootConstants.registerIndex         = 0;
        rootConstants.size                  = 16;
        rootConstants.shaderStages          = nri::StageBits::ALL;

        nri::PipelineLayoutDesc pipeline_layout_info = {};
        {
            pipeline_layout_info.descriptorSetNum = 1;
            pipeline_layout_info.descriptorSets   = &descriptorSet;
            pipeline_layout_info.rootConstantNum  = 1;
            pipeline_layout_info.rootConstants    = &rootConstants;
            pipeline_layout_info.shaderStages     = nri::StageBits::VERTEX_SHADER | nri::StageBits::FRAGMENT_SHADER;
        }
        err = bd->Interface.CreatePipelineLayout(*v->Device, pipeline_layout_info, bd->PipelineLayout);
        check_nri_result(err);
    }

    ImGui_ImplNRI_CreatePipeline(v->Device, bd->Pipeline, nri::Format::RGBA8_UNORM);

    return true;
}

void ImGui_ImplNRI_RemoveTexture(nri::DescriptorSet* descriptor_set)
{
    ImGui_ImplNRI_Data*     bd = ImGui_ImplNRI_GetBackendData();
    ImGui_ImplNRI_InitInfo* v  = &bd->NRIInitInfo;
    // TODO
}

void ImGui_ImplNRI_DestroyFontsTexture()
{
    ImGuiIO&                io = ImGui::GetIO();
    ImGui_ImplNRI_Data*     bd = ImGui_ImplNRI_GetBackendData();
    ImGui_ImplNRI_InitInfo* v  = &bd->NRIInitInfo;

    if (bd->FontDescriptorSet) {}
}

void ImGui_ImplNRI_CreateFontsTexture()
{
    ImGuiIO&                io = ImGui::GetIO();
    ImGui_ImplNRI_Data*     bd = ImGui_ImplNRI_GetBackendData();
    ImGui_ImplNRI_InitInfo* v  = &bd->NRIInitInfo;
    nri::Result             err;

    // Destroy existing texture (if any)
    if (bd->FontView || bd->FontImage || bd->FontMemory || bd->FontDescriptorSet)
    {
        bd->Interface.WaitForIdle(*v->Queue);
        ImGui_ImplNRI_DestroyFontsTexture();
    }
}

void ImGui_ImplNRI_NewFrame()
{
    ImGui_ImplNRI_Data* bd = ImGui_ImplNRI_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplVulkan_Init()?");

    if (!bd->FontDescriptorSet)
        ImGui_ImplNRI_CreateFontsTexture();
}

void ImGui_ImplNRI_RenderDrawData(ImDrawData* draw_data, nri::CommandBuffer* command_buffer)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width  = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplNRI_Data*     bd = ImGui_ImplNRI_GetBackendData();
    ImGui_ImplNRI_InitInfo* v  = &bd->NRIInitInfo;
}

void ImGui_ImplNRI_DestroyDeviceObjects() {}
