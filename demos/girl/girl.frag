#version 330 core

layout (location = 0) out vec4 color;

uniform vec4 u_Color;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;

in vec2 v_TexCoord;

void main() {
    vec4 tex1Color = texture(u_Texture1, v_TexCoord);
    vec4 tex2Color = texture(u_Texture2, v_TexCoord);
    color = mix(tex1Color, tex2Color, u_Color.w);
    color = mix(color, vec4(u_Color.xyz, 1.0), 0.3);
}
