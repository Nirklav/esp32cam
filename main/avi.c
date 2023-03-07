#include "private_include/avi.h"

static AviElement avi_chunk_start(char* four_cc, FILE* file)
{
    fwrite(four_cc, 1, 4, file);

    long size_position = ftell(file);
    uint32_t zero = 0;
    fwrite(&zero, 4, 1, file);

    AviElement e = { size_position };
    return e;
}

static AviElement avi_list_start(char* list, char* four_cc, FILE* file)
{
    fwrite(list, 1, 4, file);

    long size_position = ftell(file);
    uint32_t zero = 0;
    fwrite(&zero, 4, 1, file);

    fwrite(four_cc, 1, 4, file);

    AviElement e = { size_position };
    return e;
}

static void avi_element_close(AviElement* chunk, FILE* file)
{
    long end_position = ftell(file);
    fseek(file, chunk->size_position, 0);

    uint32_t chunk_size = end_position - chunk->size_position - 4;
    fwrite(&chunk_size, 4, 1, file);

    fseek(file, end_position, 0);
}

static void avi_main_header(uint32_t micro_sec_per_frame, uint32_t frames, uint32_t width, uint32_t height, FILE* file)
{
    uint32_t max_bytes_per_sec = 3728000;
    uint32_t padding_granularity = 0;
    uint32_t flags = 0;
    uint32_t total_frames = frames;
    uint32_t initial_frames = 0;
    uint32_t streams = 1;
    uint32_t suggested_buffer_size = 140107;
    uint32_t zero = 0;

    fwrite(&micro_sec_per_frame, sizeof(micro_sec_per_frame), 1, file);
    fwrite(&max_bytes_per_sec, sizeof(max_bytes_per_sec), 1, file);
    fwrite(&padding_granularity, sizeof(padding_granularity), 1, file);
    fwrite(&flags, sizeof(flags), 1, file);
    fwrite(&total_frames, sizeof(total_frames), 1, file);
    fwrite(&initial_frames, sizeof(initial_frames), 1, file);
    fwrite(&streams, sizeof(streams), 1, file);
    fwrite(&suggested_buffer_size, sizeof(suggested_buffer_size), 1, file);
    fwrite(&width, sizeof(width), 1, file);
    fwrite(&height, sizeof(height), 1, file);

    fwrite(&zero, sizeof(zero), 1, file);
    fwrite(&zero, sizeof(zero), 1, file);
    fwrite(&zero, sizeof(zero), 1, file);
    fwrite(&zero, sizeof(zero), 1, file);
}

static void avi_stream_header(uint32_t fps, uint32_t frames, uint16_t width, uint16_t height, FILE* file)
{
    uint32_t flags = 0;
    uint16_t priority = 0;
    uint16_t language = 0;
    uint32_t initial_frames = 0;
    uint32_t scale = 1;
    uint32_t start = 0;
    uint32_t suggested_buffer_size = 140107;
    uint32_t quality = 0;
    uint32_t sample_size = 0;
    uint16_t frame_start_w = 0;
    uint16_t frame_start_h = 0;
    uint16_t frame_end_w = width;
    uint16_t frame_end_h = height;

    fwrite("vids", 1, 4, file);
    fwrite("mjpg", 1, 4, file);
    fwrite(&flags, sizeof(flags), 1, file);
    fwrite(&priority, sizeof(priority), 1, file);
    fwrite(&language, sizeof(language), 1, file);
    fwrite(&initial_frames, sizeof(initial_frames), 1, file);
    fwrite(&scale, sizeof(scale), 1, file);
    fwrite(&fps, sizeof(fps), 1, file);
    fwrite(&start, sizeof(start), 1, file);
    fwrite(&frames, sizeof(frames), 1, file);
    fwrite(&suggested_buffer_size, sizeof(suggested_buffer_size), 1, file);
    fwrite(&quality, sizeof(quality), 1, file);
    fwrite(&sample_size, sizeof(sample_size), 1, file);
    fwrite(&frame_start_w, sizeof(frame_start_w), 1, file);
    fwrite(&frame_start_h, sizeof(frame_start_h), 1, file);
    fwrite(&frame_end_w, sizeof(frame_end_w), 1, file);
    fwrite(&frame_end_h, sizeof(frame_end_h), 1, file);
}

static void avi_stream_format(int32_t width, int32_t height, FILE* file)
{
    uint32_t size = 40;
    uint16_t planes = 1;
    uint16_t bit_count = 24;
    uint32_t compression = 1196444237;
    uint32_t size_image = 5760000;
    int32_t x_pels_per_meter = 0;
    int32_t y_pels_per_meter = 0;
    uint32_t clr_used = 0;
    uint32_t clr_important = 0;

    fwrite(&size, sizeof(size), 1, file);
    fwrite(&width, sizeof(width), 1, file);
    fwrite(&height, sizeof(height), 1, file);
    fwrite(&planes, sizeof(planes), 1, file);
    fwrite(&bit_count, sizeof(bit_count), 1, file);
    fwrite(&compression, sizeof(compression), 1, file);
    fwrite(&size_image, sizeof(size_image), 1, file);
    fwrite(&x_pels_per_meter, sizeof(x_pels_per_meter), 1, file);
    fwrite(&y_pels_per_meter, sizeof(y_pels_per_meter), 1, file);
    fwrite(&clr_used, sizeof(clr_used), 1, file);
    fwrite(&clr_important, sizeof(clr_important), 1, file);
}

Avi avi_mjpeg_start(uint32_t fps, uint32_t frames, uint16_t width, uint16_t height, FILE* file)
{
    AviElement avi_list = avi_list_start("RIFF", "AVI ", file);
    {
        AviElement hdrl_list = avi_list_start("LIST", "hdrl", file);
        {
            AviElement avih = avi_chunk_start("avih", file);
            {
                uint32_t micro_sec_per_frame = 1000000 / fps;
                avi_main_header(micro_sec_per_frame, frames, width, height, file);
            }
            avi_element_close(&avih, file);

            AviElement strl = avi_list_start("LIST", "strl", file);
            {
                AviElement strh = avi_chunk_start("strh", file);
                {
                    avi_stream_header(fps, frames, width, height, file);
                }
                avi_element_close(&strh, file);

                AviElement strf = avi_chunk_start("strf", file);
                {
                    avi_stream_format(width, height, file);
                }
                avi_element_close(&strf, file);
            }
            avi_element_close(&strl, file);
        }
        avi_element_close(&hdrl_list, file);

        AviElement movi_list = avi_list_start("LIST", "movi", file);

        Avi avi = { avi_list, movi_list };
        return avi;
    }
}

void avi_mjpeg_frame(Avi* avi, uint8_t* buffer, size_t len, FILE* file)
{
    AviElement dc = avi_chunk_start("00dc", file);
    {
        fwrite(buffer, 1, len, file);
    }
    avi_element_close(&dc, file);
}

void avi_close(Avi* avi, FILE* file)
{
    avi_element_close(&avi->movi, file);
    avi_element_close(&avi->avi, file);
}