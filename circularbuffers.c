#include <stdlib.h>
#include <jack/jack.h>
#include "circularbuffers.h"

int min(int a, int b) { if (b<a) return b; else return a; }


struct circularbuffers *circular_new(int buffers_number, unsigned long int sample_number)
    {
    sample_number+=1;   //a circular buffer with n lenght holds n-1 elements
    int i;
    struct circularbuffers *cb= malloc(sizeof(struct circularbuffers));
    cb->buffers= malloc(buffers_number*sizeof(jack_default_audio_sample_t*));
    for (i=0; i< buffers_number; i++)
        cb->buffers[i]=malloc(sample_number*sizeof(jack_default_audio_sample_t));
    cb->readposition=0;
    cb->bufferlength=sample_number;
    cb->bufferbegin=0;
    cb->bufferend=0;
    return cb;
    }

jack_default_audio_sample_t *circular_reading_data_pointer(struct circularbuffers *bf, int buffer_number)
    {
    return (bf->buffers[buffer_number])+ bf->readposition;
    }

jack_default_audio_sample_t *circular_writing_data_pointer(struct circularbuffers *bf, int buffer_number)
    {
    return (bf->buffers[buffer_number])+ bf->bufferend;
    }

unsigned long int circular_writable_continuous(struct circularbuffers *bf)
    {
    return bf->bufferlength - bf->bufferend;    //available, overwriting
    }

unsigned long int circular_write(struct circularbuffers *bf, jack_default_audio_sample_t *source, int buffer_number, unsigned long int sample_number, int overwrite)
    {
    int space=circular_free_space(bf);
    if ((sample_number<=space) || (overwrite))
        {
        int i;
        int samples_to_end= bf->bufferlength - bf->bufferend;
        for (i=0; i<min(samples_to_end, sample_number); i++)
            bf->buffers[buffer_number][bf->bufferend+i]=source[i];
        int remaining= sample_number-samples_to_end;

        if (remaining>0)
            {
            for (i=0; i<remaining; i++)
            bf->buffers[buffer_number][i]=source[i+samples_to_end];
            bf->bufferend= remaining;
            }
        else
            bf->bufferend+=sample_number;
            
        if (sample_number>=space)
            {
            unsigned long int readposition_offset=circular_get_position_offset(bf);
            bf->bufferbegin= (bf->bufferend+1) % bf->bufferlength;
            if (readposition_offset<(sample_number-space))
                bf->readposition= bf->bufferbegin;
            }
        return sample_number;
        }
    else
        return circular_write(bf, source, buffer_number, circular_free_space(bf), 0);
    }

unsigned long int circular_readable_continuous(struct circularbuffers *bf)
    {
    long int readable= bf->bufferend - bf->readposition;
    if (readable< 0)
        readable=bf->bufferlength-bf->readposition;
    return readable;
    }

unsigned long int circular_read(struct circularbuffers *bf, jack_default_audio_sample_t *destination, int buffer_number, unsigned long int sample_number)
    {
    unsigned long int i;
    unsigned long int readable= circular_readable_continuous(bf);
    unsigned long int copying= min(readable, sample_number);
    
    for (i=0; i<copying; i++)
        destination[i]= bf->buffers[buffer_number][bf->readposition+i];
        
    bf->readposition+=copying;
    bf->readposition%= bf->bufferlength;
    
    unsigned long int remaining= sample_number-copying;
    if ((remaining>0) && (circular_readable_continuous(bf)>0))
        remaining=circular_read(bf, destination+readable, buffer_number, remaining);
    return copying+remaining;
    }

unsigned long int circular_seek_relative(struct circularbuffers *bf, unsigned long int relative_position)
    {
    bf->readposition+=relative_position;
    bf->readposition%= bf->bufferlength;
    return 0;                               //warning: seek() does not currently check for bounds
    }

unsigned long int circular_write_seek_relative(struct circularbuffers *bf, unsigned long int relative_position)
    {
    bf->bufferend+=relative_position;
    bf->bufferend%= bf->bufferlength;
    return 0;                               //warning: seek() does not currently check for bounds
    }

unsigned long int circular_used_space(struct circularbuffers *bf)
    {
    if (bf->bufferend >= bf->bufferbegin)
    	return (bf->bufferend - bf->bufferbegin);
    else
    	return (bf->bufferlength - (bf->bufferend - bf->bufferbegin) -2);
    }

unsigned long int circular_free_space(struct circularbuffers *bf)
    {
    return bf->bufferlength-circular_used_space(bf)-1;
    }

void circular_seek_percentage(struct circularbuffers *bf, double percentage)
    {
    bf->readposition= bf->bufferbegin + circular_used_space(bf)*percentage;
    if (bf->readposition>= bf->bufferlength)
	    bf->readposition-=bf->bufferlength;
    }

double circular_get_position_percentage(struct circularbuffers *bf)
    {
    if (bf->readposition >= bf->bufferbegin)
        return ((double) (bf->readposition - bf->bufferbegin)) / ((double)circular_used_space(bf));
    else
        return 1- ((double)(bf->bufferbegin - bf->readposition)) / ((double)circular_used_space(bf));
    }

unsigned long int circular_get_position_offset(struct circularbuffers *bf)
    {
    if (bf->readposition >= bf->bufferbegin)
        return bf->readposition - bf->bufferbegin;
    else
        return bf->bufferlength-(bf->bufferbegin - bf->readposition);
    }

