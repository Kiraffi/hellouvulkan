#pragma once

enum class VSyncType : unsigned char
{
    FIFO_VSYNC,
    IMMEDIATE_NO_VSYNC,
    MAILBOX_VSYNC,
};

struct VulkanInitializationParameters
{
    bool showInfoMessages = false;
    bool useHDR = false;
    bool useIntegratedGpu = false;
    bool useValidationLayers = true;
    bool useVulkanDebugMarkersRenderDoc = false;
    VSyncType vsync = VSyncType::FIFO_VSYNC;
};
