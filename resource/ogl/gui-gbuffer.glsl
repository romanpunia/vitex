#ifdef VS
layout(binding = 3) uniform VertexBuffer
{
	mat4 WorldViewProjection;
};

layout (location = 0) in vec2 VS_Position;
layout (location = 1) in vec2 VS_TexCoord;
layout (location = 2) in vec4 VS_Color;
out vec2 PS_TexCoord;
out vec4 PS_Color;

void main()
{
	PS_TexCoord = VS_TexCoord;
	PS_Color = VS_Color;
	gl_Position = WorldViewProjection * vec4(VS_Position.xy, 0, 1);
}
#endif
#ifdef PS
layout (location = 0) out vec4 VS_Color;
in vec2 PS_TexCoord;
in vec4 PS_Color;

uniform sampler2D Texture;

void main()
{
	VS_Color = PS_Color * texture(Texture, PS_TexCoord.st);
}
#endif