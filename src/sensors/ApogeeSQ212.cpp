/**
 * @file ApogeeSQ212.cpp
 * @copyright 2017-2022 Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino
 * @author Written By: Anthony Aufdenkampe <aaufdenkampe@limno.com>
 * Edited by Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 * Adapted from CampbellOBS3.cpp by Sara Geleskie Damiano
 * <sdamiano@stroudcenter.org>
 *
 * @brief Implements the ApogeeSQ212 class.
 */


#include "ApogeeSQ212.h"
#include <Adafruit_ADS1015.h>


// The constructor - need the power pin and the data pin
ApogeeSQ212::ApogeeSQ212(int8_t powerPin, uint8_t adsChannel,
                         uint8_t i2cAddress, uint8_t measurementsToAverage)
    : Sensor("ApogeeSQ212", SQ212_NUM_VARIABLES, SQ212_WARM_UP_TIME_MS,
             SQ212_STABILIZATION_TIME_MS, SQ212_MEASUREMENT_TIME_MS, powerPin,
             -1, measurementsToAverage, SQ212_INC_CALC_VARIABLES) {
    _adsChannel = adsChannel;
    _i2cAddress = i2cAddress;
}
// Destructor
ApogeeSQ212::~ApogeeSQ212() {}


String ApogeeSQ212::getSensorLocation(void) {
#ifndef MS_USE_ADS1015
    String sensorLocation = F("ADS1115_0x");
#else
    String sensorLocation = F("ADS1015_0x");
#endif
    sensorLocation += String(_i2cAddress, HEX);
    sensorLocation += F("_Channel");
    sensorLocation += String(_adsChannel);
    return sensorLocation;
}


bool ApogeeSQ212::addSingleMeasurementResult(void) {
    // Variables to store the results in
    float adcVoltage  = -9999;
    float calibResult = -9999;

    // Check a measurement was *successfully* started (status bit 6 set)
    // Only go on to get a result if it was
    if (bitRead(_sensorStatus, 6)) {
        MS_DBG(getSensorNameAndLocation(), F("is reporting:"));

// Create an Auxillary ADD object
// We create and set up the ADC object here so that each sensor using
// the ADC may set the gain appropriately without effecting others.
#ifndef MS_USE_ADS1015
        Adafruit_ADS1115 ads(_i2cAddress);  // Use this for the 16-bit version
#else
        Adafruit_ADS1015 ads(_i2cAddress);  // Use this for the 12-bit version
#endif
        // ADS Library default settings:
        //  - TI1115 (16 bit)
        //    - single-shot mode (powers down between conversions)
        //    - 128 samples per second (8ms conversion time)
        //    - 2/3 gain +/- 6.144V range (limited to VDD +0.3V max)
        //  - TI1015 (12 bit)
        //    - single-shot mode (powers down between conversions)
        //    - 1600 samples per second (625µs conversion time)
        //    - 2/3 gain +/- 6.144V range (limited to VDD +0.3V max)

        // Bump the gain up to 1x = +/- 4.096V range
        // Sensor return range is 0-2.5V, but the next gain option is 2x which
        // only allows up to 2.048V
        ads.setGain(GAIN_ONE);
        // Begin ADC
        ads.begin();

        // Read Analog to Digital Converter (ADC)
        // Taking this reading includes the 8ms conversion delay.
        // We're allowing the ADS1115 library to do the bit-to-volts conversion
        // for us
        adcVoltage =
            ads.readADC_SingleEnded_V(_adsChannel);  // Getting the reading
        MS_DBG(F("  ads.readADC_SingleEnded_V("), _adsChannel, F("):"),
               adcVoltage);

        if (adcVoltage < 3.6 && adcVoltage > -0.3) {
            // Skip results out of range
            // Apogee SQ-212 Calibration Factor = 1.0 μmol m-2 s-1 per mV;
            calibResult = 1000 * adcVoltage * SQ212_CALIBRATION_FACTOR;
            MS_DBG(F("  calibResult:"), calibResult);
        } else {
            // set invalid voltages back to -9999
            adcVoltage = -9999;
        }
    } else {
        MS_DBG(getSensorNameAndLocation(), F("is not currently measuring!"));
    }

    verifyAndAddMeasurementResult(SQ212_PAR_VAR_NUM, calibResult);
    verifyAndAddMeasurementResult(SQ212_VOLTAGE_VAR_NUM, adcVoltage);

    // Unset the time stamp for the beginning of this measurement
    _millisMeasurementRequested = 0;
    // Unset the status bits for a measurement request (bits 5 & 6)
    _sensorStatus &= 0b10011111;

    if (adcVoltage < 3.6 && adcVoltage > -0.3) {
        return true;
    } else {
        return false;
    }
}
