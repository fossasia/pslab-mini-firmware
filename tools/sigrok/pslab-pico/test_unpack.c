#include "protocol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void pack_samples(
    uint32_t *words,
    size_t word_count,
    uint8_t const *samples,
    size_t sample_count,
    uint32_t pin_count
)
{
    uint32_t bits_per_word = pslab_pico_bits_per_word(pin_count);
    size_t sample = 0;

    memset(words, 0, word_count * sizeof(*words));
    for (size_t word_index = 0; word_index < word_count && sample < sample_count;
         ++word_index) {
        for (uint32_t shift = 0; shift + pin_count <= bits_per_word &&
                               sample < sample_count;
             shift += pin_count) {
            words[word_index] |= (uint32_t)samples[sample++] << shift;
        }
    }
}

static void run_case(uint32_t pin_count)
{
    uint8_t samples[37];
    uint8_t unpacked[37];
    uint32_t words[37];
    uint32_t mask = (1u << pin_count) - 1u;

    for (size_t i = 0; i < sizeof(samples); ++i) {
        samples[i] = (uint8_t)((i * 5u + pin_count) & mask);
    }

    uint32_t word_count = pslab_pico_word_count(pin_count, sizeof(samples));
    pack_samples(words, word_count, samples, sizeof(samples), pin_count);

    assert(pslab_pico_unpack_words(
        unpacked,
        sizeof(unpacked),
        words,
        word_count,
        pin_count,
        sizeof(samples)
    ));
    assert(memcmp(samples, unpacked, sizeof(samples)) == 0);
}

int main(void)
{
    run_case(1);
    run_case(2);
    run_case(3);
    run_case(4);
    run_case(5);
    run_case(7);
    run_case(8);

    puts("pslab-pico unpack tests passed");
    return 0;
}
