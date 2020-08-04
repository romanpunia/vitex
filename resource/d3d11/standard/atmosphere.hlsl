#pragma warning(disable: 4000)

struct Scatter
{
    float Sun;
    float Planet;
    float Atmos;
    float3 Rlh;
    float3 Mie;
    float RlhHeight;
    float MieHeight;
    float MieG;
};

float2 GetRSI(in float3 O, in float3 D, in float L)
{
    float A = dot(D, D);
    float B = 2.0 * dot(D, O);
    float C = dot(O, O) - (L * L);
    float R = (B * B) - 4.0 * A * C;

    [branch] if (R < 0.0)
        return float2(1e5, -1e5);

    return float2((-B - sqrt(R)) / (2.0 * A), (-B + sqrt(R)) / (2.0 * A));
}
float3 GetAtmosphere(in float3 V, in float3 O, in float3 P, in Scatter A)
{
    V = normalize(V);

    float2 Offset = GetRSI(O, V, A.Atmos);
    [branch] if (A.Sun <= 0.0 || Offset.x > Offset.y)
        return float3(0, 0, 0);

    Offset.y = min(Offset.y, GetRSI(O, V, A.Planet).x);
    float StepSize = (Offset.y - Offset.x) / float(16);
    float Time = 0.0;
    float3 TotalRlh = float3(0,0,0);
    float3 TotalMie = float3(0,0,0);
    float OdRlh = 0.0;
    float OdMie = 0.0;
    float MU = dot(V, P);
    float MU2 = MU * MU;
    float MieG2 = A.MieG * A.MieG;
    float ResRlh = 3.0 / (16.0 * 3.141592) * (1.0 + MU2);
    float ResMie = 3.0 / (8.0 * 3.141592) * ((1.0 - MieG2) * (MU2 + 1.0)) / (pow(abs(1.0 + MieG2 - 2.0 * MU * A.MieG), 1.5) * (2.0 + MieG2));

    for (int i = 0; i < 16; i++)
    {
        float3 Next = O + V * (Time + StepSize * 0.5);
        float Height = length(Next) - A.Planet;
        float OdStepRlh = exp(-Height / A.RlhHeight) * StepSize;
        float OdStepMie = exp(-Height / A.MieHeight) * StepSize;
        OdRlh += OdStepRlh;
        OdMie += OdStepMie;

        float SubstepSize = GetRSI(Next, P, A.Atmos).y / 8;
        float SubTime = 0.0;
        float SubOdRlh = 0.0;
        float SubOdMie = 0.0;
        
        for (int j = 0; j < 8; j++)
        {
            float3 SubNext = Next + P * (SubTime + SubstepSize * 0.5);
            float SubHeight = length(SubNext) - A.Planet;
            SubOdRlh += exp(-SubHeight / A.RlhHeight) * SubstepSize;
            SubOdMie += exp(-SubHeight / A.MieHeight) * SubstepSize;
            SubTime += SubstepSize;
        }
        
        float3 R = exp(-(A.Mie * (OdMie + SubOdMie) + A.Rlh * (OdRlh + SubOdRlh)));
        TotalRlh += OdStepRlh * R;
        TotalMie += OdStepMie * R;
        Time += StepSize;
    }
    
    return A.Sun * (ResRlh * A.Rlh * TotalRlh + ResMie * A.Mie * TotalMie);
}