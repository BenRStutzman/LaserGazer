#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Madgwick.h>
#include <Adafruit_FXAS21002C.h>
#include <Adafruit_FXOS8700.h>
#include <LiquidCrystal.h>

const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12, button = 13;
int counter = 0;
float last_alt = 0.0;
float last_azi = 0.0;
float alt_offset = 0.0;
float azi_offset = 0.0;
float lat = 38.45;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Create sensor instances.
Adafruit_FXAS21002C gyro = Adafruit_FXAS21002C(0x0021002C);
Adafruit_FXOS8700 accelmag = Adafruit_FXOS8700(0x8700A, 0x8700B);

// Mag calibration values are calculated via ahrs_calibration.
// These values must be determined for each baord/environment.
// See the image in this sketch folder for the values used
// below.

// Offsets applied to raw x/y/z mag values
float mag_offsets[3]            = { 9.94F, -22.57F, 36.71F };

// Soft iron error compensation matrix
float mag_softiron_matrix[3][3] = { {  0.976,  -0.066,  0.014 },
                                    {  -0.066,  1.017, -0.009 },
                                    {  0.014, -0.009,  1.012 } };

float mag_field_strength        = 49.44F;

// Offsets applied to compensate for gyro zero-drift error for x/y/z
float gyro_zero_offsets[3]      = { 0.05F, 0.02F, 0.07F };

Madgwick filter;

void setup()
{
  Serial.begin(115200);
  // Initialize the sensors.
  gyro.begin();
  accelmag.begin();

  // filter rate in samples/second
  filter.begin(60);

  lcd.begin(16, 2);
  pinMode(button, INPUT);
}

void loop(void)
{
  sensors_event_t gyro_event;
  sensors_event_t accel_event;
  sensors_event_t mag_event;

  // Get new data samples
  gyro.getEvent(&gyro_event);
  accelmag.getEvent(&accel_event, &mag_event);

  // Apply mag offset compensation (base values in uTesla)
  float x = mag_event.magnetic.x - mag_offsets[0];
  float y = mag_event.magnetic.y - mag_offsets[1];
  float z = mag_event.magnetic.z - mag_offsets[2];

  // Apply mag soft iron error compensation
  float mx = x * mag_softiron_matrix[0][0] + y * mag_softiron_matrix[0][1] + z * mag_softiron_matrix[0][2];
  float my = x * mag_softiron_matrix[1][0] + y * mag_softiron_matrix[1][1] + z * mag_softiron_matrix[1][2];
  float mz = x * mag_softiron_matrix[2][0] + y * mag_softiron_matrix[2][1] + z * mag_softiron_matrix[2][2];

  // Apply gyro zero-rate error compensation
  float gx = gyro_event.gyro.x + gyro_zero_offsets[0];
  float gy = gyro_event.gyro.y + gyro_zero_offsets[1];
  float gz = gyro_event.gyro.z + gyro_zero_offsets[2];

  // convert gyro values to degrees/sec
  gx *= 57.2958F;
  gy *= 57.2958F;
  gz *= 57.2958F;

  // Update the filter
  filter.update(gx, gy, gz,
                accel_event.acceleration.x, accel_event.acceleration.y, accel_event.acceleration.z,
                mx, my, mz);

  //float roll = filter.getRoll();
  float pitch = filter.getPitch();
  float yaw = filter.getYaw();
  float alt = pitch + alt_offset;
  float azi = fmod(yaw + azi_offset + 360, 360);

  if (counter % 100 == 0) {
    lcd.clear();
    if (abs(alt - last_alt) < 1 && abs(azi - last_azi) < 1) {
      if (digitalRead(button) == HIGH) {
        alt_offset = lat - pitch;
        azi_offset = -yaw;
        lcd.print("calibrating...");
        lcd.setCursor(0, 1);
        lcd.print(alt_offset);
        lcd.setCursor(8, 1);
        lcd.print(azi_offset);
      } else {
        lcd.print(round(alt));
        lcd.setCursor(8, 0);
        lcd.print(round(azi));
      }
    } else { lcd.print("calculating..."); }
    last_alt = alt;
    last_azi = azi;
  }

  counter++;
  /*
  Serial.print(millis());
  Serial.print(" - Orientation: ");
  Serial.print(heading);
  Serial.print(" ");
  Serial.print(pitch);
  Serial.print(" ");
  Serial.println(roll);
  */

  delay(10);
}
