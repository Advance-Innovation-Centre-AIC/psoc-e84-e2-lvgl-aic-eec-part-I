/*******************************************************************************
 * File Name:   scope.c
 *
 * Description: AIC-EEC Oscilloscope and Signal Processing Implementation
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *
 * Target: PSoC Edge E84 Evaluation Kit
 *
 ******************************************************************************/

#include "scope.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Include PSoC HAL if available */
#ifdef CY_TARGET_DEVICE
#include "cybsp.h"
#include "cy_pdm_pcm.h"
#include "cy_audioss.h"
#define HW_AUDIO_AVAILABLE 1
#else
#define HW_AUDIO_AVAILABLE 0
#endif

/*******************************************************************************
 * Private Definitions
 ******************************************************************************/

/* Math constants */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TWO_PI (2.0 * M_PI)

/* Audio buffer sizes */
#define DEFAULT_SAMPLE_RATE     (48000U)
#define AUDIO_BUFFER_SIZE       (512U)

/* FFT maximum size */
#define MAX_FFT_SIZE            (1024U)

/* Linear Feedback Shift Register for noise */
#define LFSR_SEED               (0xACE1U)

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* Initialization flags */
static bool scope_initialized = false;
static bool audio_in_initialized = false;
static bool audio_out_initialized = false;
static bool fft_initialized = false;

/* Audio configuration */
static uint32_t audio_sample_rate = DEFAULT_SAMPLE_RATE;
static bool audio_capturing = false;
static bool audio_playing = false;

/* Simulation mode */
static bool audio_simulation_mode = true;
static aic_wave_type_t sim_audio_type = AIC_WAVE_SINE;
static uint32_t sim_audio_freq = 1000;

/* Noise generator state (LFSR) */
static uint16_t lfsr_state = LFSR_SEED;

/* FFT state */
static uint16_t current_fft_size = 256;

/* Phase accumulators for continuous waveform generation (animation) */
static float phase_accumulator = 0.0f;      /* For sine wave */
static uint32_t phase_accumulator_int = 0;  /* For square, triangle, sawtooth */

/* Waveform type names */
static const char* wave_names[AIC_WAVE_COUNT] = {
    "Square",
    "Sine",
    "Triangle",
    "Sawtooth",
    "Noise",
    "Pulse"
};

/* Time division strings (for oscilloscope display) */
static const char* time_div_strings[] = {
    "10us/div", "20us/div", "50us/div", "100us/div", "200us/div",
    "500us/div", "1ms/div", "2ms/div", "5ms/div", "10ms/div"
};

/* Voltage division strings */
static const char* volt_div_strings[] = {
    "10mV/div", "20mV/div", "50mV/div", "100mV/div", "200mV/div",
    "500mV/div", "1V/div", "2V/div", "5V/div", "10V/div"
};

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Generate next LFSR random value (16-bit)
 */
static uint16_t lfsr_next(void)
{
    /* Galois LFSR with taps at 16, 14, 13, 11 */
    uint16_t bit = ((lfsr_state >> 0) ^ (lfsr_state >> 2) ^
                    (lfsr_state >> 3) ^ (lfsr_state >> 5)) & 1;
    lfsr_state = (lfsr_state >> 1) | (bit << 15);
    return lfsr_state;
}

/**
 * @brief Fast integer sine approximation
 * @param phase Phase in range 0-65535 (0 to 2*pi)
 * @return Sine value in range -32767 to 32767
 */
static int16_t fast_sine(uint16_t phase)
{
    /* Use lookup table or polynomial approximation */
    /* This is a simple polynomial approximation */
    float radians = (phase / 65536.0f) * TWO_PI;
    return (int16_t)(sinf(radians) * 32767.0f);
}

/*******************************************************************************
 * Initialization
 ******************************************************************************/

bool aic_scope_init(void)
{
    if (scope_initialized) {
        return true;
    }

    printf("[Scope] Initializing oscilloscope subsystem...\r\n");

    /* Seed noise generator and reset phase accumulators */
    lfsr_state = LFSR_SEED;
    phase_accumulator = 0.0f;
    phase_accumulator_int = 0;

#if HW_AUDIO_AVAILABLE
    /* Hardware initialization would go here */
    audio_simulation_mode = false;
#else
    audio_simulation_mode = true;
#endif

    scope_initialized = true;
    printf("[Scope] Oscilloscope subsystem initialized (%s mode)\r\n",
           audio_simulation_mode ? "simulation" : "hardware");
    return true;
}

void aic_scope_deinit(void)
{
    aic_audio_in_stop();
    aic_audio_out_stop();

    audio_in_initialized = false;
    audio_out_initialized = false;
    fft_initialized = false;
    scope_initialized = false;
}

bool aic_scope_is_ready(void)
{
    return scope_initialized;
}

/*******************************************************************************
 * Waveform Generation Functions
 ******************************************************************************/

/**
 * @brief Generate square wave with duty cycle support
 * @note Uses duty_percent from config when called via aic_scope_generate_wave()
 *       Default 50% duty when called directly
 */
void aic_scope_generate_square(int16_t *buffer, uint16_t count,
                               uint32_t freq_hz, uint32_t sample_rate,
                               int16_t amplitude)
{
    /* Direct call: use 50% duty (backward compatible) */
    aic_scope_generate_square_duty(buffer, count, freq_hz, sample_rate, amplitude, 50);
}

/**
 * @brief Generate square wave with configurable duty cycle
 * @param duty_percent Duty cycle percentage (0-100), typically 10-90 for visible effect
 */
void aic_scope_generate_square_duty(int16_t *buffer, uint16_t count,
                                    uint32_t freq_hz, uint32_t sample_rate,
                                    int16_t amplitude, uint8_t duty_percent)
{
    if (buffer == NULL || count == 0 || freq_hz == 0 || sample_rate == 0) {
        return;
    }

    /* Clamp duty cycle to valid range */
    if (duty_percent > 100) duty_percent = 100;
    if (duty_percent < 1) duty_percent = 1;

    uint32_t samples_per_cycle = sample_rate / freq_hz;
    uint32_t high_samples = (samples_per_cycle * duty_percent) / 100;

    for (uint16_t i = 0; i < count; i++) {
        uint32_t pos = (phase_accumulator_int + i) % samples_per_cycle;
        buffer[i] = (pos < high_samples) ? amplitude : -amplitude;
    }

    /* Update phase accumulator for continuous animation */
    phase_accumulator_int = (phase_accumulator_int + count) % samples_per_cycle;
}

void aic_scope_generate_sine(int16_t *buffer, uint16_t count,
                             uint32_t freq_hz, uint32_t sample_rate,
                             int16_t amplitude)
{
    if (buffer == NULL || count == 0 || freq_hz == 0 || sample_rate == 0) {
        return;
    }

    float phase_increment = TWO_PI * freq_hz / sample_rate;
    float phase = phase_accumulator;

    for (uint16_t i = 0; i < count; i++) {
        buffer[i] = (int16_t)(sinf(phase) * amplitude);
        phase += phase_increment;
        if (phase >= TWO_PI) {
            phase -= TWO_PI;
        }
    }

    phase_accumulator = phase;
}

void aic_scope_generate_triangle(int16_t *buffer, uint16_t count,
                                 uint32_t freq_hz, uint32_t sample_rate,
                                 int16_t amplitude)
{
    if (buffer == NULL || count == 0 || freq_hz == 0 || sample_rate == 0) {
        return;
    }

    uint32_t samples_per_cycle = sample_rate / freq_hz;
    uint32_t quarter_cycle = samples_per_cycle / 4;

    for (uint16_t i = 0; i < count; i++) {
        uint32_t pos = (phase_accumulator_int + i) % samples_per_cycle;
        int32_t value;

        if (pos < quarter_cycle) {
            /* Rising from 0 to max */
            value = (amplitude * pos) / quarter_cycle;
        } else if (pos < 3 * quarter_cycle) {
            /* Falling from max to min */
            value = amplitude - (2 * amplitude * (pos - quarter_cycle)) / (2 * quarter_cycle);
        } else {
            /* Rising from min to 0 */
            value = -amplitude + (amplitude * (pos - 3 * quarter_cycle)) / quarter_cycle;
        }

        buffer[i] = (int16_t)value;
    }

    /* Update phase accumulator for continuous animation */
    phase_accumulator_int = (phase_accumulator_int + count) % samples_per_cycle;
}

void aic_scope_generate_sawtooth(int16_t *buffer, uint16_t count,
                                 uint32_t freq_hz, uint32_t sample_rate,
                                 int16_t amplitude)
{
    if (buffer == NULL || count == 0 || freq_hz == 0 || sample_rate == 0) {
        return;
    }

    uint32_t samples_per_cycle = sample_rate / freq_hz;

    for (uint16_t i = 0; i < count; i++) {
        uint32_t pos = (phase_accumulator_int + i) % samples_per_cycle;
        /* Ramp from -amplitude to +amplitude */
        buffer[i] = (int16_t)(-amplitude + (2 * amplitude * pos) / samples_per_cycle);
    }

    /* Update phase accumulator for continuous animation */
    phase_accumulator_int = (phase_accumulator_int + count) % samples_per_cycle;
}

void aic_scope_generate_noise(int16_t *buffer, uint16_t count,
                              int16_t amplitude)
{
    if (buffer == NULL || count == 0) {
        return;
    }

    for (uint16_t i = 0; i < count; i++) {
        /* Generate random value in range -amplitude to +amplitude */
        int32_t random = (int32_t)lfsr_next() - 32768;
        buffer[i] = (int16_t)((random * amplitude) / 32768);
    }
}

void aic_scope_generate_wave(int16_t *buffer, uint16_t count,
                             const aic_wavegen_config_t *config)
{
    if (buffer == NULL || count == 0 || config == NULL) {
        return;
    }

    switch (config->type) {
        case AIC_WAVE_SQUARE:
            /* Square wave now supports duty cycle */
            aic_scope_generate_square_duty(buffer, count, config->frequency_hz,
                                           config->sample_rate_hz, config->amplitude,
                                           config->duty_percent);
            break;

        case AIC_WAVE_SINE:
            aic_scope_generate_sine(buffer, count, config->frequency_hz,
                                    config->sample_rate_hz, config->amplitude);
            break;

        case AIC_WAVE_TRIANGLE:
            aic_scope_generate_triangle(buffer, count, config->frequency_hz,
                                        config->sample_rate_hz, config->amplitude);
            break;

        case AIC_WAVE_SAWTOOTH:
            aic_scope_generate_sawtooth(buffer, count, config->frequency_hz,
                                        config->sample_rate_hz, config->amplitude);
            break;

        case AIC_WAVE_NOISE:
            aic_scope_generate_noise(buffer, count, config->amplitude);
            break;

        case AIC_WAVE_PULSE:
            /* Pulse wave with adjustable duty cycle */
            {
                uint32_t samples_per_cycle = config->sample_rate_hz / config->frequency_hz;
                uint32_t high_samples = (samples_per_cycle * config->duty_percent) / 100;

                for (uint16_t i = 0; i < count; i++) {
                    uint32_t pos = (phase_accumulator_int + i) % samples_per_cycle;
                    buffer[i] = (pos < high_samples) ? config->amplitude : -config->amplitude;
                }

                /* Update phase accumulator for continuous animation */
                phase_accumulator_int = (phase_accumulator_int + count) % samples_per_cycle;
            }
            break;

        default:
            memset(buffer, 0, count * sizeof(int16_t));
            break;
    }

    /* Apply DC offset */
    if (config->dc_offset != 0) {
        for (uint16_t i = 0; i < count; i++) {
            int32_t value = buffer[i] + config->dc_offset;
            /* Clamp to int16 range */
            if (value > 32767) value = 32767;
            if (value < -32768) value = -32768;
            buffer[i] = (int16_t)value;
        }
    }
}

const char* aic_scope_wave_name(aic_wave_type_t type)
{
    if (type >= AIC_WAVE_COUNT) {
        return "Unknown";
    }
    return wave_names[type];
}

/*******************************************************************************
 * Audio Input Functions (PDM Microphone)
 ******************************************************************************/

bool aic_audio_in_init(uint32_t sample_rate)
{
    audio_sample_rate = sample_rate;

#if HW_AUDIO_AVAILABLE
    /* Hardware PDM/PCM initialization */
    /* Example:
     * cy_en_pdm_pcm_status_t status = Cy_PDM_PCM_Init(CYBSP_PDM_HW, &CYBSP_PDM_config);
     * if (status == CY_PDM_PCM_SUCCESS) {
     *     Cy_PDM_PCM_Channel_Enable(CYBSP_PDM_HW, 2);
     *     audio_in_initialized = true;
     * }
     */
#endif

    audio_in_initialized = true;
    printf("[Scope] Audio input initialized (%u Hz)\r\n", (unsigned int)sample_rate);
    return audio_in_initialized;
}

bool aic_audio_in_start(void)
{
    if (!audio_in_initialized) {
        return false;
    }

#if HW_AUDIO_AVAILABLE
    /* Activate PDM channels */
    /* Cy_PDM_PCM_Activate_Channel(CYBSP_PDM_HW, 2); */
#endif

    audio_capturing = true;
    printf("[Scope] Audio capture started\r\n");
    return true;
}

void aic_audio_in_stop(void)
{
#if HW_AUDIO_AVAILABLE
    /* Deactivate PDM channels */
    /* Cy_PDM_PCM_DeActivate_Channel(CYBSP_PDM_HW, 2); */
#endif

    audio_capturing = false;
    printf("[Scope] Audio capture stopped\r\n");
}

bool aic_audio_in_is_capturing(void)
{
    return audio_capturing;
}

uint16_t aic_audio_in_get_samples(int16_t *buffer, uint16_t count)
{
    if (buffer == NULL || count == 0) {
        return 0;
    }

    if (audio_simulation_mode) {
        /* Generate simulated audio input */
        aic_wavegen_config_t config = {
            .type = sim_audio_type,
            .frequency_hz = sim_audio_freq,
            .sample_rate_hz = audio_sample_rate,
            .amplitude = 16000,
            .dc_offset = 0,
            .duty_percent = 50
        };
        aic_scope_generate_wave(buffer, count, &config);
        return count;
    }

#if HW_AUDIO_AVAILABLE
    /* Read from PDM FIFO */
    /* Example:
     * for (uint16_t i = 0; i < count; i++) {
     *     buffer[i] = (int16_t)Cy_PDM_PCM_Channel_ReadFifo(CYBSP_PDM_HW, 2);
     * }
     * return count;
     */
#endif

    return 0;
}

uint8_t aic_audio_in_get_level(void)
{
    int16_t samples[64];
    uint16_t got = aic_audio_in_get_samples(samples, 64);

    if (got == 0) {
        return 0;
    }

    int32_t rms = aic_signal_rms(samples, got);
    return (uint8_t)((rms * 100) / 32767);
}

/*******************************************************************************
 * Audio Output Functions (I2S DAC)
 ******************************************************************************/

bool aic_audio_out_init(uint32_t sample_rate)
{
    audio_sample_rate = sample_rate;

#if HW_AUDIO_AVAILABLE
    /* Hardware I2S/DAC initialization */
    /* Example:
     * Cy_AudioTDM_Init(TDM_STRUCT0, &tdm_config);
     * mtb_tlv320dac3100_init();
     */
#endif

    audio_out_initialized = true;
    printf("[Scope] Audio output initialized (%u Hz)\r\n", (unsigned int)sample_rate);
    return audio_out_initialized;
}

bool aic_audio_out_start(const int16_t *buffer, uint16_t count, bool loop)
{
    if (!audio_out_initialized || buffer == NULL || count == 0) {
        return false;
    }

    (void)loop;  /* TODO: implement looping */

#if HW_AUDIO_AVAILABLE
    /* Start I2S transmission */
    /* Cy_AudioTDM_ActivateTx(TDM_STRUCT0_TX); */
#endif

    audio_playing = true;
    printf("[Scope] Audio playback started\r\n");
    return true;
}

void aic_audio_out_stop(void)
{
#if HW_AUDIO_AVAILABLE
    /* Stop I2S transmission */
    /* Cy_AudioTDM_DeActivateTx(TDM_STRUCT0_TX); */
#endif

    audio_playing = false;
    printf("[Scope] Audio playback stopped\r\n");
}

bool aic_audio_out_is_playing(void)
{
    return audio_playing;
}

void aic_audio_out_set_volume(uint8_t volume)
{
    if (volume > 100) volume = 100;

#if HW_AUDIO_AVAILABLE
    /* Set DAC volume */
    /* mtb_tlv320dac3100_set_volume(volume); */
#endif

    printf("[Scope] Volume set to %u%%\r\n", volume);
}

/*******************************************************************************
 * Signal Processing Functions
 ******************************************************************************/

int16_t aic_signal_rms(const int16_t *buffer, uint16_t count)
{
    if (buffer == NULL || count == 0) {
        return 0;
    }

    int64_t sum_squares = 0;
    for (uint16_t i = 0; i < count; i++) {
        sum_squares += (int32_t)buffer[i] * buffer[i];
    }

    return (int16_t)sqrtf((float)sum_squares / count);
}

int32_t aic_signal_peak_to_peak(const int16_t *buffer, uint16_t count)
{
    if (buffer == NULL || count == 0) {
        return 0;
    }

    int16_t min_val = buffer[0];
    int16_t max_val = buffer[0];

    for (uint16_t i = 1; i < count; i++) {
        if (buffer[i] < min_val) min_val = buffer[i];
        if (buffer[i] > max_val) max_val = buffer[i];
    }

    return (int32_t)max_val - min_val;
}

int32_t aic_signal_find_trigger(const int16_t *buffer, uint16_t count,
                                int16_t level, bool rising)
{
    if (buffer == NULL || count < 2) {
        return -1;
    }

    for (uint16_t i = 1; i < count; i++) {
        if (rising) {
            /* Rising edge: previous < level, current >= level */
            if (buffer[i - 1] < level && buffer[i] >= level) {
                return i;
            }
        } else {
            /* Falling edge: previous >= level, current < level */
            if (buffer[i - 1] >= level && buffer[i] < level) {
                return i;
            }
        }
    }

    return -1;  /* No trigger found */
}

uint32_t aic_signal_frequency(const int16_t *buffer, uint16_t count,
                              uint32_t sample_rate)
{
    if (buffer == NULL || count < 4 || sample_rate == 0) {
        return 0;
    }

    /* Simple zero-crossing counting method */
    uint32_t zero_crossings = 0;
    int16_t prev = buffer[0];

    for (uint16_t i = 1; i < count; i++) {
        if ((prev < 0 && buffer[i] >= 0) || (prev >= 0 && buffer[i] < 0)) {
            zero_crossings++;
        }
        prev = buffer[i];
    }

    /* Frequency = (zero_crossings / 2) * (sample_rate / count) */
    return (zero_crossings * sample_rate) / (2 * count);
}

void aic_signal_remove_dc(int16_t *buffer, uint16_t count)
{
    if (buffer == NULL || count == 0) {
        return;
    }

    /* Calculate DC offset (average) */
    int32_t sum = 0;
    for (uint16_t i = 0; i < count; i++) {
        sum += buffer[i];
    }
    int16_t dc_offset = sum / count;

    /* Remove DC offset */
    for (uint16_t i = 0; i < count; i++) {
        buffer[i] -= dc_offset;
    }
}

uint16_t aic_signal_downsample(const int16_t *input, uint16_t input_count,
                               int16_t *output, uint16_t output_count)
{
    if (input == NULL || output == NULL || input_count == 0 || output_count == 0) {
        return 0;
    }

    if (output_count >= input_count) {
        /* No downsampling needed, just copy */
        memcpy(output, input, input_count * sizeof(int16_t));
        return input_count;
    }

    /* Simple decimation with averaging */
    float ratio = (float)input_count / output_count;

    for (uint16_t i = 0; i < output_count; i++) {
        uint16_t start = (uint16_t)(i * ratio);
        uint16_t end = (uint16_t)((i + 1) * ratio);
        if (end > input_count) end = input_count;

        int32_t sum = 0;
        for (uint16_t j = start; j < end; j++) {
            sum += input[j];
        }
        output[i] = sum / (end - start);
    }

    return output_count;
}

/*******************************************************************************
 * FFT Functions (Simplified)
 ******************************************************************************/

bool aic_fft_init(uint16_t fft_size)
{
    /* Validate FFT size (must be power of 2) */
    if (fft_size == 0 || (fft_size & (fft_size - 1)) != 0) {
        return false;
    }

    if (fft_size > MAX_FFT_SIZE) {
        return false;
    }

    current_fft_size = fft_size;
    fft_initialized = true;
    printf("[Scope] FFT initialized (size=%u)\r\n", fft_size);
    return true;
}

bool aic_fft_calculate(const int16_t *input, uint16_t *output)
{
    if (!fft_initialized || input == NULL || output == NULL) {
        return false;
    }

    /* Simplified FFT magnitude estimation using DFT for educational purposes */
    /* Note: A real implementation would use CMSIS-DSP arm_cfft_q15 */

    uint16_t bins = current_fft_size / 2;

    for (uint16_t k = 0; k < bins; k++) {
        float real = 0;
        float imag = 0;

        for (uint16_t n = 0; n < current_fft_size; n++) {
            float angle = -TWO_PI * k * n / current_fft_size;
            real += input[n] * cosf(angle);
            imag += input[n] * sinf(angle);
        }

        /* Calculate magnitude */
        output[k] = (uint16_t)sqrtf(real * real + imag * imag);
    }

    return true;
}

uint32_t aic_fft_bin_frequency(uint16_t bin, uint16_t fft_size, uint32_t sample_rate)
{
    return (bin * sample_rate) / fft_size;
}

uint32_t aic_fft_dominant_frequency(const uint16_t *spectrum, uint16_t bin_count,
                                    uint32_t sample_rate)
{
    if (spectrum == NULL || bin_count == 0) {
        return 0;
    }

    /* Find bin with maximum magnitude (skip DC at bin 0) */
    uint16_t max_bin = 1;
    uint16_t max_val = spectrum[1];

    for (uint16_t i = 2; i < bin_count; i++) {
        if (spectrum[i] > max_val) {
            max_val = spectrum[i];
            max_bin = i;
        }
    }

    return aic_fft_bin_frequency(max_bin, bin_count * 2, sample_rate);
}

/*******************************************************************************
 * Ring Buffer Functions
 ******************************************************************************/

void aic_ringbuf_init(aic_ringbuf_t *rb, int16_t *buffer, uint16_t size)
{
    if (rb == NULL || buffer == NULL || size == 0) {
        return;
    }

    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

uint16_t aic_ringbuf_write(aic_ringbuf_t *rb, const int16_t *data, uint16_t count)
{
    if (rb == NULL || data == NULL || count == 0) {
        return 0;
    }

    uint16_t written = 0;

    for (uint16_t i = 0; i < count; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->size;

        if (rb->count < rb->size) {
            rb->count++;
        } else {
            /* Overwrite oldest data */
            rb->tail = (rb->tail + 1) % rb->size;
        }
        written++;
    }

    return written;
}

uint16_t aic_ringbuf_read(aic_ringbuf_t *rb, int16_t *data, uint16_t count)
{
    if (rb == NULL || data == NULL || count == 0) {
        return 0;
    }

    uint16_t to_read = (count < rb->count) ? count : rb->count;

    for (uint16_t i = 0; i < to_read; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % rb->size;
        rb->count--;
    }

    return to_read;
}

uint16_t aic_ringbuf_peek(const aic_ringbuf_t *rb, int16_t *data, uint16_t count)
{
    if (rb == NULL || data == NULL || count == 0) {
        return 0;
    }

    uint16_t to_peek = (count < rb->count) ? count : rb->count;
    uint16_t index = rb->tail;

    for (uint16_t i = 0; i < to_peek; i++) {
        data[i] = rb->buffer[index];
        index = (index + 1) % rb->size;
    }

    return to_peek;
}

uint16_t aic_ringbuf_count(const aic_ringbuf_t *rb)
{
    return (rb != NULL) ? rb->count : 0;
}

void aic_ringbuf_clear(aic_ringbuf_t *rb)
{
    if (rb != NULL) {
        rb->head = 0;
        rb->tail = 0;
        rb->count = 0;
    }
}

/*******************************************************************************
 * Simulation Functions
 ******************************************************************************/

void aic_audio_set_simulation(bool enable)
{
    audio_simulation_mode = enable;
    printf("[Scope] Audio simulation: %s\r\n", enable ? "ON" : "OFF");
}

void aic_audio_in_set_simulated(aic_wave_type_t type, uint32_t freq_hz)
{
    sim_audio_type = type;
    sim_audio_freq = freq_hz;
    printf("[Scope] Simulated input: %s @ %u Hz\r\n",
           aic_scope_wave_name(type), (unsigned int)freq_hz);
}

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

void aic_scope_print_status(void)
{
    printf("\r\n=== Oscilloscope Status ===\r\n");
    printf("Initialized: %s\r\n", scope_initialized ? "Yes" : "No");
    printf("Mode: %s\r\n", audio_simulation_mode ? "Simulation" : "Hardware");
    printf("Sample Rate: %u Hz\r\n", (unsigned int)audio_sample_rate);

    printf("\r\nAudio Input:\r\n");
    printf("  Initialized: %s\r\n", audio_in_initialized ? "Yes" : "No");
    printf("  Capturing: %s\r\n", audio_capturing ? "Yes" : "No");

    printf("\r\nAudio Output:\r\n");
    printf("  Initialized: %s\r\n", audio_out_initialized ? "Yes" : "No");
    printf("  Playing: %s\r\n", audio_playing ? "Yes" : "No");

    printf("\r\nFFT:\r\n");
    printf("  Initialized: %s\r\n", fft_initialized ? "Yes" : "No");
    printf("  Size: %u\r\n", current_fft_size);

    printf("============================\r\n");
}

const char* aic_scope_time_div_str(uint8_t div_index)
{
    if (div_index >= 10) {
        return "Invalid";
    }
    return time_div_strings[div_index];
}

const char* aic_scope_volt_div_str(uint8_t div_index)
{
    if (div_index >= 10) {
        return "Invalid";
    }
    return volt_div_strings[div_index];
}

/* [] END OF FILE */
