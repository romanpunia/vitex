#include "workflow/pass"

struct Bounce
{
	float Scale;
	float Intensity;
	float Bias;
	float Power;
    float Threshold;
};

Bounce GetBounce(in float S, in float I, in float B, in float P, in float T)
{
    Bounce Result;
    Result.Scale = S;
    Result.Intensity = I;
    Result.Bias = B;
    Result.Power = P;
    Result.Threshold = T;

    return Result;
}
float RayEdgeSample(in float2 TexCoord)
{
    float2 Coord = smoothstep(0.2, 0.6, abs(float2(0.5, 0.5) - TexCoord));
    return clamp(1.0 - (Coord.x + Coord.y), 0.0, 1.0);
}
bool RayMarch(float3 Position, inout float3 Direction, in float IterationCount, out float3 HitCoord)
{
    float3 Coord; float Depth;
    const float Bias = 0.000001;
    Position += Direction;

	[loop] for (int i = 0; i < IterationCount; i++)
	{
		Coord = GetPositionUV(Position);
		Depth = Coord.z - GetDepth(Coord.xy);
        
		[branch] if (abs(Depth) < Bias)
		{
			HitCoord = Coord;
            return true;
		}

        [branch] if (Depth > 0)
            Direction *= 0.5;

		Position -= Direction * sign(Depth);
	}
	
	return false;
}
bool RayBounce(in Fragment Frag, in Bounce Ray, in float2 TexCoord, out float Result)
{
	float3 D = GetPosition(TexCoord, GetDepth(TexCoord)) - Frag.Position;
	float L = length(D) * Ray.Scale;
    float C = dot(normalize(D), Frag.Normal) * Ray.Power - Ray.Bias;
	Result = max(0.0, C) * Ray.Intensity / (1.0 + L * L);

	return C >= Ray.Threshold;
}