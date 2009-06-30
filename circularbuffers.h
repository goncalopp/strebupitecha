#include <jack/jack.h>

struct circularbuffers
    {
    jack_default_audio_sample_t **buffers;
    unsigned long int readposition, bufferlength, bufferbegin, bufferend;
    };

struct circularbuffers *circular_new(
                        int                             buffers_number,
                        unsigned long int               sample_number);

unsigned long int circular_free_space(
                        struct circularbuffers        *bf);
                        
unsigned long int circular_readable_length(
                        struct circularbuffers        *bf);

void circular_seek_percentage(
                        struct circularbuffers        *bf,
                        double                          percentage);

unsigned long int circular_write(
                        struct circularbuffers       *bf,
                        jack_default_audio_sample_t     *source,
                        int                             buffer_number,
                        unsigned long int               sample_number,
                        int                             overwrite);

unsigned long int circular_read(
                        struct circularbuffers       *bf,
                        jack_default_audio_sample_t     *destination,
                        int                             buffer_number,
                        unsigned long int               sample_number);

