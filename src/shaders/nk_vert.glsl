/**
 * This GLSL code was copied from the following source:
 * Repository: https://github.com/Immediate-Mode-UI/Nuklear
 * Original file: https://github.com/Immediate-Mode-UI/Nuklear/blob/master/demo/glfw_opengl3/nuklear_glfw_gl3.h
 * 
 * The code was originally embedded within the C header file and has been moved
 * to this separate .glsl file for better organization.
 * 
 * The original code is available under the MIT License or public domain.
 * In this project, the code is used under the MIT License.
 * See: https://github.com/Immediate-Mode-UI/Nuklear/blob/master/LICENSE
 */

#version 450 core

uniform mat4 ProjMtx;

in vec2 Position;
in vec2 TexCoord;
in vec4 Color;

out vec2 Frag_UV;
out vec4 Frag_Color;

void main() 
{
    Frag_UV = TexCoord;
    Frag_Color = Color;
    gl_Position = ProjMtx * vec4(Position.xy, 0, 1);
}
