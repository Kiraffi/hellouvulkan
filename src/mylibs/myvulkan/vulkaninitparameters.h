#pragma once

enum class VSyncType : unsigned char
{
    FIFO_VSYNC,
    IMMEDIATE_NO_VSYNC,
    MAILBOX_VSYNC,
};

struct VulkanInitializationParameters
{
    bool useHDR = false;
    bool useIntegratedGpu = false;
    bool useVulkanDebugMarkersRenderDoc = false;
    VSyncType vsync = VSyncType::FIFO_VSYNC;
};
