#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <sndfile.h>

#ifndef LINE_MAX
#define LINE_MAX 1024
#endif

void die(const char msg[]) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void error(const char msg[]) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

void usage() {
    printf("Usage:\n$ audiomerge <spacing in ms> <audio files ...> <output.flac>\n");
    exit(EXIT_FAILURE);
}

void count_to_time(sf_count_t count, int samplerate, int *hour, int *minute, int *second) {
    int glob_seconds = (int)(count / samplerate);
    *hour = glob_seconds / 3600;
    *minute = (glob_seconds - *hour * 3600) / 60;
    *second = (glob_seconds - *hour * 3600 - *minute * 60);
}

int main(int argc, char *argv[]) {
    if (argc < 4) usage();
    
    // open first file to retrieve information for writing output file
    SF_INFO firstinfo = {
        .format = 0,
    };

    SNDFILE *firstfile = sf_open(argv[2], SFM_READ, &firstinfo);
    if (firstfile == NULL) die("sf_open");

    int samplerate = firstinfo.samplerate;
    int channels = firstinfo.channels;
    int format = firstinfo.format;
    sf_count_t spacing = samplerate * atoi(argv[1]) / 1000 * channels;
    float silence[spacing];
    for (int i = 0; i < spacing; i++)
        silence[i] = 0.0f;

    if (sf_close(firstfile)) die("sf_close");

    SF_INFO outinfo = {
        .samplerate = samplerate,
        .channels = channels,
        .format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16,
    };

    SNDFILE *outputfile = sf_open(argv[argc - 1], SFM_WRITE, &outinfo);
    if (outputfile == NULL) die("sf_open");

    sf_count_t target_file_index = 0;

    for (int i = 0; i < argc - 3; i++) {
        // write all files
        int hour, minute, second;
        count_to_time(target_file_index, samplerate, &hour, &minute, &second);
        printf("Appending file: %s: %01d:%02d:%02d\n", argv[i + 2], hour, minute, second);
        fflush(stdout);
        SF_INFO ininfo = {
            .format = 0,
        };
        SNDFILE *infile = sf_open(argv[i + 2], SFM_READ, &ininfo);
        if (infile == NULL) die("sf_open");
        if (ininfo.format != format || ininfo.channels != channels || ininfo.samplerate != samplerate) {
            char msg[LINE_MAX];
            snprintf(msg, sizeof(msg), "Inconsistent file format or channel amount in file <%s>", argv[i+2]);
            error(msg);
        }
        sf_count_t items = 4096 * channels;
        sf_count_t read_items;
        float copy_buf[items];
        do {
            read_items = sf_read_float(infile, copy_buf, items);
            sf_count_t tmp;
            if ((tmp = sf_write_float(outputfile, copy_buf, read_items)) != read_items)
                error("Unknown error while writing output file");
            target_file_index += (tmp / channels);
        } while (read_items == items);
        if (sf_write_float(outputfile, silence, spacing) != spacing)
            error("Error while writing file spacing to output file");
        target_file_index += (spacing / channels);
        if (sf_close(infile)) die("sf_close");
    }
    if (sf_close(outputfile)) die("sf_close");
    printf("Done, no errors!\n");
    return 0;
}
