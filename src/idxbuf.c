#include "idxbuf.h"
#include "opengl.h"
#include <assert.h>
#include <string.h>

static u32 
idxbuf_sizeof(enum idxbuf_type type)
{
    switch (type)
    {
        case IDXBUF_UINT8:
            return sizeof(GLubyte);
        case IDXBUF_UINT16:
            return sizeof(GLushort);
        case IDXBUF_UINT32:
        default:
            return sizeof(GLuint);
    }
}

static GLenum
idxbuf_type_glenum(enum idxbuf_type type)
{
    switch (type)
    {
        case IDXBUF_UINT8:
            return GL_UNSIGNED_BYTE;
        case IDXBUF_UINT16:
            return GL_UNSIGNED_SHORT;
        case IDXBUF_UINT32:
            return GL_UNSIGNED_INT;
        default:
            assert(false);
    }
}

static void
ib_copy_buffer(u32 dest_id, u32 src_id, u32 size)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, src_id);
    void* src_buf = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dest_id);
    void* dest_buf = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

    memcpy(dest_buf, src_buf, size);

    // Unmap dest 
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    // Unmap src
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, src_id);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

static void 
idxbuf_resize(idxbuf_t* ib, bool copy)
{
    const u32 new_size = ib->array.size;

    if (ib->array.size == ib->size)
        return;

    u32 new_id;

    // Create New Buffer
    glGenBuffers(1, &new_id);
    // Bind
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_id);
    // Allocate new buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, new_size, NULL, GL_DYNAMIC_DRAW);

    // Copy old buffer from new buffer.
    if (copy)
    {
        const u32 copy_size = (new_size > ib->array.size) ? ib->size : new_size;
        ib_copy_buffer(new_id, ib->id, copy_size);
    }

    // Delete old buffer;
    glDeleteBuffers(1, &ib->id);

    // Set new buffer;
    ib->id = new_id;
    ib->size = new_size;
}

static void
ibo_array_resize(array_t* array)
{
    idxbuf_t* ib = array->user_data;
    idxbuf_resize(ib, false);
}

void 
idxbuf_init(idxbuf_t* idxbuf, enum idxbuf_type type, const void* data, u32 count)
{
    idxbuf->type = idxbuf_type_glenum(type);
    idxbuf->size = count * idxbuf_sizeof(type);
    glGenBuffers(1, &idxbuf->id);
    idxbuf_bind(idxbuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxbuf->size, data, GL_DYNAMIC_DRAW);

    array_init(&idxbuf->array, idxbuf_sizeof(type), count);
    array_set_resize_cb(&idxbuf->array, ibo_array_resize, idxbuf);
}

void 
idxbuf_del(idxbuf_t* idxbuf)
{
    glDeleteBuffers(1, &idxbuf->id);
    array_del(&idxbuf->array);
}

void 
idxbuf_bind(const idxbuf_t* idxbuf)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbuf->id);
}

void
idxbuf_submit(const idxbuf_t* ib)
{
    u32 size = ib->array.count * ib->array.ele_size;
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 
                    0, 
                    size,
                    ib->array.buf);
}

void 
idxbuf_unbind(void)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void
idxbuf_memcpy(const idxbuf_t* ib, const void* data, u64 size)
{
    idxbuf_bind(ib);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data);
}
