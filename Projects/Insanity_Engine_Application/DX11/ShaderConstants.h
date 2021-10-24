#pragma once
#include "CommonInclude.h"
#include "Insanity_Math.h"

namespace InsanityEngine::DX11::StaticMesh
{
    namespace Registers
    {
        namespace VS
        {
            inline constexpr UINT appConstants = 0;
            inline constexpr UINT cameraConstants = 1;
            inline constexpr UINT objectConstants = 2;
        }

        namespace PS
        {
            inline constexpr UINT albedoTexture = 0;
            inline constexpr UINT albedoSampler = 0;

            inline constexpr UINT appConstants = 0;
            inline constexpr UINT materialConstants = 2;
        }
    };
    namespace Constants
    {
        struct Application
        {

        };

        struct Camera
        {
            Math::Types::Matrix4x4f viewProjMatrix;
        };

        struct VSMesh
        {
            Math::Types::Matrix4x4f worldMatrix;
        };

        struct PSMaterial
        {
            Math::Types::Vector4f color;
        };
    }
}