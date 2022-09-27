/* cbase64 - public domain base64 encoder/decoder

    Do this:
        #define CBASE64_IMPLEMENTATION
    before you include this file in *one* C or C++ file to create the implementation.

    // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define CBASE64_IMPLEMENTATION
   #include "cbase64.h"

*/

#ifndef CBASE64_H
#define CBASE64_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    step_A, step_B, step_C, step_D
} cbase64_step;

typedef struct
{
    cbase64_step step;
    unsigned char result;
} cbase64_encodestate;

typedef struct
{
    cbase64_step step;
    char result;
} cbase64_decodestate;

void cbase64_init_encodestate(cbase64_encodestate* state_in);
void cbase64_init_decodestate(cbase64_decodestate* state_in);

unsigned int cbase64_calc_encoded_length(unsigned int length_in);
unsigned int cbase64_calc_decoded_length(const char* code_in, unsigned int length_in);

unsigned int cbase64_encode_block(const unsigned char* data_in, unsigned int length_in,
                                  char* code_out, cbase64_encodestate* state_in);

unsigned int cbase64_decode_block(const char* code_in, unsigned int length_in,
                                  unsigned char* data_out, cbase64_decodestate* state_in);

unsigned int cbase64_encode_blockend(char* code_out, cbase64_encodestate* state_in);

#ifdef __cplusplus
}
#endif

#endif // CBASE64_H

#ifdef CBASE64_IMPLEMENTATION

char cbase64__encode_value(unsigned char value_in)
{
    static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    return encoding[(int)value_in];
}

char cbase64__decode_value(char value_in)
{
    static const char decoding[] = {62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51};
    static const char decoding_size = sizeof(decoding);
    value_in -= 43;
    if (value_in < 0 || value_in >= decoding_size) return -1;
    return decoding[(int)value_in];
}

void cbase64_init_encodestate(cbase64_encodestate* state_in)
{
    state_in->step = step_A;
    state_in->result = '\0';
}

void cbase64_init_decodestate(cbase64_decodestate* state_in)
{
    state_in->step = step_A;
    state_in->result = '\0';
}

unsigned int cbase64_calc_encoded_length(unsigned int length_in)
{
    return 4 * (length_in / 3) + ((length_in % 3 != 0) ? 4 : 0);
}

unsigned int cbase64_calc_decoded_length(const char* code_in, unsigned int length_in)
{
    if (length_in == 0 || ((length_in & 3) != 0))
    {
        return 0;
    }
    const char secondlast = code_in[length_in - 2];
    const char last = code_in[length_in - 1];
    return 3 * (length_in / 4) - (secondlast == '=') - (last == '=');
}

unsigned int cbase64_encode_block(const unsigned char* data_in, unsigned int length_in,
                                  char* code_out, cbase64_encodestate* state_in)
{
    const unsigned char* datachar = data_in;
    const unsigned char* const datatextend = data_in + length_in;
    char* codechar = code_out;
    unsigned char result = state_in->result;
    unsigned char fragment;

    switch (state_in->step)
    {
        for (;;)
        {
    case step_A:
            if (datachar == datatextend)
            {
                state_in->step = step_A;
                state_in->result = result;
                return uint32_t(codechar - code_out);
            }
            fragment = *datachar++;
            result = (fragment & 0x0fc) >> 2;
            *codechar++ = cbase64__encode_value(result);
            result = (fragment & 0x003) << 4;
    case step_B:
            if (datachar == datatextend)
            {
                state_in->step = step_B;
                state_in->result = result;
                return uint32_t(codechar - code_out);
            }
            fragment = *datachar++;
            result |= (fragment & 0x0f0) >> 4;
            *codechar++ = cbase64__encode_value(result);
            result = (fragment & 0x00f) << 2;
    case step_C:
            if (datachar == datatextend)
            {
                state_in->step = step_C;
                state_in->result = result;
                return uint32_t(codechar - code_out);
            }
            fragment = *datachar++;
            result |= (fragment & 0x0c0) >> 6;
            *codechar++ = cbase64__encode_value(result);
            result  = (fragment & 0x03f) >> 0;
            *codechar++ = cbase64__encode_value(result);
        }
    }
    // control should not reach here
    return uint32_t(codechar - code_out);
}

unsigned int cbase64_decode_block(const char* code_in, unsigned int length_in,
                                  unsigned char* data_out, cbase64_decodestate* state_in)
{
    const char* codechar = code_in;
    const char* const codeend = code_in + length_in;
    unsigned char* datachar = data_out;
    char fragment;
    char overwrite = state_in->result;
    
    switch (state_in->step)
    {
        for (;;)
        {
    case step_A:
            do {
                if (codechar == codeend)
                {
                    state_in->step = step_A;
                    state_in->result = overwrite;
                    return uint32_t(datachar - data_out);
                }
                fragment = cbase64__decode_value(*codechar++);
            } while (fragment < 0);
            *datachar = (fragment & 0x03f) << 2;
    case step_B:
            do {
                if (codechar == codeend)
                {
                    state_in->step = step_B;
                    state_in->result = overwrite;
                    return uint32_t(datachar - data_out);
                }
                fragment = cbase64__decode_value(*codechar++);
            } while (fragment < 0);
            *datachar++ |= (fragment & 0x030) >> 4;
            overwrite    = (fragment & 0x00f) << 4;
    case step_C:
            do {
                if (codechar == codeend)
                {
                    state_in->step = step_C;
                    state_in->result = overwrite;
                    return uint32_t(datachar - data_out);
                }
                fragment = cbase64__decode_value(*codechar++);
            } while (fragment < 0);
            *datachar++ = overwrite | (fragment & 0x03c) >> 2;
            overwrite   = (fragment & 0x003) << 6;
    case step_D:
            do {
                if (codechar == codeend)
                {
                    state_in->step = step_D;
                    state_in->result = overwrite;
                    return uint32_t(datachar - data_out);
                }
                fragment = cbase64__decode_value(*codechar++);
            } while (fragment < 0);
            *datachar++ = overwrite | (fragment & 0x03f);
        }
    }
    // control should not reach here
    return uint32_t(datachar - data_out);
}

unsigned int cbase64_encode_blockend(char* code_out, cbase64_encodestate* state_in)
{
    char* codechar = code_out;
    switch (state_in->step)
    {
    case step_B:
        *codechar++ = cbase64__encode_value(state_in->result);
        *codechar++ = '=';
        *codechar++ = '=';
        break;
    case step_C:
        *codechar++ = cbase64__encode_value(state_in->result);
        *codechar++ = '=';
        break;
    case step_A:
        break;
    }
    return uint32_t(codechar - code_out);
}

#endif // CBASE64_IMPLEMENTATION
