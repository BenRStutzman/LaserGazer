#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Madgwick.h>
#include <Adafruit_FXAS21002C.h>
#include <Adafruit_FXOS8700.h>
#include <LiquidCrystal.h>
#include "RTClib.h"

const int num_bodies = 36;
const int button1 = 13, button2 = 12;
const float lat = 38.4714, lon = -78.8824;
float sidereal;
unsigned long counter;
float pitch, yaw, alt, azi;
float last_alt, last_azi, alt_offset, azi_offset;
float coords[num_bodies][2];
int closest; float closest_dist; String closest_name;
float x, y, z, mx, my, mz, gx, gy, gz;
float alt_dist, azi_dist;
byte up_char[8] = {0, 0, 4, 14, 21, 4, 4, 0};
byte down_char[8] = {0, 4, 4, 21, 14, 4, 0, 0};

DateTime now;
LiquidCrystal lcd(11, 10, 9, 8, 7, 6);
RTC_DS1307 rtc;
Adafruit_FXAS21002C gyro = Adafruit_FXAS21002C(0x0021002C);
Adafruit_FXOS8700 accelmag = Adafruit_FXOS8700(0x8700A, 0x8700B);
Madgwick filter;

// Mag calibration values are calculated via ahrs_calibration.
// These values must be determined for each baord/environment.

// Offsets applied to raw x/y/z mag values
float mag_offsets[3]            = { 12.75F, -24.92F, 37.93F };

// Soft iron error compensation matrix
float mag_softiron_matrix[3][3] = { {  0.960,  -0.066,  0.007 },
                                    {  -0.066,  0.997, 0.013 },
                                    {  0.007, 0.013,  1.050 }
};

float mag_field_strength        = 51.56F;

// Offsets applied to compensate for gyro zero-drift error for x/y/z
float gyro_zero_offsets[3]      = { 0.02F, 0.00F, 0.02F };

void get_name() {
  switch (closest) {
    case 0: closest_name = "Sirius";          break;
    case 1: closest_name = "Arcturus";        break;
    case 2: closest_name = "Vega";            break;
    case 3: closest_name = "Capella";         break;
    case 4: closest_name = "Rigel";           break;
    case 5: closest_name = "Procyon";         break;
    case 6: closest_name = "Betelgeuse";      break;
    case 7: closest_name = "Altair";          break;
    case 8: closest_name = "Aldebaran";       break;
    case 9: closest_name = "Antares";         break;
    case 10: closest_name = "Spica";          break;
    case 11: closest_name = "Pollux";         break;
    case 12: closest_name = "Fomalhaut";      break;
    case 13: closest_name = "Deneb";          break;
    case 14: closest_name = "Regulus";        break;
    case 15: closest_name = "Adhara";         break;
    case 16: closest_name = "Shaula";         break;
    case 17: closest_name = "Castor";         break;
    case 18: closest_name = "Bellatrix";      break;
    case 19: closest_name = "Elnath";         break;
    case 20: closest_name = "Alnilam";        break;
    case 21: closest_name = "Alioth";         break;
    case 22: closest_name = "Alnitak";        break;
    case 23: closest_name = "Dubhe";          break;
    case 24: closest_name = "Mirfak";         break;
    case 25: closest_name = "Wezen";          break;
    case 26: closest_name = "Sargas";         break;
    case 27: closest_name = "E Sagittar";     break;
    case 28: closest_name = "Alkaid";         break;
    case 29: closest_name = "Menkalinan";     break;
    case 30: closest_name = "Alhena";         break;
    case 31: closest_name = "Mirzam";         break;
    case 32: closest_name = "Alphard";        break;
    case 33: closest_name = "Polaris";        break;
    case 34: closest_name = "Hamal";          break;
    case 35: closest_name = "Algieba";        break;
    default: closest_name = "Tralfamadore";   break;
  }
}

float celes[num_bodies][2] = {  {101.2870, -0.2668},
                                {213.9153, 0.3348},
                                {279.2347, 0.6769},
                                {79.1723, 0.8028},
                                {78.6386, -0.1361},
                                {114.8255, 0.0912},
                                {88.7932, 0.1293},
                                {297.6958, 0.1548},
                                {68.9802, 0.2881},
                                {247.3518, -0.4462},
                                {201.2983, -0.1892},
                                {116.3289, 0.4891},
                                {344.4127, -0.4953},
                                {310.3580, 0.7903},
                                {152.0930, 0.2089},
                                {104.6565, -0.4717},
                                {263.4022, -0.6440},
                                {113.6500, 0.5566},
                                {81.2829, 0.1108},
                                {81.5730, 0.4993},
                                {84.0533, -0.0139},
                                {193.5073, 0.9767},
                                {85.1896, -0.0010},
                                {165.9320, 1.0778},
                                {51.0807, 0.8702},
                                {107.0979, -0.4469},
                                {264.3297, -0.7156},
                                {276.0430, -0.5867},
                                {206.8852, 0.8607},
                                {89.8822, 0.7845},
                                {99.4280, 0.2862},
                                {95.6749, -0.2800},
                                {141.8968, -0.1281},
                                {37.9545, 1.5580},
                                {31.7934, 0.4095},
                                {154.9931, 0.3463} };

void calc_sidereal() {
  // formula from https://aa.usno.navy.mil/faq/docs/GAST.php

  float days = (now.year() - 2019) * 365 + (now.year() - 2017) / 4;               // days in full years since 1/1/19
  int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};  // lengths of months
  for (int i = 0; i < now.month() - 1; i++) {
    days += months[i];  // add days in full months
  }
  if (now.month() > 2 && now.year() % 4 == 0) {
    days += 1;  // add day from a leap year
  }
  days += (now.day() - 1) + now.hour() / 24.0 + now.minute() / 1440.0;                  // add days from this month
  sidereal = fmod(6.6907 + 24.0657 * days, 24);                       // sidereal time in hours
}

void calc_coords() {
  // formulae from https://aa.usno.navy.mil/faq/docs/Alt_Az.php

  for (int i = 0; i < num_bodies; i++) {
    float LHA = (sidereal * 15 - celes[i][0] + lon) / 57.2958;                // local hour angle
    float lat_rad = lat / 57.2958;                                            // lat in radians
    float dec = celes[i][1];                                                  // declination in radians

    coords[i][0] = asin(cos(LHA) * cos(dec) * cos(lat_rad) + sin(dec) * sin(lat_rad)) * 57.2958;  // altitude of the body
    coords[i][1] = fmod(atan2(-sin(LHA), tan(dec) * cos(lat_rad) - sin(lat_rad) * cos(LHA))       // azimuth of the body
                        * 57.2958 + 360, 360);
  }
}

void get_orientation() {
  sensors_event_t gyro_event;
  sensors_event_t accel_event;
  sensors_event_t mag_event;

  // Get new data samples
  gyro.getEvent(&gyro_event);
  accelmag.getEvent(&accel_event, &mag_event);

  // Apply mag offset compensation (base values in uTesla)
  x = mag_event.magnetic.x - mag_offsets[0];
  y = mag_event.magnetic.y - mag_offsets[1];
  z = mag_event.magnetic.z - mag_offsets[2];

  // Apply mag soft iron error compensation
  mx = x * mag_softiron_matrix[0][0] + y * mag_softiron_matrix[0][1] + z * mag_softiron_matrix[0][2];
  my = x * mag_softiron_matrix[1][0] + y * mag_softiron_matrix[1][1] + z * mag_softiron_matrix[1][2];
  mz = x * mag_softiron_matrix[2][0] + y * mag_softiron_matrix[2][1] + z * mag_softiron_matrix[2][2];

  // Apply gyro zero-rate error compensation
  gx = gyro_event.gyro.x + gyro_zero_offsets[0];
  gy = gyro_event.gyro.y + gyro_zero_offsets[1];
  gz = gyro_event.gyro.z + gyro_zero_offsets[2];

  // convert gyro values to degrees/sec
  gx *= 57.2958F;
  gy *= 57.2958F;
  gz *= 57.2958F;

  // Update the filter
  filter.update(gx, gy, gz,
                accel_event.acceleration.x, accel_event.acceleration.y, accel_event.acceleration.z,
                mx, my, mz);

  //float roll = filter.getRoll();
  pitch = filter.getPitch();
  yaw = filter.getYaw();
  alt = pitch + alt_offset;
  azi = fmod((720 - yaw - azi_offset), 360);
}

void find_closest() {
  // formula from http://spiff.rit.edu/classes/phys373/lectures/radec/radec.html
  float alt_rad = alt / 57.2958;                        // convert to radians
  float azi_rad = azi / 57.2958;
  float min_dist = 3.1416;                              // reset closest and min_dist
  closest = -1;
  
  for (int i = 0; i < num_bodies; i++) {
    float targ_alt_rad = coords[i][0] / 57.2958;                            // target coords in radians
    float targ_azi_rad = coords[i][1] / 57.2958;
    float dist = acos(cos(1.5708 - alt_rad) * cos(1.5708 - targ_alt_rad) +  // angular distance
                      sin(1.5708 - alt_rad) * sin(1.5708 - targ_alt_rad) *
                      cos(azi_rad - targ_azi_rad));
    if (dist < min_dist) {                                                  // set closest to this one if it's closer
      min_dist = dist;
      closest = i;
      closest_dist = min_dist * 57.2958;                                    // convert to degrees
      alt_dist = coords[i][0] - alt;
      azi_dist = coords[i][1] - azi;
      if (azi_dist > 180) { azi_dist -= 360; }
      else if (azi_dist < -180) { azi_dist += 360; }
    }
  }
  get_name();
}

void print_screen1() {
  lcd.clear();
  lcd.print(closest_name);
  lcd.setCursor(10, 0);
  if (alt_dist > 1) { lcd.write((byte)0); }
  else if (alt_dist < -1){ lcd.write(1); }
  else { lcd.write(42); }
  lcd.setCursor(14, 0);
  if (azi_dist > 1) { lcd.write(126); }
  else if (azi_dist < -1){ lcd.write(127); }
  else { lcd.write(42); }
  lcd.setCursor(0, 1); lcd.print(round(closest_dist));
  lcd.setCursor(2, 1); lcd.write(223);
  lcd.setCursor(4, 1); lcd.print("from");
  lcd.setCursor(9, 1); lcd.print(round(alt));
  lcd.setCursor(12, 1); lcd.print(",");
  lcd.setCursor(13, 1); lcd.print(round(azi));
}

void print_screen2() {
  lcd.clear();
  lcd.print(now.year()); lcd.print("-");
  lcd.print(now.month()); lcd.print("-");
  lcd.print(now.day()); lcd.print(" ");
  lcd.print(now.hour()); lcd.print(":");
  lcd.print(now.minute());
  lcd.setCursor(0, 1); lcd.print(alt_offset);
  lcd.setCursor(8, 1); lcd.print(azi_offset);
}

void calibrate(){
  alt_offset = lat - pitch;
  azi_offset = fmod(-yaw, 360);
}

void update_coords() {
  now = rtc.now();
  calc_sidereal();
  calc_coords();
}

void stall() {
  lcd.clear();
  lcd.print("CALCULATING...");
  lcd.setCursor(0,1); lcd.print("(searching sky)");
}

void setup()
{

  gyro.begin();
  accelmag.begin();
  rtc.begin();
  filter.begin(60); // filter rate in samples/second
  lcd.begin(16, 2);
  Serial.begin(115200);
  
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  lcd.createChar(0, up_char);
  lcd.createChar(1, down_char);

  now = rtc.now();
  calc_sidereal();
  calc_coords();

}

void loop(void)
{
  get_orientation();
  
  if (counter % 100 == 0) {
    if (counter % 18000 == 0) { update_coords(); }  // Update coords about every 5 minutes
    if (digitalRead(button1) == HIGH && digitalRead(button2) == HIGH) {
      alt_offset = 0;
      azi_offset = 0;
    } else if (digitalRead(button2) == HIGH) {
      calibrate();
    } else if (digitalRead(button1) == HIGH) {
      print_screen2();
    } else {
      if (abs(alt - last_alt) < 10 && (abs(azi - last_azi) < 10 | abs(azi - last_azi) > 350)) {
        find_closest();
        print_screen1();
      } else { stall(); }
    }
    last_alt = alt;
    last_azi = azi;
  }

  counter++;
  delay(10);
}
