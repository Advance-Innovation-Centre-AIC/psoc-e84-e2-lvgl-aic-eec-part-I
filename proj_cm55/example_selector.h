/*******************************************************************************
 * File Name:   example_selector.h
 *
 * Description: Example selector for Embedded C for IoT Course
 *              Select which example to run by changing SELECTED_EXAMPLE
 *
 * Usage:
 *   1. Change SELECTED_PART to choose part (1-5) or demo ('A','B','C')
 *   2. Change SELECTED_EXAMPLE to choose example number
 *
 ******************************************************************************/

#ifndef EXAMPLE_SELECTOR_H
#define EXAMPLE_SELECTOR_H

#include "lvgl.h"

/*******************************************************************************
 * CONFIGURATION - Change these values to select different examples
 ******************************************************************************/

/**
 * Select which part's examples to use:
 *   - 1   = Part 1 (LVGL Basics + GPIO)
 *   - 2   = Part 2 (Sensor Data Visualization)
 *   - 3   = Part 3 (Oscilloscope & Signal Processing)
 *   - 4   = Part 4 (IPC, Logging, Event Bus)
 *   - 5   = Part 5 (WiFi Manager & IoT)
 *   - 'A' = LVGL Music Demo
 *   - 'B' = LVGL Benchmark Demo
 *   - 'C' = LVGL Widgets Demo (may cause GPU hang on 4.3" display)
 */
#define SELECTED_PART       1

/**
 * Select which example to run:
 *
 * Part 1 Examples (LVGL Basics + GPIO):
 *   Section I (UI Only):
 *     1 = Hello World Label
 *     2 = Button with Counter
 *     3 = LED Widget Control
 *     4 = Switch Toggle (GPIO concept)
 *     5 = GPIO Dashboard
 *   Section II (Hardware Integration):
 *     6 = Hardware LED Control (3 LEDs)
 *     7 = Hardware Button Status
 *     8 = Hardware ADC (Potentiometer)
 *     9 = Hardware GPIO Dashboard
 *    10 = CAPSENSE UI Mockup
 *    11 = CAPSENSE Simulation (UX/UI Demo)
 *
 * Part 2 Examples (Sensor Visualization):
 *   Section I (Simulated Data):
 *     1 = Slider & Bar (ADC)
 *     2 = Arc Gauge
 *     3 = Chart Time-Series (Accelerometer)
 *     4 = Scale Temperature
 *     5 = Sensor Dashboard (TabView)
 *     6 = Chart Dashboard (Bar/Area/Scatter/Line)
 *   Section II (Real Hardware):
 *     7 = Real IMU Visualization (based on Ex3)
 *     8 = Real Sensor Dashboard (based on Ex5)
 *     9 = Real Arc Gauge (based on Ex2, uses POTEN)
 *    10 = Real Scale Gauge (based on Ex4, uses POTEN)
 *    11 = Real Chart Dashboard (based on Ex6, uses IMU)
 *
 * Part 3 Examples (Oscilloscope & Signal Processing):
 *   Section I (Simulated Data):
 *     1 = Waveform Generator (Square, Sine, Triangle)
 *     2 = Noise Generator
 *     3 = Audio Waveform Display
 *     4 = Microphone Visualizer
 *     5 = Full Oscilloscope UI
 *     6 = FFT Spectrum Analyzer
 *   Section II (Integration):
 *     7 = Custom Panel Scope (Maximum Chart Size - Simulated)
 *     8 = Hardware Integrated Scope (POTEN + LED3 - Real Hardware)
 *
 * Part 4 Examples (IPC, Logging, Event Bus):
 *   Section I (Simulated/UI):
 *     1 = IPC Ping Test
 *     2 = IPC Remote Logging
 *     3 = IPC Sensor Data
 *     4 = Event Bus Demo
 *   Section II (Hardware IPC):
 *     5 = HW IPC LED Control
 *     6 = HW IPC Button Events
 *     7 = HW IPC Dashboard
 *     8 = Advanced Features (All combined)
 *     9 = CAPSENSE via IPC (CM33 I2C -> CM55 display)
 *
 * Part 5 Examples (WiFi Manager & IoT):
 *   Section I (WiFi Basics):
 *     1 = WiFi Network List
 *     2 = WiFi Connect/Disconnect
 *     3 = TCP/IP Information
 *     4 = Hardware Information
 *   Section II (Full WiFi Integration):
 *     5 = Full WiFi Manager (macOS Style)
 *     6 = WiFi Status Dashboard
 *     7 = IoT Dashboard (WiFi + Sensors)
 *     8 = MQTT Preview (UI Only)
 *
 * (SELECTED_EXAMPLE is ignored when SELECTED_PART = 'A','B','C')
 */
#define SELECTED_EXAMPLE    1

/*******************************************************************************
 * Include appropriate headers based on selection
 ******************************************************************************/
#if SELECTED_PART == 1
    #include "part1/part1_examples.h"
#elif SELECTED_PART == 2
    #include "part2/part2_examples.h"
#elif SELECTED_PART == 3
    #include "part3/part3_examples.h"
    #include "part3/part3_scope_example.h"
    #include "part3/part3_hw_scope_example.h"
#elif SELECTED_PART == 4
    #include "part4/part4_examples.h"
#elif SELECTED_PART == 5
    #include "part5/part5_examples.h"
#endif

/*******************************************************************************
 * Function to run the selected example
 ******************************************************************************/
static inline void run_selected_example(void)
{
#if SELECTED_PART == 'A'
    /* LVGL Music Demo */
    extern void lv_demo_music(void);
    lv_demo_music();

#elif SELECTED_PART == 'B'
    /* LVGL Benchmark Demo */
    extern void lv_demo_benchmark(void);
    lv_demo_benchmark();

#elif SELECTED_PART == 'C'
    /* LVGL Widgets Demo */
    extern void lv_demo_widgets(void);
    lv_demo_widgets();

#elif SELECTED_PART == 1
    /* Part 1 Examples */
    /* Section I: UI Only (1-5) */
    #if SELECTED_EXAMPLE == 1
        part1_ex1_hello_world();
    #elif SELECTED_EXAMPLE == 2
        part1_ex2_button_counter();
    #elif SELECTED_EXAMPLE == 3
        part1_ex3_led_control();
    #elif SELECTED_EXAMPLE == 4
        part1_ex4_switch_toggle();
    #elif SELECTED_EXAMPLE == 5
        part1_ex5_gpio_dashboard();
    /* Section II: Hardware Integration (6-11) */
    #elif SELECTED_EXAMPLE == 6
        part1_ex6_hw_led_control();
    #elif SELECTED_EXAMPLE == 7
        part1_ex7_hw_button_status();
    #elif SELECTED_EXAMPLE == 8
        part1_ex8_hw_adc_display();
    #elif SELECTED_EXAMPLE == 9
        part1_ex9_hw_gpio_dashboard();
    #elif SELECTED_EXAMPLE == 10
        part1_ex10_capsense_mockup();
    #elif SELECTED_EXAMPLE == 11
        part1_ex11_capsense_hardware();
    #else
        #error "Invalid SELECTED_EXAMPLE for Part 1 (must be 1-11)"
    #endif

#elif SELECTED_PART == 2
    /* Part 2 Examples */
    /* Section I: Simulated Data (1-6) */
    #if SELECTED_EXAMPLE == 1
        part2_ex1_slider_bar();
    #elif SELECTED_EXAMPLE == 2
        part2_ex2_arc_gauge();
    #elif SELECTED_EXAMPLE == 3
        part2_ex3_chart_timeseries();
    #elif SELECTED_EXAMPLE == 4
        part2_ex4_scale_temperature();
    #elif SELECTED_EXAMPLE == 5
        part2_ex5_sensor_dashboard();
    #elif SELECTED_EXAMPLE == 6
        part2_ex6_chart_dashboard();
    /* Section II: Real Hardware (7-11) */
    #elif SELECTED_EXAMPLE == 7
        part2_ex7_real_imu_display();
    #elif SELECTED_EXAMPLE == 8
        part2_ex8_real_sensor_dashboard();
    #elif SELECTED_EXAMPLE == 9
        part2_ex9_real_arc_gauge();
    #elif SELECTED_EXAMPLE == 10
        part2_ex10_real_scale_gauge();
    #elif SELECTED_EXAMPLE == 11
        part2_ex11_real_chart_dashboard();
    #else
        #error "Invalid SELECTED_EXAMPLE for Part 2 (must be 1-11)"
    #endif

#elif SELECTED_PART == 3
    /* Part 3 Examples */
    /* Section I: Simulated Data (1-6) */
    #if SELECTED_EXAMPLE == 1
        part3_ex1_waveform_generator();
    #elif SELECTED_EXAMPLE == 2
        part3_ex2_noise_generator();
    #elif SELECTED_EXAMPLE == 3
        part3_ex3_audio_waveform();
    #elif SELECTED_EXAMPLE == 4
        part3_ex4_mic_visualizer();
    #elif SELECTED_EXAMPLE == 5
        part3_ex5_oscilloscope_ui();
    #elif SELECTED_EXAMPLE == 6
        part3_ex6_spectrum_analyzer();
    /* Section II: Integration (7-8) */
    #elif SELECTED_EXAMPLE == 7
        part3_ex7_custom_panel_scope();
    #elif SELECTED_EXAMPLE == 8
        part3_ex8_hw_scope();
    #else
        #error "Invalid SELECTED_EXAMPLE for Part 3 (must be 1-8)"
    #endif

#elif SELECTED_PART == 4
    /* Part 4 Examples - IPC, Logging, Event Bus */
    /* Section I: Simulated/UI (1-4) */
    #if SELECTED_EXAMPLE == 1
        part4_ex1_ipc_ping();
    #elif SELECTED_EXAMPLE == 2
        part4_ex2_ipc_log();
    #elif SELECTED_EXAMPLE == 3
        part4_ex3_ipc_sensor();
    #elif SELECTED_EXAMPLE == 4
        part4_ex4_event_bus();
    /* Section II: Hardware IPC (5-9) */
    #elif SELECTED_EXAMPLE == 5
        part4_ex5_hw_ipc_led();
    #elif SELECTED_EXAMPLE == 6
        part4_ex6_hw_ipc_button();
    #elif SELECTED_EXAMPLE == 7
        part4_ex7_hw_ipc_dashboard();
    #elif SELECTED_EXAMPLE == 8
        part4_ex8_advanced();
    #elif SELECTED_EXAMPLE == 9
        part4_ex9_capsense_ipc();
    #else
        #error "Invalid SELECTED_EXAMPLE for Part 4 (must be 1-9)"
    #endif

#elif SELECTED_PART == 5
    /* Part 5 Examples - WiFi Manager & IoT */
    /* Section I: WiFi Basics (1-4) */
    #if SELECTED_EXAMPLE == 1
        part5_ex1_wifi_list();
    #elif SELECTED_EXAMPLE == 2
        part5_ex2_wifi_connect();
    #elif SELECTED_EXAMPLE == 3
        part5_ex3_tcpip_info();
    #elif SELECTED_EXAMPLE == 4
        part5_ex4_hardware_info();
    /* Section II: Full WiFi Integration (5-8) */
    #elif SELECTED_EXAMPLE == 5
        part5_ex5_wifi_manager();
    #elif SELECTED_EXAMPLE == 6
        part5_ex6_wifi_status();
    #elif SELECTED_EXAMPLE == 7
        part5_ex7_iot_dashboard();
    #elif SELECTED_EXAMPLE == 8
        part5_ex8_mqtt_preview();
    #else
        #error "Invalid SELECTED_EXAMPLE for Part 5 (must be 1-8)"
    #endif

#else
    #error "Invalid SELECTED_PART (must be 1-5 or 'A','B','C')"
#endif
}

/*******************************************************************************
 * Helper function to print current selection
 ******************************************************************************/
static inline void print_example_info(void)
{
#if SELECTED_PART == 'A'
    printf("Running: LVGL Music Demo\r\n");
#elif SELECTED_PART == 'B'
    printf("Running: LVGL Benchmark Demo\r\n");
#elif SELECTED_PART == 'C'
    printf("Running: LVGL Widgets Demo\r\n");
#elif SELECTED_PART == 1
    printf("Running: Part 1 - Example %d\r\n", SELECTED_EXAMPLE);
    #if SELECTED_EXAMPLE == 1
        printf("  -> Hello World Label\r\n");
    #elif SELECTED_EXAMPLE == 2
        printf("  -> Button with Counter\r\n");
    #elif SELECTED_EXAMPLE == 3
        printf("  -> LED Widget Control\r\n");
    #elif SELECTED_EXAMPLE == 4
        printf("  -> Switch Toggle\r\n");
    #elif SELECTED_EXAMPLE == 5
        printf("  -> GPIO Dashboard\r\n");
    #elif SELECTED_EXAMPLE == 6
        printf("  -> [HW] Hardware LED Control\r\n");
    #elif SELECTED_EXAMPLE == 7
        printf("  -> [HW] Hardware Button Status\r\n");
    #elif SELECTED_EXAMPLE == 8
        printf("  -> [HW] Hardware ADC Display\r\n");
    #elif SELECTED_EXAMPLE == 9
        printf("  -> [HW] Hardware GPIO Dashboard\r\n");
    #elif SELECTED_EXAMPLE == 10
        printf("  -> [HW] CAPSENSE UI Mockup\r\n");
    #elif SELECTED_EXAMPLE == 11
        printf("  -> [HW] CAPSENSE Hardware\r\n");
    #endif
#elif SELECTED_PART == 2
    printf("Running: Part 2 - Example %d\r\n", SELECTED_EXAMPLE);
    #if SELECTED_EXAMPLE == 1
        printf("  -> Slider & Bar (ADC)\r\n");
    #elif SELECTED_EXAMPLE == 2
        printf("  -> Arc Gauge\r\n");
    #elif SELECTED_EXAMPLE == 3
        printf("  -> Chart Time-Series\r\n");
    #elif SELECTED_EXAMPLE == 4
        printf("  -> Scale Temperature\r\n");
    #elif SELECTED_EXAMPLE == 5
        printf("  -> Sensor Dashboard\r\n");
    #elif SELECTED_EXAMPLE == 6
        printf("  -> Chart Dashboard (Bar/Area/Scatter/Line)\r\n");
    #elif SELECTED_EXAMPLE == 7
        printf("  -> [HW] Real IMU Visualization\r\n");
    #elif SELECTED_EXAMPLE == 8
        printf("  -> [HW] Real Sensor Dashboard\r\n");
    #elif SELECTED_EXAMPLE == 9
        printf("  -> [HW] Real Arc Gauge (POTEN)\r\n");
    #elif SELECTED_EXAMPLE == 10
        printf("  -> [HW] Real Scale Gauge (POTEN)\r\n");
    #elif SELECTED_EXAMPLE == 11
        printf("  -> [HW] Real Chart Dashboard (IMU)\r\n");
    #endif
#elif SELECTED_PART == 3
    printf("Running: Part 3 - Example %d\r\n", SELECTED_EXAMPLE);
    #if SELECTED_EXAMPLE == 1
        printf("  -> Waveform Generator\r\n");
    #elif SELECTED_EXAMPLE == 2
        printf("  -> Noise Generator\r\n");
    #elif SELECTED_EXAMPLE == 3
        printf("  -> Audio Waveform Display\r\n");
    #elif SELECTED_EXAMPLE == 4
        printf("  -> Microphone Visualizer\r\n");
    #elif SELECTED_EXAMPLE == 5
        printf("  -> Oscilloscope UI\r\n");
    #elif SELECTED_EXAMPLE == 6
        printf("  -> Spectrum Analyzer (FFT)\r\n");
    #elif SELECTED_EXAMPLE == 7
        printf("  -> [HW] Custom Panel Scope (Simulated)\r\n");
    #elif SELECTED_EXAMPLE == 8
        printf("  -> [HW] Hardware Scope (POTEN + LED3)\r\n");
    #endif
#elif SELECTED_PART == 4
    printf("Running: Part 4 - Example %d\r\n", SELECTED_EXAMPLE);
    #if SELECTED_EXAMPLE == 1
        printf("  -> IPC Ping Test\r\n");
    #elif SELECTED_EXAMPLE == 2
        printf("  -> IPC Remote Logging\r\n");
    #elif SELECTED_EXAMPLE == 3
        printf("  -> IPC Sensor Data\r\n");
    #elif SELECTED_EXAMPLE == 4
        printf("  -> Event Bus Demo\r\n");
    #elif SELECTED_EXAMPLE == 5
        printf("  -> [HW] HW IPC LED Control\r\n");
    #elif SELECTED_EXAMPLE == 6
        printf("  -> [HW] HW IPC Button Events\r\n");
    #elif SELECTED_EXAMPLE == 7
        printf("  -> [HW] HW IPC Dashboard\r\n");
    #elif SELECTED_EXAMPLE == 8
        printf("  -> [HW] Advanced Features\r\n");
    #elif SELECTED_EXAMPLE == 9
        printf("  -> [HW] CAPSENSE via IPC\r\n");
    #endif
#elif SELECTED_PART == 5
    printf("Running: Part 5 - Example %d\r\n", SELECTED_EXAMPLE);
    #if SELECTED_EXAMPLE == 1
        printf("  -> WiFi Network List\r\n");
    #elif SELECTED_EXAMPLE == 2
        printf("  -> WiFi Connect/Disconnect\r\n");
    #elif SELECTED_EXAMPLE == 3
        printf("  -> TCP/IP Information\r\n");
    #elif SELECTED_EXAMPLE == 4
        printf("  -> Hardware Information\r\n");
    #elif SELECTED_EXAMPLE == 5
        printf("  -> [HW] Full WiFi Manager\r\n");
    #elif SELECTED_EXAMPLE == 6
        printf("  -> [HW] WiFi Status Dashboard\r\n");
    #elif SELECTED_EXAMPLE == 7
        printf("  -> [HW] IoT Dashboard\r\n");
    #elif SELECTED_EXAMPLE == 8
        printf("  -> [HW] MQTT Preview\r\n");
    #endif
#endif
}

#endif /* EXAMPLE_SELECTOR_H */
