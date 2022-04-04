#include "VectorExtension.h"
#include "MatrixExtension.h"

namespace InsanityEngine::Math::Vector
{
    Types::Vector3f ScreenToWorldPosition(Types::Vector2f position, Types::Vector2f screenResolution, const Types::Matrix4x4f& viewMatrix, const Types::Matrix4x4f& projectionMatrix, float depthMin, float depthMax, float depth)
    {
        const Types::Matrix4x4f viewProjection = Matrix::ViewProjectionMatrix(viewMatrix, projectionMatrix);
        const Types::Matrix4x4f inverse = Matrix::Inverse(viewProjection);

        position /= screenResolution;
        position.y() = 1 - position.y();

        position *= 2;
        position -= Scalar(1);

        const float normalizedDepth = (1 + depthMax) * depth - depthMin;
        Math::Types::Vector4f inversePosition = inverse * Types::Vector4f(position, normalizedDepth, 1);

        return inversePosition / inversePosition.w();
    }
}
