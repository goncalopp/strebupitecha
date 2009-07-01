#include <stdlib.h>
#include <jack/jack.h>

int min(int a, int b) { if (b<a) return b; else return a; }


struct circularbuffers
    {
    jack_default_audio_sample_t **buffers;
    unsigned long int readposition, bufferlength, bufferbegin, bufferend;
    };

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

jack_default_audio_sample_t *circular_position_data_pointer(struct circularbuffers *bf, int buffer_number)
    {
    return (bf->buffers[buffer_number])+ bf->readposition;
    }

unsigned long int circular_seek(struct circularbuffers *bf, unsigned long int relative_position)
    {
    bf->readposition+=relative_position;
    bf->readposition%= bf->bufferlength;
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

double circular_get_percentage(struct circularbuffers *bf)
    {
    if (bf->readposition >= bf->bufferbegin)
        return ((double) (bf->readposition - bf->bufferbegin)) / ((double)circular_used_space(bf));
    else
        return 1- ((double)(bf->bufferbegin - bf->readposition)) / ((double)circular_used_space(bf));
    
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
        
        for (i=0; i<remaining; i++)
            bf->buffers[buffer_number][i]=source[i+samples_to_end];
        if (remaining>0)
            bf->bufferend= remaining;
        else
            bf->bufferend+=sample_number;
        if (sample_number>=space)
            {
            bf->bufferbegin= (bf->bufferend+1) % bf->bufferlength;
            bf->readposition= bf->bufferbegin;
            }
        return sample_number;
        }
    else
        return circular_write(bf, source, buffer_number, circular_free_space(bf), 0);
    }

unsigned long int circular_readable_continuous(struct circularbuffers *bf)
    {
    unsigned long int readable= bf->bufferend - bf->readposition;
    if (readable<0)
        readable=bf->bufferlength-bf->readposition;
    return readable;
    }

unsigned long int circular_read(struct circularbuffers *bf, jack_default_audio_sample_t *destination, int buffer_number, unsigned long int sample_number)
    {
    
    int readable= circular_readable_continuous(bf);
    if (sample_number>readable)
        sample_number=readable;

    int i;
    for (i=0; i<sample_number; i++)
        destination[i]= bf->buffers[buffer_number][bf->readposition+i];
    bf->readposition+=sample_number;
    bf->readposition%= bf->bufferlength;
    return sample_number;
    }
