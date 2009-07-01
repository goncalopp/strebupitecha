#include <jack/jack.h>

struct circularbuffers
    {
    jack_default_audio_sample_t **buffers;
    unsigned long int readposition, bufferlength, bufferbegin, bufferend;
    };

struct circularbuffers *circular_new(
                        int                             buffers_number,
                        unsigned long int               sample_number);

jack_default_audio_sample_t *circular_position_data_pointer(
                        struct circularbuffers          *bf,
                        int                             buffer_number);

unsigned long int circular_seek(
                        struct circularbuffers          *bf,
                        unsigned long int               relative_position);

unsigned long int circular_free_space(
                        struct circularbuffers          *bf);
                        
unsigned long int circular_used_space(
                        struct circularbuffers          *bf);

void circular_seek_percentage(
                        struct circularbuffers          *bf,
                        double                          percentage);

double circular_get_percentage(
                        struct circularbuffers          *bf);

unsigned long int circular_write(
                        struct circularbuffers          *bf,
                        jack_default_audio_sample_t     *source,
                        int                             buffer_number,
                        unsigned long int               sample_number,
                        int                             overwrite);

unsigned long int circular_readable_continuous(
                        struct circularbuffers          *bf);

unsigned long int circular_read(
                        struct circularbuffers          *bf,
                        jack_default_audio_sample_t     *destination,
                        int                             buffer_number,
                        unsigned long int               sample_number);

