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

precision mediump float;
uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 Out_Color;

void main()
{
    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
}
