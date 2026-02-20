/*******************************************************************************
 * File Name:   scope.h
 *
 * Description: AIC-EEC Oscilloscope and Signal Processing API
 *              Embedded Systems Engineering, Faculty of Engineering,
 *              Burapha University
 *
 * Author:      Assoc. Prof. Wiroon Sriborrirux (wiroon@eng.buu.ac.th)
 *
 * Components:
 *   - Waveform Generation (Square, Sine, Triangle, Sawtooth, Noise)
 *   - Audio Input (PDM Microphone)
 *   - Audio Output (I2S DAC)
 *   - Signal Processing (FFT)
 *
 * Target: PSoC Edge E84 Evaluation Kit
 *
 * Usage:
 *   #include "scope.h"
 *   aic_scope_init();
 *   aic_scope_generate_sine(buffer, 100, 1000, 48000, 32767);
 *
 ******************************************************************************/

#ifndef AIC_SCOPE_H
#define AIC_SCOPE_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Waveform Type Definitions
 ******************************************************************************/

/** Waveform types for generation */
typedef enum {
    AIC_WAVE_SQUARE = 0,    /**< Square wave (50% duty cycle) */
    AIC_WAVE_SINE,          /**< Sine wave */
    AIC_WAVE_TRIANGLE,      /**< Triangle wave */
    AIC_WAVE_SAWTOOTH,      /**< Sawtooth wave (ramp up) */
    AIC_WAVE_NOISE,         /**< White noise */
    AIC_WAVE_PULSE,         /**< Pulse wave (adjustable duty) */
    AIC_WAVE_COUNT          /**< Number of waveform types */
} aic_wave_type_t;

/** Audio source types */
typedef enum {
    AIC_AUDIO_NONE = 0,     /**< No audio source */
    AIC_AUDIO_MIC,          /**< PDM Microphone input */
    AIC_AUDIO_DAC,          /**< I2S DAC output (loopback) */
    AIC_AUDIO_GENERATED     /**< Software-generated waveform */
} aic_audio_source_t;

/*******************************************************************************
 * Oscilloscope Configuration
 ******************************************************************************/

/** Oscilloscope display configuration */
typedef struct {
    uint16_t buffer_size;       /**< Number of samples to display */
    uint32_t sample_rate_hz;    /**< Display sample rate */
    int16_t trigger_level;      /**< Trigger level (-32768 to 32767) */
    bool trigger_rising;        /**< true = rising edge, false = falling */
    bool auto_trigger;          /**< Auto trigger if no edge found */
    uint8_t time_div;           /**< Time division index (0-9) */
    uint8_t volt_div;           /**< Voltage division index (0-9) */
} aic_scope_config_t;

/** Waveform generator configuration */
typedef struct {
    aic_wave_type_t type;       /**< Waveform type */
    uint32_t frequency_hz;      /**< Output frequency */
    uint32_t sample_rate_hz;    /**< Sample rate for generation */
    int16_t amplitude;          /**< Amplitude (0-32767) */
    int16_t dc_offset;          /**< DC offset (-32768 to 32767) */
    uint8_t duty_percent;       /**< Duty cycle for pulse wave (0-100) */
} aic_wavegen_config_t;

/*******************************************************************************
 * Initialization
 ******************************************************************************/

/**
 * @brief Initialize oscilloscope subsystem
 * @return true if initialization successful
 */
bool aic_scope_init(void);

/**
 * @brief Deinitialize oscilloscope subsystem
 */
void aic_scope_deinit(void);

/**
 * @brief Check if scope is initialized
 * @return true if initialized
 */
bool aic_scope_is_ready(void);

/*******************************************************************************
 * Waveform Generation Functions
 ******************************************************************************/

/**
 * @brief Generate square wave samples (50% duty cycle)
 * @param buffer Output buffer for samples
 * @param count Number of samples to generate
 * @param freq_hz Frequency in Hz
 * @param sample_rate Sample rate in Hz
 * @param amplitude Peak amplitude (0-32767)
 */
void aic_scope_generate_square(int16_t *buffer, uint16_t count,
                               uint32_t freq_hz, uint32_t sample_rate,
                               int16_t amplitude);

/**
 * @brief Generate square wave samples with configurable duty cycle
 * @param buffer Output buffer for samples
 * @param count Number of samples to generate
 * @param freq_hz Frequency in Hz
 * @param sample_rate Sample rate in Hz
 * @param amplitude Peak amplitude (0-32767)
 * @param duty_percent Duty cycle percentage (1-100)
 */
void aic_scope_generate_square_duty(int16_t *buffer, uint16_t count,
                                    uint32_t freq_hz, uint32_t sample_rate,
                                    int16_t amplitude, uint8_t duty_percent);

/**
 * @brief Generate sine wave samples
 * @param buffer Output buffer for samples
 * @param count Number of samples to generate
 * @param freq_hz Frequency in Hz
 * @param sample_rate Sample rate in Hz
 * @param amplitude Peak amplitude (0-32767)
 */
void aic_scope_generate_sine(int16_t *buffer, uint16_t count,
                             uint32_t freq_hz, uint32_t sample_rate,
                             int16_t amplitude);

/**
 * @brief Generate triangle wave samples
 * @param buffer Output buffer for samples
 * @param count Number of samples to generate
 * @param freq_hz Frequency in Hz
 * @param sample_rate Sample rate in Hz
 * @param amplitude Peak amplitude (0-32767)
 */
void aic_scope_generate_triangle(int16_t *buffer, uint16_t count,
                                 uint32_t freq_hz, uint32_t sample_rate,
                                 int16_t amplitude);

/**
 * @brief Generate sawtooth wave samples
 * @param buffer Output buffer for samples
 * @param count Number of samples to generate
 * @param freq_hz Frequency in Hz
 * @param sample_rate Sample rate in Hz
 * @param amplitude Peak amplitude (0-32767)
 */
void aic_scope_generate_sawtooth(int16_t *buffer, uint16_t count,
                                 uint32_t freq_hz, uint32_t sample_rate,
                                 int16_t amplitude);

/**
 * @brief Generate noise samples
 * @param buffer Output buffer for samples
 * @param count Number of samples to generate
 * @param amplitude Peak amplitude (0-32767)
 */
void aic_scope_generate_noise(int16_t *buffer, uint16_t count,
                              int16_t amplitude);

/**
 * @brief Generate waveform using configuration
 * @param buffer Output buffer for samples
 * @param count Number of samples to generate
 * @param config Waveform generator configuration
 */
void aic_scope_generate_wave(int16_t *buffer, uint16_t count,
                             const aic_wavegen_config_t *config);

/**
 * @brief Get waveform type name
 * @param type Waveform type
 * @return Waveform name string
 */
const char* aic_scope_wave_name(aic_wave_type_t type);

/*******************************************************************************
 * Audio Input Functions (PDM Microphone)
 ******************************************************************************/

/**
 * @brief Initialize audio input (PDM microphone)
 * @param sample_rate Desired sample rate (16000, 22050, 44100, 48000)
 * @return true if initialization successful
 */
bool aic_audio_in_init(uint32_t sample_rate);

/**
 * @brief Start audio capture
 * @return true if started successfully
 */
bool aic_audio_in_start(void);

/**
 * @brief Stop audio capture
 */
void aic_audio_in_stop(void);

/**
 * @brief Check if audio capture is running
 * @return true if capturing
 */
bool aic_audio_in_is_capturing(void);

/**
 * @brief Get captured audio samples
 * @param buffer Output buffer for samples
 * @param count Number of samples to get
 * @return Number of samples actually copied
 */
uint16_t aic_audio_in_get_samples(int16_t *buffer, uint16_t count);

/**
 * @brief Get current audio input level (RMS)
 * @return Audio level (0-100)
 */
uint8_t aic_audio_in_get_level(void);

/*******************************************************************************
 * Audio Output Functions (I2S DAC)
 ******************************************************************************/

/**
 * @brief Initialize audio output (I2S DAC)
 * @param sample_rate Desired sample rate
 * @return true if initialization successful
 */
bool aic_audio_out_init(uint32_t sample_rate);

/**
 * @brief Start audio playback
 * @param buffer Audio data buffer
 * @param count Number of samples
 * @param loop true to loop playback
 * @return true if started successfully
 */
bool aic_audio_out_start(const int16_t *buffer, uint16_t count, bool loop);

/**
 * @brief Stop audio playback
 */
void aic_audio_out_stop(void);

/**
 * @brief Check if audio playback is running
 * @return true if playing
 */
bool aic_audio_out_is_playing(void);

/**
 * @brief Set audio output volume
 * @param volume Volume level (0-100)
 */
void aic_audio_out_set_volume(uint8_t volume);

/*******************************************************************************
 * Signal Processing Functions
 ******************************************************************************/

/**
 * @brief Calculate RMS (Root Mean Square) of signal
 * @param buffer Input samples
 * @param count Number of samples
 * @return RMS value
 */
int16_t aic_signal_rms(const int16_t *buffer, uint16_t count);

/**
 * @brief Calculate peak-to-peak amplitude
 * @param buffer Input samples
 * @param count Number of samples
 * @return Peak-to-peak value
 */
int32_t aic_signal_peak_to_peak(const int16_t *buffer, uint16_t count);

/**
 * @brief Find trigger point in buffer
 * @param buffer Input samples
 * @param count Number of samples
 * @param level Trigger level
 * @param rising true for rising edge, false for falling
 * @return Index of trigger point, or -1 if not found
 */
int32_t aic_signal_find_trigger(const int16_t *buffer, uint16_t count,
                                int16_t level, bool rising);

/**
 * @brief Calculate frequency from buffer
 * @param buffer Input samples
 * @param count Number of samples
 * @param sample_rate Sample rate in Hz
 * @return Estimated frequency in Hz
 */
uint32_t aic_signal_frequency(const int16_t *buffer, uint16_t count,
                              uint32_t sample_rate);

/**
 * @brief Apply DC offset removal (high-pass filter)
 * @param buffer Input/output samples
 * @param count Number of samples
 */
void aic_signal_remove_dc(int16_t *buffer, uint16_t count);

/**
 * @brief Downsample signal
 * @param input Input samples
 * @param input_count Input sample count
 * @param output Output samples
 * @param output_count Desired output count
 * @return Actual output count
 */
uint16_t aic_signal_downsample(const int16_t *input, uint16_t input_count,
                               int16_t *output, uint16_t output_count);

/*******************************************************************************
 * FFT Functions (Spectrum Analyzer)
 ******************************************************************************/

/**
 * @brief Initialize FFT processor
 * @param fft_size FFT size (must be power of 2: 64, 128, 256, 512, 1024)
 * @return true if initialization successful
 */
bool aic_fft_init(uint16_t fft_size);

/**
 * @brief Calculate FFT magnitude spectrum
 * @param input Time-domain samples (fft_size samples)
 * @param output Frequency-domain magnitudes (fft_size/2 bins)
 * @return true if calculation successful
 */
bool aic_fft_calculate(const int16_t *input, uint16_t *output);

/**
 * @brief Get frequency for FFT bin
 * @param bin FFT bin index
 * @param fft_size FFT size
 * @param sample_rate Sample rate
 * @return Frequency in Hz
 */
uint32_t aic_fft_bin_frequency(uint16_t bin, uint16_t fft_size, uint32_t sample_rate);

/**
 * @brief Find dominant frequency
 * @param spectrum FFT magnitude spectrum
 * @param bin_count Number of bins (fft_size/2)
 * @param sample_rate Sample rate
 * @return Dominant frequency in Hz
 */
uint32_t aic_fft_dominant_frequency(const uint16_t *spectrum, uint16_t bin_count,
                                    uint32_t sample_rate);

/*******************************************************************************
 * Ring Buffer Functions (for continuous capture)
 ******************************************************************************/

/** Ring buffer structure */
typedef struct {
    int16_t *buffer;        /**< Data buffer */
    uint16_t size;          /**< Buffer size */
    uint16_t head;          /**< Write position */
    uint16_t tail;          /**< Read position */
    uint16_t count;         /**< Current count */
} aic_ringbuf_t;

/**
 * @brief Initialize ring buffer
 * @param rb Ring buffer structure
 * @param buffer Data buffer
 * @param size Buffer size
 */
void aic_ringbuf_init(aic_ringbuf_t *rb, int16_t *buffer, uint16_t size);

/**
 * @brief Write samples to ring buffer
 * @param rb Ring buffer
 * @param data Samples to write
 * @param count Number of samples
 * @return Number of samples written
 */
uint16_t aic_ringbuf_write(aic_ringbuf_t *rb, const int16_t *data, uint16_t count);

/**
 * @brief Read samples from ring buffer
 * @param rb Ring buffer
 * @param data Output buffer
 * @param count Number of samples to read
 * @return Number of samples read
 */
uint16_t aic_ringbuf_read(aic_ringbuf_t *rb, int16_t *data, uint16_t count);

/**
 * @brief Peek samples without removing
 * @param rb Ring buffer
 * @param data Output buffer
 * @param count Number of samples to peek
 * @return Number of samples peeked
 */
uint16_t aic_ringbuf_peek(const aic_ringbuf_t *rb, int16_t *data, uint16_t count);

/**
 * @brief Get number of samples in buffer
 * @param rb Ring buffer
 * @return Number of samples available
 */
uint16_t aic_ringbuf_count(const aic_ringbuf_t *rb);

/**
 * @brief Clear ring buffer
 * @param rb Ring buffer
 */
void aic_ringbuf_clear(aic_ringbuf_t *rb);

/*******************************************************************************
 * Simulation Functions
 ******************************************************************************/

/**
 * @brief Enable audio simulation mode
 * @param enable true to enable simulation
 */
void aic_audio_set_simulation(bool enable);

/**
 * @brief Set simulated audio input source
 * @param type Waveform type to simulate
 * @param freq_hz Frequency for simulated input
 */
void aic_audio_in_set_simulated(aic_wave_type_t type, uint32_t freq_hz);

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Print scope status information
 */
void aic_scope_print_status(void);

/**
 * @brief Get time division string
 * @param div_index Division index (0-9)
 * @return Time division string (e.g., "1ms/div")
 */
const char* aic_scope_time_div_str(uint8_t div_index);

/**
 * @brief Get voltage division string
 * @param div_index Division index (0-9)
 * @return Voltage division string (e.g., "1V/div")
 */
const char* aic_scope_volt_div_str(uint8_t div_index);

#endif /* AIC_SCOPE_H */
