#include "RTClib.h"
DateTime now;
RTC_DS1307 rtc;
float d;
float sidereal;
float orbs[6][6]; //N, i, w, a, e, M
float r2d = 57.2958;
float ecl = 0.4090;


void calc_sidereal() {
  // formula from https://aa.usno.navy.mil/faq/docs/GAST.php

  d = (now.year() - 2019) * 365 + (now.year() - 2017) / 4;               // days in full years since 1/1/19
  int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};  // lengths of months
  for (int i = 0; i < now.month() - 1; i++) {
    d += months[i];  // add days in full months
  }
  if (now.month() > 2 && now.year() % 4 == 0) {
    d += 1;  // add day from a leap year
  }
  d += (now.day() - 1) + now.hour() / 24.0 + now.minute() / 1440.0;                  // add days from this month
  sidereal = fmod(6.6907 + 24.0657 * d, 24);                       // sidereal time in hours
  d += 6941;
}

void calc_orbs() {
  orbs[0][0] = 0;
  orbs[0][1] = 0;
  orbs[0][2] = 282.9404 + 4.70935E-5 * d;
  orbs[0][3] = 1;
  orbs[0][4] = 0.016709 - 1.151E-9 * d;
  orbs[0][5] = 356.0470 + 0.9856002585 * d;
  orbs[1][0] = 48.3313 + 3.24587E-5 * d;
  orbs[1][1] = 7.0047 + 5.00E-8 * d;
  orbs[1][2] = 29.1241 + 1.01444E-5 * d;
  orbs[1][3] = 0.387098;
  orbs[1][4] = 0.205635 + 5.59E-10 * d;
  orbs[1][5] = 168.6562 + 4.0923344368 * d;
  orbs[2][0] = 76.6799 + 2.46590E-5 * d;
  orbs[2][1] = 3.3946 + 2.75E-8 * d;
  orbs[2][2] = 54.8910 + 1.38374E-5 * d;
  orbs[2][3] = 0.723330;
  orbs[2][4] = 0.006773 - 1.302E-9 * d;
  orbs[2][5] = 48.0052 + 1.6021302244 * d;
  orbs[3][0] = 49.5574 + 2.11081E-5 * d;
  orbs[3][1] = 1.8497 - 1.78E-8 * d; 
  orbs[3][2] = 286.5016 + 2.92961E-5 * d;
  orbs[3][3] = 1.523688;
  orbs[3][4] = 0.093405 + 2.516E-9 * d;
  orbs[3][5] = 18.6021 + 0.5240207766 * d;
  orbs[4][0] = 100.4542 + 2.76854E-5 * d;
  orbs[4][1] = 1.3030 - 1.557E-7 * d;
  orbs[4][2] = 273.8777 + 1.64505E-5 * d;
  orbs[4][3] = 5.20256;
  orbs[4][4] = 0.048498 + 4.469E-9 * d;
  orbs[4][5] = 19.8950 + 0.0830853001 * d;
  orbs[5][0] = 113.6634 + 2.38980E-5 * d;
  orbs[5][1] = 2.4886 - 1.081E-7 * d;
  orbs[5][2] = 339.3939 + 2.97661E-5 * d;
  orbs[5][3] = 9.55475;
  orbs[5][4] = 0.055546 - 9.499E-9 * d;
  orbs[5][5] = 316.9670 + 0.0334442282 * d;

  for (int i = 0; i < 6; i++) {
    orbs[i][1] = fmod(orbs[i][1], 360) / r2d;
    orbs[i][2] = fmod(orbs[i][2], 360) / r2d;
    orbs[i][0] = fmod(orbs[i][0], 360) / r2d;
    orbs[i][5] = fmod(orbs[i][5], 360) / r2d;
  }
}

void calc_planets(){
  float E, E1, xv, yv, v, r, xh, yh, zh, xs, ys, xg, yg, zg, xe, ye, ze, RA, DE;
  E = orbs[0][5] + orbs[0][4] * sin (orbs[0][5]) * (1 + orbs[0][4] * cos(orbs[0][5]));
  xv = cos(E) - orbs[0][4];
  yv = sqrt(1 - orbs[0][4] * orbs[0][4]) * sin(E);
  v = atan2 (yv, xv);
  float rs = sqrt(xv * xv + yv * yv);
  float lons = v + orbs[0][2];
  for (int i = 1; i < 6; i++) {
    E = orbs[i][5] + orbs[i][4] * sin (orbs[i][5]) * (1 + orbs[i][4] * cos(orbs[i][5]));
    E1 = E + 1;
    while (abs(E - E1) > 2E-5) {
      E1 = E;
      E = E1 - ( E1 - orbs[i][4] * sin(E1) - orbs[i][5]) / (1 - orbs[i][4] * cos(E1));
    }
    xv = orbs[i][3] * (cos(E) - orbs[i][4]);
    yv = orbs[i][3] * (sqrt(1 - orbs[i][4] * orbs[i][4]) * sin(E));
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
    ye = yg * cos(ecl) - zg * sin(ecl);
    ze = yg * sin(ecl) + zg * cos(ecl);
    RA = fmod(atan2(ye, xe) * r2d + 360, 360);
    DE = atan2(ze, sqrt(xe * xe + ye * ye)) * r2d;
    Serial.print(i); Serial.print(" "); Serial.print(RA); Serial.print(" "); Serial.println(DE);
  }
}

void setup() {
  Serial.begin(9600);
  rtc.begin();
  now = rtc.now();
  calc_sidereal();
  calc_orbs();
  calc_planets();  
}

void loop() {
}
