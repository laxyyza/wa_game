#include "texture.h"
#include "opengl.h"
#include <stb/stb_image.h>

texture_t* 
texture_load(const char* filename, enum filter filter)
{
    texture_t* texture = calloc(1, sizeof(texture_t));
    texture_init(texture, filename, filter);
    return texture;
}

void 
texture_init(texture_t* text, const char* filename, enum filter filter)
{
    stbi_set_flip_vertically_on_load(0);
    text->buf = stbi_load(filename, &text->w, &text->h, &text->bpp, 4);

    glGenTextures(1, &text->id);
    glBindTexture(GL_TEXTURE_2D, text->id);

	i32 gl_filter = (filter == TEXTURE_LINEAR) ? GL_LINEAR : GL_NEAREST;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, text->w, text->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, text->buf);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (text->buf)
        stbi_image_free(text->buf);
}

void 
texture_del(texture_t* text)
{
    glDeleteTextures(1, &text->id);
    free(text);
}

void 
texture_bind(const texture_t* text, u32 slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    debug("Binding texture ID: %u into slot: %u\n",
           text->id, slot);
    // glBindTextureUnit(slot, text->id);
    glBindTexture(GL_TEXTURE_2D, text->id);
}

void 
texture_unbind(void)
{
    glBindTexture(GL_TEXTURE_2D, 0);
}
