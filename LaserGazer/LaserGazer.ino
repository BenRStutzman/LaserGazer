#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_Sensor.h>
#include <Madgwick.h>
#include <Adafruit_FXAS21002C.h>
#include <Adafruit_FXOS8700.h>
#include "RTClib.h"

//Set your coordinates here:

const float lat = 38.4714, lon = -78.8824; // EMU Hill
//const float lat = 40.0424, lon = -76.3165; // Pine St. Backyard
//const float lat = 39.8441, lon = -76.2867; // Muddy Run Observatory
//const float lat = 37.4263, lon =  -81.5083; // Kimball, WV

const byte num_bodies = 40;
const byte button1 = 13, button2 = 12;
const byte up_char[8] = {0, 0, 4, 14, 21, 4, 4, 0};
const byte down_char[8] = {0, 4, 4, 21, 14, 4, 0, 0};
const byte up_left_char[8] = {0, 0, 0, 30, 24, 20, 18, 1};
const byte up_right_char[8] = {0, 0, 0, 15, 3, 5, 9, 16};
const byte down_left_char[8] = {1, 18, 20, 24, 30, 0, 0, 0};
const byte down_right_char[8] = {16, 9, 5, 3, 15, 0, 0, 0};
const byte months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const float r2d = 57.2958;

byte row;
byte col;
int alt_dist, azi_dist;
float sidereal;
float d;
float orbs[5][6]; // N, i, w, M, e, a
float coords[num_bodies][2];
float pitch, yaw, alt, azi;
float alt_offset, azi_offset;
float x, y, z, mx, my, mz, gx, gy, gz;
float closest_dist;
unsigned long counter;
String closest_name;


DateTime now;
LiquidCrystal lcd(11, 10, 9, 8, 7, 6);
RTC_DS1307 rtc;
Adafruit_FXAS21002C gyro = Adafruit_FXAS21002C(0x0021002C);
Adafruit_FXOS8700 accelmag = Adafruit_FXOS8700(0x8700A, 0x8700B);
Madgwick filter;

// To recalibrate, run ahrs_calibration on the Arduino
// and the MotionCal app on the computer, spin it around a bunch,
// and enter the magnetic offset values below.
// Last calibrated: 10/23/19 on the EMU Hill

// Offsets applied to raw x/y/z mag values
float mag_offsets[3]            = { 16.18, -11.87, 32.63 };

// Soft iron error compensation matrix
float mag_softiron_matrix[3][3] = { {  1.012,  0.008,  -0.009 },
                                    {  0.007,  0.965, -0.001 },
                                    {  -0.009, -0.001,  1.024 }
};

// Offsets applied to compensate for gyro zero-drift error for x/y/z
float gyro_zero_offsets[3]      = { 0.02, 0.00, 0.02 };


// For getting the name of each planet/star;
// A list didn't work for some reason
void get_name(byte i) {
  switch (i) {
    case 0: closest_name = "Mercury";         break;
    case 1: closest_name = "Venus";           break;
    case 2: closest_name = "Mars";            break;
    case 3: closest_name = "Jupiter";         break;
    case 4: closest_name = "Saturn";          break;
    case 5: closest_name = "Sirius";          break;
    case 6: closest_name = "Arcturus";        break;
    case 7: closest_name = "Vega";            break;
    case 8: closest_name = "Capella";         break;
    case 9: closest_name = "Rigel";           break;
    case 10: closest_name = "Procyon";        break;
    case 11: closest_name = "Betelgeuse";     break;
    case 12: closest_name = "Altair";         break;
    case 13: closest_name = "Aldebaran";      break;
    case 14: closest_name = "Antares";        break;
    case 15: closest_name = "Spica";          break;
    case 16: closest_name = "Pollux";         break;
    case 17: closest_name = "Fomalhaut";      break;
    case 18: closest_name = "Deneb";          break;
    case 19: closest_name = "Regulus";        break;
    case 20: closest_name = "Adhara";         break;
    case 21: closest_name = "Shaula";         break;
    case 22: closest_name = "Castor";         break;
    case 23: closest_name = "Bellatrix";      break;
    case 24: closest_name = "Elnath";         break;
    case 25: closest_name = "Alnilam";        break;
    case 26: closest_name = "Alioth";         break;
    case 27: closest_name = "Alnitak";        break;
    case 28: closest_name = "Dubhe";          break;
    case 29: closest_name = "Mirfak";         break;
    case 30: closest_name = "Wezen";          break;
    case 31: closest_name = "Sargas";         break;
    case 32: closest_name = "Kaus Australis"; break;
    case 33: closest_name = "Alkaid";         break;
    case 34: closest_name = "Menkalinan";     break;
    case 35: closest_name = "Alhena";         break;
    case 36: closest_name = "Mirzam";         break;
    case 37: closest_name = "Alphard";        break;
    case 38: closest_name = "Polaris";        break;
    case 39: closest_name = "Hamal";          break;  
  }
}

//Celestial coordinates of each body (which don't change for stars)
float celes[num_bodies][2] = {  {0, 0},
                                {0, 0},
                                {0, 0},
                                {0, 0},
                                {0, 0},
                                {101.2870, -0.2668},
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
                                {31.7934, 0.4095}
                                };


void calc_sidereal() {
  // calculates the current sidereal time
  // formula from https://aa.usno.navy.mil/faq/docs/GAST.php

  d = (now.year() - 2019) * 365 + (now.year() - 2017) / 4;         // days in full years since 1/1/19
  for (byte i = 0; i < now.month() - 1; i++) {
    d += months[i];                                                // add days in full months
  }
  if (now.month() > 2 && now.year() % 4 == 0) {
    d += 1;                                                        // add day from a leap year
  }
  d += (now.day() - 1) + now.hour() / 24.0 + now.minute() / 1440.0;// add days from this month
  sidereal = fmod(6.6907 + 24.0657 * d, 24);                       // sidereal time in hours
  d += 6941;
}
 
void calc_planets(){
  // calculates the celestial coordinates of the planets
  // formulae from https://stjarnhimlen.se/comp/ppcomp.html
  
  orbs[0][0] = 48.3313 + 3.24587E-5 * d;
  orbs[0][1] = 7.0047 + 5.00E-8 * d;
  orbs[0][2] = 29.1241 + 1.01444E-5 * d;
  orbs[0][3] = 168.6562 + 4.0923344368 * d;
  orbs[0][4] = 0.205635 + 5.59E-10 * d;
  orbs[0][5] = 0.387098;
  orbs[1][0] = 76.6799 + 2.46590E-5 * d;
  orbs[1][1] = 3.3946 + 2.75E-8 * d;
  orbs[1][2] = 54.8910 + 1.38374E-5 * d;
  orbs[1][3] = 48.0052 + 1.6021302244 * d;
  orbs[1][4] = 0.006773 - 1.302E-9 * d;
  orbs[1][5] = 0.723330;
  orbs[2][0] = 49.5574 + 2.11081E-5 * d;
  orbs[2][1] = 1.8497 - 1.78E-8 * d; 
  orbs[2][2] = 286.5016 + 2.92961E-5 * d;
  orbs[2][3] = 18.6021 + 0.5240207766 * d;
  orbs[2][4] = 0.093405 + 2.516E-9 * d;
  orbs[2][5] = 1.523688;
  orbs[3][0] = 100.4542 + 2.76854E-5 * d;
  orbs[3][1] = 1.3030 - 1.557E-7 * d;
  orbs[3][2] = 273.8777 + 1.64505E-5 * d;
  orbs[3][3] = 19.8950 + 0.0830853001 * d;
  orbs[3][4] = 0.048498 + 4.469E-9 * d;
  orbs[3][5] = 5.20256;
  orbs[4][0] = 113.6634 + 2.38980E-5 * d;
  orbs[4][1] = 2.4886 - 1.081E-7 * d;
  orbs[4][2] = 339.3939 + 2.97661E-5 * d;
  orbs[4][3] = 316.9670 + 0.0334442282 * d;
  orbs[4][4] = 0.055546 - 9.499E-9 * d;
  orbs[4][5] = 9.55475;
  
  for (byte i = 0; i < 5; i++) {
    for (byte j = 0; j < 4; j++)
    orbs[i][j] = fmod(orbs[i][j], 360) / r2d;
  }
  
  float E, E1, xv, yv, v, r, xh, yh, zh, xs, ys, xg, yg, zg, xe, ye, ze;
  float s3 = fmod(356.0470 + 0.9856002585 * d, 360) / r2d;
  float s4 = 0.016709 - 1.151E-9 * d;
  E = s3 + s4 * sin (s3) * (1 + s4 * cos(s3));
  xv = cos(E) - s4;
  yv = sqrt(1 - s4 * s4) * sin(E);
  v = atan2 (yv, xv);
  float rs = sqrt(xv * xv + yv * yv);
  float lons = v + fmod(282.9404 + 4.70935E-5 * d, 360) / r2d;
  for (byte i = 0; i < 5; i++) {
    E = orbs[i][3] + orbs[i][4] * sin (orbs[i][3]) * (1 + orbs[i][4] * cos(orbs[i][3]));
    E1 = E + 1;
    while (abs(E - E1) > 2E-5) {
      E1 = E;
      E = E1 - ( E1 - orbs[i][4] * sin(E1) - orbs[i][3]) / (1 - orbs[i][4] * cos(E1));
    }
    xv = orbs[i][5] * (cos(E) - orbs[i][4]);
    yv = orbs[i][5] * (sqrt(1 - orbs[i][4] * orbs[i][4]) * sin(E));
    v = atan2(yv, xv);
    r = sqrt(xv * xv + yv * yv);
    xh = r * (cos(orbs[i][0]) * cos(v + orbs[i][2]) - sin(orbs[i][0]) * sin(v + orbs[i][2]) * cos(orbs[i][1]));
    yh = r * (sin(orbs[i][0]) * cos(v + orbs[i][2]) + cos(orbs[i][0]) * sin(v + orbs[i][2]) * cos(orbs[i][1]));
    zh = r * (sin(v + orbs[i][2]) * sin(orbs[i][1]));
    xs = rs * cos(lons);
    ys = rs * sin(lons);
    xg = xh + xs;
    yg = yh + ys;
    zg = zh;
    xe = xg;
    ye = yg * 0.9175 - zg * 0.3977;
    ze = yg * 0.3977 + zg * 0.9175;
    celes[i][0] = fmod(atan2(ye, xe) * r2d + 360, 360);
    celes[i][1] = atan2(ze, sqrt(xe * xe + ye * ye));
  }
}

void calc_coords() {
  // calculates the current topocentric (land-based) coordinates of each
  // body from the celestial coordinates 
  // formulae from https://aa.usno.navy.mil/faq/docs/Alt_Az.php

  for (byte i = 0; i < num_bodies; i++) {
    float LHA = (sidereal * 15 - celes[i][0] + lon) / r2d;                                                        // local hour angle
    
    coords[i][0] = asin(cos(LHA) * cos(celes[i][1]) * cos(lat / r2d) + sin(celes[i][1]) * sin(lat / r2d)) * r2d;  // altitude of the body
    coords[i][1] = fmod(atan2(-sin(LHA), tan(celes[i][1]) * cos(lat / r2d) - sin(lat / r2d) * cos(LHA))           // azimuth of the body
                        * r2d + 360, 360);
  }
}

void update_coords() {
  // re-calculates the topocentric coordinates of everything
  // (this is used once every 5 minutes to keep up with the rotating earth)
  
  now = rtc.now(); // Get the new current time from the clock
  calc_sidereal();
  calc_planets();
  calc_coords();
}

void get_orientation() {
  // From the Arduino AHRS example sketch
  // Uses a Madgwick filter to make the orientation more accurate
  // with the accelerometer (senses gravity) and magnetometer (senses North).
  
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
  gx *= r2d;
  gy *= r2d;
  gz *= r2d;

  // Update the filter
  filter.update(gx, gy, gz,
                accel_event.acceleration.x, accel_event.acceleration.y, accel_event.acceleration.z,
                mx, my, mz);
                
  pitch = filter.getPitch();
  yaw = filter.getYaw();
  alt = pitch + alt_offset;
  azi = fmod((720 - yaw - azi_offset), 360);
  delay(10);
}

void calibrate(){
  // Recenters the coordinate system based on Polaris, so you can
  // adjust for it not being at magnetic north
  // (this is used when you press "A")
  
  alt_offset = lat - pitch;
  azi_offset = fmod(-yaw, 360);
}

void find_closest() {
  // From the topocentric coordinates and the current orientation,
  // calculates which body you're closest to pointing at.
  // formula from http://spiff.rit.edu/classes/phys373/lectures/radec/radec.html
  
  float min_dist = 4;                              // reset min_dist
  byte closest;
  
  for (byte i = 0; i < num_bodies; i++) {
    float dist = acos(cos(1.5708 - alt / r2d) * cos(1.5708 - coords[i][0] / r2d) +  // angular distance
                      sin(1.5708 - alt / r2d) * sin(1.5708 - coords[i][0] / r2d) *
                      cos(azi / r2d - coords[i][1] / r2d));
    if (dist < min_dist) {                                                          // set closest to this one if it's closer
      min_dist = dist;
      closest = i;
    }
  }
  closest_dist = round(min_dist * r2d);
  alt_dist = round(coords[closest][0] - alt);
  azi_dist = round(coords[closest][1] - azi);
  if (azi_dist > 180) { azi_dist -= 360; }
  else if (azi_dist < -180) { azi_dist += 360; }
  get_name(closest);
}

void print_screen1() {
  // Prints the main screen which tells you which star/planet is closest,
  // how many degrees away it is, and which way to adjust to get closer.
  
  lcd.clear();
  if (closest_dist < 10 ) { lcd.setCursor(1, 0); }
  lcd.print(closest_dist, 0); lcd.write(223);
  lcd.setCursor(4, 0); lcd.print("away from");
  lcd.setCursor(6 - int(closest_name.length() - 1) / 2, 1);
  lcd.print(closest_name);

  if (closest_dist == 0) {
    lcd.setCursor(14, 0); lcd.print("**");
    lcd.setCursor(14, 1); lcd.print("**");
  } else if (alt_dist > 0) {
    if (azi_dist > 0) { lcd.setCursor(15, 0); lcd.write(4); }
    else if (azi_dist < 0) { lcd.setCursor(14, 0); lcd.write(3); }
    else { lcd.setCursor(14, 0); lcd.write(1); lcd.write(1); }
  } else if (alt_dist < 0) {
    if (azi_dist > 0) { lcd.setCursor(15, 1); lcd.write(6); }
    else if (azi_dist < 0) { lcd.setCursor(14, 1); lcd.write(5); }
    else { lcd.setCursor(14, 1); lcd.write(2); lcd.write(2);}
  } else {
    if (azi_dist > 0) {
      lcd.setCursor(15, 0); lcd.write(126);
      lcd.setCursor(15, 1); lcd.write(126);
    } else {
      lcd.setCursor(14, 0); lcd.write(127);
      lcd.setCursor(14, 1); lcd.write(127);
    }
  }
}

void print_screen2() {
  // Prints the "B" screen which tells you the current time (UTC)
  // and geographic coordinates (latitude and longitude), so you can
  // check that these are both working correctly.
  
  lcd.clear();
  lcd.print(now.year()); lcd.print("-");
  if (now.month() < 10) { lcd.print(0); }
  lcd.print(now.month()); lcd.print("-");
  if (now.day() < 10) { lcd.print(0); }
  lcd.print(now.day()); lcd.print(" ");
  if (now.hour() < 10) { lcd.print(0); }
  lcd.print(now.hour()); lcd.print(":");
  if (now.minute() < 10) { lcd.print(0); }
  lcd.print(now.minute());
  lcd.setCursor(0, 1);
  if (lat < 0) {
    lcd.print(-lat); lcd.write(223); lcd.print("S");
  } else { lcd.print(lat); lcd.write(223); lcd.print("N"); }
  if (abs(lon) < 100) { lcd.setCursor(9, 1); } else { lcd.setCursor(8, 1); }
  if (lon < 0) {
    lcd.print(-lon); lcd.write(223); lcd.print("W");
  } else { lcd.print(lon); lcd.write(223); lcd.print("E"); }

}

void setup() {

  // Initialize the sensors, filter, clock, and LCD
  gyro.begin();
  accelmag.begin();
  filter.begin(60); // filter rate in samples/second
  rtc.begin();
  lcd.begin(16, 2);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  // Create six new arrow characters for the LCD to show directions
  lcd.createChar(1, up_char);
  lcd.createChar(2, down_char);
  lcd.createChar(3, up_left_char);
  lcd.createChar(4, up_right_char);
  lcd.createChar(5, down_left_char);
  lcd.createChar(6, down_right_char);

  // Print a little fancy intro:
  // Makes asterisks (read "stars") pup up at random
  lcd.setCursor(5, 0); lcd.print("LASER");
  lcd.setCursor(6, 1); lcd.print("GAZER");
  delay(1000);
  randomSeed(analogRead(0));
  for (int i = 0; i < 150; i ++) {
    row = random(0, 2);
    col = random(0, 16);
    lcd.setCursor(col, row); lcd.print("*");
    for (byte j = 0; j < 5; j++) {get_orientation(); }
  }

  // Wait for the user to calibrate it by pointing at the north star
  lcd.setCursor(0, 0); lcd.print("Point at Polaris");
  lcd.setCursor(0, 1); lcd.print("and then press A");
  while (digitalRead(button1) == LOW) { get_orientation(); }
  calibrate();
}

void loop(void) {

  get_orientation(); // Constantly read new orientation

  // One out of every 100 cycles, do some more heavy calculations
  if (counter % 100 == 0) {
    if (counter % 18000 == 0) {                     // Update coords about every 5 minutes
      update_coords();
    }  
    if (digitalRead(button1) == HIGH) {
      calibrate();                                  // If "A" is pressed, recalibrate
    } else if (digitalRead(button2) == HIGH) {
      print_screen2();                              // If "B" is pressed, show screen 2 (time and geo coords)
    } else {
        find_closest();                             // Otherwise, calculate the closest star/planet
        print_screen1();                            // and display the default screen
    }
  }

  counter++;
}
