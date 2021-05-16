#include "std/channels/effect"
#include "std/core/position"

float Rayprefix(float3 Eye, float3 Direction)
{
    return 1.0 - smoothstep(0.1, 0.75, dot(-Eye, Direction));
}
float Raypostfix(float2 TexCoord, float3 Direction)
{
    float2 Size = smoothstep(0.1, 0.2, TexCoord) * (1.0 - smoothstep(0.9, 1.0, TexCoord));
    return Size.x * Size.y * smoothstep(-0.05, 0.0, dot(GetNormal(TexCoord), -Direction));
}
float Rayreduce(float3 Position, float3 TexCoord, float Power)
{
    float Size = length(GetPosition(TexCoord.xy, TexCoord.z) - Position) * Power;
    return 1.0 / (1.0 + 6.0 * Size * Size);
}
float3 Raysearch(float3 Ray1, float3 Ray2)
{
    const float Steps = 12.0;
    const float Bias = 0.00005;
	float3 Low = Ray1;
	float3 High = Ray2;
	float3 Result = 0.0;

	[unroll] for (float i = 0; i < Steps; i++)
	{
		Result = lerp(Low, High, 0.5);
		float Depth = GetDepth(Result.xy);

		[branch] if (Result.z > Depth + Bias)
			High = Result;
		else
			Low = Result;
	}

    return Result;
}
float3 Raymarch(float3 Position, float3 Direction, float Iterations, float Distance)
{
    const float Bias = 0.00005;
    float Step = Distance / Iterations;
    float3 Ray = Direction * Step;
    float3 Sample = 0.0;

	[loop] for (float i = 0; i < Iterations; i++)
	{
		float3 Next = GetPositionUV(Position + Ray * i);
        [branch] if (!IsInPixelGrid(Next.xy))
            break;

		float Depth = GetDepth(Next.xy);	
		[branch] if (Next.z >= Depth + Bias)
            return Raysearch(Sample, Next);
        
        Sample = Next;
	}

    return -1.0;
}