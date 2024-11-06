#include "framebuffer.h"
#include "opengl.h"

void 
framebuffer_init(fbo_t* fbo, i32 w, i32 h)
{
	fbo->w = w;
	fbo->h = h;

	glGenFramebuffers(1, &fbo->id);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->id);

	texture_init_empty(&fbo->texture, w, h);
	texture_bind(&fbo->texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->texture.id, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		fprintf(stderr, "Framebuffer not complete.\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void 
framebuffer_bind(const fbo_t* fbo)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->id);
	glViewport(0, 0, fbo->w, fbo->h);
}

void 
framebuffer_unbind(ren_t* ren)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, ren->viewport.x, ren->viewport.y);
}
