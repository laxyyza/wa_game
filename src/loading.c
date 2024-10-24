#include "loading.h"
#include "array.h"
#include <wa.h>
#include "file.h"
#include "opengl.h"
#include <stb/stb_image.h>
#include <pthread.h>

#ifdef __linux__
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

typedef void* (*pth_callback_t)(void*);

struct loading
{
	pthread_mutex_t mutex;
	array_t* files;
	u32 file_idx;
	texture_t** textures;
	u32 textures_count;
	u32 textures_loaded;
};

static bool
still_loading(struct loading* data, u32* idx)
{
    pthread_mutex_lock(&data->mutex);

    if (data->file_idx >= data->files->count)
    {
        pthread_mutex_unlock(&data->mutex);
        return false;
    }

    *idx = data->file_idx;
    data->file_idx++;
    pthread_mutex_unlock(&data->mutex);
    return true;
}

static void*
th_load_texture(struct loading* data)
{
    u32 idx;
    while (still_loading(data, &idx))
    {
        char* filepath = *(char**)array_idx(data->files, idx);
        if ((data->textures[idx] = texture_load(filepath)))
        {
            printf("Texture loaded (%u/%u): '%s'\n", 
                   idx + 1, data->files->count, filepath);
        }
        free(filepath);
    }
    return NULL;
}

u32 
get_threads_count(void)
{
#ifdef __linux__
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
#ifdef _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	return sysinfo.dwNumberOfProcessors;
#endif
}

texture_t** 
waapp_load_textures_threaded(const char* dir, u32* count)
{
	struct loading data;
	memset(&data, 0, sizeof(struct loading));
	data.textures_loaded = 1;
	u32 n_threads = get_threads_count();
    pthread_t* threads = calloc(n_threads, sizeof(void*));

    pthread_mutex_init(&data.mutex, NULL);

    data.files = dir_files(dir);
    data.textures_count = data.files->count;

    data.textures = malloc(sizeof(texture_t**) * data.files->count);

    for (u32 i = 0; i < n_threads; i++)
        pthread_create(threads + i, NULL, (pth_callback_t)th_load_texture, &data);

    for (u32 i = 0; i < n_threads; i++)
        pthread_join(threads[i], NULL);

    array_del(data.files);
    free(data.files);
	free(threads);
    pthread_mutex_destroy(&data.mutex);

    for (u32 i = 0; i < data.textures_count; i++)
    {
        texture_t* text = data.textures[i];
        glGenTextures(1, &text->id);
        glBindTexture(GL_TEXTURE_2D, text->id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, text->w, text->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, text->buf);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (text->buf)
            stbi_image_free(text->buf);
    }
	*count = data.textures_count;
	return data.textures;
}
